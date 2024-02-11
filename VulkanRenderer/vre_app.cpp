#include "vre_app.hpp"

#include "vre_camera.hpp"
#include "simple_render_system.hpp"
#include "keyboard_movement_controller.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

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
        VreCamera camera{};
        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = VreGameObject::createGameObject();
        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

		while (!vreWindow.shoudClose()) {
			glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();

            currentTime = newTime;

            cameraController.moveInPlaneXZ(vreWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = vreRenderer.getAspectRatio();
            //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1f, 10.f);
			
			if (auto commandBuffer = vreRenderer.beginFrame()) {
				
				vreRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
				vreRenderer.endSwapChainRenderPass(commandBuffer);
				vreRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(vreDevice.device());
	}

	void VreApp::loadGameObjects()
	{
		std::shared_ptr<VreModel> vreModel = VreModel::createModelFromFile(vreDevice, "models/smooth_vase.obj");

        auto gameObj = VreGameObject::createGameObject();
		gameObj.model = vreModel;
		gameObj.transform.translation = { .0f, .0f, 2.5f };
		gameObj.transform.scale = glm::vec3(3.f);
        gameObjects.push_back(std::move(gameObj));
	}
}