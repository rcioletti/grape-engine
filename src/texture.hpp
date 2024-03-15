#pragma once

#include "buffer.hpp"

namespace grape {

	class Texture {

	public:
		Texture(Device& device, std::string texturePath);
		~Texture();

		void createTextureImage(std::string texturePath);

		void createImage(
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory);

		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		void createTextureImageView();

		VkImageView createImageView(VkImage image, VkFormat format);

		void createTextureSampler();

		void cleanup();

		VkImageView getTextureImageView() { return textureImageView; }
		VkSampler getTextureSampler() { return textureSampler; }

	private:

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
		Device& grapeDevice;
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	};
}