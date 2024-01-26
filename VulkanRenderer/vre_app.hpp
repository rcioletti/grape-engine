#pragma once

#include "vre_window.hpp"
#include "vre_pipeline.hpp"
#include "vre_device.hpp"

namespace vre {
	class VreApp {

	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		void run();

	private:
		VreWindow vreWindow{ WIDTH, HEIGHT, "Vulkan 3D Renderer" };
		VreDevice vreDevice{ vreWindow };
		VrePipeline vrePipeline{ vreDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", VrePipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
	};
}