#pragma once

#include "vre_camera.hpp"
#include "vre_pipeline.hpp"
#include "vre_device.hpp"
#include "vre_game_object.hpp"

#include <memory>
#include <vector>

namespace vre {
	class SimpleRenderSystem {

	public:

		SimpleRenderSystem(VreDevice &device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VreGameObject> &gameObjects, const VreCamera& camera);

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass);

		VreDevice &vreDevice;

		std::unique_ptr<VrePipeline> vrePipeline;
		VkPipelineLayout pipelineLayout;
	};
}