#pragma once

#include "../camera.hpp"
#include "../pipeline.hpp"
#include "../device.hpp"
#include "../game_object.hpp"
#include "../frame_info.hpp"

#include <memory>
#include <vector>

namespace grape {
	class PointLightSystem {

	public:

		PointLightSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		PointLightSystem(const PointLightSystem&) = delete;
		PointLightSystem& operator=(const PointLightSystem&) = delete;

		void update(FrameInfo& frameInfo, GlobalUbo& ubo);
		void render(FrameInfo& frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		Device &grapeDevice;

		std::unique_ptr<Pipeline> grapePipeline;
		VkPipelineLayout pipelineLayout;
	};
}