#pragma once

#include "buffer.hpp"
#include <glm/glm.hpp>

namespace grape {

	class Texture {

	public:
		Texture(Device& device);
		~Texture();

		void createTextureFromFile(std::string texturePath);

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


		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

		void createTextureSampler();

		void createTextureFromColor(const glm::vec4& color);

		void cleanup();

		VkImageView getTextureImageView() const { return textureImageView; }
		VkSampler getTextureSampler() const { return textureSampler; }

	private:

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
		Device& grapeDevice;
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	};
}