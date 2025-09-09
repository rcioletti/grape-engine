#include "app.hpp"

#include "camera.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "keyboard_movement_controller.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "map_manager.hpp"
#include "ui.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>

static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

namespace grape {

	App::App()
	{
		globalPool = DescriptorPool::Builder(grapeDevice)
			.setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT * 2)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * 64)
			.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
			.build();

		imGuiImagePool = DescriptorPool::Builder(grapeDevice)
			.setMaxSets(1000)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
			.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
			.build();

		loadGameObjects();
	}

	App::~App()
	{
		vkDeviceWaitIdle(grapeDevice.device());

		// Cleanup textures from the map
		loadedTextures.clear(); // This calls the destructors for all unique_ptr<Texture> objects

		// Use UI class for ImGui shutdown
		UI::shutdown();
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
			// Binding 0 for UBO, no variable descriptor count flag
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)

			// Binding 1 for textures, this is the last binding and gets the variable count flag
			.addBinding(
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT,
				64)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
		std::cout << "Created globalDescriptorSets with size: " << globalDescriptorSets.size() << std::endl;

		// --- Collect All Unique Textures from All GameObjects ---
		std::vector<std::string> allUniqueTexturePaths;
		for (auto const& [id, obj] : gameObjects) {
			if (obj.model) {
				const auto& modelPaths = obj.model->getTexturePaths();
				for (const auto& path : modelPaths) {
					if (!path.empty()) {
						bool found = false;
						for (const auto& uniquePath : allUniqueTexturePaths) {
							if (uniquePath == path) {
								found = true;
								break;
							}
						}
						if (!found) {
							allUniqueTexturePaths.push_back(path);
						}
					}
				}
			}
		}

		std::unique_ptr<Texture> defaultTexture;
		if (loadedTextures.empty()) {
			defaultTexture = std::make_unique<Texture>(grapeDevice);
			defaultTexture->createTextureFromColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
		const Texture* fallbackTexture = loadedTextures.empty() ? defaultTexture.get() : loadedTextures.begin()->second.get();

		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			const int MAX_TEXTURES_IN_SET = 20;
			std::vector<VkDescriptorImageInfo> descriptorInfos(MAX_TEXTURES_IN_SET);

			for (uint32_t j = 0; j < MAX_TEXTURES_IN_SET; j++) {
				if (j < allUniqueTexturePaths.size()) {
					const auto& path = allUniqueTexturePaths[j];
					if (loadedTextures.find(path) != loadedTextures.end()) {
						const auto& texture = *loadedTextures.at(path);
						descriptorInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						descriptorInfos[j].sampler = texture.getTextureSampler();
						descriptorInfos[j].imageView = texture.getTextureImageView();
					}
					else {
						descriptorInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						descriptorInfos[j].sampler = fallbackTexture->getTextureSampler();
						descriptorInfos[j].imageView = fallbackTexture->getTextureImageView();
					}
				}
				else {
					descriptorInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					descriptorInfos[j].sampler = fallbackTexture->getTextureSampler();
					descriptorInfos[j].imageView = fallbackTexture->getTextureImageView();
				}
			}

			uint32_t textureCount = static_cast<uint32_t>(std::min(static_cast<size_t>(MAX_TEXTURES_IN_SET), allUniqueTexturePaths.size()));

			DescriptorWriter writer(*globalSetLayout, *globalPool);

			if (!writer.writeBuffer(0, &bufferInfo)
				.writeImages(1, textureCount, descriptorInfos.data())
				.build(globalDescriptorSets[i], textureCount)) {
				throw std::runtime_error("Failed to allocate descriptor sets!");
			}
		}

		SimpleRenderSystem simpleRenderSystem{ grapeDevice, grapeRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		PointLightSystem pointLightSystem{ grapeDevice, grapeRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		Camera camera{};
		camera.setViewTarget(glm::vec3(-1.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f));

		auto viewerObject = GameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
		KeyboardMovementController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		// Use UI class for ImGui initialization
		UI::init(
			grapeWindow.getGLFWwindow(),
			grapeDevice.getInstance(),
			grapeDevice.device(),
			grapeDevice.getPhysicalDevice(),
			grapeRenderer.getSwapChainRenderPass(),
			grapeDevice.graphicsQueue(),
			SwapChain::MAX_FRAMES_IN_FLIGHT
		);

		// After loading game objects, before the main loop:
		UI::setGameObjects(&gameObjects);

		VkExtent2D viewportExtent = { 1280, 720 };  // Default size
		VkExtent2D pendingViewportExtent = { 1280, 720 };
		bool needsViewportResize = false;
		std::unique_ptr<ViewportRenderer> viewportRenderer = nullptr;

		while (!grapeWindow.shoudClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();

			currentTime = newTime;

			glm::vec3 cameraPosition = viewerObject.transform.translation;
			glm::vec3 forwardDirection = glm::normalize(viewerObject.transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f));

			cameraController.moveInPlaneXZ(grapeWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewDirection(cameraPosition, forwardDirection, glm::vec3(0.0f, -1.0f, 0.0f));

			float aspect = grapeRenderer.getAspectRatio();
			camera.setPerspectiveProjection(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

			// PHYSICS UPDATE
			physics.StepPhysics(frameTime);

			for (auto& kv : gameObjects) {
				auto& obj = kv.second;
				if (obj.hasPhysics()) {
					obj.updatePhysics();
				}
			}

			for (auto& kv : gameObjects) {
				auto& obj = kv.second;
				if (obj.hasPhysics() && obj.physicsComponent->isKinematic) {
					glm::vec3 movement(0.f);
					if (glfwGetKey(grapeWindow.getGLFWwindow(), GLFW_KEY_I) == GLFW_PRESS) movement.z -= 1.f;
					if (glfwGetKey(grapeWindow.getGLFWwindow(), GLFW_KEY_K) == GLFW_PRESS) movement.z += 1.f;
					if (glfwGetKey(grapeWindow.getGLFWwindow(), GLFW_KEY_J) == GLFW_PRESS) movement.x -= 1.f;
					if (glfwGetKey(grapeWindow.getGLFWwindow(), GLFW_KEY_L) == GLFW_PRESS) movement.x += 1.f;

					if (glm::length(movement) > 0.f) {
						movement = glm::normalize(movement) * frameTime * 3.f;
						glm::vec3 newPos = obj.transform.translation + movement;
						obj.setPhysicsTransform(newPos);
					}
					break;
				}
			}

			auto extent = grapeWindow.getExtent();
			if (extent.width == 0 || extent.height == 0) {
				continue;
			}

			if (needsViewportResize) {
				vkDeviceWaitIdle(grapeDevice.device());
				viewportExtent = pendingViewportExtent;
				viewportRenderer->resize(viewportExtent);
				needsViewportResize = false;
			}

			// Use UI class for ImGui frame begin
			UI::beginFrame();

			// --- Your custom UI ---
			UI::renderUI();

			// Get viewport size for resize detection
			ImVec2 viewportPanelSize = UI::getViewportPanelSize();

			uint32_t newWidth = 0;
			uint32_t newHeight = 0;

			if (viewportPanelSize.x > 0.0f && viewportPanelSize.y > 0.0f &&
				viewportPanelSize.x <= 16384.0f && viewportPanelSize.y <= 16384.0f) {
				newWidth = static_cast<uint32_t>(viewportPanelSize.x);
				newHeight = static_cast<uint32_t>(viewportPanelSize.y);
			}

			if (!viewportRenderer && newWidth > 0 && newHeight > 0) {
				try {
					std::cout << "Creating ViewportRenderer with size: " << newWidth << "x" << newHeight << std::endl;
					viewportExtent = { newWidth, newHeight };
					viewportRenderer = std::make_unique<ViewportRenderer>(grapeDevice, viewportExtent);
					std::cout << "ViewportRenderer created successfully" << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "Failed to create ViewportRenderer: " << e.what() << std::endl;
				}
			}

			if (viewportRenderer && needsViewportResize) {
				try {
					std::cout << "Resizing viewport from " << viewportExtent.width << "x" << viewportExtent.height
						<< " to " << pendingViewportExtent.width << "x" << pendingViewportExtent.height << std::endl;
					vkDeviceWaitIdle(grapeDevice.device());
					viewportExtent = pendingViewportExtent;
					viewportRenderer->resize(viewportExtent);
					needsViewportResize = false;
					std::cout << "Viewport resize completed" << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "Error during viewport resize: " << e.what() << std::endl;
					needsViewportResize = false;
				}
			}

			if (viewportRenderer && newWidth > 0 && newHeight > 0 &&
				(newWidth != viewportExtent.width || newHeight != viewportExtent.height)) {
				pendingViewportExtent = { newWidth, newHeight };
				needsViewportResize = true;
			}

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

				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				ubo.inverseView = camera.getInverseView();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				if (!needsViewportResize && viewportRenderer) {
					try {
						viewportRenderer->beginRenderPass(commandBuffer, frameIndex);
						simpleRenderSystem.renderGameObjects(frameInfo);
						pointLightSystem.render(frameInfo);
						viewportRenderer->endRenderPass(commandBuffer);
					}
					catch (const std::exception& e) {
						std::cerr << "Error during viewport rendering: " << e.what() << std::endl;
					}
				}

				grapeRenderer.beginSwapChainRenderPass(commandBuffer);

				// UI::renderUI() already called above

				UI::renderViewport(
					viewportRenderer ? viewportRenderer->getImGuiDescriptorSet(frameIndex) : VK_NULL_HANDLE,
					viewportRenderer && !needsViewportResize,
					needsViewportResize
				);

				// Use UI class for ImGui rendering
				UI::renderDrawData(commandBuffer);

				grapeRenderer.endSwapChainRenderPass(commandBuffer);
				grapeRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(grapeDevice.device());
	}

	void App::loadGameObjects()
	{
		std::shared_ptr<Model> marioModel = Model::createModelFromFile(grapeDevice, "models/Asteroids.obj");

		const auto& modelTexturePaths = marioModel->getTexturePaths();
		for (const auto& path : modelTexturePaths) {
			if (!path.empty() && loadedTextures.find(path) == loadedTextures.end()) {
				auto newTexture = std::make_unique<Texture>(grapeDevice);
				newTexture->createTextureFromFile("textures/" + path);

				loadedTextures.emplace(path, std::move(newTexture));
			}
		}

		auto mario = GameObject::createPhysicsObject(physics, glm::vec3(0.f, -5.f, 0.f), true, false);
		mario.name = "Arcade";
		mario.model = marioModel;
		mario.transform.scale = glm::vec3(.5f);
		mario.transform.rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(1.f, 0.f, 0.f));
		mario.addBoxCollider(physics, glm::vec3(0.5f, 0.5f, 0.5f));
		gameObjects.emplace(mario.getId(), std::move(mario));

		std::vector<glm::vec3> lightColors{
		  {1.f, .1f, .1f},
		  {.1f, .1f, 1.f},
		  {.1f, 1.f, .1f},
		  {1.f, 1.f, .1f},
		  {.1f, 1.f, 1.f},
		  {1.f, 1.f, 1.f}
		};

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = GameObject::makePointLight(1.2f);
			pointLight.color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.f), 
				(i * glm::two_pi<float>()) / lightColors.size(), 
				{ 0.f, -1.f, 0.f });
			pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}

		//TODO: load game objects from map on disk
		//MapManager mapManager("Main Map", "mainMap.json");
		//mapManager.loadMap("mainMap.json");
	}
}