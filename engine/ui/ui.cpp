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
    int s_selectedObjectIndex = -1;
    uint32_t s_selectedObjectId = 0;
    std::vector<std::string> s_availableMaterials = { "Default Material" };
}

void UI::init(GLFWwindow* window, VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice,
              VkRenderPass renderPass, VkQueue queue, uint32_t imageCount) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.Fonts->Clear();

    io.Fonts->AddFontFromFileTTF("../resources/fonts/Inter-VariableFont_opsz,wght.ttf", 16.0f);
    io.Fonts->Build();

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
    style.Colors[ImGuiCol_FrameBg]         = ImVec4(0.05f, 0.05f, 0.05f, 1.f);
    style.Colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.03f, 0.03f, 0.03f, 0.8f);
    style.Colors[ImGuiCol_FrameBgActive]   = ImVec4(0.02f, 0.02f, 0.02f, 0.8f);

    style.Colors[ImGuiCol_Header]          = ImVec4(0.106f, 0.0f, 0.149f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.106f, 0.0f, 0.149f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.106f, 0.0f, 0.149f, 1.0f);



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
    renderContentBrowser();
    renderSceneInspector();
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

void UI::renderContentBrowser() {
    ImGui::Begin("Content Browser");
    
	ImGui::End();
}

void UI::renderSceneInspector() {
    static std::vector<uint32_t> ids;
    static std::vector<std::string> names;

    ImGui::Begin("Scene Inspector");

    if (!s_gameObjects) {
        ImGui::Text("No game objects available.");
        ImGui::End();
        return;
    }

    // Rebuild the list if the size changes
    if (ids.size() != s_gameObjects->size()) {
        ids.clear();
        names.clear();
        for (const auto& [id, obj] : *s_gameObjects) {
            ids.push_back(id);
            if (!obj.name.empty()) {
                names.push_back(obj.name);
            }
            else {
                names.push_back("GameObject_" + std::to_string(id));
            }
        }

        // Validate selection after rebuild
        if (s_selectedObjectIndex >= (int)ids.size()) {
            s_selectedObjectIndex = -1;
            s_selectedObjectId = 0;
        }
    }

    if (!ids.empty()) {
        ImGui::Text("Scene Objects (%zu):", ids.size());
        ImGui::Separator();

        // List all objects as selectable items
        for (int i = 0; i < (int)ids.size(); ++i) {
            bool isSelected = (s_selectedObjectIndex == i);

            // Get object reference to show additional info
            auto it = s_gameObjects->find(ids[i]);
            if (it == s_gameObjects->end()) continue;

            const auto& obj = it->second;

            // Create display text with object type info
            std::string displayText = names[i];
            if (obj.model) displayText += " [Model]";
            if (obj.pointLight) displayText += " [Light]";
            if (obj.hasPhysics()) displayText += " [Physics]";

            if (ImGui::Selectable(displayText.c_str(), isSelected)) {
                s_selectedObjectIndex = i;
                s_selectedObjectId = ids[i];
            }

            // Show selection indicator
            if (isSelected) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "<--");
            }
        }

        ImGui::Separator();

        // Show selected object quick info
        if (s_selectedObjectIndex >= 0 && s_selectedObjectIndex < (int)ids.size()) {
            auto it = s_gameObjects->find(ids[s_selectedObjectIndex]);
            if (it != s_gameObjects->end()) {
                const auto& obj = it->second;
                ImGui::Text("Selected: %s", names[s_selectedObjectIndex].c_str());
                ImGui::Text("ID: %u", ids[s_selectedObjectIndex]);
                ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                    obj.transform.translation.x,
                    obj.transform.translation.y,
                    obj.transform.translation.z);
                ImGui::Text("Color: (%.2f, %.2f, %.2f)", obj.color.r, obj.color.g, obj.color.b);
            }
        }
    }
    else {
        ImGui::Text("No objects in scene.");
        s_selectedObjectIndex = -1;
        s_selectedObjectId = 0;
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

    ImGui::End();
}

void UI::renderModelsPanel() {
    ImGui::Begin("Object Viewer");

    if (!s_gameObjects) {
        ImGui::Text("No game objects available.");
        ImGui::End();
        return;
    }

    // Check if we have a valid selection
    if (s_selectedObjectIndex < 0) {
        ImGui::Text("No object selected.");
        ImGui::Text("Select an object in the Scene Inspector to edit its properties.");
        ImGui::End();
        return;
    }

    // Verify the selected object still exists
    auto it = s_gameObjects->find(s_selectedObjectId);
    if (it == s_gameObjects->end()) {
        ImGui::Text("Selected object no longer exists.");
        s_selectedObjectIndex = -1;
        s_selectedObjectId = 0;
        ImGui::End();
        return;
    }

    auto& obj = it->second;

    // Header with object info
    ImGui::Text("Editing: %s",
        obj.name.empty() ? ("GameObject_" + std::to_string(s_selectedObjectId)).c_str() : obj.name.c_str());
    ImGui::Text("ID: %u", s_selectedObjectId);
    ImGui::Separator();

    // Object Properties section
    if (ImGui::CollapsingHeader("Object Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Name editing
        static char nameBuffer[128] = "";
        static bool nameBufferInitialized = false;

        if (!nameBufferInitialized || ImGui::IsWindowAppearing()) {
            size_t len = obj.name.length();
            size_t maxCopy = (sizeof(nameBuffer) - 1 < len) ? sizeof(nameBuffer) - 1 : len;
            memcpy(nameBuffer, obj.name.c_str(), maxCopy);
            nameBuffer[maxCopy] = '\0';
            nameBufferInitialized = true;
        }

        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            obj.name = std::string(nameBuffer);
        }

        // Color picker
        ImGui::Text("Color");
        ImGui::ColorEdit3("##ObjectColor", &obj.color.r);
    }

    ImGui::Spacing();

    // Transform section
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Position");
        if (ImGui::DragFloat3("##Position", &obj.transform.translation.x, 0.1f, -100.0f, 100.0f, "%.2f")) {
            // If object has physics, update physics transform too
            if (obj.hasPhysics()) {
                obj.setPhysicsTransform(obj.transform.translation, obj.transform.rotation);
            }
        }

        ImGui::Text("Rotation (degrees)");
        glm::vec3 euler = obj.transform.getEulerDegrees();
        if (ImGui::DragFloat3("##Rotation", &euler.x, 1.0f, -180.0f, 180.0f, "%.1f")) {
            obj.transform.setEulerDegrees(euler);
            if (obj.hasPhysics()) {
                obj.setPhysicsTransform(obj.transform.translation, obj.transform.rotation);
            }
        }

        ImGui::Text("Scale");
        ImGui::DragFloat3("##Scale", &obj.transform.scale.x, 0.01f, 0.001f, 20.0f, "%.3f");
    }

    ImGui::Spacing();

    // Model and Materials section
    if (obj.model && ImGui::CollapsingHeader("Model & Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Model: %s", obj.model ? "Loaded" : "None");

        if (obj.model) {
            // For now, we'll assume single material - you can extend this based on your Model class
            static std::string currentMaterial = "Default Material";

            ImGui::Separator();
            ImGui::Text("Material Assignment:");

            if (ImGui::BeginCombo("Material", currentMaterial.c_str())) {
                for (const std::string& material : s_availableMaterials) {
                    bool isSelected = (currentMaterial == material);
                    if (ImGui::Selectable(material.c_str(), isSelected)) {
                        currentMaterial = material;
                        // Here you would apply the material to your model
                        // This depends on your Model class implementation
                        // obj.model->setMaterial(material); // Example
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Material properties button
            if (currentMaterial != "Default Material") {
                ImGui::SameLine();
                if (ImGui::SmallButton("Edit Material")) {
                    // Open material editor
                }
            }

            // Model statistics (if available)
            ImGui::Separator();
            ImGui::Text("Model Info:");
            // You can add model-specific info here based on your Model class
            // ImGui::Text("Vertices: %d", obj.model->getVertexCount());
            // ImGui::Text("Triangles: %d", obj.model->getTriangleCount());
        }
    }

    ImGui::Spacing();

    // Point Light section
    if (obj.pointLight && ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Light Properties:");
        ImGui::SliderFloat("Intensity", &obj.pointLight->lightIntensity, 0.0f, 100.0f, "%.2f");

        // You can add more light properties here
        // ImGui::ColorEdit3("Light Color", &obj.lightColor.r); // if you add this to PointLightComponent
    }

    ImGui::Spacing();

    // Physics section
    if (obj.hasPhysics() && ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& physics = *obj.physicsComponent;

        ImGui::Text("Physics Properties:");
        ImGui::Text("Type: %s", physics.isDynamic ? (physics.isKinematic ? "Kinematic" : "Dynamic") : "Static");

        if (physics.isDynamic) {
            if (ImGui::SliderFloat("Mass", &physics.mass, 0.1f, 100.0f, "%.2f")) {
                // Update mass in PhysX
                if (physics.actor) {
                    PxRigidDynamic* dynamicActor = static_cast<PxRigidDynamic*>(physics.actor);
                    PxRigidBodyExt::updateMassAndInertia(*dynamicActor, physics.mass);
                }
            }
        }

        ImGui::Separator();
        ImGui::Text("Material Properties:");

        bool materialChanged = false;
        materialChanged |= ImGui::SliderFloat("Static Friction", &physics.staticFriction, 0.0f, 2.0f, "%.2f");
        materialChanged |= ImGui::SliderFloat("Dynamic Friction", &physics.dynamicFriction, 0.0f, 2.0f, "%.2f");
        materialChanged |= ImGui::SliderFloat("Restitution", &physics.restitution, 0.0f, 1.0f, "%.2f");

        if (materialChanged && physics.shape) {
            // Update material properties in PhysX
            PxMaterial* materials[1];
            if (physics.shape->getMaterials(materials, 1) == 1) {
                materials[0]->setStaticFriction(physics.staticFriction);
                materials[0]->setDynamicFriction(physics.dynamicFriction);
                materials[0]->setRestitution(physics.restitution);
            }
        }

        // Physics controls
        ImGui::Separator();
        if (physics.isDynamic && !physics.isKinematic) {
            if (ImGui::Button("Add Force Up")) {
                if (physics.actor) {
                    PxRigidDynamic* dynamicActor = static_cast<PxRigidDynamic*>(physics.actor);
                    dynamicActor->addForce(PxVec3(0, 10.0f, 0), PxForceMode::eIMPULSE);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Velocity")) {
                if (physics.actor) {
                    PxRigidDynamic* dynamicActor = static_cast<PxRigidDynamic*>(physics.actor);
                    dynamicActor->setLinearVelocity(PxVec3(0, 0, 0));
                    dynamicActor->setAngularVelocity(PxVec3(0, 0, 0));
                }
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Object actions
    if (ImGui::CollapsingHeader("Actions")) {
        if (ImGui::Button("Duplicate Object")) {
            // Create a duplicate of this object
            // This would depend on your object creation system
        }

        ImGui::SameLine();
        if (ImGui::Button("Delete Object")) {
            // Mark for deletion - you'd handle this in your main loop
            // Don't delete immediately as we're iterating
        }

        if (!obj.hasPhysics() && ImGui::Button("Add Physics")) {
            // This would require access to your Physics system
            // obj.addPhysicsComponent(yourPhysicsSystem);
        }

        if (!obj.pointLight && ImGui::Button("Add Point Light")) {
            obj.pointLight = std::make_unique<PointLightComponent>();
        }
    }

    ImGui::End();
}

// Add this method to set available materials
void UI::setAvailableMaterials(const std::vector<std::string>& materials) {
    s_availableMaterials = materials;
    if (s_availableMaterials.empty()) {
        s_availableMaterials.push_back("Default Material");
    }
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
