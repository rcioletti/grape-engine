#include "descriptors.hpp"

#include <cassert>
#include <stdexcept>
#include <array>

namespace grape {

    // *************** Descriptor Set Layout Builder *********************

    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count) {

        bindings[binding].binding = binding;
        bindings[binding].descriptorType = descriptorType;
        bindings[binding].descriptorCount = count;
        bindings[binding].stageFlags = stageFlags;

        // Set default flags (no special flags)
        perBindingFlags[binding] = 0;

        return *this;
    }

    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        VkDescriptorBindingFlagsEXT flags,
        uint32_t count) {

        bindings[binding].binding = binding;
        bindings[binding].descriptorType = descriptorType;
        bindings[binding].descriptorCount = count;
        bindings[binding].stageFlags = stageFlags;

        // Store flags for this specific binding
        perBindingFlags[binding] = flags;

        return *this;
    }

    std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build() const {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
        std::vector<VkDescriptorBindingFlagsEXT> bindingFlagsVector;

        setLayoutBindings.reserve(bindings.size());
        bindingFlagsVector.reserve(bindings.size());

        // Find the highest binding number to validate variable count placement
        uint32_t highestBinding = 0;
        bool hasVariableCount = false;

        for (auto const& pair : bindings) {
            highestBinding = std::max(highestBinding, pair.first);
            if (perBindingFlags.count(pair.first) &&
                (perBindingFlags.at(pair.first) & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)) {
                hasVariableCount = true;
            }
        }

        // Validate that variable count is only on the highest binding
        if (hasVariableCount) {
            for (auto const& pair : perBindingFlags) {
                if ((pair.second & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT) &&
                    pair.first != highestBinding) {
                    throw std::runtime_error("VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT can only be used on the highest binding!");
                }
            }
        }

        // Build the vectors in binding order
        for (auto const& pair : bindings) {
            setLayoutBindings.push_back(pair.second);

            // Get flags for this binding, default to 0 if not specified
            VkDescriptorBindingFlagsEXT flags = 0;
            if (perBindingFlags.count(pair.first)) {
                flags = perBindingFlags.at(pair.first);
            }
            bindingFlagsVector.push_back(flags);
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo{};
        bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
        bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlagsVector.size());
        bindingFlagsInfo.pBindingFlags = bindingFlagsVector.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        layoutInfo.pBindings = setLayoutBindings.data();
        layoutInfo.pNext = &bindingFlagsInfo;

        return std::make_unique<DescriptorSetLayout>(grapeDevice, layoutInfo, bindings, perBindingFlags);
    }

    // *************** Descriptor Set Layout *********************

    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::setDescriptorBindingFlags(
        VkDescriptorBindingFlagsEXT flags) {
        globalBindingFlags = flags;
        return *this;
    }

    DescriptorSetLayout::DescriptorSetLayout(
        Device& device,
        const VkDescriptorSetLayoutCreateInfo& createInfo,
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings,
        std::unordered_map<uint32_t, VkDescriptorBindingFlagsEXT> bindingFlags)
        : grapeDevice{ device }, bindings{ std::move(bindings) }, bindingFlags{ std::move(bindingFlags) } {

        if (vkCreateDescriptorSetLayout(
            grapeDevice.device(),
            &createInfo,
            nullptr,
            &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(grapeDevice.device(), descriptorSetLayout, nullptr);
    }

    bool DescriptorSetLayout::hasVariableDescriptorCount() const {
        for (const auto& pair : bindingFlags) {
            if (pair.second & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT) {
                return true;
            }
        }
        return false;
    }

    uint32_t DescriptorSetLayout::getVariableDescriptorBinding() const {
        for (const auto& pair : bindingFlags) {
            if (pair.second & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT) {
                return pair.first;
            }
        }
        return UINT32_MAX; // No variable binding found
    }

    // *************** Descriptor Pool Builder *********************

    DescriptorPool::Builder& DescriptorPool::Builder::addPoolSize(
        VkDescriptorType descriptorType, uint32_t count) {
        poolSizes.push_back({ descriptorType, count });
        return *this;
    }

    DescriptorPool::Builder& DescriptorPool::Builder::setPoolFlags(
        VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }

    DescriptorPool::Builder& DescriptorPool::Builder::setMaxSets(uint32_t count) {
        maxSets = count;
        return *this;
    }

    std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const {
        return std::make_unique<DescriptorPool>(grapeDevice, maxSets, poolFlags, poolSizes);
    }

    // *************** Descriptor Pool *********************

    DescriptorPool::DescriptorPool(
        Device& grapeDevice,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes)
        : grapeDevice{ grapeDevice } {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(grapeDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    DescriptorPool::~DescriptorPool() {
        vkDestroyDescriptorPool(grapeDevice.device(), descriptorPool, nullptr);
    }

    bool DescriptorPool::allocateDescriptor(
        const VkDescriptorSetLayout descriptorSetLayout,
        VkDescriptorSet& descriptor,
        uint32_t variableDescriptorCount) const {

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        // Handle variable descriptor count if specified
        VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
        if (variableDescriptorCount > 0) {
            variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
            variableCountInfo.descriptorSetCount = 1;
            variableCountInfo.pDescriptorCounts = &variableDescriptorCount;
            allocInfo.pNext = &variableCountInfo;
        }

        if (vkAllocateDescriptorSets(grapeDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
            return false;
        }
        return true;
    }

    void DescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
        vkFreeDescriptorSets(
            grapeDevice.device(),
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());
    }

    void DescriptorPool::resetPool() {
        vkResetDescriptorPool(grapeDevice.device(), descriptorPool, 0);
    }

    // *************** Descriptor Writer *********************

    DescriptorWriter::DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool)
        : setLayout{ setLayout }, pool{ pool } {
    }

    DescriptorWriter& DescriptorWriter::writeBuffer(
        uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    DescriptorWriter& DescriptorWriter::writeImage(
        uint32_t binding, VkDescriptorImageInfo* imageInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    DescriptorWriter& DescriptorWriter::writeImages(uint32_t binding, uint32_t descriptorCount, VkDescriptorImageInfo* pImageInfos) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = binding;
        write.descriptorType = setLayout.bindings.at(binding).descriptorType;
        write.descriptorCount = descriptorCount;
        write.pImageInfo = pImageInfos;

        writes.push_back(write);
        return *this;
    }

    bool DescriptorWriter::build(VkDescriptorSet& set, uint32_t variableDescriptorCount) {
        bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set, variableDescriptorCount);
        if (!success) {
            return false;
        }
        overwrite(set);
        return true;
    }

    void DescriptorWriter::overwrite(VkDescriptorSet& set) {
        for (auto& write : writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(pool.grapeDevice.device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

}