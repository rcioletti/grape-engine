#include "vre_app.hpp"

#include "vre_camera.hpp"
#include "simple_render_system.hpp"
#include "keyboard_movement_controller.hpp"
#include "vre_buffer.hpp"
#include "map_manager.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace vre {

	struct GlobalUbo {
		glm::mat4 projectionView{ 1.f };
		glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f, -3.f, -1.f});
	};

	VreApp::VreApp()
	{
		loadGameObjects();
	}

	VreApp::~VreApp()
	{
	
	}

	void VreApp::run()
	{
		std::vector<std::unique_ptr<VreBuffer>> uboBuffers(VreSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<VreBuffer>(
				vreDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				vreDevice.properties.limits.minUniformBufferOffsetAlignment
				);
			uboBuffers[i]->map();
		}

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
				
				int frameIndex = vreRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera
				};

				// update
				GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				vreRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo, gameObjects, camera);
				vreRenderer.endSwapChainRenderPass(commandBuffer);
				vreRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(vreDevice.device());
	}

	void VreApp::loadGameObjects()
	{
		std::shared_ptr<VreModel> modelVase = VreModel::createModelFromFile(vreDevice, "models/smooth_vase.obj");
		std::shared_ptr<VreModel> modelChair = VreModel::createModelFromFile(vreDevice, "models/chair.obj");

		//vase
        auto gameObj = VreGameObject::createGameObject();
		gameObj.model = modelVase;
		gameObj.transform.translation = { .0f, .5f, 2.5f };
		gameObj.transform.scale = glm::vec3(3.f);
        gameObjects.push_back(std::move(gameObj));

		//chair
		auto gameObjChair = VreGameObject::createGameObject();
		gameObjChair.model = modelChair;
		gameObjChair.transform.translation = { -.5f, .5f, 2.5f };
		gameObjChair.transform.rotation = { 0.f, 2.5f, 3.15f };
		gameObjChair.transform.scale = glm::vec3(1.f);
		gameObjects.push_back(std::move(gameObjChair));

		//TODO: load game objects from map on disk
		MapManager mapManager("Main Map", "mainMap.json");
		mapManager.loadMap("mainMap.json");
	}
}