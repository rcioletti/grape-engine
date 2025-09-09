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
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

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
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * 8)
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

		for (auto& texture : textures){
			texture.cleanup();
		}

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		if (imguiDescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(grapeDevice.device(), imguiDescriptorPool, nullptr);
			imguiDescriptorPool = VK_NULL_HANDLE;
		}
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
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(8))
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
		std::cout << "Created globalDescriptorSets with size: " << globalDescriptorSets.size() << std::endl;

		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();

			const int TEXTURE_ARRAY_SIZE = 8;

			VkDescriptorImageInfo descriptorInfo[TEXTURE_ARRAY_SIZE];

			for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; i++) {
				if (i < textures.size()) {
					descriptorInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					descriptorInfo[i].sampler = textures[i].getTextureSampler();
					descriptorInfo[i].imageView = textures[i].getTextureImageView();
				}
				else {
					//filling descriptors that are supposed to be empty to suppress warning FIX LATER
					descriptorInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					descriptorInfo[i].sampler = textures[0].getTextureSampler();
					descriptorInfo[i].imageView = textures[0].getTextureImageView();
				}
			}

			DescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImages(1, descriptorInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{ grapeDevice, grapeRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		PointLightSystem pointLightSystem{ grapeDevice, grapeRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		Camera camera{};
		camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

		auto viewerObject = GameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
		KeyboardMovementController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		initImGui();

		// Don't create viewport renderer here - wait for valid ImGui dimensions
		// viewportRenderer will be created in the render loop when we have valid size

		VkExtent2D viewportExtent = { 1280, 720 };  // Default size
		VkExtent2D pendingViewportExtent = { 1280, 720 };
		bool needsViewportResize = false;
		std::unique_ptr<ViewportRenderer> viewportRenderer = nullptr;

		while (!grapeWindow.shoudClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();

			currentTime = newTime;

			cameraController.moveInPlaneXZ(grapeWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, glm::eulerAngles(viewerObject.transform.rotation));

			float aspect = grapeRenderer.getAspectRatio();
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1f, 100.f);

			// PHYSICS UPDATE
			physics.StepPhysics(frameTime);

			// Update all physics-enabled game objects
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

			// Handle viewport resize BEFORE starting ImGui frame
			if (needsViewportResize) {
				vkDeviceWaitIdle(grapeDevice.device());
				viewportExtent = pendingViewportExtent;
				viewportRenderer->resize(viewportExtent);
				needsViewportResize = false;
			}

			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Get viewport size for resize detection
			ImGui::Begin("Viewport");
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			ImGui::End();

			// Validate the viewport size before casting to uint32_t
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

			// Handle viewport resize AFTER we have a valid renderer
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
					needsViewportResize = false; // Reset flag to prevent continuous retry
				}
			}

			// Check for resize and set flag for next frame
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

				// update
				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				ubo.inverseView = camera.getInverseView();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render to viewport (skip if resizing)
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

				UI::renderUI();

				ImGui::Begin("Viewport");

				// Only display image if we have a valid renderer and not currently resizing
				if (viewportRenderer && !needsViewportResize) {
					VkDescriptorSet texId = viewportRenderer->getImGuiDescriptorSet(frameIndex);
					if (texId != VK_NULL_HANDLE) {
						// Use the actual panel size, but make sure it's valid
						ImVec2 displaySize = ImGui::GetContentRegionAvail();
						if (displaySize.x > 0 && displaySize.y > 0) {
							ImGui::Image(texId, displaySize);
						}
						else {
							ImGui::Text("Invalid panel size");
						}
					}
					else {
						ImGui::Text("Invalid texture descriptor");
					}
				}
				else if (needsViewportResize) {
					ImGui::Text("Resizing viewport...");
				}
				else {
					ImGui::Text("Viewport not ready");
				}

				ImGui::End();
				ImGui::SetNextWindowBgAlpha(1.0f);
				ImGui::Begin("Models");
				ImGui::Text("3D Model");
				ImGui::End();

				ImGui::Render();
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();

				grapeRenderer.endSwapChainRenderPass(commandBuffer);
				grapeRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(grapeDevice.device());
	}

	void App::loadGameObjects()
	{
		std::shared_ptr<Model> grapeModel = Model::createModelFromFile(grapeDevice, "models/Mario.obj");

		//mario
		auto mario = GameObject::createPhysicsObject(physics, glm::vec3(0.f, 5.f, 0.f), true, false);
		mario.model = grapeModel;
		mario.transform.scale = glm::vec3(.5f);
		mario.transform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.f, 0.f, 0.f));
		mario.addBoxCollider(physics, glm::vec3(0.5f, 0.5f, 0.5f));
		Texture marioTexture = Texture(grapeDevice);
		marioTexture.createTextureFromFile("textures/ceramic.jpg");
		textures.push_back(marioTexture);
		mario.imgIndex = static_cast<uint32_t>(textures.size() - 1);
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

	void App::initImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f; // Make fully opaque
		}

		ImGui_ImplGlfw_InitForVulkan(grapeWindow.getGLFWwindow(), true);

		// Create a separate, larger descriptor pool for ImGui
		VkDescriptorPool imguiDescriptorPool;
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		if (vkCreateDescriptorPool(grapeDevice.device(), &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create ImGui descriptor pool!");
		}

		ImGui_ImplVulkan_InitInfo info = {};
		info.DescriptorPool = imguiDescriptorPool;  // Use the new ImGui-specific pool
		info.RenderPass = grapeRenderer.getSwapChainRenderPass();
		info.Device = grapeDevice.device();
		info.PhysicalDevice = grapeDevice.getPhysicalDevice();
		info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
		info.MinImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		info.Queue = grapeDevice.graphicsQueue();
		info.CheckVkResultFn = check_vk_result;
		info.Instance = grapeDevice.getInstance();

		ImGui_ImplVulkan_Init(&info);

		this->imguiDescriptorPool = imguiDescriptorPool;
	}
}