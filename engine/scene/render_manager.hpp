#pragma once
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "renderer/viewport_renderer.hpp"
#include "renderer/frame_info.hpp"
#include "renderer/renderer.hpp"

#include <memory>
#include <vulkan/vulkan.h>

namespace grape {
    class RenderManager {
    public:
        RenderManager(Device& device, Renderer& renderer, VkDescriptorSetLayout globalSetLayout);
        ~RenderManager() = default;

        void render(FrameInfo& frameInfo, std::unique_ptr<ViewportRenderer>& viewportRenderer, bool needsViewportResize);
        void updateLights(FrameInfo& frameInfo, GlobalUbo& ubo);

    private:
        SimpleRenderSystem simpleRenderSystem;
        PointLightSystem pointLightSystem;
        Device& device;
        Renderer& renderer;
    };
}