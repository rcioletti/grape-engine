#pragma once
#include "renderer/camera.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/device.hpp"
#include "renderer/frame_info.hpp"
#include "scene/game_object.hpp"
#include <memory>
#include <vector>

namespace grape {

    // Debug system enums and structs
    enum class DebugMode : int {
        NORMAL = 0,
        SHOW_NORMALS = 1,
        SHOW_UVS = 2,
        SHOW_WORLD_POS = 3,
        SHOW_LIGHT_DISTANCE = 4,
        SHOW_LIGHT_DIRECTION = 5,
        SHOW_DOT_PRODUCT = 6,
        TEXTURE_ONLY = 7,
        LIGHTING_ONLY = 8
    };

    struct DebugSettings {
        DebugMode currentMode = DebugMode::NORMAL;
        bool showWireframe = false;
        bool showPhysicsDebug = false;

        // Singleton pattern for easy access
        static DebugSettings& getInstance() {
            static DebugSettings instance;
            return instance;
        }
    };

    // Updated push constant structure
    struct SimplePushConstantData {
        glm::mat4 modelMatrix{ 1.f };
        glm::mat4 normalMatrix{ 1.f };
        alignas(4) int imgIndex{ 0 };
        alignas(4) int debugMode{ 0 };  // Added debug mode
    };

    class SimpleRenderSystem {
    public:
        SimpleRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~SimpleRenderSystem();
        SimpleRenderSystem(const SimpleRenderSystem&) = delete;
        SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;
        void renderGameObjects(FrameInfo& frameInfo);

    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);
        void createWireframePipeline(VkRenderPass renderPass);  // Optional: for wireframe support

        Device& grapeDevice;
        std::unique_ptr<Pipeline> grapePipeline;
        std::unique_ptr<Pipeline> grapeWireframePipeline;  // Optional: for wireframe support
        VkPipelineLayout pipelineLayout;
    };
}