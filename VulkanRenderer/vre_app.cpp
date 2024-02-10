#include "vre_app.hpp"

#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace vre {

	VreApp::VreApp()
	{
		loadGameObjects();
	}

	VreApp::~VreApp()
	{
	
	}

	void VreApp::run()
	{
		SimpleRenderSystem simpleRenderSystem{ vreDevice, vreRenderer.getSwapChainRenderPass() };

		while (!vreWindow.shoudClose()) {
			glfwPollEvents();
			
			if (auto commandBuffer = vreRenderer.beginFrame()) {
				
				vreRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
				vreRenderer.endSwapChainRenderPass(commandBuffer);
				vreRenderer.endFrame();
			}
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
}