#include "vre_app.hpp"

#include <stdexcept>
#include <array>

namespace vre {

	VreApp::VreApp()
	{
		loadModels();
		createPipelineLayout();
		createPipeline();
		createCommandBuffers();
	}

	VreApp::~VreApp()
	{
		vkDestroyPipelineLayout(vreDevice.device(), pipelineLayout, nullptr);
	}

	void VreApp::run()
	{
		while (!vreWindow.shoudClose()) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(vreDevice.device());
	}

	void VreApp::loadModels()
	{
		std::vector<VreModel::Vertex> vertices{
			{{0.0f, -0.5}, {1.0, 0.0f, 0.0f}},
			{{0.5f, 0.5}, {0.0, 1.0f, 0.0f}},
			{{-0.5f, 0.5}, {0.0, 0.0f, 1.0f}}
		};

		vreModel = std::make_unique<VreModel>(vreDevice, vertices);
	}

	void VreApp::createPipelineLayout()
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(vreDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void VreApp::createPipeline()
	{
		auto pipelineConfig = VrePipeline::defaultPipelineConfigInfo(vreSwapChain.width(), vreSwapChain.height());
		pipelineConfig.renderPass = vreSwapChain.getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		vrePipeline = std::make_unique<VrePipeline>(vreDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
	}

	void VreApp::createCommandBuffers()
	{
		commandBuffers.resize(vreSwapChain.imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = vreDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(vreDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		for (int i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = vreSwapChain.getRenderPass();
			renderPassInfo.framebuffer = vreSwapChain.getFrameBuffer(i);

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = vreSwapChain.getSwapChainExtent();

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vrePipeline->bind(commandBuffers[i]);
			vreModel->bind(commandBuffers[i]);
			vreModel->draw(commandBuffers[i]);

			vkCmdEndRenderPass(commandBuffers[i]);
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer");
			}
		}
	}

	void VreApp::drawFrame()
	{
		uint32_t imageIndex;
		auto result = vreSwapChain.acquireNextImage(&imageIndex);

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		result = vreSwapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}
}