#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../QuadBatch.h"
#include "../Shader.h"
#include "Light.h"

// Forward declaration for obstacles
struct Obstacle;

class LightRenderer2D {
public:
    LightRenderer2D(int windowWidth, int windowHeight);
    ~LightRenderer2D();

    // Main rendering function - draws the lighting overlay
    void DrawLightingOverlay(const std::vector<Light>& lights, const LightConfig& config = LightConfig{});
    
    // Light management
    void AddLight(const Light& light);
    void AddLights(const std::vector<Light>& lights);
    void ClearLights();
    void RemoveLight(size_t index);
    void UpdateLight(size_t index, const Light& light);
    
    // Convenience methods for creating specific light types
    void AddPointLight(const glm::vec2& position, float range, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    void AddSpotLight(const glm::vec2& position, const glm::vec2& direction, float range, float angle, 
                     const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    void AddDirectionalLight(const glm::vec2& direction, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    
    // Advanced spot light with inner/outer angles
    void AddAdvancedSpotLight(const glm::vec2& position, const glm::vec2& direction, float range, 
                             float innerAngle, float outerAngle, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    
    // Obstacle management (for shadow casting)
    void AddObstacle(const glm::vec2& position, const glm::vec2& size);
    void AddObstacles(const std::vector<Obstacle>& obstacles);
    void ClearObstacles();
    void RemoveObstacle(size_t index);
    
    // Lighting configuration
    void SetLightConfig(const LightConfig& config);
    const LightConfig& GetLightConfig() const;
    
    // Window management
    void SetWindowSize(int width, int height);
    
    // Utility functions
    bool IsPositionLit(const glm::vec2& position, float threshold = 0.1f) const;
    float GetLightIntensityAtPosition(const glm::vec2& position) const;
    glm::vec3 GetLightColorAtPosition(const glm::vec2& position) const;
    
    // Debug functions
    void SetDebugMode(bool enabled);
    void DrawLightsDebug();

private:
    // Core rendering components
    QuadBatch* m_QuadBatch;
    Shader* m_LightShader;
    
    // Window properties
    int m_WindowWidth, m_WindowHeight;
    
    // Lighting configuration
    LightConfig m_Config;
    
    // Lights and obstacles
    std::vector<Light> m_Lights;
    std::vector<Obstacle> m_Obstacles;
    
    // Debug mode
    bool m_DebugMode;
    
    // Helper functions
    void UpdateShaderUniforms(const std::vector<Light>& lights, const LightConfig& config);
    bool RayIntersectsBox(const glm::vec2& rayStart, const glm::vec2& rayDir, 
                         const glm::vec2& boxCenter, const glm::vec2& boxSize, 
                         float& hitDistance) const;
    bool IsInLightCone(const glm::vec2& worldPos, const glm::vec2& lightPos, 
                      const glm::vec2& lightDir, float lightAngle, bool isDirectional) const;
    float CalculateLightContribution(const Light& light, const glm::vec2& position) const;
}; 