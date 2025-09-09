#pragma once

#include "window.hpp"
#include "device.hpp"
#include "swap_chain.hpp"

#include <memory>
#include <vector>
#include <cassert>

namespace grape {
	class Renderer {

	public:

		Renderer(Window &window, Device &device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return grapeSwapChain->getRenderPass(); }
		float getAspectRatio() const { return grapeSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame not in progress");
			return currentFrameIndex;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void beginOffscreenRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRenderPass renderPass, VkExtent2D extent);
		void endOffscreenRenderPass(VkCommandBuffer commandBuffer);
		VkImageView getSwapChainImageView(int index) {return grapeSwapChain->getImageView(index);}
		size_t getSwapChainImageCount() { return grapeSwapChain->imageCount(); }

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		Window &grapeWindow;
		Device &grapeDevice;
		std::unique_ptr<SwapChain> grapeSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex;
		bool isFrameStarted = false;
	};
}