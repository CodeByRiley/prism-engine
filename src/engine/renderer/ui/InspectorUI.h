#pragma once

#include <memory>
#include <string>
#include <GLFW/glfw3.h>
#include <engine/scene/Scene.h>
#include <engine/scene/entity/Entity.h>
#include <engine/scene/component/Component.h>

// Forward declarations to avoid full ImGui include in header
struct ImGuiIO;

class InspectorUI {
public:
    InspectorUI();
    virtual ~InspectorUI();
    
    // Initialization and cleanup
    bool Initialize(GLFWwindow* window);
    void Shutdown();
    
    // Rendering
    virtual void Render(Scene* scene);
    virtual void RenderContent(Scene* scene);  // Renders content without frame management
    
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