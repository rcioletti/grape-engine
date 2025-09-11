#include "viewport_renderer.hpp"
#include "texture.hpp"
#include "imgui/imgui_impl_vulkan.h"
#include "swap_chain.hpp"

#include <stdexcept>
#include <array>
#include <iostream>

namespace grape {

    ViewportRenderer::ViewportRenderer(Device& device, VkExtent2D extent)
        : device{ device }, extent{ extent } {
        createResources();
    }

    ViewportRenderer::~ViewportRenderer() {
        cleanup();
    }

    void ViewportRenderer::createResources() {
        // --- 1. Sampler (shared for all frames) ---
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create viewport sampler!");
        }

        // --- 2. Images, Views ---
        size_t imageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
        images.resize(imageCount);
        memories.resize(imageCount);
        imageViews.resize(imageCount);
        framebuffers.resize(imageCount);
        descriptorSets.resize(imageCount);

        depthImages.resize(imageCount);
        depthMemories.resize(imageCount);
        depthImageViews.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++) {
            Texture t(device);
            t.createImage(
                extent.width,
                extent.height,
                VK_FORMAT_B8G8R8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                images[i],
                memories[i]
            );

            imageViews[i] = t.createImageView(images[i], VK_FORMAT_B8G8R8A8_SRGB);

            // ImGui descriptor for this image
            descriptorSets[i] =
                ImGui_ImplVulkan_AddTexture(sampler, imageViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create depth image, memory, and view
            VkImageCreateInfo depthImageInfo{};
            depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
            depthImageInfo.extent.width = extent.width;
            depthImageInfo.extent.height = extent.height;
            depthImageInfo.extent.depth = 1;
            depthImageInfo.mipLevels = 1;
            depthImageInfo.arrayLayers = 1;
            depthImageInfo.format = findDepthFormat();
            depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(device.device(), &depthImageInfo, nullptr, &depthImages[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create viewport depth image!");
            }

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(device.device(), depthImages[i], &memReqs);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(device.device(), &allocInfo, nullptr, &depthMemories[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate viewport depth image memory!");
            }

            vkBindImageMemory(device.device(), depthImages[i], depthMemories[i], 0);

            VkImageViewCreateInfo depthViewInfo{};
            depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            depthViewInfo.image = depthImages[i];
            depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depthViewInfo.format = findDepthFormat();
            depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthViewInfo.subresourceRange.baseMipLevel = 0;
            depthViewInfo.subresourceRange.levelCount = 1;
            depthViewInfo.subresourceRange.baseArrayLayer = 0;
            depthViewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device.device(), &depthViewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create viewport depth image view!");
            }
        }

        // --- 3. RenderPass ---
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstSubpass = 0;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        rpInfo.pAttachments = attachments.data();
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = 1;
        rpInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device.device(), &rpInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create viewport render pass!");
        }

        // --- 4. Framebuffers ---
        for (size_t i = 0; i < imageCount; i++) {
            std::array<VkImageView, 2> attachments = { imageViews[i], depthImageViews[i] };

            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = renderPass;
            fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // Correct the count
            fbInfo.pAttachments = attachments.data();
            fbInfo.width = extent.width;
            fbInfo.height = extent.height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(device.device(), &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create viewport framebuffer!");
            }
        }
    }

    VkFormat ViewportRenderer::findDepthFormat() {
        return device.findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void ViewportRenderer::cleanupImGuiDescriptors() {
        if (imguiDescriptorsCleanedUp) {
            return; // Already cleaned up
        }

        std::cout << "Cleaning up ImGui descriptors early..." << std::endl;

        // Clean up ImGui descriptors while ImGui context is still valid
        for (size_t i = 0; i < descriptorSets.size(); i++) {
            if (descriptorSets[i] != VK_NULL_HANDLE) {
                std::cout << "Removing ImGui texture " << i << std::endl;
                try {
                    ImGui_ImplVulkan_RemoveTexture(descriptorSets[i]);
                }
                catch (const std::exception& e) {
                    std::cout << "Warning: Failed to remove ImGui texture " << i << ": " << e.what() << std::endl;
                }
                catch (...) {
                    std::cout << "Warning: Unknown error removing ImGui texture " << i << std::endl;
                }
                descriptorSets[i] = VK_NULL_HANDLE;
            }
        }

        imguiDescriptorsCleanedUp = true;
        std::cout << "ImGui descriptors cleaned up successfully" << std::endl;
    }

    void ViewportRenderer::cleanup() {
        std::cout << "Starting cleanup..." << std::endl;

        if (device.device() == VK_NULL_HANDLE) {
            std::cout << "Device is null, skipping cleanup" << std::endl;
            return;
        }

        // CRITICAL: Wait for all operations to complete first
        vkDeviceWaitIdle(device.device());

        try {
            // 1. Clean up ImGui descriptors FIRST (before destroying any Vulkan objects they reference)
            for (size_t i = 0; i < descriptorSets.size(); i++) {
                if (descriptorSets[i] != VK_NULL_HANDLE) {
                    std::cout << "Removing ImGui texture " << i << std::endl;
                    try {
                        ImGui_ImplVulkan_RemoveTexture(descriptorSets[i]);
                    }
                    catch (const std::exception& e) {
                        std::cout << "Warning: Failed to remove ImGui texture " << i << ": " << e.what() << std::endl;
                    }
                    catch (...) {
                        std::cout << "Warning: Unknown error removing ImGui texture " << i << std::endl;
                    }
                    descriptorSets[i] = VK_NULL_HANDLE;
                }
            }

            // Wait again after ImGui cleanup
            vkDeviceWaitIdle(device.device());

            // 2. Destroy framebuffers (they depend on image views and render pass)
            for (size_t i = 0; i < framebuffers.size(); i++) {
                if (framebuffers[i] != VK_NULL_HANDLE) {
                    std::cout << "Destroying framebuffer " << i << std::endl;
                    vkDestroyFramebuffer(device.device(), framebuffers[i], nullptr);
                    framebuffers[i] = VK_NULL_HANDLE;
                }
            }

            // 3. Destroy render pass (framebuffers depend on it)
            if (renderPass != VK_NULL_HANDLE) {
                std::cout << "Destroying render pass" << std::endl;
                vkDestroyRenderPass(device.device(), renderPass, nullptr);
                renderPass = VK_NULL_HANDLE;
            }

            // 4. Destroy image views (images depend on them being destroyed first)
            for (size_t i = 0; i < imageViews.size(); i++) {
                if (imageViews[i] != VK_NULL_HANDLE) {
                    std::cout << "Destroying image view " << i << std::endl;
                    vkDestroyImageView(device.device(), imageViews[i], nullptr);
                    imageViews[i] = VK_NULL_HANDLE;
                }
            }

            for (size_t i = 0; i < depthImageViews.size(); i++) {
                if (depthImageViews[i] != VK_NULL_HANDLE) {
                    std::cout << "Destroying depth image view " << i << std::endl;
                    vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
                    depthImageViews[i] = VK_NULL_HANDLE;
                }
            }

            // 5. Destroy images (must be done before freeing their memory)
            for (size_t i = 0; i < images.size(); i++) {
                if (images[i] != VK_NULL_HANDLE) {
                    std::cout << "Destroying image " << i << std::endl;
                    vkDestroyImage(device.device(), images[i], nullptr);
                    images[i] = VK_NULL_HANDLE;
                }
            }

            for (size_t i = 0; i < depthImages.size(); i++) {
                if (depthImages[i] != VK_NULL_HANDLE) {
                    std::cout << "Destroying depth image " << i << std::endl;
                    vkDestroyImage(device.device(), depthImages[i], nullptr);
                    depthImages[i] = VK_NULL_HANDLE;
                }
            }

            // 6. Free memory LAST (after all objects using it are destroyed)
            for (size_t i = 0; i < memories.size(); i++) {
                if (memories[i] != VK_NULL_HANDLE) {
                    std::cout << "Freeing memory " << i << std::endl;
                    vkFreeMemory(device.device(), memories[i], nullptr);
                    memories[i] = VK_NULL_HANDLE;
                }
            }

            for (size_t i = 0; i < depthMemories.size(); i++) {
                if (depthMemories[i] != VK_NULL_HANDLE) {
                    std::cout << "Freeing depth memory " << i << std::endl;
                    vkFreeMemory(device.device(), depthMemories[i], nullptr);
                    depthMemories[i] = VK_NULL_HANDLE;
                }
            }

            // 7. Destroy sampler (independent, can be done anytime after ImGui cleanup)
            if (sampler != VK_NULL_HANDLE) {
                std::cout << "Destroying sampler" << std::endl;
                vkDestroySampler(device.device(), sampler, nullptr);
                sampler = VK_NULL_HANDLE;
            }

            // 8. Clear vectors
            images.clear();
            memories.clear();
            imageViews.clear();
            framebuffers.clear();
            descriptorSets.clear();
            depthImages.clear();
            depthMemories.clear();
            depthImageViews.clear();

            std::cout << "Cleanup completed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during cleanup: " << e.what() << std::endl;
            // Don't rethrow during destruction - log and continue
        }
        catch (...) {
            std::cerr << "Unknown exception during cleanup" << std::endl;
            // Don't rethrow during destruction - log and continue
        }
    }

    VkDescriptorSet ViewportRenderer::getImGuiDescriptorSet(uint32_t frameIndex) {
        if (descriptorSets.empty()) {
            std::cout << "descriptorSets vector is empty!" << std::endl;
            return VK_NULL_HANDLE;
        }

        if (frameIndex >= descriptorSets.size()) {
            std::cout << "frameIndex out of range!" << std::endl;
            return VK_NULL_HANDLE;
        }

        // Check if descriptor set is valid
        if (descriptorSets[frameIndex] == VK_NULL_HANDLE) {
            std::cout << "descriptor set at index " << frameIndex << " is null!" << std::endl;
            return VK_NULL_HANDLE;
        }

        return descriptorSets[frameIndex];
    }

    void ViewportRenderer::beginRenderPass(VkCommandBuffer cmd, uint32_t frameIndex) {
        if (framebuffers.empty()) {
            std::cout << "ERROR: framebuffers vector is empty!" << std::endl;
            return;
        }

        if (frameIndex >= framebuffers.size()) {
            std::cout << "ERROR: frameIndex " << frameIndex << " >= framebuffers.size() " << framebuffers.size() << std::endl;
            return;
        }

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = renderPass;

        // Use the supplied frameIndex to pick the framebuffer
        rpInfo.framebuffer = framebuffers[frameIndex];

        rpInfo.renderArea.offset = { 0, 0 };
        rpInfo.renderArea.extent = extent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{ {0,0}, extent };
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void ViewportRenderer::resize(VkExtent2D newExtent) {
        // Validate extent dimensions
        if (newExtent.width == 0 || newExtent.height == 0) {
            std::cerr << "ERROR: Invalid viewport extent (zero): " << newExtent.width << "x" << newExtent.height << std::endl;
            return;
        }

        // Don't resize if dimensions are the same
        if (newExtent.width == extent.width && newExtent.height == extent.height) {
            return;
        }

        std::cout << "Resizing viewport from " << extent.width << "x" << extent.height
            << " to " << newExtent.width << "x" << newExtent.height << std::endl;

        try {
            // CRITICAL: Ensure no operations are pending AND that we're not in the middle of another resize
            static bool resizing = false;
            if (resizing) {
                std::cout << "Resize already in progress, skipping..." << std::endl;
                return;
            }

            resizing = true;
            vkDeviceWaitIdle(device.device());
            cleanup();
            extent = newExtent;
            createResources();
            resizing = false;

            std::cout << "Viewport resize completed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR during viewport resize: " << e.what() << std::endl;
            throw;
        }
    }

    void ViewportRenderer::endRenderPass(VkCommandBuffer cmd) {
        vkCmdEndRenderPass(cmd);
    }

} // namespace grape
