#pragma once
#include "window.hpp"
#include "renderer/device.hpp"
#include "renderer/renderer.hpp"
#include "renderer/viewport_renderer.hpp"

#include "scene/scene_manager.hpp"
#include "scene/resource_manager.hpp"
#include "scene/camera_controller.hpp"
#include "scene/render_manager.hpp"

#include "systems/physics.hpp"

#include <memory>
#include <chrono>

namespace grape {
    class App {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        App();
        ~App();

        App(const App&) = delete;
        App& operator=(const App&) = delete;

        void run();

    private:
        void updateViewport();
        void renderFrame();

        // Core systems
        Window grapeWindow{ WIDTH, HEIGHT, "Grape Engine" };
        Device grapeDevice{ grapeWindow };
        Renderer grapeRenderer{ grapeWindow, grapeDevice };
        Physics physics{};

        // Managers
        std::unique_ptr<SceneManager> sceneManager;
        std::unique_ptr<ResourceManager> resourceManager;
        std::unique_ptr<CameraController> cameraController;
        std::unique_ptr<RenderManager> renderManager;

        // Viewport management
        std::unique_ptr<ViewportRenderer> viewportRenderer;
        VkExtent2D viewportExtent = { 1280, 720 };
        VkExtent2D pendingViewportExtent = { 1280, 720 };
        bool needsViewportResize = false;

        // Timing
        std::chrono::high_resolution_clock::time_point currentTime;
        float frameTime = 0.0f;
    };
}