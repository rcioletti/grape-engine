#include "vre_renderer.hpp"

#include <stdexcept>
#include <array>
#include <cassert>

namespace vre {

	VreRenderer::VreRenderer(VreWindow& window, VreDevice& device) : vreWindow{ window }, vreDevice{device}{
		
		recreateSwapChain();
		createCommandBuffers();
	}

	VreRenderer::~VreRenderer(){
		
		freeCommandBuffers();
	}

	VkCommandBuffer VreRenderer::beginFrame(){
		
		assert(!isFrameStarted && "Can't call beginFrame while already in progress");

		auto result = vreSwapChain->acquireNextImage(&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		return commandBuffer;
	}

	void VreRenderer::endFrame(){
		
		assert(isFrameStarted && "Can't call endFrame while already in progress");

		auto commandBuffer = getCurrentCommandBuffer();

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer");
		}

		auto result = vreSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vreWindow.wasWindowResized()) {
			vreWindow.resetWindowResizedFlag();
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % VreSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void VreRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer){

		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = vreSwapChain->getRenderPass();
		renderPassInfo.framebuffer = vreSwapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vreSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(vreSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(vreSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0,0}, vreSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void VreRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");

		vkCmdEndRenderPass(commandBuffer);
	}

	void VreRenderer::createCommandBuffers()
	{
		commandBuffers.resize(VreSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = vreDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(vreDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void VreRenderer::freeCommandBuffers()
	{
		vkFreeCommandBuffers(
			vreDevice.device(),
			vreDevice.getCommandPool(),
			static_cast<float>(commandBuffers.size()),
			commandBuffers.data());
		commandBuffers.clear();
	}

	void VreRenderer::recreateSwapChain()
	{
		auto extent = vreWindow.getExtent();
		while (extent.width == 0 || extent.height == 0)
		{
			extent = vreWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(vreDevice.device());

		if (vreSwapChain == nullptr) {
			vreSwapChain = std::make_unique<VreSwapChain>(vreDevice, extent);
		}
		else {
			std::shared_ptr<VreSwapChain> oldSwapChain = std::move(vreSwapChain);
			vreSwapChain = std::make_unique<VreSwapChain>(vreDevice, extent, oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*vreSwapChain.get())) {
				throw std::runtime_error("Swap chain image(or depthh) format has changed!");
			}
		}

		//if render pass is compatible do not recreate pipeline
	}
}