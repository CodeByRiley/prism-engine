#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../QuadBatch.h"
#include "../Shader.h"

// Forward declaration for obstacles
struct Obstacle;

// Structure to hold fog parameters (Guards and Thieves style)
struct FogConfig {
    float range = 400.0f;           // Maximum visibility distance
    float shadowSoftness = 0.3f;    // Edge softness for shadows behind obstacles
    glm::vec4 fogColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.85f); // Color for dark/blocked areas
};

class FogRenderer2D {
public:
    FogRenderer2D(int windowWidth, int windowHeight);
    ~FogRenderer2D();

    // Main rendering function - draws Guards and Thieves style fog (omnidirectional line-of-sight)
    void DrawFogQuad(const glm::vec2& playerPos, const FogConfig& config = FogConfig{});
    
    // Overload with direction for compatibility (ignores direction, uses omnidirectional)
    void DrawFogQuad(const glm::vec2& playerPos, const glm::vec2& playerDirection, 
                     const FogConfig& config = FogConfig{});
    
    // Legacy function for backward compatibility (omnidirectional fog)
    void DrawFogQuad(const glm::vec2& playerPos, float radius, float softness, const glm::vec4& fogColor);

    // Obstacle management (for shadow casting)
    void AddObstacle(const glm::vec2& position, const glm::vec2& size);
    void AddObstacles(const std::vector<Obstacle>& obstacles);
    void ClearObstacles();
    void RemoveObstacle(size_t index);
    
    // Fog configuration
    void SetFogConfig(const FogConfig& config);
    const FogConfig& GetFogConfig() const;

    // Window management
    void SetWindowSize(int width, int height);
    
    // Utility functions
    bool IsPositionVisible(const glm::vec2& position, const glm::vec2& playerPos, 
                          const glm::vec2& playerDirection, const FogConfig& config) const;
    float GetVisibilityAtPosition(const glm::vec2& position, const glm::vec2& playerPos, 
                                 const glm::vec2& playerDirection, const FogConfig& config) const;
    
    // Debug functions
    void SetDebugMode(bool enabled);
    void DrawObstaclesDebug();

private:
    // Core rendering components
    QuadBatch* m_QuadBatch;
    Shader* m_FogShader;
    
    // Window properties
    int m_WindowWidth, m_WindowHeight;
    
    // Fog configuration
    FogConfig m_Config;
    
    // Obstacles
    std::vector<Obstacle> m_Obstacles;
    
    // Debug mode
    bool m_DebugMode;
    
    // Helper functions
    void UpdateShaderUniforms(const glm::vec2& playerPos, const FogConfig& config);
    bool RayIntersectsBox(const glm::vec2& rayStart, const glm::vec2& rayDir, 
                         const glm::vec2& boxCenter, const glm::vec2& boxSize, 
                         float& hitDistance) const;
    bool IsInVisionCone(const glm::vec2& worldPos, const glm::vec2& playerPos, 
                       const glm::vec2& playerDir, float visionAngle) const;
};