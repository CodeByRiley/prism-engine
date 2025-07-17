#include "Game.h"

#include <engine/core/Input.h>
#include <engine/utils/Time.h>
#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include "engine/utils/Logger.h"
#include <GLFW/glfw3.h>
#include "../engine/renderer/fog/FogRenderer2D.h"
#include "../engine/renderer/vision/VisionRenderer2D.h"
#include "../engine/renderer/lighting/LightRenderer2D.h"
#include "../engine/renderer/lighting/Light.h"
#include <engine/renderer/QuadBatch.h>
#include <engine/renderer/Shader.h>
#include <glm/glm.hpp>
#include <algorithm>

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;
    Logger::Info("Framebuffer resized to: " + std::to_string(width) + "x" + std::to_string(height));
    glViewport(0, 0, width, height);
    game->OnResize(width, height);
}

void Game::setupObstacles() {
    visionRenderer->ClearObstacles();
    lightRenderer->ClearObstacles();
    fogRenderer->ClearObstacles();
    m_Obstacles.push_back(Obstacle(glm::vec2(400, 300), glm::vec2(100, 200)));
    m_Obstacles.push_back(Obstacle(glm::vec2(800, 400), glm::vec2(150, 80)));
    m_Obstacles.push_back(Obstacle(glm::vec2(200, 500), glm::vec2(120, 120)));
    m_Obstacles.push_back(Obstacle(glm::vec2(1000, 200), glm::vec2(80, 300)));
    visionRenderer->AddObstacles(m_Obstacles);
    lightRenderer->AddObstacles(m_Obstacles);
    fogRenderer->AddObstacles(m_Obstacles);
}

void Game::setupLights() {
    m_Lights.clear();
    
    // Single point light with good dispersion
    m_Lights.emplace_back(glm::vec2(640, 360), 1204.0f, glm::vec3(1.0f, 1.0f, 1.0f), 5.25f);
}

Game::Game(int width, int height, const char* title)
    : Engine(width, height, title),
      m_Player(glm::vec2(width * 0.5f, height * 0.5f), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), 700.0f),
      windowWidth(width),
      windowHeight(height),
      renderer(nullptr),
      fogRenderer(nullptr),
      visionRenderer(nullptr),
      lightRenderer(nullptr),
      m_RenderMode(RenderMode::LIGHTING),
      m_playerMovementSystem(nullptr)
{
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferSizeCallback);
    renderer = new Renderer2D(width, height);
    fogRenderer = new FogRenderer2D(width, height);
    visionRenderer = new VisionRenderer2D(width, height);
    lightRenderer = new LightRenderer2D(width, height);
    
    // Create ECS scene
    m_scene = std::make_unique<Scene>("GameScene", 1);
    
    // Setup legacy obstacles and lights (for compatibility)
    setupObstacles();
    setupLights();
    
    // Setup fog system with obstacles
    fogRenderer->AddObstacles(m_Obstacles);
    
    // Configure vision system
    m_VisionConfig.range = 1024.0f;
    m_VisionConfig.angle = 1.0472f; // 60 degrees in radians
    m_VisionConfig.shadowLength = 900.0f;
    m_VisionConfig.shadowSoftness = 0.82f;
    m_VisionConfig.darkColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.85f);
    
    // Configure lighting system
    m_LightConfig.ambientLight = 0.45f;
    m_LightConfig.ambientColor = glm::vec3(0.75f, 0.75f, 0.75f);
    m_LightConfig.shadowSoftness = 0.4f;    
    m_LightConfig.shadowLength = 1000.0f;
    m_LightConfig.enableShadows = true;
    m_LightConfig.lightType = LightType::DIRECTIONAL_LIGHT;
    m_LightConfig.bloom = 0.5f;
    
    // Setup ECS systems and entities
    SetupECSScene();
}

Game::~Game() {
    OnShutdown();
}

void Game::OnInit() {
    Logger::Info("Game Init");
    Logger::Info("Press TAB to cycle between Fog, Vision, and Lighting systems");
    Logger::Info("Use WASD to move and change facing direction");
    Logger::Info("Press F5 to save scene, F9 to load scene");
    Logger::Info("Press F1 to toggle ECS Inspector");
    
    // Initialize Inspector UI
    m_inspectorUI = std::make_unique<GameInspectorUI>();
    if (!m_inspectorUI->Initialize(m_Window)) {
        Logger::Error<Game>("Failed to initialize Inspector UI", this);
    } else {
        // Set up entity destruction callback
        m_inspectorUI->SetEntityDestructionCallback([this](EntityID entityID) {
            m_scene->DestroyEntity(entityID);
        });
    }
}

void Game::SetupECSScene() {
    // Register all systems
    m_playerMovementSystem = m_scene->RegisterSystem<PlayerMovementSystem>(windowWidth, windowHeight);
    
    // Create player entity
    m_playerEntity = m_scene->CreateEntity("Player");
    m_playerEntity.AddComponent<TransformComponent>(
        glm::vec3(windowWidth * 0.5f, windowHeight * 0.5f, 0.0f)
    );
    m_playerEntity.AddComponent<RenderableComponent>();
    auto* playerComp = m_playerEntity.AddComponent<PlayerComponent>();
    if (playerComp) {
        playerComp->speed = 700.0f;
        playerComp->size = glm::vec2(32.0f, 32.0f);
    }
    m_playerEntity.AddComponent<InputComponent>();
    m_playerEntity.AddComponent<TagComponent>("player");
    
    // Set player color (purple like original)
    auto* playerRenderable = m_playerEntity.GetComponent<RenderableComponent>();
    if (playerRenderable) {
        playerRenderable->color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
    }
    
    // Setup obstacles and lights
    SetupECSObstacles();
    SetupECSLights();
    
    Logger::Info("ECS Scene setup complete with " + 
                std::to_string(m_scene->GetAllEntities().size()) + " entities");
}

void Game::SetupECSObstacles() {
    // Clear old obstacle data from renderers
    visionRenderer->ClearObstacles();
    lightRenderer->ClearObstacles();
    fogRenderer->ClearObstacles();
    
    // Create obstacle entities matching the original positions
    struct ObstacleData {
        glm::vec2 position;
        glm::vec2 size;
    };
    
    std::vector<ObstacleData> obstacleData = {
        {glm::vec2(400, 300), glm::vec2(100, 200)},
        {glm::vec2(800, 400), glm::vec2(150, 80)},
        {glm::vec2(200, 500), glm::vec2(120, 120)},
        {glm::vec2(1000, 200), glm::vec2(80, 300)}
    };
    
    for (size_t i = 0; i < obstacleData.size(); ++i) {
        Entity obstacle = m_scene->CreateEntity("Obstacle_" + std::to_string(i));
        obstacle.AddComponent<TransformComponent>(
            glm::vec3(obstacleData[i].position.x, obstacleData[i].position.y, 0.0f)
        );
        obstacle.AddComponent<ObstacleComponent>(obstacleData[i].size);
        auto* renderable = obstacle.AddComponent<RenderableComponent>();
        if (renderable) {
            renderable->color = glm::vec4(1.0f, 0.25f, 0.45f, 1.0f);
        }
        obstacle.AddComponent<TagComponent>("obstacle");
    }
    
    // Update renderers with new obstacle data
    UpdateRenderersFromECS();
}

void Game::SetupECSLights() {
    // Create light entity matching the original setup
    Entity light = m_scene->CreateEntity("MainLight");
    light.AddComponent<TransformComponent>(glm::vec3(640, 360, 0.0f));
    
    // Create a proper Light struct for the LightComponent
    Light lightData(glm::vec2(640, 360), 1204.0f, glm::vec3(1.0f, 1.0f, 1.0f), 5.25f);
    light.AddComponent<LightComponent>(lightData);
    
    light.AddComponent<TagComponent>("light");
}

void Game::UpdateRenderersFromECS() {
    // Update renderer obstacle lists from ECS entities
    std::vector<Obstacle> obstacles;
    
    auto obstacleEntities = m_scene->GetEntitiesWith<TransformComponent, ObstacleComponent>();
    for (const Entity& entity : obstacleEntities) {
        auto* transform = entity.GetComponent<TransformComponent>();
        auto* obstacleComp = entity.GetComponent<ObstacleComponent>();
        
        if (transform && obstacleComp) {
            obstacles.emplace_back(
                glm::vec2(transform->position), 
                obstacleComp->size
            );
        }
    }
    
    // Update all renderers
    visionRenderer->AddObstacles(obstacles);
    lightRenderer->AddObstacles(obstacles);
    fogRenderer->AddObstacles(obstacles);
    
    // Update lights
    m_Lights.clear();
    auto lightEntities = m_scene->GetEntitiesWith<TransformComponent, LightComponent>();
    for (const Entity& entity : lightEntities) {
        auto* transform = entity.GetComponent<TransformComponent>();
        auto* lightComp = entity.GetComponent<LightComponent>();
        
        if (transform && lightComp) {
            m_Lights.emplace_back(
                glm::vec2(transform->position),
                lightComp->light.range,
                lightComp->light.color,
                lightComp->light.intensity
            );
        }
    }
}

void Game::OnUpdate() {
    if (Input::IsKeyPressed(GLFW_KEY_ESCAPE)) {
        this->m_Running = false;
    }
    
    // Cycle between rendering systems
    if (Input::IsKeyPressed(GLFW_KEY_TAB)) {
        m_RenderMode = static_cast<RenderMode>((static_cast<int>(m_RenderMode) + 1) % 3);
        switch(m_RenderMode) {
            case RenderMode::FOG: Logger::Info("Fog system enabled"); break;
            case RenderMode::VISION: Logger::Info("Vision system enabled"); break;
            case RenderMode::LIGHTING: Logger::Info("Lighting system enabled"); break;
        }
    }
    
    // Toggle Inspector
    if (Input::IsKeyPressed(GLFW_KEY_F1)) {
        if (m_inspectorUI) {
            m_inspectorUI->ToggleVisibility();
            Logger::Info("Inspector " + std::string(m_inspectorUI->IsVisible() ? "ENABLED" : "DISABLED"));
        }
    }
    
    // Save/Load scene
    if (Input::IsKeyPressed(GLFW_KEY_F5)) {
        bool success = m_scene->SaveToFile("game_scene.yaml");
        Logger::Info("Scene save: " + std::string(success ? "SUCCESS" : "FAILED"));
    }
    
    if (Input::IsKeyPressed(GLFW_KEY_F9)) {
        bool success = m_scene->LoadFromFile("game_scene.yaml");
        if (success) {
            Logger::Info("Scene loaded successfully");
            // Re-setup systems and update renderers
            m_playerMovementSystem = m_scene->GetSystem<PlayerMovementSystem>();
            if (!m_playerMovementSystem) {
                m_playerMovementSystem = m_scene->RegisterSystem<PlayerMovementSystem>(windowWidth, windowHeight);
            }
            
            // Find the player entity again
            auto playerEntities = m_scene->GetEntitiesWith<PlayerComponent>();
            if (!playerEntities.empty()) {
                m_playerEntity = playerEntities[0];
            }
            
            UpdateRenderersFromECS();
        } else {
            Logger::Error<Game>("Failed to load scene", this);
        }
    }
    
    // Update ECS scene
    m_scene->Update(Time::DeltaTime());
}

void Game::OnDraw() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Begin batch rendering
    renderer->BeginBatch(renderer->GetBaseShader());
    
    // Draw player from ECS
    auto playerEntities = m_scene->GetEntitiesWith<TransformComponent, PlayerComponent, RenderableComponent>();
    for (const Entity& entity : playerEntities) {
        auto* transform = entity.GetComponent<TransformComponent>();
        auto* player = entity.GetComponent<PlayerComponent>();
        auto* renderable = entity.GetComponent<RenderableComponent>();
        
        if (transform && player && renderable && renderable->visible) {
            glm::vec2 position(transform->position);
            renderer->DrawRectRot(position, player->size, transform->rotation.z, renderable->color);
            
            // Draw direction indicator
            glm::vec2 directionPos = player->GetDirectionIndicatorPos(position);
            glm::vec4 indicatorColor(-renderable->color.x, -renderable->color.y, -renderable->color.z, 1.0f);
            renderer->DrawRect(directionPos, glm::vec2(8, 8), indicatorColor);
        }
    }
    
    // Draw obstacles from ECS
    auto obstacleEntities = m_scene->GetEntitiesWith<TransformComponent, ObstacleComponent, RenderableComponent>();
    for (const Entity& entity : obstacleEntities) {
        auto* transform = entity.GetComponent<TransformComponent>();
        auto* obstacle = entity.GetComponent<ObstacleComponent>();
        auto* renderable = entity.GetComponent<RenderableComponent>();
        
        if (transform && obstacle && renderable && renderable->visible) {
            renderer->DrawRect(glm::vec2(transform->position), obstacle->size, renderable->color);
        }
    }
    
    renderer->EndBatch();

    // Enable blending for overlays
    glEnable(GL_BLEND);
    
    // Get player position for rendering systems
    glm::vec2 playerPos(0.0f);
    glm::vec2 playerDirection(0.0f, -1.0f);
    
    if (m_playerEntity.IsValid()) {
        auto* transform = m_playerEntity.GetComponent<TransformComponent>();
        auto* player = m_playerEntity.GetComponent<PlayerComponent>();
        if (transform && player) {
            playerPos = glm::vec2(transform->position);
            playerDirection = player->direction;
        }
    }
    
    switch(m_RenderMode) {
        case RenderMode::FOG: {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            FogConfig fogConfig;
            fogConfig.range = 500.0f;
            fogConfig.shadowSoftness = 0.4f;
            fogConfig.fogColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.9f);
            
            fogRenderer->DrawFogQuad(playerPos, fogConfig);
            break;
        }
        case RenderMode::VISION:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            visionRenderer->DrawVisionOverlay(playerPos, playerDirection, m_VisionConfig);
            break;
            
        case RenderMode::LIGHTING:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            lightRenderer->DrawLightingOverlay(m_Lights, m_LightConfig);
            break;
    }
    
    glDisable(GL_BLEND);
    
    // Render Inspector UI
    if (m_inspectorUI) {
        m_inspectorUI->Render(m_scene.get());
        
        // Update renderers if any changes were made in the inspector
        UpdateRenderersFromECS();
    }
}

void Game::OnResize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    renderer->SetWindowSize(width, height);
    fogRenderer->SetWindowSize(width, height);
    visionRenderer->SetWindowSize(width, height);
    lightRenderer->SetWindowSize(width, height);
    
    // Update movement system window size
    if (m_playerMovementSystem) {
        m_playerMovementSystem->SetWindowSize(width, height);
    }
}

void Game::OnShutdown() {
    Logger::Info("Game Shutdown");
    
    // Save scene before shutdown
    if (m_scene) {
        m_scene->SaveToFile("autosave_scene.yaml");
        Logger::Info("Auto-saved scene to autosave_scene.yaml");
    }
    
    // Shutdown inspector
    if (m_inspectorUI) {
        m_inspectorUI->Shutdown();
        m_inspectorUI.reset();
    }
    
    delete renderer;
    delete fogRenderer;
    delete visionRenderer;
    delete lightRenderer;
}

// Legacy collision detection helper functions (kept for compatibility)
bool Game::CheckCollision(const Player& player, const Obstacle& obstacle) const {
    // Convert center-based positions to AABB bounds
    glm::vec2 playerMin = player.GetMinBounds();
    glm::vec2 playerMax = player.GetMaxBounds();
    
    glm::vec2 obstacleMin = obstacle.position - obstacle.size * 0.5f;
    glm::vec2 obstacleMax = obstacle.position + obstacle.size * 0.5f;
    
    // AABB collision detection
    return (playerMin.x < obstacleMax.x && playerMax.x > obstacleMin.x &&
            playerMin.y < obstacleMax.y && playerMax.y > obstacleMin.y);
}

glm::vec2 Game::ResolveCollision(const Player& player, const glm::vec2& newPos) const {
    glm::vec2 resolvedPos = newPos;
    
    // Create a temporary player with the new position for collision testing
    Player tempPlayer = player;
    tempPlayer.position = resolvedPos;
    
    // Check collision with all obstacles
    for (const auto& obstacle : m_Obstacles) {
        if (CheckCollision(tempPlayer, obstacle)) {
            // Calculate overlap and resolve collision
            glm::vec2 playerMin = tempPlayer.GetMinBounds();
            glm::vec2 playerMax = tempPlayer.GetMaxBounds();
            
            glm::vec2 obstacleMin = obstacle.position - obstacle.size * 0.5f;
            glm::vec2 obstacleMax = obstacle.position + obstacle.size * 0.5f;
            
            // Calculate overlap in both axes
            float overlapX = std::min(playerMax.x - obstacleMin.x, obstacleMax.x - playerMin.x);
            float overlapY = std::min(playerMax.y - obstacleMin.y, obstacleMax.y - playerMin.y);
            
            // Resolve collision by moving along the axis with minimum overlap
            if (overlapX < overlapY) {
                // Resolve horizontally
                if (resolvedPos.x < obstacle.position.x) {
                    // Player is to the left of obstacle
                    resolvedPos.x = obstacleMin.x - player.size.x * 0.5f;
                } else {
                    // Player is to the right of obstacle
                    resolvedPos.x = obstacleMax.x + player.size.x * 0.5f;
                }
            } else {
                // Resolve vertically
                if (resolvedPos.y < obstacle.position.y) {
                    // Player is above obstacle
                    resolvedPos.y = obstacleMin.y - player.size.y * 0.5f;
                } else {
                    // Player is below obstacle
                    resolvedPos.y = obstacleMax.y + player.size.y * 0.5f;
                }
            }
            
            // Update temp player position for subsequent collision checks
            tempPlayer.position = resolvedPos;
        }
    }
    
    return resolvedPos;
}