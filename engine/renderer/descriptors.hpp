#pragma once

#include "device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace grape {

    class DescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(Device& grapeDevice) : grapeDevice{ grapeDevice } {}

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1);

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                VkDescriptorBindingFlagsEXT flags,
                uint32_t count = 1);

            Builder& setDescriptorBindingFlags(VkDescriptorBindingFlagsEXT flags);

            std::unique_ptr<DescriptorSetLayout> build() const;

        private:
            Device& grapeDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
            std::unordered_map<uint32_t, VkDescriptorBindingFlagsEXT> perBindingFlags{};
            VkDescriptorBindingFlagsEXT globalBindingFlags = 0;
        };

        DescriptorSetLayout(
            Device& grapeDevice,
            const VkDescriptorSetLayoutCreateInfo& createInfo,
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings,
            std::unordered_map<uint32_t, VkDescriptorBindingFlagsEXT> bindingFlags);
        ~DescriptorSetLayout();
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

        bool hasVariableDescriptorCount() const;
        uint32_t getVariableDescriptorBinding() const;

    private:
        Device& grapeDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
        std::unordered_map<uint32_t, VkDescriptorBindingFlagsEXT> bindingFlags;

        friend class DescriptorWriter;
    };

    class DescriptorPool {
    public:
        class Builder {
        public:
            Builder(Device& grapeDevice) : grapeDevice{ grapeDevice } {}

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<DescriptorPool> build() const;

        private:
            Device& grapeDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        DescriptorPool(
            Device& grapeDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~DescriptorPool();
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        bool allocateDescriptor(
            const VkDescriptorSetLayout descriptorSetLayout,
            VkDescriptorSet& descriptor,
            uint32_t variableDescriptorCount = 0) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();

        VkDescriptorPool getDescriptorPool() const { return descriptorPool; }

    private:
        Device& grapeDevice;
        VkDescriptorPool descriptorPool;

        friend class DescriptorWriter;
    };

    class DescriptorWriter {
    public:
        DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

        DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);
        DescriptorWriter& writeImages(uint32_t binding, uint32_t descriptorCount, VkDescriptorImageInfo* pImageInfos);

        bool build(VkDescriptorSet& set, uint32_t variableDescriptorCount = 0);
        void overwrite(VkDescriptorSet& set);

    private:
        DescriptorSetLayout& setLayout;
        DescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

}