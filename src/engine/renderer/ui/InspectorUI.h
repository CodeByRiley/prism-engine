#pragma once

#include <memory>
#include <string>
#include <GLFW/glfw3.h>
#include <engine/scene/Scene.h>
#include <engine/scene/entity/Entity.h>
#include <engine/scene/component/Component.h>
#include <glm/glm.hpp>
#include <imgui.h>

// Forward declarations to avoid full ImGui include in header
// struct ImGuiIO;
// struct ImVec2;
// struct ImVec4;
// struct ImTextureID;
// struct ImGuiInputTextFlags;
// struct ImGuiTabBarFlags;
// struct ImGuiTabItemFlags;
// struct ImGuiTableFlags;
// struct ImGuiTableRowFlags;
// struct ImGuiID;
// struct ImGuiWindowFlags;
// struct ImGuiTableColumnFlags;
// struct ImGuiCond;

class InspectorUI {
public:
    InspectorUI() = default;
    virtual ~InspectorUI();
    
    // Initialization and cleanup
    bool Initialize(GLFWwindow* window);
    void Shutdown();
    
    // Rendering
    virtual void Render(Scene* scene);
    virtual void RenderContent(Scene* scene);  // Renders content without frame management
    
    // ImGui Wrappers
    // bool    Button(const char* label);
    // bool    Button(const char* label, const ImVec2& size);
    // bool    InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0);
    // bool    Checkbox(const char* label, bool* v);
    // void    InputInt(const char* label, int* v);
    // void    InputFloat(const char* label, float* v);
    // void    InputDouble(const char* label, double* v);
    // void    TextBox(const char* fmt, ...);
    // void    TextBoxColored(const ImVec4& color, const char* fmt, ...);
    // void    TextBoxUnformatted(const char* text);
    // void    TextBoxWrapped(const char* fmt, ...);
    // void    TextBoxColoredWrapped(const ImVec4& color, const char* fmt, ...);
    // void    TextBoxUnformattedWrapped(const char* fmt, ...);
    // void    SliderFloat(const char* label, float* v, float speed = 1.0f, float min = 0.0f, float max = 0.0f);
    // void    Vector2(const char* label, glm::vec2* v);
    // void    Vector3(const char* label, glm::vec3* v);
    // void    ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_items = -1);
    // void    SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);
    // void    SetTooltip(const char* text);
    // void    Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
    // bool    ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
    // void    Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_items = -1);
    // void    Combo(const char* label, int* current_item, const std::vector<std::string>& items);
    // void    SetNextWindowSize(const ImVec2& size, ImGuiCond cond = 0);
    // bool    Begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
    // bool    BeginTabBar(const char* str_id, ImGuiTabBarFlags flags = 0);
    // bool    BeginTabItem(const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0);
    // void    SetNextItemWidth(float item_width);
    // void    Columns(int count, const char* id = nullptr, bool border = true);
    // bool    BeginChild(const char* str_id, const ImVec2& size = ImVec2(0,0), bool border = false, ImGuiWindowFlags flags = 0);
    // bool    BeginTable(const char* str_id, int columns, ImGuiTableFlags flags = 0);
    // void    TableSetupColumn(const char* label, ImGuiTableColumnFlags flags = 0, float init_width_or_weight = 0.0f, ImGuiID user_id = 0);
    // void    TableNextRow(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f);
    // void    SetScrollHereY(float center_y_ratio = 0.5f);
    // bool    BeginMenu(const char* label, const char* shortcut = NULL, bool selected = false, bool enabled = true);
    // void    BeginChildFrame(const char* str_id, const ImVec2& size = ImVec2(0, 0), ImGuiWindowFlags flags = 0);
    // void    TableHeadersRow();
    // void    TableNextColumn();
    // void    BeginDockSpace();
    // void    EndChildFrame();
    // float   GetScrollMaxY();
    // void    EndDockSpace();
    // void    EndTabItem();
    // void    NextColumn();
    // float   GetScrollY();
    // void    BeginGroup();
    // void    EndTabBar();
    // void    Separator();
    // void    EndTable();
    // void    EndChild();
    // void    EndGroup();
    // void    EndMenu();
    // void    Spacing();
    // void    End();


    // State management
    void SetVisible(bool visible) { m_showInspector = visible; }
    bool IsVisible() const { return m_showInspector; }
    void ToggleVisibility() { m_showInspector = !m_showInspector; }
    
    // Scene management
    void SetScene(Scene* scene) { m_currentScene = scene; }
    
    // Get initialization status
    bool IsInitialized() const { return m_initialized; }

protected:
    // Internal rendering methods (protected so they can be overridden)
    virtual void DrawEntityInspector(Entity& entity);
    virtual void DrawComponentInspector(const std::string& componentName, Component* component);
    void RenderEntityList(Scene* scene);
    void RenderEntityDetails(Scene* scene);
    
    // State variables (protected so derived classes can access them)
    bool m_showInspector = true;
    bool m_initialized = false;
    EntityID m_selectedEntityID = INVALID_ENTITY_ID;
    Scene* m_currentScene = nullptr;
    
    // Window reference (for ImGui backend)
    GLFWwindow* m_window = nullptr;
}; 