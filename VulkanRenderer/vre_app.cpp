#include "vre_app.hpp"

#include "vre_camera.hpp"
#include "simple_render_system.hpp"
#include "keyboard_movement_controller.hpp"
#include "vre_buffer.hpp"
#include "vre_texture.hpp"
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
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };
		glm::vec3 lightPosition{ -1.f };
		alignas(16) glm::vec4 lightColor{ 1.f };
	};

	VreApp::VreApp()
	{
		globalPool = VreDescriptorPool::Builder(vreDevice)
			.setMaxSets(VreSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VreSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();

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

		auto globalSetLayout = VreDescriptorSetLayout::Builder(vreDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(VreSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();

			VkDescriptorImageInfo descriptorInfo[3];

			for (uint32_t i = 0; i < textures.size(); i++) {

				descriptorInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				descriptorInfo[i].sampler = textures[i].getTextureSampler();
				descriptorInfo[i].imageView = textures[i].getTextureImageView();
			}

			VreDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImages(1, descriptorInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{ vreDevice, vreRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
        VreCamera camera{};
        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = VreGameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
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
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1f, 100.f);
			
			if (auto commandBuffer = vreRenderer.beginFrame()) {
				
				int frameIndex = vreRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					gameObjects
				};

				// update
				GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				vreRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);
				vreRenderer.endSwapChainRenderPass(commandBuffer);
				vreRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(vreDevice.device());
	}

	void VreApp::loadGameObjects()
	{
		std::shared_ptr<VreModel> vreModel = VreModel::createModelFromFile(vreDevice, "models/smooth_vase.obj");

		//vase
        auto smoothVase = VreGameObject::createGameObject();
		smoothVase.model = vreModel;
		smoothVase.transform.translation = { .0f, .5f, 0.f };
		smoothVase.transform.scale = glm::vec3(3.f);
		VreTexture smoothVaseTexture = VreTexture(vreDevice, "textures/ceramic.jpg");
		textures.push_back(smoothVaseTexture);
		smoothVase.transform.imgIndex = textures.size() - 1;
        gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

		//chair
		vreModel = VreModel::createModelFromFile(vreDevice, "models/chair.obj");
		auto chair = VreGameObject::createGameObject();
		chair.model = vreModel;
		chair.transform.translation = { -.5f, .67f, 0.f };
		chair.transform.rotation = { 0.f, 2.5f, 3.15f };
		chair.transform.scale = glm::vec3(1.f);
		VreTexture chairTexture = VreTexture(vreDevice, "textures/leather.jpg");
		textures.push_back(chairTexture);
		chair.transform.imgIndex = textures.size() - 1;
		gameObjects.emplace(chair.getId(), std::move(chair));

		//floor
		vreModel = VreModel::createModelFromFile(vreDevice, "models/quad.obj");
		auto floor = VreGameObject::createGameObject();
		floor.model = vreModel;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = glm::vec3(3.f, 1.f, 3.f);
		VreTexture floorTexture = VreTexture(vreDevice, "textures/concrete.png");
		textures.push_back(floorTexture);
		floor.transform.imgIndex = textures.size() - 1;
		gameObjects.emplace(floor.getId(), std::move(floor));

		//TODO: load game objects from map on disk
		//MapManager mapManager("Main Map", "mainMap.json");
		//mapManager.loadMap("mainMap.json");
	}
}