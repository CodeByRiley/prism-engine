#include "InspectorUI.h"
#include "../../utils/Logger.h"
#include "../../scene/component/CommonComponents.h"
#include <cstring>
#include <engine/core/audio/AudioManager.h>

// ImGui includes
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Forward declarations for custom components
class PlayerComponent;
class ObstacleComponent;
class LightComponent;
class InputComponent;

InspectorUI::~InspectorUI() {
    Shutdown();
}

bool InspectorUI::Initialize(GLFWwindow* window) {
    if (m_initialized) {
        Logger::Info("InspectorUI already initialized");
        return true;
    }
    
    if (!window) {
        Logger::Error<InspectorUI>("Invalid window pointer provided to InspectorUI::Initialize", this);
        return false;
    }
    
    m_window = window;
    
    Logger::Info("Initializing ImGui Inspector UI");
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = NULL;


    // Ensure GLFW window context is current
    glfwMakeContextCurrent(m_window);
    
    // Setup Platform/Renderer backends
    bool glfwResult = ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    if (!glfwResult) {
        Logger::Error<InspectorUI>("Failed to initialize ImGui GLFW backend", this);
        return false;
    }
    
    bool openglResult = ImGui_ImplOpenGL3_Init("#version 330 core");
    if (!openglResult) {
        Logger::Error<InspectorUI>("Failed to initialize ImGui OpenGL3 backend", this);
        ImGui_ImplGlfw_Shutdown();
        return false;
    }
    
    m_initialized = true;
    Logger::Info("ImGui Inspector UI initialized successfully");
    return true;
}

void InspectorUI::Shutdown() {
    if (m_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
        m_window = nullptr;
        Logger::Info("ImGui Inspector UI shutdown complete");
    }
}

void InspectorUI::Render(Scene* scene) {
    if (!m_initialized) {
        Logger::Info("InspectorUI not initialized, skipping render");
        return;
    }
    
    if (!scene) {
        Logger::Info("No scene provided to InspectorUI::Render");
        return;
    }
    
    if (!m_showInspector) {
        return; // Inspector is hidden
    }
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Render content
    RenderContent(scene);
    
    // Render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void InspectorUI::RenderContent(Scene* scene) {
    if (!m_initialized) {
        return;
    }
    
    if (!scene) {
        return;
    }
    
    if (!m_showInspector) {
        return; // Inspector is hidden
    }
    
    // Main Inspector Window
    if (ImGui::Begin("ECS Inspector", &m_showInspector)) {
        ImGui::Text("Scene: %s (ID: %u)", scene->GetName().c_str(), scene->GetId());
        ImGui::Separator();
        
        // Entity List
        RenderEntityList(scene);
        
        ImGui::Separator();
        
        // Entity Details
        RenderEntityDetails(scene);
    }
    ImGui::End();
}

void InspectorUI::RenderEntityList(Scene* scene) {
    ImGui::Text("Entities (%zu total):", scene->GetAllEntities().size());
    
    auto entities = scene->GetAllEntities();
    for (auto& entity : entities) {
        EntityID entityID = entity.GetID();
        
        // Get entity name
        std::string entityName = "Entity_" + std::to_string(entityID);
        auto* tagComp = entity.GetComponent<TagComponent>();
        if (tagComp && !tagComp->tag.empty()) {
            entityName = tagComp->tag + " (" + std::to_string(entityID) + ")";
        }
        
        // Selectable entity entry
        bool isSelected = (m_selectedEntityID == entityID);
        if (ImGui::Selectable(entityName.c_str(), isSelected)) {
            m_selectedEntityID = entityID;
        }
    }
}

void InspectorUI::RenderEntityDetails(Scene* scene) {
    if (m_selectedEntityID != INVALID_ENTITY_ID) {
        Entity selectedEntity = scene->GetEntity(m_selectedEntityID);
        if (selectedEntity.IsValid()) {
            DrawEntityInspector(selectedEntity);
        } else {
            ImGui::Text("Selected entity is no longer valid.");
            m_selectedEntityID = INVALID_ENTITY_ID;
        }
    } else {
        ImGui::Text("Select an entity to view its components.");
    }
}

void InspectorUI::DrawEntityInspector(Entity& entity) {
    EntityID entityID = entity.GetID();
    ImGui::Text("Entity ID: %u", entityID);
    
    // Entity actions
    if (ImGui::Button("Destroy Entity")) {
        // Note: We can't directly destroy the entity here since we don't own the scene
        // The caller will need to handle this. For now, just deselect.
        ImGui::Text("Entity destruction must be handled by the game logic");
        m_selectedEntityID = INVALID_ENTITY_ID;
        return;
    }
    
    ImGui::Separator();
    ImGui::Text("Components:");
    
    // Transform Component
    if (auto* transform = entity.GetComponent<TransformComponent>()) {
        if (ImGui::CollapsingHeader("TransformComponent")) {
            ImGui::DragFloat3("Position", &transform->position.x, 1.0f);
            ImGui::DragFloat3("Rotation", &transform->rotation.x, 0.1f);
            ImGui::DragFloat3("Scale", &transform->scale.x, 0.1f, 0.1f);
        }
    }
    
    // Renderable Component
    if (auto* renderable = entity.GetComponent<RenderableComponent>()) {
        if (ImGui::CollapsingHeader("RenderableComponent")) {
            ImGui::Checkbox("Visible", &renderable->visible);
            ImGui::ColorEdit4("Color", &renderable->color.x);
            ImGui::DragInt("Render Layer", &renderable->renderLayer);
            
            char meshBuffer[256];
            strncpy(meshBuffer, renderable->meshName.c_str(), sizeof(meshBuffer));
            meshBuffer[sizeof(meshBuffer) - 1] = '\0';
            if (ImGui::InputText("Mesh Name", meshBuffer, sizeof(meshBuffer))) {
                renderable->meshName = std::string(meshBuffer);
            }
            
            char materialBuffer[256];
            strncpy(materialBuffer, renderable->materialName.c_str(), sizeof(materialBuffer));
            materialBuffer[sizeof(materialBuffer) - 1] = '\0';
            if (ImGui::InputText("Material Name", materialBuffer, sizeof(materialBuffer))) {
                renderable->materialName = std::string(materialBuffer);
            }
        }
    }
    
    // Tag Component
    if (auto* tag = entity.GetComponent<TagComponent>()) {
        if (ImGui::CollapsingHeader("TagComponent")) {
            char buffer[256];
            strncpy(buffer, tag->tag.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';
            if (ImGui::InputText("Tag", buffer, sizeof(buffer))) {
                tag->tag = std::string(buffer);
            }
        }
    }
    
    // Physics Component
    if (auto* physics = entity.GetComponent<PhysicsComponent>()) {
        if (ImGui::CollapsingHeader("PhysicsComponent")) {
            ImGui::DragFloat3("Velocity", &physics->velocity.x, 1.0f);
            ImGui::DragFloat3("Acceleration", &physics->acceleration.x, 1.0f);
            ImGui::DragFloat("Mass", &physics->mass, 0.1f, 0.1f, 100.0f);
            ImGui::DragFloat("Drag", &physics->drag, 0.01f, 0.0f, 1.0f);
            ImGui::Checkbox("Use Gravity", &physics->useGravity);
        }
    }
    
    // Camera Component
    if (auto* camera = entity.GetComponent<CameraComponent>()) {
        if (ImGui::CollapsingHeader("CameraComponent")) {
            ImGui::Checkbox("Is Primary", &camera->isPrimary);
            ImGui::DragFloat("FOV", &camera->fov, 1.0f, 1.0f, 180.0f);
            ImGui::DragFloat("Near Plane", &camera->nearPlane, 0.1f, 0.1f, 100.0f);
            ImGui::DragFloat("Far Plane", &camera->farPlane, 10.0f, 1.0f, 10000.0f);
        }
    }
    
    // Audio Component
    if (auto* audio = entity.GetComponent<AudioComponent>()) {
        if (ImGui::CollapsingHeader("AudioComponent")) {
            char audioBuffer[256];
            strncpy(audioBuffer, audio->audioClipName.c_str(), sizeof(audioBuffer));
            audioBuffer[sizeof(audioBuffer) - 1] = '\0';
            if (ImGui::InputText("Audio Clip", audioBuffer, sizeof(audioBuffer))) {
                audio->audioClipName = std::string(audioBuffer);
            }
            
            ImGui::DragFloat("Volume", &audio->volume, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Pitch", &audio->pitch, 0.01f, 0.1f, 3.0f);
            ImGui::Checkbox("Is Looping", &audio->isLooping);
            ImGui::Checkbox("Play On Create", &audio->playOnCreate);
            ImGui::Checkbox("3D Audio", &audio->is3D);
            if (audio->is3D) {
                ImGui::DragFloat("Min Distance", &audio->minDistance, 1.0f, 0.0f, 1000.0f);
                ImGui::DragFloat("Max Distance", &audio->maxDistance, 10.0f, 0.0f, 10000.0f);
            }
        }
    }
    
    // Try to get custom components via component type checking
    // This is a bit of a workaround since we can't easily get component type info at runtime
    // For now, we'll check for the specific game components using their names
    
    // We need to add the custom component types here, but we need access to them
    // This will be handled by checking if the entity has these components
    
    // Note: Custom components would need to be handled by passing the entity to a callback
    // or by having a registration system for component inspectors
}

void InspectorUI::DrawComponentInspector(const std::string& componentName, Component* component) {
    // This method can be used for more advanced component editing if needed
    ImGui::Text("Component: %s", componentName.c_str());
    ImGui::Text("Advanced component editing not yet implemented");
}


// ImGui Wrappers Implementation
// Why do I spend 2 hours doing this then fucking remove its
// void InspectorUI::BeginDockSpace() {
//     ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
// }

// void InspectorUI::EndDockSpace() {
//     // No-op for symmetry
// }

// bool InspectorUI::Button(const char* label) {
//     bool pressed = ImGui::Button(label);
//     if (pressed)  {
//         Audio::PlaySound("gui_click");
//         Logger::Info("Button pressed: " + std::string(label));
//     }
//     return pressed;
// }

// bool InspectorUI::Checkbox(const char* label, bool* v) {
//     bool changed = ImGui::Checkbox(label, v);
//     if (changed) Audio::PlaySound("gui_click");
//     return changed;
// }

// bool InspectorUI::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags) {
//     bool changed = ImGui::InputText(label, buf, buf_size, flags);
//     if (changed) Audio::PlaySound("gui_click");
//     return changed;
// }

// void InspectorUI::InputInt(const char* label, int* v) {
//     if (ImGui::InputInt(label, v)) {
//         Audio::PlaySound("gui_click");
//     }
// }

// void InspectorUI::InputFloat(const char* label, float* v) {
//     if (ImGui::InputFloat(label, v)) {
//         Audio::PlaySound("gui_click");
//     }
// }

// void InspectorUI::InputDouble(const char* label, double* v) {
//     if (ImGui::InputDouble(label, v)) {
//         Audio::PlaySound("gui_click");
//     }
// }

// void InspectorUI::TextBox(const char* fmt, ...) {
//     char buf[512];
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buf, sizeof(buf), fmt, args);
//     va_end(args);
//     ImGui::Text("%s", buf);
// }

// void InspectorUI::TextBoxColored(const ImVec4& color, const char* fmt, ...) {
//     char buf[512];
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buf, sizeof(buf), fmt, args);
//     va_end(args);
//     ImGui::TextColored(color, "%s", buf);
// }

// void InspectorUI::TextBoxUnformatted(const char* text) {
//     ImGui::TextUnformatted(text);
// }

// void InspectorUI::TextBoxWrapped(const char* fmt, ...) {
//     char buf[512];
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buf, sizeof(buf), fmt, args);
//     va_end(args);
//     ImGui::TextWrapped("%s", buf);
// }

// void InspectorUI::TextBoxColoredWrapped(const ImVec4& color, const char* fmt, ...) {
//     char buf[512];
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buf, sizeof(buf), fmt, args);
//     va_end(args);
//     ImGui::PushStyleColor(ImGuiCol_Text, color);
//     ImGui::TextWrapped("%s", buf);
//     ImGui::PopStyleColor();
// }

// void InspectorUI::TextBoxUnformattedWrapped(const char* fmt, ...) {
//     char buf[512];
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buf, sizeof(buf), fmt, args);
//     va_end(args);
//     ImGui::PushTextWrapPos(0.0f);
//     ImGui::TextUnformatted(buf);
//     ImGui::PopTextWrapPos();
// }

// void InspectorUI::SliderFloat(const char* label, float* v, float speed, float min, float max) {
//     if (ImGui::SliderFloat(label, v, min, max, "%.3f", speed)) {
//         Audio::PlaySound("gui_click");
//     }
// }

// void InspectorUI::Vector2(const char* label, glm::vec2* v) {
//     if (ImGui::DragFloat2(label, &(*v)[0], 1.0f)) {
//         Audio::PlaySound("gui_click");
//     }
// }

// void InspectorUI::Vector3(const char* label, glm::vec3* v) {
//     if (ImGui::DragFloat3(label, &(*v)[0], 1.0f)) {
//         Audio::PlaySound("gui_click");
//     }
// }

// void InspectorUI::ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_items) {
//     if (ImGui::ListBox(label, current_item, items, items_count, height_items)) {
//         Audio::PlaySound("gui_click");
//     }
// }

// void InspectorUI::SameLine(float offset_from_start_x, float spacing) {
//     ImGui::SameLine(offset_from_start_x, spacing);
// }

// void InspectorUI::SetTooltip(const char* text) {
//     if (ImGui::IsItemHovered()) {
//         ImGui::SetTooltip("%s", text);
//     }
// }

// void InspectorUI::Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col) {
//     ImGui::Image(user_texture_id, size, uv0, uv1, tint_col, border_col);
// }

// bool InspectorUI::ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col) {
//     bool pressed = ImGui::ImageButton(user_texture_id, size, uv0, uv1, frame_padding, bg_col, tint_col);
//     if (pressed) Audio::PlaySound("gui_click");
//     return pressed;
// }

// void InspectorUI::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_items) {
//     if (ImGui::Combo(label, current_item, items, items_count, height_items)) {
//         Audio::PlaySound("gui_click");
//     }
// }

// void InspectorUI::Combo(const char* label, int* current_item, const std::vector<std::string>& items) {
//     std::vector<const char*> cstrs;
//     cstrs.reserve(items.size());
//     for (const auto& s : items) cstrs.push_back(s.c_str());
//     if (ImGui::Combo(label, current_item, cstrs.data(), static_cast<int>(cstrs.size()))) {
//         Audio::PlaySound("gui_click");
//     }
// }

// bool InspectorUI::BeginMenu(const char* label, const char* shortcut, bool selected, bool enabled) {
//     bool opened = ImGui::BeginMenu(label, enabled);
//     if (opened && ImGui::IsItemClicked()) Audio::PlaySound("gui_click");
//     return opened;
// }

// void InspectorUI::EndMenu() {
//     ImGui::EndMenu();
// }

// bool InspectorUI::BeginChild(const char* str_id, const ImVec2& size, bool border, ImGuiWindowFlags flags) {
//     return ImGui::BeginChild(str_id, size, border, flags);
// }

// void InspectorUI::EndChild() {
//     ImGui::EndChild();
// }

// void InspectorUI::BeginChildFrame(const char* str_id, const ImVec2& size, ImGuiWindowFlags flags) {
//     ImGui::BeginChild(str_id, size, false, flags);
// }

// void InspectorUI::EndChildFrame() {
//     ImGui::EndChild();
// }

// void InspectorUI::BeginGroup() {
//     ImGui::BeginGroup();
// }

// void InspectorUI::EndGroup() {
//     ImGui::EndGroup();
// }

// void InspectorUI::Separator() {
//     ImGui::Separator();
// }

// void InspectorUI::Spacing() {
//     ImGui::Spacing();
// } 

// bool InspectorUI::Button(const char* label, const ImVec2& size) {
//     bool pressed = ImGui::Button(label, size);
//     if (pressed) Audio::PlaySound("gui_click");
//     return pressed;
// }

// void InspectorUI::SetNextWindowSize(const ImVec2& size, ImGuiCond cond) {
//     ImGui::SetNextWindowSize(size, cond);
// }

// bool InspectorUI::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags) {
//     return ImGui::Begin(name, p_open, flags);
// }

// void InspectorUI::End() {
//     ImGui::End();
// }

// bool InspectorUI::BeginTabBar(const char* str_id, ImGuiTabBarFlags flags) {
//     return ImGui::BeginTabBar(str_id, flags);
// }

// void InspectorUI::EndTabBar() {
//     ImGui::EndTabBar();
// }

// bool InspectorUI::BeginTabItem(const char* label, bool* p_open, ImGuiTabItemFlags flags) {
//     return ImGui::BeginTabItem(label, p_open, flags);
// }

// void InspectorUI::EndTabItem() {
//     ImGui::EndTabItem();
// }

// void InspectorUI::SetNextItemWidth(float item_width) {
//     ImGui::SetNextItemWidth(item_width);
// }

// void InspectorUI::Columns(int count, const char* id, bool border) {
//     ImGui::Columns(count, id, border);
// }

// void InspectorUI::NextColumn() {
//     ImGui::NextColumn();
// }

// bool InspectorUI::BeginTable(const char* str_id, int columns, ImGuiTableFlags flags) {
//     return ImGui::BeginTable(str_id, columns, flags);
// }

// void InspectorUI::TableSetupColumn(const char* label, ImGuiTableColumnFlags flags, float init_width_or_weight, ImGuiID user_id) {
//     ImGui::TableSetupColumn(label, flags, init_width_or_weight, user_id);
// }

// void InspectorUI::TableHeadersRow() {
//     ImGui::TableHeadersRow();
// }

// void InspectorUI::TableNextRow(ImGuiTableRowFlags row_flags, float min_row_height) {
//     ImGui::TableNextRow(row_flags, min_row_height);
// }

// void InspectorUI::TableNextColumn() {
//     ImGui::TableNextColumn();
// }

// void InspectorUI::EndTable() {
//     ImGui::EndTable();
// }

// float InspectorUI::GetScrollY() {
//     return ImGui::GetScrollY();
// }

// float InspectorUI::GetScrollMaxY() {
//     return ImGui::GetScrollMaxY();
// }

// void InspectorUI::SetScrollHereY(float center_y_ratio) {
//     ImGui::SetScrollHereY(center_y_ratio);
// } 