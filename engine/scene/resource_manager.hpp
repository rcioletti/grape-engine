#pragma once
#include "renderer/descriptors.hpp"
#include "renderer/buffer.hpp"
#include "renderer/texture.hpp"
#include "renderer/frame_info.hpp"
#include "game_object_loader.hpp"
#include "game_object.hpp"
#include <vector>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>

namespace grape {
    struct GlobalUbo;

    class ResourceManager {
    public:
        ResourceManager(Device& device);
        ~ResourceManager() = default;

        void setupDescriptors(const std::unordered_map<GameObject::id_t, GameObject>& gameObjects, const GameObjectLoader& loader);
        void updateUBO(int frameIndex, const GlobalUbo& ubo);

        std::unique_ptr<DescriptorSetLayout>& getGlobalSetLayout() { return globalSetLayout; }
        std::vector<VkDescriptorSet>& getGlobalDescriptorSets() { return globalDescriptorSets; }
        VkDescriptorSet getGlobalDescriptorSet(int frameIndex) const { return globalDescriptorSets[frameIndex]; }

    private:
        void createDescriptorPools();
        void createUBOBuffers();
        void createDescriptorSetLayout();
        void createDescriptorSets(const std::unordered_map<GameObject::id_t, GameObject>& gameObjects, const GameObjectLoader& loader);
        std::vector<std::string> collectUniqueTexturePaths(const std::unordered_map<GameObject::id_t, GameObject>& gameObjects);

        Device& device;
        std::unique_ptr<DescriptorPool> globalPool;
        std::unique_ptr<DescriptorPool> imGuiImagePool;
        std::unique_ptr<DescriptorSetLayout> globalSetLayout;
        std::vector<std::unique_ptr<Buffer>> uboBuffers;
        std::vector<VkDescriptorSet> globalDescriptorSets;
    };
}