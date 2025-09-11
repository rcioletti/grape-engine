#pragma once

#include "device.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace grape {

    class ViewportRenderer {
    public:
        ViewportRenderer(Device& device, VkExtent2D extent);
        ~ViewportRenderer();

        ViewportRenderer(const ViewportRenderer&) = delete;
        ViewportRenderer& operator=(const ViewportRenderer&) = delete;

        void resize(VkExtent2D newExtent);

        // Render pass control
        void beginRenderPass(VkCommandBuffer cmd, uint32_t frameIndex);
        void endRenderPass(VkCommandBuffer cmd);

        // For ImGui::Image()
        VkDescriptorSet getImGuiDescriptorSet(uint32_t frameIndex);

        VkFormat findDepthFormat();

        VkRenderPass getRenderPass() const { return renderPass; }

        void cleanupImGuiDescriptors();

    private:
        Device& device;

        VkExtent2D extent{};
        VkRenderPass renderPass{ VK_NULL_HANDLE };

        std::vector<VkImage> images;
        std::vector<VkDeviceMemory> memories;
        std::vector<VkImageView> imageViews;
        std::vector<VkFramebuffer> framebuffers;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthMemories;
        std::vector<VkImageView> depthImageViews;

        VkSampler sampler{ VK_NULL_HANDLE };

        bool imguiDescriptorsCleanedUp = false;

        void createResources();
        void cleanup();
    };

} // namespace grape
