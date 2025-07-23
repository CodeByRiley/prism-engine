#pragma once

// GLM
#include <glm/glm.hpp>

// Standard library includes
#include <vector>
#include <memory>
#include <unordered_map>

// Engine Includes
#include "../engine/core/Engine.h"
#include "../engine/renderer/Renderer2D.h"
#include "../engine/renderer/Shader.h"
#include "../engine/renderer/fog/FogRenderer2D.h"
#include "../engine/renderer/vision/VisionRenderer2D.h"
#include "../engine/renderer/lighting/LightRenderer2D.h"
#include "../engine/utils/Time.h"
#include "../engine/core/networking/NetworkManager.h"
#include "../engine/core/networking/Packet.h"
#include "../engine/core/audio/AudioManager.h"
#include "../engine/core/audio/Sound.h"

#include "Player.h"

// ECS includes
#include "../engine/scene/Scene.h"
#include "../engine/scene/component/CommonComponents.h"
#include "../engine/scene/system/CommonSystems.h"
#include <engine/core/input/Input.h>
#include "../engine/utils/Logger.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GUI
#include "../engine/renderer/ui/GuiLayout.h"

enum class RenderMode {
    FOG,
    VISION,
    LIGHTING
};

// Custom ECS components for the game
class PlayerComponent : public Component {
public:
    float speed = 700.0f;
    glm::vec2 direction{0.0f, -1.0f}; // Facing up initially
    glm::vec2 size{32.0f, 32.0f};
    ::SoundAsset footsteps[3]; // 3 different footsteps sounds

    COMPONENT_TYPE(PlayerComponent)
    
    glm::vec2 GetDirectionIndicatorPos(const glm::vec2& position) const {
        return position + direction * 20.0f;
    }
    
    glm::vec2 GetMinBounds(const glm::vec2& position) const {
        return position - size * 0.5f;
    }
    
    glm::vec2 GetMaxBounds(const glm::vec2& position) const {
        return position + size * 0.5f;
    }
    
    void UpdateDirectionFromMouse(const glm::vec2& position, const glm::vec2& mousePos) {
        glm::vec2 toMouse = mousePos - position;
        if (glm::length(toMouse) > 0.001f) {
            direction = glm::normalize(toMouse);
        }
    }

    YAML::Node Serialize() const override {
        YAML::Node node;
        node["speed"] = speed;
        node["direction"]["x"] = direction.x;
        node["direction"]["y"] = direction.y;
        node["size"]["x"] = size.x;
        node["size"]["y"] = size.y;
        for (int i = 0; i < 3; i++) {
            Logger::Info("Serializing footsteps: " + footsteps[i].name);
            node["audio"]["footsteps"][i]["name"] = footsteps[i].name;
            node["audio"]["footsteps"][i]["filePath"] = footsteps[i].filePath;
            node["audio"]["footsteps"][i]["volume"] = footsteps[i].volume;
            node["audio"]["footsteps"][i]["pitch"] = footsteps[i].pitch;
            node["audio"]["footsteps"][i]["pan"] = footsteps[i].pan;
        }
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        speed = node["speed"].as<float>(700.0f);
        if (node["direction"]) {
            direction.x = node["direction"]["x"].as<float>(0.0f);
            direction.y = node["direction"]["y"].as<float>(-1.0f);
        }
        if (node["size"]) {
            size.x = node["size"]["x"].as<float>(32.0f);
            size.y = node["size"]["y"].as<float>(32.0f);
        }
        if (node["audio"]) {
            for (int i = 0; i < 3; i++) {
                Logger::Info("Deserializing footsteps: " + node["audio"]["footsteps"][i]["name"].as<std::string>(""));
                auto footstep = ::SoundAsset(node["audio"]["footsteps"][i]["name"].as<std::string>(""),
                                             node["audio"]["footsteps"][i]["filePath"].as<std::string>(""),
                                             node["audio"]["footsteps"][i]["volume"].as<float>(1.0f),
                                             node["audio"]["footsteps"][i]["pitch"].as<float>(1.0f),
                                             node["audio"]["footsteps"][i]["pan"].as<float>(0.0f));
                footsteps[i] = footstep;
            }
        }
    }
};

class ObstacleComponent : public Component {
public:
    glm::vec2 size{100.0f, 100.0f};
    
    COMPONENT_TYPE(ObstacleComponent)
    
    ObstacleComponent() = default;
    ObstacleComponent(const glm::vec2& obstacleSize) : size(obstacleSize) {}

    YAML::Node Serialize() const override {
        YAML::Node node;
        node["size"]["x"] = size.x;
        node["size"]["y"] = size.y;
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        if (node["size"]) {
            size.x = node["size"]["x"].as<float>(100.0f);
            size.y = node["size"]["y"].as<float>(100.0f);
        }
    }
};

// Input component for entities that can be controlled
class InputComponent : public Component {
public:
    bool enabled = true;
    
    COMPONENT_TYPE(InputComponent)

    YAML::Node Serialize() const override {
        YAML::Node node;
        node["enabled"] = enabled;
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        enabled = node["enabled"].as<bool>(true);
    }
};

// Game-specific ECS system for player movement and collision
class PlayerMovementSystem : public ECSSystem<PlayerMovementSystem> {
private:
    int windowWidth, windowHeight;
    
public:
    SYSTEM_TYPE(PlayerMovementSystem)
    
    PlayerMovementSystem(int width, int height) 
        : windowWidth(width), windowHeight(height) {}

    void Update(float deltaTime) override {
        auto entities = GetEntitiesWith<TransformComponent, PlayerComponent, InputComponent>();
        
        for (EntityID entityID : entities) {
            auto* transform = GetComponent<TransformComponent>(entityID);
            auto* player = GetComponent<PlayerComponent>(entityID);
            auto* input = GetComponent<InputComponent>(entityID);
            
            if (!transform || !player || !input || !input->enabled) continue;
            
            // Store old position for collision resolution
            glm::vec3 oldPosition = transform->position;
            glm::vec2 movementDelta(0.0f);
            
            // Handle movement input
            if (Input::IsKeyHeld(GLFW_KEY_W)) {
                movementDelta.y -= player->speed * deltaTime;
                PlayFootstepSound(entityID);
            }
            if (Input::IsKeyHeld(GLFW_KEY_S)) {
                movementDelta.y += player->speed * deltaTime;
                PlayFootstepSound(entityID);
            }
            if (Input::IsKeyHeld(GLFW_KEY_A)) {
                movementDelta.x -= player->speed * deltaTime;
                PlayFootstepSound(entityID);
            }
            if (Input::IsKeyHeld(GLFW_KEY_D)) {
                movementDelta.x += player->speed * deltaTime;
                PlayFootstepSound(entityID);
            }
            
            // Normalize movement delta
            if (glm::length(movementDelta) > 0.001f) {
                movementDelta = glm::normalize(movementDelta);
            }

            // Apply movement
            glm::vec2 newPosition2D = glm::vec2(transform->position) + movementDelta;
            
            // Resolve collisions
            newPosition2D = ResolveCollision(entityID, newPosition2D, player);
            
            // Update position
            transform->position.x = newPosition2D.x;
            transform->position.y = newPosition2D.y;
            
            // Handle mouse look
            glm::vec2 mousePos(Input::GetMouseX(), Input::GetMouseY());
            player->UpdateDirectionFromMouse(glm::vec2(transform->position), mousePos);
            transform->rotation.z = atan2(mousePos.y - transform->position.y, 
                                        -(mousePos.x - transform->position.x));
            
            // Clamp to screen bounds
            float halfWidth = player->size.x * 0.5f;
            float halfHeight = player->size.y * 0.5f;
            transform->position.x = glm::clamp(transform->position.x, halfWidth, 
                                              (float)windowWidth - halfWidth);
            transform->position.y = glm::clamp(transform->position.y, halfHeight, 
                                              (float)windowHeight - halfHeight);
        }
    }
    void PlayFootstepSound(EntityID entityID) {
        auto* player = m_componentManager->GetComponent<PlayerComponent>(entityID);
        if (!player) return;
        if (player->footsteps[0].name.empty()) return;
        int randomIndex = rand() % 3;
        if (player->footsteps[randomIndex].isPlaying) return;

        Audio::PlaySound(player->footsteps[randomIndex].name);
        player->footsteps[randomIndex].isPlaying = true;
    }

    void SetWindowSize(int width, int height) {
        windowWidth = width;
        windowHeight = height;
    }

private:
    glm::vec2 ResolveCollision(EntityID playerID, const glm::vec2& newPos, PlayerComponent* player) {
        auto obstacles = GetEntitiesWith<TransformComponent, ObstacleComponent>();
        glm::vec2 resolvedPos = newPos;
        
        for (EntityID obstacleID : obstacles) {
            if (obstacleID == playerID) continue; // Skip self
            
            auto* obstacleTransform = GetComponent<TransformComponent>(obstacleID);
            auto* obstacle = GetComponent<ObstacleComponent>(obstacleID);
            
            if (!obstacleTransform || !obstacle) continue;
            
            if (CheckCollision(resolvedPos, player->size, 
                            glm::vec2(obstacleTransform->position), obstacle->size)) {
                
                // Calculate bounds
                glm::vec2 playerMin = resolvedPos - player->size * 0.5f;
                glm::vec2 playerMax = resolvedPos + player->size * 0.5f;
                
                glm::vec2 obstaclePos = glm::vec2(obstacleTransform->position);
                glm::vec2 obstacleMin = obstaclePos - obstacle->size * 0.5f;
                glm::vec2 obstacleMax = obstaclePos + obstacle->size * 0.5f;
                
                // Calculate overlap
                float overlapX = std::min(playerMax.x - obstacleMin.x, obstacleMax.x - playerMin.x);
                float overlapY = std::min(playerMax.y - obstacleMin.y, obstacleMax.y - playerMin.y);
                
                // Resolve collision
                if (overlapX < overlapY) {
                    if (resolvedPos.x < obstaclePos.x) {
                        resolvedPos.x = obstacleMin.x - player->size.x * 0.5f;
                    } else {
                        resolvedPos.x = obstacleMax.x + player->size.x * 0.5f;
                    }
                } else {
                    if (resolvedPos.y < obstaclePos.y) {
                        resolvedPos.y = obstacleMin.y - player->size.y * 0.5f;
                    } else {
                        resolvedPos.y = obstacleMax.y + player->size.y * 0.5f;
                    }
                }
            }
        }
        
        return resolvedPos;
    }
    
    bool CheckCollision(const glm::vec2& pos1, const glm::vec2& size1,
                       const glm::vec2& pos2, const glm::vec2& size2) {
        glm::vec2 min1 = pos1 - size1 * 0.5f;
        glm::vec2 max1 = pos1 + size1 * 0.5f;
        glm::vec2 min2 = pos2 - size2 * 0.5f;
        glm::vec2 max2 = pos2 + size2 * 0.5f;
        
        return (min1.x < max2.x && max1.x > min2.x &&
                min1.y < max2.y && max1.y > min2.y);
    }
};

class Game : public Engine {
protected:
    unsigned int sceneFBO = 0;
    unsigned int sceneTexture = 0;
    int windowWidth = 1280;
    int windowHeight = 720;

public:
    Game(int width, int height, const char* title);
    ~Game();
    
    void OnInit() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnShutdown() override;
    void OnResize(int width, int height);
    
    // Networking methods
    void SetupNetworkingHandlers();
    void SendPlayerJoinToServer();
    void SendPlayerLeaveToServer(uint32_t playerID);
    void SendPlayerLeaveToClients(uint32_t playerID);
    void SendPlayerJoinToClients();
    void SendAllPlayersToClient(uint32_t clientID);
    void SendPlayerMovement();
    void ClearNetworkPlayers();
    void DisconnectFromServer();
    void RenderUI();
    
    // Audio methods
    void SetupAudioSystem();
    void LoadGameAudio();
    void HandleAudioEvents(const AudioEvent& event);
    
    int viewportX = 0, viewportY = 0, viewportWidth = 1280, viewportHeight = 720;

  void UpdateComponentsList(EntityID selectedEntity) {
if (selectedEntity == INVALID_ENTITY_ID) {
        Logger::Warn<Game>("Cannot update components list: Invalid entity ID");
        return;
    }

    std::unordered_map<std::string, std::string> variables;
    std::vector<std::string> componentItems;

    // Get entity name if available
    auto* entityManager = m_scene->GetEntityManager();
    if (entityManager) {
        std::string entityName = entityManager->GetEntityName(selectedEntity);
        variables["selected_entity_name"] = entityName;
        variables["selected_entity_id"] = std::to_string(selectedEntity);
        Logger::Info("Updating components for entity: " + entityName + " (ID: " + std::to_string(selectedEntity) + ")");
    }

    // Get component manager from scene
    auto* componentManager = m_scene->GetComponentManager();
    if (!componentManager) {
        Logger::Error<Game>("Component manager is null");
        return;
    }

    // Check for each component type and add to the list if present
    if (componentManager->HasComponent<TransformComponent>(selectedEntity)) {
        auto* transform = componentManager->GetComponent<TransformComponent>(selectedEntity);
        componentItems.push_back("TransformComponent");
        variables["transform_position_x"] = std::to_string(transform->position.x);
        variables["transform_position_y"] = std::to_string(transform->position.y);
        variables["transform_position_z"] = std::to_string(transform->position.z);
    }

    if (componentManager->HasComponent<PlayerComponent>(selectedEntity)) {
        auto* player = componentManager->GetComponent<PlayerComponent>(selectedEntity);
        componentItems.push_back("PlayerComponent");
        variables["player_speed"] = std::to_string(player->speed);
        variables["player_direction_x"] = std::to_string(player->direction.x);
        variables["player_direction_y"] = std::to_string(player->direction.y);
    }

    if (componentManager->HasComponent<ObstacleComponent>(selectedEntity)) {
        auto* obstacle = componentManager->GetComponent<ObstacleComponent>(selectedEntity);
        componentItems.push_back("ObstacleComponent");
        variables["obstacle_size_x"] = std::to_string(obstacle->size.x);
        variables["obstacle_size_y"] = std::to_string(obstacle->size.y);
    }

    if (componentManager->HasComponent<InputComponent>(selectedEntity)) {
        auto* input = componentManager->GetComponent<InputComponent>(selectedEntity);
        componentItems.push_back("InputComponent");
        variables["input_enabled"] = input->enabled ? "1" : "0";
    }

    // Add similar checks for other component types

    m_selectedEntityID = selectedEntity;

    // Convert vector to comma-separated string for the UI
    std::string componentsList;
    for (size_t i = 0; i < componentItems.size(); ++i) {
        componentsList += componentItems[i];
        if (i < componentItems.size() - 1) {
            componentsList += ",";
        }
    }

    // Set the components_list variable in multiple formats to ensure compatibility
    variables["components_list"] = componentsList;
    variables["selected_entity"] = componentsList;  // Try alternate variable name
    variables["entity_components"] = componentsList; // Try another alternate name

    // Log the component list and all variables for debugging
    Logger::Info("Components list for entity " + std::to_string(selectedEntity) + ": " + componentsList);
    Logger::Info("Variable count: " + std::to_string(variables.size()));

    // Explicitly set entity_list variable if needed by any widget with that source
    variables["entity_list"] = componentsList;

    // Set a standard name for specific entity details
    std::string entityName = variables["selected_entity_name"];
    if (!entityName.empty()) {
        variables["entity_name"] = entityName;
    }

    // Render with the component variables
    if (m_DebugInspector) {
        m_DebugInspector->Reset(); // Reset UI state before rendering
        m_DebugInspector->Render(variables);
        Logger::Info("Updated debug inspector UI for entity " + std::to_string(selectedEntity));
    } else if (m_EcsInspector) {
        m_EcsInspector->Reset(); // Reset UI state before rendering
        m_EcsInspector->Render(variables);
        Logger::Info("Updated ECS inspector UI for entity " + std::to_string(selectedEntity));
    } else {
        Logger::Warn<Game>("No inspector UI available to render component data", this);
    }
}


    std::vector<Obstacle> m_Obstacles;
    void setupObstacles();
    void setupLights();
    
    // Collision detection helpers (legacy - kept for compatibility)
    bool CheckCollision(const Player& player, const Obstacle& obstacle) const;
    glm::vec2 ResolveCollision(const Player& player, const glm::vec2& newPos) const;
    
protected:
    Player m_Player; // Legacy player - kept for compatibility
    Renderer2D* renderer;
    FogRenderer2D* fogRenderer;
    VisionRenderer2D* visionRenderer;
    LightRenderer2D* lightRenderer;
    VisionConfig m_VisionConfig;    // Vision system configuration
    LightConfig m_LightConfig;      // Lighting system configuration
    std::vector<Light> m_Lights;    // Scene lights
    RenderMode m_RenderMode = RenderMode::LIGHTING;
    
    // Network UI
    //std::unique_ptr<NetworkUI> m_networkUI;
    
    // Audio
    std::unique_ptr<AudioManager> m_audioManager;

private:
    // ECS components
    std::unique_ptr<Scene> m_scene;
    Entity m_playerEntity;
    PlayerMovementSystem* m_playerMovementSystem;
    
    // Networking
    uint32_t m_localPlayerNetworkID;
    std::unordered_map<uint32_t, Entity> m_networkPlayers;
    
    // ECS setup methods
    void SetupECSScene();
    void SetupECSObstacles();
    void SetupECSLights();
    void UpdateRenderersFromECS();

    // GUIs
    std::unique_ptr<GuiLayout> m_GameInspector;
    std::unique_ptr<GuiLayout> m_EcsInspector;
    std::unique_ptr<GuiLayout> m_NetworkManager;
    std::unique_ptr<GuiLayout> m_DebugInspector;

    bool m_ImGuiInitialized = false;

    // Inspector UI
    //std::unique_ptr<GameInspectorUI> m_inspectorUI;
    EntityID m_selectedEntityID = INVALID_ENTITY_ID;
    int m_buttonClickCount = 0;

};
