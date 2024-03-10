#pragma once

#include "vre_buffer.hpp"

namespace vre {

	class VreTexture {

	public:
		VreTexture(VreDevice& device, std::string texturePath);
		~VreTexture();

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

		VkImageView getTextureImageView() { return textureImageView; }
		VkSampler getTextureSampler() { return textureSampler; }

	private:

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
		VreDevice& vreDevice;
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	};
}