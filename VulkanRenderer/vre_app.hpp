#pragma once

#include "vre_window.hpp"
#include "vre_pipeline.hpp"
#include "vre_device.hpp"
#include "vre_swap_chain.hpp"
#include "vre_model.hpp"

#include <memory>
#include <vector>

namespace vre {
	class VreApp {

	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		VreApp();
		~VreApp();

		VreApp(const VreApp&) = delete;
		VreApp& operator=(const VreApp&) = delete;

		void run();

	private:
		void loadModels();
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void drawFrame();

		VreWindow vreWindow{ WIDTH, HEIGHT, "Vulkan 3D Renderer" };
		VreDevice vreDevice{ vreWindow };
		VreSwapChain vreSwapChain{ vreDevice, vreWindow.getExtent() };
		std::unique_ptr<VrePipeline> vrePipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<VreModel> vreModel;
	};
}