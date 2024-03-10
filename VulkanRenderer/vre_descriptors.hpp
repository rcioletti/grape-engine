#pragma once

#include "vre_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace vre {

    class VreDescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(VreDevice& vreDevice) : vreDevice{ vreDevice } {}

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1);
            std::unique_ptr<VreDescriptorSetLayout> build() const;

        private:
            VreDevice& vreDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };

        VreDescriptorSetLayout(
            VreDevice& vreDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~VreDescriptorSetLayout();
        VreDescriptorSetLayout(const VreDescriptorSetLayout&) = delete;
        VreDescriptorSetLayout& operator=(const VreDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        VreDevice& vreDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class VreDescriptorWriter;
    };

    class VreDescriptorPool {
    public:
        class Builder {
        public:
            Builder(VreDevice& vreDevice) : vreDevice{ vreDevice } {}

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<VreDescriptorPool> build() const;

        private:
            VreDevice& vreDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        VreDescriptorPool(
            VreDevice& vreDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~VreDescriptorPool();
        VreDescriptorPool(const VreDescriptorPool&) = delete;
        VreDescriptorPool& operator=(const VreDescriptorPool&) = delete;

        bool allocateDescriptor(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();

    private:
        VreDevice& vreDevice;
        VkDescriptorPool descriptorPool;

        friend class VreDescriptorWriter;
    };

    class VreDescriptorWriter {
    public:
        VreDescriptorWriter(VreDescriptorSetLayout& setLayout, VreDescriptorPool& pool);

        VreDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        VreDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);
        VreDescriptorWriter& writeImages(uint32_t binding, VkDescriptorImageInfo imageInfo[3]);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);

    private:
        VreDescriptorSetLayout& setLayout;
        VreDescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

}