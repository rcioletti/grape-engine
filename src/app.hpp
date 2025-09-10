#pragma once
#include "window.hpp"
#include "device.hpp"
#include "game_object.hpp"
#include "renderer.hpp"
#include "descriptors.hpp"
#include "physics.hpp"
#include "viewport_renderer.hpp"
#include "texture.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "game_object_loader.hpp"

namespace grape {
    class App {
    public:
        static constexpr int WIDTH = 1280;
        static constexpr int HEIGHT = 720;

        App();
        ~App();
        App(const App&) = delete;
        App& operator=(const App&) = delete;

        void run();

    private:

        // Core engine components
        Window grapeWindow{ WIDTH, HEIGHT, "Grape Engine" };
        Device grapeDevice{ grapeWindow };
        Renderer grapeRenderer{ grapeWindow, grapeDevice };

        // Descriptor pools
        std::unique_ptr<DescriptorPool> globalPool{};
        std::unique_ptr<DescriptorPool> imGuiImagePool{};
        VkDescriptorPool imguiDescriptorPool;

        // Game objects and textures
        GameObjectLoader loader{};
        GameObject::Map gameObjects;

        // Physics
        Physics physics;

        // You might want to define this constant
        static constexpr int MAX_TEXTURES_IN_DESCRIPTOR_SET = 32;
    };
}