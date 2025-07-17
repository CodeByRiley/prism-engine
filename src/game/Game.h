#pragma once

// GLM
#include <glm/glm.hpp>

// Standard library includes
#include <vector>
#include <memory>

// Engine Includes
#include "../engine/core/Engine.h"
#include "../engine/renderer/Renderer2D.h"
#include "../engine/renderer/Shader.h"
#include "../engine/renderer/fog/FogRenderer2D.h"
#include "../engine/renderer/vision/VisionRenderer2D.h"
#include "../engine/renderer/lighting/LightRenderer2D.h"

#include "Player.h"

// ECS includes
#include "../engine/scene/Scene.h"
#include "../engine/scene/component/CommonComponents.h"
#include "../engine/scene/system/CommonSystems.h"
#include <engine/core/input/Input.h>
#include "../engine/utils/Logger.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Inspector UI
#include "../engine/renderer/ui/GameInspectorUI.h"

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
            }
            if (Input::IsKeyHeld(GLFW_KEY_S)) {
                movementDelta.y += player->speed * deltaTime;
            }
            if (Input::IsKeyHeld(GLFW_KEY_A)) {
                movementDelta.x -= player->speed * deltaTime;
            }
            if (Input::IsKeyHeld(GLFW_KEY_D)) {
                movementDelta.x += player->speed * deltaTime;
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
    int viewportX = 0, viewportY = 0, viewportWidth = 1280, viewportHeight = 720;

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

private:
    // ECS components
    std::unique_ptr<Scene> m_scene;
    Entity m_playerEntity;
    PlayerMovementSystem* m_playerMovementSystem;
    
    // ECS setup methods
    void SetupECSScene();
    void SetupECSObstacles();
    void SetupECSLights();
    void UpdateRenderersFromECS();
    
    // Inspector UI
    std::unique_ptr<GameInspectorUI> m_inspectorUI;
};
