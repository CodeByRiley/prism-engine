#include "GameInspectorUI.h"
#include "../../utils/Logger.h"
#include <cstring>

// ImGui includes
#include "imgui.h"

// Define component interfaces for the custom game components
// This allows us to handle them without circular dependencies
struct PlayerComponentInterface {
    float speed;
    float directionX, directionY;
    float sizeX, sizeY;
};

struct ObstacleComponentInterface {
    float sizeX, sizeY;
};

struct LightComponentInterface {
    float radius;
    float colorR, colorG, colorB;
    float intensity;
};

struct InputComponentInterface {
    bool enabled;
};

GameInspectorUI::GameInspectorUI() : InspectorUI() {
    // Constructor
}

void GameInspectorUI::Render(Scene* scene) {
    // Call the base class render method
    InspectorUI::Render(scene);
}

void GameInspectorUI::DrawEntityInspector(Entity& entity) {
    EntityID entityID = entity.GetID();
    ImGui::Text("Entity ID: %u", entityID);
    
    // Entity actions
    if (ImGui::Button("Destroy Entity")) {
        if (m_entityDestructionCallback) {
            m_entityDestructionCallback(entityID);
            m_selectedEntityID = INVALID_ENTITY_ID;
            return;
        } else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(Callback not set)");
        }
    }
    
    ImGui::Separator();
    ImGui::Text("Components:");
    
    // First draw the standard components using the base class method
    InspectorUI::DrawEntityInspector(entity);
    
    // Then draw the game-specific components
    DrawGameComponents(entity);
}

void GameInspectorUI::DrawGameComponents(Entity& entity) {
    // Handle custom game components
    // Since we can't easily include the game component headers without circular dependencies,
    // we'll use a registration-based approach or component interfaces
    
    ImGui::Separator();
    ImGui::Text("Game Components:");
    
    // For now, we'll show placeholder information and suggest how to extend this
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Custom components would be shown here");
    ImGui::Text("To add custom component inspection:");
    
    if (ImGui::TreeNode("Implementation Options")) {
        ImGui::BulletText("Option 1: Component Registration System");
        ImGui::Indent();
        ImGui::TextWrapped("Register component types with custom draw functions");
        ImGui::Unindent();
        
        ImGui::BulletText("Option 2: Component Callbacks");
        ImGui::Indent();
        ImGui::TextWrapped("Pass drawing callbacks from the game class");
        ImGui::Unindent();
        
        ImGui::BulletText("Option 3: Include Game Components");
        ImGui::Indent();
        ImGui::TextWrapped("Include game component headers directly (may cause circular deps)");
        ImGui::Unindent();
        
        ImGui::BulletText("Option 4: Reflection System");
        ImGui::Indent();
        ImGui::TextWrapped("Use RTTI or custom reflection to discover components");
        ImGui::Unindent();
        
        ImGui::TreePop();
    }
    
    // Show a message about how to enable custom component inspection
    if (ImGui::Button("Enable Custom Components")) {
        ImGui::OpenPopup("Custom Components Help");
    }
    
    if (ImGui::BeginPopup("Custom Components Help")) {
        ImGui::Text("To enable custom component inspection:");
        ImGui::Separator();
        ImGui::TextWrapped("1. The Game class should register component draw functions");
        ImGui::TextWrapped("2. Use SetComponentDrawCallback() for each component type");
        ImGui::TextWrapped("3. Or extend GameInspectorUI with game-specific includes");
        ImGui::EndPopup();
    }
} 