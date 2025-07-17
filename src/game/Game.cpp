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
      m_RenderMode(RenderMode::LIGHTING)  // Start with lighting system (0=fog, 1=vision, 2=lighting)
{
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferSizeCallback);
        renderer = new Renderer2D(width, height);
    fogRenderer = new FogRenderer2D(width, height);
    visionRenderer = new VisionRenderer2D(width, height);
    lightRenderer = new LightRenderer2D(width, height);
    
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
}

Game::~Game() {
    OnShutdown();
}

void Game::OnInit() {
    Logger::Info("Game Init");
    Logger::Info("Press TAB to cycle between Fog, Vision, and Lighting systems");
    Logger::Info("Use WASD to move and change facing direction");
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
    
    // Store old position for collision resolution
    glm::vec2 oldPosition = m_Player.position;
    glm::vec2 moveDirection(0.0f);
    bool isMoving = false;
    
    // Calculate movement delta
    glm::vec2 movementDelta(0.0f);
    
    if (Input::IsKeyHeld(GLFW_KEY_W)) {
        movementDelta.y -= m_Player.speed * Time::DeltaTime();
        moveDirection.y -= 1.0f;
        isMoving = true;
    }
    if (Input::IsKeyHeld(GLFW_KEY_S)) {
        movementDelta.y += m_Player.speed * Time::DeltaTime();
        moveDirection.y += 1.0f;
        isMoving = true;
    }
    if (Input::IsKeyHeld(GLFW_KEY_A)) {
        movementDelta.x -= m_Player.speed * Time::DeltaTime();
        moveDirection.x -= 1.0f;
        isMoving = true;
    }
    if (Input::IsKeyHeld(GLFW_KEY_D)) {
        movementDelta.x += m_Player.speed * Time::DeltaTime();
        moveDirection.x += 1.0f;
        isMoving = true;
    }
    
    // Apply movement with collision detection
    glm::vec2 newPosition = m_Player.position + movementDelta;
    m_Player.position = ResolveCollision(m_Player, newPosition);

    // Player faces mouse
    glm::vec2 mousePos = glm::vec2(Input::GetMouseX(), Input::GetMouseY());
    m_Player.UpdateDirectionFromMouse(mousePos);
    m_Player.rotation = atan2((mousePos.y - m_Player.position.y), -(mousePos.x - m_Player.position.x));

    // Clamp position to screen bounds (accounting for player size)
    float halfPlayerWidth = m_Player.size.x * 0.5f;
    float halfPlayerHeight = m_Player.size.y * 0.5f;
    m_Player.position.x = glm::clamp(m_Player.position.x, halfPlayerWidth, (float)windowWidth - halfPlayerWidth);
    m_Player.position.y = glm::clamp(m_Player.position.y, halfPlayerHeight, (float)windowHeight - halfPlayerHeight);
}

void Game::OnDraw() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the player first
    renderer->BeginBatch(renderer->GetBaseShader());
    renderer->DrawRectRot(m_Player.position, m_Player.size, m_Player.rotation, m_Player.color);
    
    // Draw obstacles
    for (const auto& obstacle : m_Obstacles) {
        renderer->DrawRect(obstacle.position, obstacle.size, glm::vec4(1.0f, 0.25f, 0.45f, 1.0f));
    }

    // Draw direction indicator
    glm::vec2 directionIndicatorPos = m_Player.GetDirectionIndicatorPos();
    renderer->DrawRect(directionIndicatorPos, {8, 8}, glm::vec4(-m_Player.color.x, -m_Player.color.y, -m_Player.color.b, 1.0f));
    renderer->EndBatch();

    // Enable blending for overlays
    glEnable(GL_BLEND);
    
    switch(m_RenderMode) {
        case RenderMode::FOG: { // Fog system (Guards and Thieves style)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // Create fog config for Guards and Thieves style visibility
            FogConfig fogConfig;
            fogConfig.range = 500.0f;        // How far player can see
            fogConfig.shadowSoftness = 0.4f; // Soft shadows behind obstacles
            fogConfig.fogColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.9f); // Dark areas
            
            fogRenderer->DrawFogQuad(m_Player.position, fogConfig);
            break;
        }
        case RenderMode::VISION: // Vision system
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            visionRenderer->DrawVisionOverlay(m_Player.position, m_Player.direction, m_VisionConfig);
            break;
            
        case RenderMode::LIGHTING: // Lighting system
            glBlendFunc(GL_DST_COLOR, GL_ZERO); // Multiply blend mode for lighting
            lightRenderer->DrawLightingOverlay(m_Lights, m_LightConfig);
            break;
        default:
            Logger::Error("Invalid render mode", this);
            break;
    }   
    glDisable(GL_BLEND);
}

void Game::OnResize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    renderer->SetWindowSize(width, height);
    fogRenderer->SetWindowSize(width, height);
    visionRenderer->SetWindowSize(width, height);
    lightRenderer->SetWindowSize(width, height);
}

void Game::OnShutdown() {
    Logger::Info("Game Shutdown");
    delete renderer;
    delete fogRenderer;
    delete visionRenderer;
    delete lightRenderer;
}

// Collision detection helper functions
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