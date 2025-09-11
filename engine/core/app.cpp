#include "app.hpp"
#include "ui/ui.hpp"
#include "renderer/frame_info.hpp"

#include <stdexcept>
#include <iostream>

namespace grape {
    App::App() {
        // Initialize managers
        sceneManager = std::make_unique<SceneManager>(grapeDevice, physics);
        resourceManager = std::make_unique<ResourceManager>(grapeDevice);
        cameraController = std::make_unique<CameraController>();

        // Load scene and setup resources
        sceneManager->loadScene();
        resourceManager->setupDescriptors(sceneManager->getGameObjects(), sceneManager->getLoader());

        // Initialize render manager after resources are set up
        renderManager = std::make_unique<RenderManager>(grapeDevice, grapeRenderer,
            resourceManager->getGlobalSetLayout()->getDescriptorSetLayout());

        currentTime = std::chrono::high_resolution_clock::now();
    }

    App::~App() {
        std::cout << "Starting App cleanup..." << std::endl;

        try {
            // Wait for all GPU operations to complete before cleanup
            vkDeviceWaitIdle(grapeDevice.device());

            // Clean up ImGui descriptors FIRST while ImGui context is still valid
            if (viewportRenderer) {
                std::cout << "Cleaning up ImGui descriptors..." << std::endl;
                viewportRenderer->cleanupImGuiDescriptors();
            }

            // Shutdown UI after ImGui descriptors are cleaned up
            std::cout << "Shutting down UI..." << std::endl;
            UI::shutdown();

            // Now clean up viewport renderer (Vulkan objects only)
            if (viewportRenderer) {
                std::cout << "Cleaning up viewport renderer..." << std::endl;
                viewportRenderer.reset();  // This calls ViewportRenderer destructor
            }

            // Wait again to ensure everything is done
            vkDeviceWaitIdle(grapeDevice.device());

            std::cout << "App cleanup completed" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during App cleanup: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "Unknown exception during App cleanup" << std::endl;
        }
    }

    void App::run() {
        // Initialize UI
        UI::init(
            grapeWindow.getGLFWwindow(),
            grapeDevice.getInstance(),
            grapeDevice.device(),
            grapeDevice.getPhysicalDevice(),
            grapeRenderer.getSwapChainRenderPass(),
            grapeDevice.graphicsQueue(),
            SwapChain::MAX_FRAMES_IN_FLIGHT
        );

        UI::setGameObjects(&sceneManager->getGameObjects());

        while (!grapeWindow.shoudClose()) {
            glfwPollEvents();

            // Update timing
            auto newTime = std::chrono::high_resolution_clock::now();
            frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            auto extent = grapeWindow.getExtent();
            if (extent.width == 0 || extent.height == 0) {
                continue;
            }

            // Update systems
            cameraController->update(grapeWindow.getGLFWwindow(), frameTime, grapeRenderer.getAspectRatio());
            sceneManager->updateScene(frameTime, grapeWindow.getGLFWwindow());

            updateViewport();
            renderFrame();
        }

        vkDeviceWaitIdle(grapeDevice.device());
    }

    void App::updateViewport() {
        if (needsViewportResize) {
            vkDeviceWaitIdle(grapeDevice.device());
            viewportExtent = pendingViewportExtent;
            viewportRenderer->resize(viewportExtent);
            needsViewportResize = false;
        }

        UI::beginFrame();
        UI::renderUI();

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
                viewportExtent = { newWidth, newHeight };
                viewportRenderer = std::make_unique<ViewportRenderer>(grapeDevice, viewportExtent);
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to create ViewportRenderer: " << e.what() << std::endl;
            }
        }

        if (viewportRenderer && newWidth > 0 && newHeight > 0 &&
            (newWidth != viewportExtent.width || newHeight != viewportExtent.height)) {
            pendingViewportExtent = { newWidth, newHeight };
            needsViewportResize = true;
        }
    }

    void App::renderFrame() {
        if (auto commandBuffer = grapeRenderer.beginFrame()) {
            int frameIndex = grapeRenderer.getFrameIndex();

            FrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                cameraController->getCamera(),
                resourceManager->getGlobalDescriptorSet(frameIndex),
                sceneManager->getGameObjects(),
                [this](const std::string& texturePath) -> int {
                    return sceneManager->getLoader().getTextureDescriptorIndex(texturePath);
                }
            };

            // Update UBO
            GlobalUbo ubo{};
            ubo.projection = cameraController->getCamera().getProjection();
            ubo.view = cameraController->getCamera().getView();
            ubo.inverseView = cameraController->getCamera().getInverseView();

            renderManager->updateLights(frameInfo, ubo);
            resourceManager->updateUBO(frameIndex, ubo);

            // Render to viewport
            renderManager->render(frameInfo, viewportRenderer, needsViewportResize);

            // Render to swap chain
            grapeRenderer.beginSwapChainRenderPass(commandBuffer);

            UI::renderViewport(
                viewportRenderer ? viewportRenderer->getImGuiDescriptorSet(frameIndex) : VK_NULL_HANDLE,
                viewportRenderer && !needsViewportResize,
                needsViewportResize
            );

            UI::renderDrawData(commandBuffer);
            grapeRenderer.endSwapChainRenderPass(commandBuffer);
            grapeRenderer.endFrame();
        }
    }
}