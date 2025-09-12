#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace grape {

    SimpleRenderSystem::SimpleRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : grapeDevice{ device }
    {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
        createWireframePipeline(renderPass);  // Create wireframe pipeline
    }

    SimpleRenderSystem::~SimpleRenderSystem()
    {
        vkDestroyPipelineLayout(grapeDevice.device(), pipelineLayout, nullptr);
    }

    void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);  // Now includes debugMode

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(grapeDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void SimpleRenderSystem::createPipeline(VkRenderPass renderPass)
    {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        grapePipeline = std::make_unique<Pipeline>(grapeDevice, "resources/shaders/simple_shader.vert.spv", "resources/shaders/simple_shader.frag.spv", pipelineConfig);
    }

    void SimpleRenderSystem::createWireframePipeline(VkRenderPass renderPass)
    {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);

        // Enable wireframe mode
        pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
        pipelineConfig.rasterizationInfo.lineWidth = 1.0f;

        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        grapeWireframePipeline = std::make_unique<Pipeline>(grapeDevice, "resources/shaders/simple_shader.vert.spv", "resources/shaders/simple_shader.frag.spv", pipelineConfig);
    }

    void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo)
    {
        // Get debug settings
        const auto& debugSettings = DebugSettings::getInstance();

        // Choose pipeline based on wireframe mode
        if (debugSettings.showWireframe && grapeWireframePipeline) {
            grapeWireframePipeline->bind(frameInfo.commandBuffer);
        }
        else {
            grapePipeline->bind(frameInfo.commandBuffer);
        }

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr
        );

        for (auto& kv : frameInfo.gameObjects) {
            auto& obj = kv.second;
            if (obj.model == nullptr) continue;

            // Push constants for the entire object (common data)
            SimplePushConstantData push{};
            push.modelMatrix = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();
            push.debugMode = static_cast<int>(debugSettings.currentMode);  // Set debug mode

            // Get the texture paths from the model
            const auto& modelTexturePaths = obj.model->getTexturePaths();

            // Render each submesh with its corresponding texture
            for (size_t i = 0; i < obj.model->getSubmeshes().size(); ++i) {
                const auto& submesh = obj.model->getSubmeshes()[i];

                // Find the correct texture index for this material
                int textureIndex = 0; // Default fallback texture

                if (submesh.materialId >= 0 && submesh.materialId < modelTexturePaths.size()) {
                    const std::string& texturePath = modelTexturePaths[submesh.materialId];

                    // Use the function from FrameInfo to get the texture index
                    textureIndex = frameInfo.getTextureIndex(texturePath);

#ifdef DEBUG_RENDERING
                    std::cout << "Rendering submesh " << i << ": materialId=" << submesh.materialId
                        << ", texture='" << texturePath << "', descriptorIndex=" << textureIndex << std::endl;
#endif
                }

                push.imgIndex = textureIndex;

                // Bind vertex and index buffers for this submesh
                obj.model->bindSubmesh(frameInfo.commandBuffer, i);

                // Push constants for this specific submesh
                vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(SimplePushConstantData),
                    &push);

                // Draw this submesh
                obj.model->drawSubmesh(frameInfo.commandBuffer, i);
            }
        }
    }
}