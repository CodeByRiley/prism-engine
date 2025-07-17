#pragma once

#include "../engine/core/Engine.h"
#include "../engine/renderer/Renderer2D.h"
#include <glm/glm.hpp>
#include "../engine/renderer/Shader.h"
#include "../engine/renderer/fog/FogRenderer2D.h"
#include "../engine/renderer/vision/VisionRenderer2D.h"
#include "../engine/renderer/lighting/LightRenderer2D.h"
#include "Player.h"
#include <vector>


enum class RenderMode {
    FOG,
    VISION,
    LIGHTING
};

class Game : public Engine {
private:

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
    
    // Collision detection helpers
    bool CheckCollision(const Player& player, const Obstacle& obstacle) const;
    glm::vec2 ResolveCollision(const Player& player, const glm::vec2& newPos) const;
    
private:
    Player m_Player;
    Renderer2D* renderer;
    FogRenderer2D* fogRenderer;
    VisionRenderer2D* visionRenderer;
    LightRenderer2D* lightRenderer;
    VisionConfig m_VisionConfig;    // Vision system configuration
    LightConfig m_LightConfig;      // Lighting system configuration
    std::vector<Light> m_Lights;    // Scene lights
    RenderMode m_RenderMode = RenderMode::LIGHTING;               // 0=fog, 1=vision, 2=lighting
};
