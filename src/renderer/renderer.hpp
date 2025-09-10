#pragma once

#include "core/window.hpp"
#include "device.hpp"
#include "swap_chain.hpp"

#include <memory>
#include <vector>
#include <cassert>

namespace grape {
	class Renderer {

	public:

		Renderer(Window& window, Device& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return grapeSwapChain->getRenderPass(); }
		float getAspectRatio() const { return grapeSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		// Corrected to use currentFrame consistently
		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return commandBuffers[currentFrame];
		}

		// Corrected to use currentFrame consistently
		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame not in progress");
			return currentFrame;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void beginOffscreenRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRenderPass renderPass, VkExtent2D extent);
		void endOffscreenRenderPass(VkCommandBuffer commandBuffer);
		VkImageView getSwapChainImageView(int index) { return grapeSwapChain->getImageView(index); }
		size_t getSwapChainImageCount() { return grapeSwapChain->imageCount(); }

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();
		void createSyncObjects();

		Window& grapeWindow;
		Device& grapeDevice;
		std::unique_ptr<SwapChain> grapeSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight; // to track which image is being rendered
		size_t currentFrame = 0; // The primary frame index

		uint32_t currentImageIndex = 0;
		bool isFrameStarted = false;
	};
}
