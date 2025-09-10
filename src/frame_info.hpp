#pragma once
#include "camera.hpp"
#include "game_object.hpp"
#include <vulkan/vulkan.h>
#include <functional>

namespace grape {
#define MAX_LIGHTS 10

    struct PointLight {
        glm::vec4 position{};
        glm::vec4 color{};
    };

    struct GlobalUbo {
        glm::mat4 projection{ 1.f };
        glm::mat4 view{ 1.f };
        glm::mat4 inverseView{ 1.f };
        glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };
        PointLight pointLights[MAX_LIGHTS];
        alignas(16) int numLights;
    };

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        Camera& camera;
        VkDescriptorSet globalDescriptorSet;
        GameObject::Map& gameObjects;
        std::function<int(const std::string&)> getTextureIndex; // Function to get texture index
    };
}