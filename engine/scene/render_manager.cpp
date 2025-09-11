#include "render_manager.hpp"
#include <iostream>

namespace grape {
    RenderManager::RenderManager(Device& device, Renderer& renderer, VkDescriptorSetLayout globalSetLayout)
        : device(device), renderer(renderer),
        simpleRenderSystem(device, renderer.getSwapChainRenderPass(), globalSetLayout),
        pointLightSystem(device, renderer.getSwapChainRenderPass(), globalSetLayout) {
    }

    void RenderManager::render(FrameInfo& frameInfo, std::unique_ptr<ViewportRenderer>& viewportRenderer, bool needsViewportResize) {
        if (!needsViewportResize && viewportRenderer) {
            try {
                viewportRenderer->beginRenderPass(frameInfo.commandBuffer, frameInfo.frameIndex);
                simpleRenderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);
                viewportRenderer->endRenderPass(frameInfo.commandBuffer);
            }
            catch (const std::exception& e) {
                std::cerr << "Error during viewport rendering: " << e.what() << std::endl;
            }
        }
    }

    void RenderManager::updateLights(FrameInfo& frameInfo, GlobalUbo& ubo) {
        pointLightSystem.update(frameInfo, ubo);
    }
}