#include "vre_app.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace vre {

	struct SimplePushConstantData {
		glm::mat2 transform{1.f};
		glm::vec2 offset;
		alignas(16) glm::vec3 color;
	};

	VreApp::VreApp()
	{
		loadGameObjects();
		createPipelineLayout();
		recreateSwapChain();
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

	void VreApp::loadGameObjects()
	{
		std::vector<VreModel::Vertex> vertices{
			{{0.0f, -0.5}, {1.0, 0.0f, 0.0f}},
			{{0.5f, 0.5}, {0.0, 1.0f, 0.0f}},
			{{-0.5f, 0.5}, {0.0, 0.0f, 1.0f}}
		};

		auto vreModel = std::make_shared<VreModel>(vreDevice, vertices);

		auto triangle = VreGameObject::createGameObject();
		triangle.model = vreModel;
		triangle.color = { .1f, .8f, .1f };
		triangle.transform2d.translation.x = .2f;
		triangle.transform2d.scale = { 2.f, .5f };
		triangle.transform2d.rotation = .25f * glm::two_pi<float>();

		gameObjects.push_back(std::move(triangle));
	}

	void VreApp::createPipelineLayout()
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(vreDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void VreApp::createPipeline()
	{
		assert(vreSwapChain != nullptr && "Cannot create pipeline before swap chain");
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		VrePipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = vreSwapChain->getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		vrePipeline = std::make_unique<VrePipeline>(vreDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
	}

	void VreApp::createCommandBuffers()
	{
		commandBuffers.resize(vreSwapChain->imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = vreDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(vreDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void VreApp::freeCommandBuffers()
	{
		vkFreeCommandBuffers(
			vreDevice.device(),
			vreDevice.getCommandPool(),
			static_cast<float>(commandBuffers.size()),
			commandBuffers.data());
		commandBuffers.clear();
	}
	
	void VreApp::recordCommandBuffer(int imageIndex) {

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = vreSwapChain->getRenderPass();
		renderPassInfo.framebuffer = vreSwapChain->getFrameBuffer(imageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vreSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(vreSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(vreSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0,0}, vreSwapChain->getSwapChainExtent()};
		vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
		vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

		renderGameObjects(commandBuffers[imageIndex]);

		vkCmdEndRenderPass(commandBuffers[imageIndex]);
		if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer");
		}
	}

	void VreApp::renderGameObjects(VkCommandBuffer commandBuffer)
	{
		vrePipeline->bind(commandBuffer);

		for (auto& obj : gameObjects) {
			obj.transform2d.rotation = glm::mod(obj.transform2d.rotation + 0.01f, glm::two_pi<float>());

			SimplePushConstantData push{};

			push.offset = obj.transform2d.translation;
			push.color = obj.color;
			push.transform = obj.transform2d.mat2();

			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push
			);
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}

	void VreApp::recreateSwapChain()
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
			vreSwapChain = std::make_unique<VreSwapChain>(vreDevice, extent, std::move(vreSwapChain));
			if (vreSwapChain->imageCount() != commandBuffers.size()) {
				freeCommandBuffers();
				createCommandBuffers();
			}
		}

		//if render pass is compatible do not reacreate pipeline
		createPipeline();
	}

	void VreApp::drawFrame()
	{
		uint32_t imageIndex;
		auto result = vreSwapChain->acquireNextImage(&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		recordCommandBuffer(imageIndex);

		result = vreSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vreWindow.wasWindowResized()) {
			vreWindow.resetWindowResizedFlag();
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}
}