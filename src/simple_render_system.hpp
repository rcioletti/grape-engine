#pragma once

#include "camera.hpp"
#include "pipeline.hpp"
#include "device.hpp"
#include "game_object.hpp"
#include "frame_info.hpp"

#include <memory>
#include <vector>

namespace grape {
	class SimpleRenderSystem {

	public:

		SimpleRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(FrameInfo &frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		Device &grapeDevice;

		std::unique_ptr<Pipeline> grapePipeline;
		VkPipelineLayout pipelineLayout;
	};
}