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
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * 2)
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
		for (auto& texture : textures){
			texture.cleanup();
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

		SimpleRenderSystem simpleRenderSystem{ grapeDevice, grapeRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
		PointLightSystem pointLightSystem{ grapeDevice, grapeRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        Camera camera{};
        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = GameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

		initImGui();

		//createViewportImages();

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


			//Nvidia PhysX setup FIX LATER
			//physics.StepPhysics(frameTime);

			//gameObjects.at(3).transform.translation = {body->getGlobalPose().p.x, -body->getGlobalPose().p.y, body->getGlobalPose().p.z };
			//glm::quat q(body->getGlobalPose().q.x, body->getGlobalPose().q.y, body->getGlobalPose().q.z, body->getGlobalPose().q.w);
			//glm::vec3 euler = glm::eulerAngles(q);
			//gameObjects.at(3).transform.rotation = { euler.x, -euler.y, euler.z };
			
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
				ubo.inverseView = camera.getProjection();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				grapeRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);

				ImGui_ImplVulkan_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				UI::renderUI();

				ImGui::Begin("Viewport");
				
				//ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
				//ImGui::Image(dSet[frameInfo.frameIndex], ImVec2{viewportPanelSize.x, viewportPanelSize.y});

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
		std::shared_ptr<Model> grapeModel = Model::createModelFromFile(grapeDevice, "models/smooth_vase.obj");

		//vase
		auto smoothVase = GameObject::createGameObject();
		smoothVase.model = grapeModel;
		smoothVase.transform.translation = { .0f, .5f, 0.f };
		smoothVase.transform.scale = glm::vec3(3.f);
		Texture smoothVaseTexture = Texture(grapeDevice);
		smoothVaseTexture.createTextureFromFile("textures/ceramic.jpg");
		textures.push_back(smoothVaseTexture);
		smoothVase.imgIndex = static_cast<uint32_t>(textures.size() - 1);
		gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

		//chair
		grapeModel = Model::createModelFromFile(grapeDevice, "models/chair.obj");
		auto chair = GameObject::createGameObject();
		chair.model = grapeModel;
		chair.transform.translation = { 0.f, 0.1f, 0.f };
		chair.transform.rotation = { 0.f, 4.0f, 3.15f };
		chair.transform.scale = glm::vec3(.001f);
		Texture chairTexture = Texture(grapeDevice);
		chairTexture.createTextureFromFile("textures/chair.png");
		textures.push_back(chairTexture);
		chair.imgIndex = static_cast<uint32_t>(textures.size() - 1);
		gameObjects.emplace(chair.getId(), std::move(chair));

		//floor
		grapeModel = Model::createModelFromFile(grapeDevice, "models/quad.obj");
		auto floor = GameObject::createGameObject();
		floor.model = grapeModel;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = glm::vec3(6.f, 2.f, 6.f);
		Texture floorTexture = Texture(grapeDevice);
		floorTexture.createTextureFromFile("textures/wood_floor.jpg");
		textures.push_back(floorTexture);
		floor.imgIndex = static_cast<uint32_t>(textures.size() - 1);
		gameObjects.emplace(floor.getId(), std::move(floor));

		grapeModel = Model::createModelFromFile(grapeDevice, "models/cube.obj");
		auto box = GameObject::createGameObject();
		box.model = grapeModel;
		box.transform.translation = { 0.f, -5.f, 0.f };
		box.transform.scale = glm::vec3(0.5f, 0.5f, 0.5f);
		gameObjects.emplace(box.getId(), std::move(box));

		//PxShape* shape = physics.CreateBoxShape(0.5f, 0.5f, 0.5f, box.transform.toPxTransform(), physics.GetDefaultMaterial());
		//body = physics.CreateRigidDynamic(TransformComponent().mat4(), shape);

		//PxShape* shape2 = physics.CreateBoxShape(0.5f, 0.5f, 0.5f, PxTransform(), physics.GetDefaultMaterial());
		//PxRigidDynamic* body2 = physics.CreateRigidDynamic(TransformComponent().mat4(), shape2);

		std::vector<glm::vec3> lightColors{
		  {1.f, .1f, .1f},
		  {.1f, .1f, 1.f},
		  {.1f, 1.f, .1f},
		  {1.f, 1.f, .1f},
		  {.1f, 1.f, 1.f},
		  {1.f, 1.f, 1.f}  //
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
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 0.0f;
		}

		ImGui_ImplGlfw_InitForVulkan(grapeWindow.getGLFWwindow(), true);

		ImGui_ImplVulkan_InitInfo info = {};
		info.DescriptorPool = globalPool->getDescriptorPool();
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
	}

	void App::createViewportImages()
	{
		//Create and render vulkan image to ImGui window
		VkSampler viewportSampler;
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = grapeDevice.properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(grapeDevice.device(), &samplerInfo, nullptr, &viewportSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		std::vector<VkImage> viewportImages;
		std::vector<VkDeviceMemory> dstImageMemory;
		std::vector<VkImageView> viewportImageViews;

		viewportImages.resize(grapeRenderer.getSwapChainImageCount());
		dstImageMemory.resize(grapeRenderer.getSwapChainImageCount());

		//Render Images to ImGui window

		for (uint32_t i = 0; i < grapeRenderer.getSwapChainImageCount(); i++) {
			Texture t = Texture(grapeDevice);
			t.createImage(512,
				512,
				VK_FORMAT_B8G8R8A8_SRGB,
				VK_IMAGE_TILING_LINEAR,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				viewportImages[i],
				dstImageMemory[i]);

			t.transitionImageLayout(viewportImages[i], VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		viewportImageViews.resize(viewportImages.size());

		for (uint32_t i = 0; i < viewportImages.size(); i++) {
			Texture t = Texture(grapeDevice);
			viewportImageViews[i] = t.createImageView(viewportImages[i], VK_FORMAT_B8G8R8A8_SRGB);
		}

		std::vector<VkDescriptorSet> dSet;
		dSet.resize(viewportImageViews.size());

		//grapeRenderer.getSwapChainImageView();

		for (uint32_t i = 0; i < viewportImageViews.size(); i++) {
			dSet[i] = ImGui_ImplVulkan_AddTexture(viewportSampler, viewportImageViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}
}