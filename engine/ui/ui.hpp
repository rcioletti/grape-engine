#pragma once

#include <vulkan/vulkan.h>
#include "imgui/imgui.h"
#include "scene/game_object.hpp"

#include <unordered_map>
struct GLFWwindow;

namespace grape {

class UI {

public:
    // Call once at startup
    static void init(GLFWwindow* window, VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice,
                     VkRenderPass renderPass, VkQueue queue, uint32_t imageCount);

    // Call at the start of each frame
    static void beginFrame();

    // Call after all ImGui:: windows are built, before rendering
    static void renderDrawData(VkCommandBuffer commandBuffer);

    // Call once at shutdown
    static void shutdown();

    // Your custom UI logic (menus, dockspace, etc)
    static void renderUI();

    static void renderDebugPanel();

    static void renderContentBrowser();

    static void renderSceneInspector();

    static ImVec2 getViewportPanelSize();

    static void renderDockspace();
    static void renderViewport(VkDescriptorSet texId, bool isValid, bool isResizing);
    static void renderModelsPanel();

    static void setGameObjects(std::unordered_map<uint32_t, GameObject>* objects);

    static void setAvailableMaterials(const std::vector<std::string>& materials);
};

}