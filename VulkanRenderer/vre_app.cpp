#include "vre_app.hpp"

#include <stdexcept>

namespace vre {

	VreApp::VreApp()
	{
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
		}
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
	}

	void VreApp::drawFrame()
	{
	}
}