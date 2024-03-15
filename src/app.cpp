#include "app.hpp"

#include "camera.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "keyboard_movement_controller.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "map_manager.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace grape {

	struct GlobalUbo {
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };
		glm::vec3 lightPosition{ -1.f };
		alignas(16) glm::vec4 lightColor{ 1.f };
	};

	App::App()
	{
		globalPool = DescriptorPool::Builder(grapeDevice)
			.setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();

		loadGameObjects();
	}

	App::~App()
	{
	
	}

	void App::run()
	{
		std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<Buffer>(
				grapeDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				grapeDevice.properties.limits.minUniformBufferOffsetAlignment
				);
			uboBuffers[i]->map();
		}

		auto globalSetLayout = DescriptorSetLayout::Builder(grapeDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(textures.size()))
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();

			const int TEXTURE_ARRAY_SIZE = 8;

			VkDescriptorImageInfo descriptorInfo[TEXTURE_ARRAY_SIZE];

			for (uint32_t i = 0; i < textures.size(); i++) {

				descriptorInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				descriptorInfo[i].sampler = textures[i].getTextureSampler();
				descriptorInfo[i].imageView = textures[i].getTextureImageView();
			}

			DescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImages(1, descriptorInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{ grapeDevice, grapeRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
		PointLightSystem pointLightSystem{ grapeDevice, grapeRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        Camera camera{};
        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = GameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

		while (!grapeWindow.shoudClose()) {
			glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();

            currentTime = newTime;

            cameraController.moveInPlaneXZ(grapeWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = grapeRenderer.getAspectRatio();
            //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1f, 100.f);
			
			if (auto commandBuffer = grapeRenderer.beginFrame()) {
				
				int frameIndex = grapeRenderer.getFrameIndex();
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
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				grapeRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);
				grapeRenderer.endSwapChainRenderPass(commandBuffer);
				grapeRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(grapeDevice.device());
	}

	void App::loadGameObjects()
	{
		std::shared_ptr<Model> grapeModel = Model::createModelFromFile(grapeDevice, "models/smooth_vase.obj");

		//vase
        auto smoothVase = GameObject::createGameObject();
		smoothVase.model = grapeModel;
		smoothVase.transform.translation = { .0f, .5f, 0.f };
		smoothVase.transform.scale = glm::vec3(3.f);
		Texture smoothVaseTexture = Texture(grapeDevice, "textures/ceramic.jpg");
		textures.push_back(smoothVaseTexture);
		smoothVase.imgIndex = static_cast<uint32_t>(textures.size() - 1);
        gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

		//chair
		grapeModel = Model::createModelFromFile(grapeDevice, "models/chair.obj");
		auto chair = GameObject::createGameObject();
		chair.model = grapeModel;
		chair.transform.translation = { -1.f, 0.5f, 0.f };
		chair.transform.rotation = { 0.f, 4.5f, 3.15f };
		chair.transform.scale = glm::vec3(.01f);
		Texture chairTexture = Texture(grapeDevice, "textures/chair.jpg");
		textures.push_back(chairTexture);
		chair.imgIndex = static_cast<uint32_t>(textures.size() - 1);
		gameObjects.emplace(chair.getId(), std::move(chair));

		//floor
		grapeModel = Model::createModelFromFile(grapeDevice, "models/quad.obj");
		auto floor = GameObject::createGameObject();
		floor.model = grapeModel;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = glm::vec3(3.f, 1.f, 3.f);
		Texture floorTexture = Texture(grapeDevice, "textures/concrete.png");
		textures.push_back(floorTexture);
		floor.imgIndex = static_cast<uint32_t>(textures.size() - 1);
		gameObjects.emplace(floor.getId(), std::move(floor));

		//TODO: load game objects from map on disk
		//MapManager mapManager("Main Map", "mainMap.json");
		//mapManager.loadMap("mainMap.json");
	}
}