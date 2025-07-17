#pragma once

#include "InspectorUI.h"
#include <functional>

// Forward declarations for game-specific components
class PlayerComponent;
class ObstacleComponent;
class LightComponent;
class InputComponent;

class GameInspectorUI : public InspectorUI {
public:
    GameInspectorUI();
    ~GameInspectorUI() override = default;
    
    // Override to handle game-specific components
    void Render(Scene* scene) override;
    
    // Add entity destruction support
    void SetEntityDestructionCallback(std::function<void(EntityID)> callback) {
        m_entityDestructionCallback = callback;
    }

private:
    // Override to add game-specific component handling
    void DrawEntityInspector(Entity& entity) override;
    void DrawGameComponents(Entity& entity);
    
    // Callback for entity destruction
    std::function<void(EntityID)> m_entityDestructionCallback;
}; 