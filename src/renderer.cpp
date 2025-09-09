#include "renderer.hpp"

#include <stdexcept>
#include <array>
#include <cassert>

namespace grape {

	Renderer::Renderer(Window& window, Device& device) : grapeWindow{ window }, grapeDevice{ device } {
		recreateSwapChain();
		createCommandBuffers();
	}

	Renderer::~Renderer() {
		vkDeviceWaitIdle(grapeDevice.device());

		 // Destroy per-frame semaphores
		for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
			if (imageAvailableSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(grapeDevice.device(), imageAvailableSemaphores[i], nullptr);
		}
		imageAvailableSemaphores.clear();

		// Destroy per-image semaphores
		for (size_t i = 0; i < renderFinishedSemaphores.size(); i++) {
			if (renderFinishedSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(grapeDevice.device(), renderFinishedSemaphores[i], nullptr);
		}
		renderFinishedSemaphores.clear();

		// Destroy fences
		for (size_t i = 0; i < inFlightFences.size(); i++) {
			if (inFlightFences[i] != VK_NULL_HANDLE)
				vkDestroyFence(grapeDevice.device(), inFlightFences[i], nullptr);
		}
		inFlightFences.clear();

		freeCommandBuffers();
	}

	VkCommandBuffer Renderer::beginFrame() {
		assert(!isFrameStarted && "Can't call beginFrame while already in progress");

		// Wait for the fence for this frame
		vkWaitForFences(grapeDevice.device(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		// Acquire the next image, signaling the per-frame semaphore
		VkResult result = vkAcquireNextImageKHR(
			grapeDevice.device(),
			grapeSwapChain->getSwapChain(),
			UINT64_MAX,
			imageAvailableSemaphores[currentFrame],
			VK_NULL_HANDLE,
			&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		// If a previous frame is using this image, wait for it
		if (imagesInFlight[currentImageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(grapeDevice.device(), 1, &imagesInFlight[currentImageIndex], VK_TRUE, UINT64_MAX);
		}
		imagesInFlight[currentImageIndex] = inFlightFences[currentFrame];

		isFrameStarted = true;

		auto commandBuffer = commandBuffers[currentFrame];

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		return commandBuffer;
	}

	void Renderer::createSyncObjects() {
		size_t imageCount = grapeSwapChain->imageCount();
		imageAvailableSemaphores.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(imageCount);
		inFlightFences.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		// Create per-frame imageAvailableSemaphores and inFlightFences
		for (size_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(grapeDevice.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create imageAvailableSemaphore!");
			}
			if (vkCreateFence(grapeDevice.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create inFlightFence!");
			}
		}
		// Create per-image renderFinishedSemaphores
		for (size_t i = 0; i < imageCount; i++) {
			if (vkCreateSemaphore(grapeDevice.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create renderFinishedSemaphore!");
			}
		}
	}

	void Renderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not started");

		auto commandBuffer = commandBuffers[currentFrame];

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer");
		}

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentImageIndex] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// Reset the fence before using it for this frame
		vkResetFences(grapeDevice.device(), 1, &inFlightFences[currentFrame]);

		if (vkQueueSubmit(grapeDevice.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { grapeSwapChain->getSwapChain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &currentImageIndex;

		auto result = vkQueuePresentKHR(grapeDevice.presentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || grapeWindow.wasWindowResized()) {
			grapeWindow.resetWindowResizedFlag();
			isFrameStarted = false;
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentFrame = (currentFrame + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {

		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = grapeSwapChain->getRenderPass();
		renderPassInfo.framebuffer = grapeSwapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = grapeSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(grapeSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(grapeSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0,0}, grapeSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");

		vkCmdEndRenderPass(commandBuffer);
	}

	void Renderer::createCommandBuffers()
	{
		commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = grapeDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(grapeDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void Renderer::freeCommandBuffers()
	{
		vkFreeCommandBuffers(
			grapeDevice.device(),
			grapeDevice.getCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());
		commandBuffers.clear();
	}

	void Renderer::recreateSwapChain() {
		auto extent = grapeWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = grapeWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(grapeDevice.device());

		if (grapeSwapChain == nullptr) {
			grapeSwapChain = std::make_unique<SwapChain>(grapeDevice, extent);
			createSyncObjects();
		} else {
			std::shared_ptr<SwapChain> oldSwapChain = std::move(grapeSwapChain);
			grapeSwapChain = std::make_unique<SwapChain>(grapeDevice, extent, oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*grapeSwapChain.get())) {
				throw std::runtime_error("Swap chain image (or depth) format has changed!");
			}

			// If the number of images changed, destroy old semaphores and recreate
			if (imagesInFlight.size() != grapeSwapChain->imageCount()) {
				for (auto sem : renderFinishedSemaphores) {
					if (sem != VK_NULL_HANDLE)
						vkDestroySemaphore(grapeDevice.device(), sem, nullptr);
				}
				renderFinishedSemaphores.clear();

				for (auto sem : imageAvailableSemaphores) {
					if (sem != VK_NULL_HANDLE)
						vkDestroySemaphore(grapeDevice.device(), sem, nullptr);
				}
				imageAvailableSemaphores.clear();

				for (auto fence : inFlightFences) {
					if (fence != VK_NULL_HANDLE)
						vkDestroyFence(grapeDevice.device(), fence, nullptr);
				}
				inFlightFences.clear();

				createSyncObjects();
			}
		}
	}

	void Renderer::beginOffscreenRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRenderPass renderPass, VkExtent2D extent) {
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, extent };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void Renderer::endOffscreenRenderPass(VkCommandBuffer commandBuffer) {
		vkCmdEndRenderPass(commandBuffer);
	}

}
