#include "resource_manager.hpp"
#include "renderer/swap_chain.hpp"
#include "renderer/descriptors.hpp"

#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace grape {
    ResourceManager::ResourceManager(Device& device) : device(device) {
        createDescriptorPools();
        createUBOBuffers();
        createDescriptorSetLayout();
    }

    void ResourceManager::createDescriptorPools() {
        globalPool = DescriptorPool::Builder(device)
            .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * 64)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .build();

        imGuiImagePool = DescriptorPool::Builder(device)
            .setMaxSets(1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .build();
    }

    void ResourceManager::createUBOBuffers() {
        uboBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<Buffer>(
                device,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                device.properties.limits.minUniformBufferOffsetAlignment
            );
            uboBuffers[i]->map();
        }
    }

    void ResourceManager::createDescriptorSetLayout() {
        globalSetLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .addBinding(
                1,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT,
                64)
            .build();
    }

    void ResourceManager::setupDescriptors(const std::unordered_map<GameObject::id_t, GameObject>& gameObjects, const GameObjectLoader& loader) {
        createDescriptorSets(gameObjects, loader);
    }

    std::vector<std::string> ResourceManager::collectUniqueTexturePaths(const std::unordered_map<GameObject::id_t, GameObject>& gameObjects) {
        std::vector<std::string> allUniqueTexturePaths;
        for (auto const& [id, obj] : gameObjects) {
            if (obj.model) {
                const auto& modelPaths = obj.model->getTexturePaths();
                for (const auto& path : modelPaths) {
                    if (!path.empty()) {
                        if (std::find(allUniqueTexturePaths.begin(), allUniqueTexturePaths.end(), path) == allUniqueTexturePaths.end()) {
                            allUniqueTexturePaths.push_back(path);
                        }
                    }
                }
            }
        }
        return allUniqueTexturePaths;
    }

    void ResourceManager::createDescriptorSets(const std::unordered_map<GameObject::id_t, GameObject>& gameObjects, const GameObjectLoader& loader) {
        globalDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

        // Get ordered texture paths from loader instead of collecting them here
        const auto& orderedTexturePaths = loader.getOrderedTexturePaths();

        // Get fallback texture from loader (index 0 is always fallback)
        const Texture* fallbackTexture = loader.getTextureAtIndex(0);

        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            const int MAX_TEXTURES_IN_SET = 20;
            std::vector<VkDescriptorImageInfo> descriptorInfos(MAX_TEXTURES_IN_SET);

            // Setup fallback texture at index 0
            descriptorInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorInfos[0].sampler = fallbackTexture->getTextureSampler();
            descriptorInfos[0].imageView = fallbackTexture->getTextureImageView();

            // Setup actual textures
            for (uint32_t j = 1; j < MAX_TEXTURES_IN_SET; j++) {
                const Texture* texture = loader.getTextureAtIndex(j);
                descriptorInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                descriptorInfos[j].sampler = texture->getTextureSampler();
                descriptorInfos[j].imageView = texture->getTextureImageView();
            }

            uint32_t textureCount = static_cast<uint32_t>(std::min(static_cast<size_t>(MAX_TEXTURES_IN_SET), orderedTexturePaths.size() + 1));

            DescriptorWriter writer(*globalSetLayout, *globalPool);
            if (!writer.writeBuffer(0, &bufferInfo)
                .writeImages(1, textureCount, descriptorInfos.data())
                .build(globalDescriptorSets[i], textureCount)) {
                throw std::runtime_error("Failed to allocate descriptor sets!");
            }
        }
    }

    void ResourceManager::updateUBO(int frameIndex, const GlobalUbo& ubo) {
        uboBuffers[frameIndex]->writeToBuffer(const_cast<void*>(static_cast<const void*>(&ubo)));
        uboBuffers[frameIndex]->flush();
    }
}