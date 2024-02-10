#pragma once

#include "vre_window.hpp"
#include "vre_device.hpp"
#include "vre_swap_chain.hpp"

#include <memory>
#include <vector>
#include <cassert>

namespace vre {
	class VreRenderer {

	public:

		VreRenderer(VreWindow &window, VreDevice &device);
		~VreRenderer();

		VreRenderer(const VreRenderer&) = delete;
		VreRenderer& operator=(const VreRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return vreSwapChain->getRenderPass(); }
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

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		VreWindow &vreWindow;
		VreDevice &vreDevice;
		std::unique_ptr<VreSwapChain> vreSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex;
		bool isFrameStarted = false;
	};
}