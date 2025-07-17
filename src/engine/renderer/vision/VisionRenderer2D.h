#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../QuadBatch.h"
#include "../Shader.h"

// Structure to represent obstacles that block vision
struct Obstacle {
    glm::vec2 position;
    glm::vec2 size;
    
    Obstacle(const glm::vec2& pos, const glm::vec2& sz) 
        : position(pos), size(sz) {}
};

// Structure to hold vision parameters
struct VisionConfig {
    float range = 300.0f;           // Maximum vision distance
    float angle = 1.0472f;          // Vision cone angle in radians (60 degrees default)
    float shadowLength = 500.0f;    // How far shadows extend
    float shadowSoftness = 0.5f;    // Edge softness for shadows
    glm::vec4 darkColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.8f); // Color for dark areas
};

class VisionRenderer2D {
public:
    VisionRenderer2D(int windowWidth, int windowHeight);
    ~VisionRenderer2D();

    // Main rendering function - draws the vision overlay
    void DrawVisionOverlay(const glm::vec2& playerPos, const glm::vec2& playerDirection, 
                          const VisionConfig& config = VisionConfig{});
    
    // Obstacle management
    void AddObstacle(const glm::vec2& position, const glm::vec2& size);
    void AddObstacles(const std::vector<Obstacle>& obstacles);
    void AddObstacle(const Obstacle& obstacle);
    void ClearObstacles();
    void RemoveObstacle(size_t index);
    
    // Vision configuration
    void SetVisionConfig(const VisionConfig& config);
    const VisionConfig& GetVisionConfig() const;
    
    // Window management
    void SetWindowSize(int width, int height);
    
    // Utility functions
    bool IsPositionVisible(const glm::vec2& position, const glm::vec2& playerPos, 
                          const glm::vec2& playerDirection, const VisionConfig& config) const;
    float GetVisibilityAtPosition(const glm::vec2& position, const glm::vec2& playerPos, 
                                 const glm::vec2& playerDirection, const VisionConfig& config) const;
    
    // Debug functions
    void SetDebugMode(bool enabled);
    void DrawObstaclesDebug();

private:
    // Core rendering components
    QuadBatch* m_QuadBatch;
    Shader* m_VisionShader;
    
    // Window properties
    int m_WindowWidth, m_WindowHeight;
    
    // Vision configuration
    VisionConfig m_Config;
    
    // Obstacles
    std::vector<Obstacle> m_Obstacles;
    
    // Debug mode
    bool m_DebugMode;
    
    // Helper functions
    void UpdateShaderUniforms(const glm::vec2& playerPos, const glm::vec2& playerDirection, 
                             const VisionConfig& config);
    bool RayIntersectsBox(const glm::vec2& rayStart, const glm::vec2& rayDir, 
                         const glm::vec2& boxCenter, const glm::vec2& boxSize, 
                         float& hitDistance) const;
    bool IsInVisionCone(const glm::vec2& worldPos, const glm::vec2& playerPos, 
                       const glm::vec2& playerDir, float visionAngle) const;
}; 