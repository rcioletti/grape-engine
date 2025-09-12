#include "ui.hpp"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "systems/simple_render_system.hpp"

#include <stdexcept>
#include <unordered_map>

namespace grape {

namespace {
    // Store the descriptor pool so we can destroy it on shutdown
    VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
    bool imguiInitialized = false;
    VkDevice imguiDevice = VK_NULL_HANDLE;
    std::unordered_map<uint32_t, GameObject>* s_gameObjects = nullptr;
}

void UI::init(GLFWwindow* window, VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice,
              VkRenderPass renderPass, VkQueue queue, uint32_t imageCount) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // Make all backgrounds semi-transparent black
    style.Colors[ImGuiCol_WindowBg]           = ImVec4(0.01f, 0.01f, 0.01f, 1.f);
    style.Colors[ImGuiCol_ChildBg]            = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_PopupBg]            = ImVec4(0.01f, 0.01f, 0.01f, 0.98f);
    style.Colors[ImGuiCol_Border]             = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_MenuBarBg]          = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_TitleBg]            = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0, 0, 0, 0.7f);

    // Set tab colors to semi-transparent black
    style.Colors[ImGuiCol_Tab]                = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_TabActive]          = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_TabHovered]         = ImVec4(0, 0, 0, 0.9f);
    style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_Button] = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0, 0, 0, 0.5f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0, 0, 0, 0.7f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0, 0, 0, 0.5f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0, 0, 0, 0.7f);

    // Add these to your style.Colors setup:
    style.Colors[ImGuiCol_FrameBg]         = ImVec4(0.05f, 0.05f, 0.05f, 1.f);
    style.Colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.03f, 0.03f, 0.03f, 0.8f);
    style.Colors[ImGuiCol_FrameBgActive]   = ImVec4(0.02f, 0.02f, 0.02f, 0.8f);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForVulkan(window, true);

    // Descriptor pool for ImGui
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ImGui descriptor pool!");
    }

    ImGui_ImplVulkan_InitInfo info = {};
    info.Instance = instance;
    info.PhysicalDevice = physicalDevice;
    info.Device = device;
    info.Queue = queue;
    info.DescriptorPool = imguiDescriptorPool;
    info.RenderPass = renderPass;
    info.ImageCount = imageCount;
    info.MinImageCount = imageCount;
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.CheckVkResultFn = nullptr;

    ImGui_ImplVulkan_Init(&info);

    // Upload fonts
    // The user must submit a command buffer with ImGui_ImplVulkan_CreateFontsTexture()
    imguiInitialized = true;
    imguiDevice = device;
}   

void UI::beginFrame() {
    if (!imguiInitialized) return;
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::renderDrawData(VkCommandBuffer commandBuffer) {
    if (!imguiInitialized) return;
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    // Handle multi-viewport windows
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

ImVec2 UI::getViewportPanelSize() {
    ImGui::Begin("Viewport");
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::End();
    return size;
}

void UI::shutdown() {
    if (!imguiInitialized) return;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (imguiDevice && imguiDescriptorPool) {
        vkDestroyDescriptorPool(imguiDevice, imguiDescriptorPool, nullptr);
        imguiDescriptorPool = VK_NULL_HANDLE;
    }
    imguiInitialized = false;
}

void UI::renderUI() {
    renderDockspace();
    renderModelsPanel();
    renderDebugPanel();
}

void UI::renderDebugPanel() {
    auto& debugSettings = DebugSettings::getInstance();

    ImGui::Begin("Debug Settings");

    // Shader debug modes
    ImGui::Text("Shader Debug Modes:");
    ImGui::Separator();

    const char* debugModeNames[] = {
        "Normal Rendering",
        "Show Normals",
        "Show UVs",
        "Show World Position",
        "Show Light Distance",
        "Show Light Direction",
        "Show Dot Product",
        "Texture Only",
        "Lighting Only"
    };

    int currentModeIndex = static_cast<int>(debugSettings.currentMode);
    if (ImGui::Combo("Debug Mode", &currentModeIndex, debugModeNames, IM_ARRAYSIZE(debugModeNames))) {
        debugSettings.currentMode = static_cast<DebugMode>(currentModeIndex);
    }

    // Add helpful descriptions for each mode
    if (ImGui::IsItemHovered()) {
        const char* descriptions[] = {
            "Standard PBR rendering with lighting",
            "Visualize surface normals as RGB colors",
            "Show UV coordinates as red/green colors",
            "Display world space positions",
            "Show distance to first light source",
            "Visualize light direction vectors",
            "Show dot product between normals and light",
            "Display textures without lighting",
            "Show only lighting contribution"
        };
        ImGui::SetTooltip("%s", descriptions[currentModeIndex]);
    }

    ImGui::Separator();

    // Other debug toggles
    ImGui::Text("Rendering Options:");
    ImGui::Checkbox("Wireframe Mode", &debugSettings.showWireframe);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle wireframe rendering (requires pipeline recreation)");
    }

    ImGui::Checkbox("Physics Debug", &debugSettings.showPhysicsDebug);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show physics collision shapes and debug info");
    }

    ImGui::Separator();

    // Quick preset buttons
    ImGui::Text("Quick Presets:");
    if (ImGui::Button("Reset to Normal")) {
        debugSettings.currentMode = DebugMode::NORMAL;
        debugSettings.showWireframe = false;
        debugSettings.showPhysicsDebug = false;
    }
    ImGui::SameLine();

    if (ImGui::Button("Debug Lighting")) {
        debugSettings.currentMode = DebugMode::LIGHTING_ONLY;
        debugSettings.showWireframe = false;
    }

    if (ImGui::Button("Debug Normals")) {
        debugSettings.currentMode = DebugMode::SHOW_NORMALS;
        debugSettings.showWireframe = true;
    }
    ImGui::SameLine();

    if (ImGui::Button("Debug Textures")) {
        debugSettings.currentMode = DebugMode::TEXTURE_ONLY;
        debugSettings.showWireframe = false;
    }

    ImGui::Separator();

    // Performance info section
    ImGui::Text("Performance Info:");
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Frame Rate: %.1f FPS", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);

    // Physics debug info
    if (debugSettings.showPhysicsDebug) {
        ImGui::Separator();
        ImGui::Text("Physics Debug Info:");
        ImGui::Text("Status: Active");
        ImGui::Text("Debug Lines: %zu", 0); // You'll need to pass physics instance to get actual count
        // Once you connect physics instance:
        // ImGui::Text("Active Bodies: %d", physicsInstance->getActiveBodyCount());
        // ImGui::Text("Debug Lines: %zu", physicsInstance->getDebugLines().size());
    }

    ImGui::End();
}

void UI::renderDockspace() {
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    } else {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
            ImGui::MenuItem("Padding", NULL, &opt_padding);
            ImGui::Separator();

            if (ImGui::MenuItem("Flag: NoDockingOverCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingOverCentralNode) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingOverCentralNode; }
            if (ImGui::MenuItem("Flag: NoDockingSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingSplit) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingSplit; }
            if (ImGui::MenuItem("Flag: NoUndocking", "", (dockspace_flags & ImGuiDockNodeFlags_NoUndocking) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoUndocking; }
            if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen)) { dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
            ImGui::Separator();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void UI::renderModelsPanel() {
    if (!s_gameObjects) return;

    static int selectedIndex = 0;
    static std::vector<uint32_t> ids;
    static std::vector<std::string> names;

    // Build the list of IDs and names if the size changes
    if (ids.size() != s_gameObjects->size()) {
        ids.clear();
        names.clear();
        for (const auto& [id, obj] : *s_gameObjects) {
            ids.push_back(id);
            if (!obj.name.empty())
                names.push_back(obj.name);
            else
                names.push_back("Object " + std::to_string(id));
        }
        if (selectedIndex >= ids.size()) selectedIndex = 0;
    }

    ImGui::Begin("Models");

    if (!ids.empty()) {
        if (ImGui::Combo("Select Model", &selectedIndex,
            [](void* data, int idx, const char** out_text) {
                const auto& names = *static_cast<const std::vector<std::string>*>(data);
                *out_text = names[idx].c_str();
                return true;
            },
            (void*)&names, (int)names.size())) {
            // Selection changed
        }

        auto& obj = s_gameObjects->at(ids[selectedIndex]);

        // Show field names above sliders
        ImGui::Text("Position");
        ImGui::SliderFloat3("##Position", &obj.transform.translation.x, -10.0f, 10.0f, "%.2f");

        ImGui::Text("Rotation");
        glm::vec3 euler = glm::degrees(glm::eulerAngles(obj.transform.rotation));
        if (ImGui::SliderFloat3("##Rotation", &euler.x, -180.0f, 180.0f, "%.1f")) {
            obj.transform.rotation = glm::quat(glm::radians(euler));
        }

        ImGui::Text("Scale");
        ImGui::SliderFloat3("##Scale", &obj.transform.scale.x, 0.01f, 10.0f, "%.2f");
    } else {
        ImGui::Text("No models in scene.");
    }

    ImGui::End();
}

void UI::renderViewport(VkDescriptorSet texId, bool isValid, bool isResizing) {
    ImGui::Begin("Viewport");
    if (isValid && texId != VK_NULL_HANDLE) {
        ImVec2 displaySize = ImGui::GetContentRegionAvail();
        if (displaySize.x > 0 && displaySize.y > 0) {
            ImGui::Image(texId, displaySize);
        } else {
            ImGui::Text("Invalid panel size");
        }
    } else if (isResizing) {
        ImGui::Text("Resizing viewport...");
    } else {
        ImGui::Text("Viewport not ready");
    }
    ImGui::End();
}

void UI::setGameObjects(std::unordered_map<uint32_t, GameObject>* objects) {
    s_gameObjects = objects;
}

} // namespace grape
