#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "vre_buffer.hpp"

namespace vre {

	class VreTexture {

	public:
		VreTexture(VreDevice& device);
		~VreTexture();

		void createTextureImage();

		void createImage(
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory);

		VkCommandBuffer beginSingleTimeCommands();

		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	private:

		VreDevice& vreDevice;
	};
}