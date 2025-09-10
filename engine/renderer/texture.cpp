#include "texture.hpp"
#include "descriptors.hpp"
#include "swap_chain.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <glm/ext/vector_float4.hpp>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif
#include <iostream>

namespace grape {

	Texture::Texture(Device& device) : grapeDevice{device}
	{
	
	}

	Texture::~Texture()
	{
		cleanup();
	}

	void Texture::createTextureFromFile(std::string texturePath)
	{
		createTextureImage(ENGINE_DIR + texturePath);
		createTextureImageView();
		createTextureSampler();
	}

	void Texture::createTextureImage(std::string texturePath)
	{
		stbi_set_flip_vertically_on_load(true);

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		stbi_set_flip_vertically_on_load(false);

		if (!pixels) {
			throw std::runtime_error("Failed to load texture");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		grapeDevice.createBuffer(
			imageSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;
		vkMapMemory(grapeDevice.device(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(grapeDevice.device(), stagingBufferMemory);

		stbi_image_free(pixels);

		createImage(
			texWidth, 
			texHeight, 
			VK_FORMAT_R8G8B8A8_SRGB, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			textureImage, 
			textureImageMemory);

		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		grapeDevice.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(grapeDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(grapeDevice.device(), stagingBufferMemory, nullptr);
	}

	void Texture::createImage(
		uint32_t width, 
		uint32_t height, 
		VkFormat format, 
		VkImageTiling tiling, 
		VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties, 
		VkImage& image, 
		VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;

		if (vkCreateImage(grapeDevice.device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Image");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(grapeDevice.device(), image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = grapeDevice.findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(grapeDevice.device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate image memory!");
		}

		vkBindImageMemory(grapeDevice.device(), image, imageMemory, 0);
	}

	void Texture::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = grapeDevice.beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		grapeDevice.endSingleTimeCommands(commandBuffer);
	}

	// In texture.cpp
	void Texture::createTextureFromColor(const glm::vec4& color) {
		int texWidth = 1;
		int texHeight = 1;
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		// Create a 1x1 pixel with the specified color
		uint8_t pixels[4];
		pixels[0] = static_cast<uint8_t>(color.r * 255.0f);
		pixels[1] = static_cast<uint8_t>(color.g * 255.0f);
		pixels[2] = static_cast<uint8_t>(color.b * 255.0f);
		pixels[3] = static_cast<uint8_t>(color.a * 255.0f);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		grapeDevice.createBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(grapeDevice.device(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(grapeDevice.device(), stagingBufferMemory);

		createImage(
			texWidth,
			texHeight,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			textureImage,
			textureImageMemory);

		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		grapeDevice.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(grapeDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(grapeDevice.device(), stagingBufferMemory, nullptr);

		createTextureImageView();
		createTextureSampler();
	}

	void Texture::createTextureImageView()
	{
		textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
	}

	VkImageView Texture::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(grapeDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		return imageView;
	}

	void Texture::createTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = grapeDevice.properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(grapeDevice.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	void Texture::cleanup()
	{
		if (textureImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(grapeDevice.device(), textureImageView, nullptr);
			textureImageView = VK_NULL_HANDLE;
		}
		if (textureSampler != VK_NULL_HANDLE) {
			vkDestroySampler(grapeDevice.device(), textureSampler, nullptr);
			textureSampler = VK_NULL_HANDLE;
		}
		if (textureImage != VK_NULL_HANDLE) {
			vkDestroyImage(grapeDevice.device(), textureImage, nullptr);
			textureImage = VK_NULL_HANDLE;
		}
		if (textureImageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(grapeDevice.device(), textureImageMemory, nullptr);
			textureImageMemory = VK_NULL_HANDLE;
		}
	}
}
