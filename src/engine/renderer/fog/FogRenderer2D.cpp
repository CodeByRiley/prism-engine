#include "FogRenderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <engine/utils/Logger.h>
#include <algorithm>
#include <cmath>
#include "../vision/VisionRenderer2D.h" // For Obstacle struct

FogRenderer2D::FogRenderer2D(int windowWidth, int windowHeight)
    : m_WindowWidth(windowWidth), m_WindowHeight(windowHeight), m_DebugMode(false)
{
    m_FogShader = new Shader("shaders/FogVertex.vert.glsl", "shaders/FogFrag.frag.glsl");
    m_QuadBatch = new QuadBatch();
    Logger::Info("Fog shader created with ID: " + std::to_string(m_FogShader->GetID()));
}

FogRenderer2D::~FogRenderer2D() {
    delete m_QuadBatch;
    delete m_FogShader;
}

void FogRenderer2D::DrawFogQuad(const glm::vec2& playerPos, const glm::vec2& playerDirection, 
                               const FogConfig& config) {
    // For Guards and Thieves style, we ignore playerDirection and use omnidirectional visibility
    DrawFogQuad(playerPos, config);
}

void FogRenderer2D::DrawFogQuad(const glm::vec2& playerPos, const FogConfig& config) {
    // Start the quad batch with our fog shader
    m_QuadBatch->Begin(m_FogShader);
    
    // Create the projection matrix for the current window size
    glm::mat4 projection = glm::ortho(0.0f, (float)m_WindowWidth, (float)m_WindowHeight, 0.0f);
    
    // Set uniforms AFTER Begin() to ensure they're set on the active shader
    m_FogShader->SetMat4("uProjection", projection);
    
    // Update all shader uniforms (omnidirectional)
    UpdateShaderUniforms(playerPos, config);
    
    // Create a quad that covers the entire screen - NO ROTATION
    QuadInstance fogInstance;
    fogInstance.position = glm::vec2(m_WindowWidth * 0.5f, m_WindowHeight * 0.5f);
    fogInstance.size = glm::vec2(m_WindowWidth + 100.0f, m_WindowHeight + 100.0f); // Slightly larger to ensure full coverage
    fogInstance.rotation = 0.0f; // No rotation
    fogInstance.color = glm::vec4(1.0f); // White color (fog color handled in shader)
    fogInstance.texIndex = 0.0f;
    
    m_QuadBatch->Add(fogInstance);
    m_QuadBatch->End();
}

// Legacy function for backward compatibility
void FogRenderer2D::DrawFogQuad(const glm::vec2& playerPos, float radius, float softness, const glm::vec4& fogColor) {
    // Convert old parameters to new Guards and Thieves style system
    FogConfig legacyConfig;
    legacyConfig.range = radius;
    legacyConfig.shadowSoftness = softness * 0.01f; // Convert old softness to shadow softness
    legacyConfig.fogColor = fogColor;
    
    // Use omnidirectional line-of-sight (Guards and Thieves style)
    DrawFogQuad(playerPos, legacyConfig);
}

void FogRenderer2D::AddObstacle(const glm::vec2& position, const glm::vec2& size) {
    m_Obstacles.emplace_back(position, size);
}

void FogRenderer2D::AddObstacles(const std::vector<Obstacle>& obstacles) {
    m_Obstacles.insert(m_Obstacles.end(), obstacles.begin(), obstacles.end());
}

void FogRenderer2D::ClearObstacles() {
    m_Obstacles.clear();
}

void FogRenderer2D::RemoveObstacle(size_t index) {
    if (index < m_Obstacles.size()) {
        m_Obstacles.erase(m_Obstacles.begin() + index);
    }
}

void FogRenderer2D::SetFogConfig(const FogConfig& config) {
    m_Config = config;
}

const FogConfig& FogRenderer2D::GetFogConfig() const {
    return m_Config;
}

void FogRenderer2D::SetWindowSize(int width, int height) {
    m_WindowWidth = width;
    m_WindowHeight = height;
}

bool FogRenderer2D::IsPositionVisible(const glm::vec2& position, const glm::vec2& playerPos, 
                                     const glm::vec2& playerDirection, const FogConfig& config) const {
    return GetVisibilityAtPosition(position, playerPos, playerDirection, config) > 0.1f;
}

float FogRenderer2D::GetVisibilityAtPosition(const glm::vec2& position, const glm::vec2& playerPos, 
                                            const glm::vec2& playerDirection, const FogConfig& config) const {
    // Guards and Thieves style - omnidirectional visibility (ignore playerDirection)
    
    // 1. Check distance
    float distance = glm::length(position - playerPos);
    if (distance > config.range) return 0.0f;
    
    // 2. Check line of sight (omnidirectional)
    glm::vec2 rayDir = glm::normalize(position - playerPos);
    float rayLength = distance;
    
    for (const auto& obstacle : m_Obstacles) {
        float hitDistance;
        if (RayIntersectsBox(playerPos, rayDir, obstacle.position, obstacle.size, hitDistance)) {
            if (hitDistance < rayLength) {
                return 0.0f; // Blocked by obstacle
            }
        }
    }
    
    // 3. Apply distance falloff
    float falloff = 1.0f - (distance / config.range);
    return falloff;
}

void FogRenderer2D::SetDebugMode(bool enabled) {
    m_DebugMode = enabled;
}

void FogRenderer2D::DrawObstaclesDebug() {
    if (!m_DebugMode) return;
    
    // This method can be used with a separate debug renderer if needed
    // For now, we'll rely on the shader's debug visualization
    Logger::Info("Drawing " + std::to_string(m_Obstacles.size()) + " obstacles");
}

void FogRenderer2D::UpdateShaderUniforms(const glm::vec2& playerPos, const FogConfig& config) {
    // Vision parameters (omnidirectional for Guards and Thieves style)
    m_FogShader->SetVec2("uPlayerPos", playerPos);
    m_FogShader->SetFloat("uVisionRange", config.range);
    m_FogShader->SetVec4("uFogColor", config.fogColor);
    
    // Shadow parameters
    m_FogShader->SetFloat("uShadowSoftness", config.shadowSoftness);
    
    // Obstacle parameters
    int obstacleCount = std::min((int)m_Obstacles.size(), 32); // Limit to 32 obstacles
    m_FogShader->SetInt("uObstacleCount", obstacleCount);
    
    // Set obstacle positions and sizes
    for (int i = 0; i < obstacleCount; i++) {
        std::string posUniform = "uObstacles[" + std::to_string(i) + "]";
        std::string sizeUniform = "uObstacleSizes[" + std::to_string(i) + "]";
        
        m_FogShader->SetVec2(posUniform, m_Obstacles[i].position);
        m_FogShader->SetVec2(sizeUniform, m_Obstacles[i].size);
    }
}

bool FogRenderer2D::RayIntersectsBox(const glm::vec2& rayStart, const glm::vec2& rayDir, 
                                    const glm::vec2& boxCenter, const glm::vec2& boxSize, 
                                    float& hitDistance) const {
    glm::vec2 boxMin = boxCenter - boxSize * 0.5f;
    glm::vec2 boxMax = boxCenter + boxSize * 0.5f;
    
    // Handle division by zero
    glm::vec2 invDir;
    invDir.x = (rayDir.x != 0.0f) ? 1.0f / rayDir.x : 1e30f;
    invDir.y = (rayDir.y != 0.0f) ? 1.0f / rayDir.y : 1e30f;
    
    glm::vec2 t1 = (boxMin - rayStart) * invDir;
    glm::vec2 t2 = (boxMax - rayStart) * invDir;
    
    glm::vec2 tMin = glm::min(t1, t2);
    glm::vec2 tMax = glm::max(t1, t2);
    
    float tNear = std::max(tMin.x, tMin.y);
    float tFar = std::min(tMax.x, tMax.y);
    
    hitDistance = tNear;
    return tNear >= 0.0f && tNear <= tFar;
}

bool FogRenderer2D::IsInVisionCone(const glm::vec2& worldPos, const glm::vec2& playerPos, 
                                  const glm::vec2& playerDir, float visionAngle) const {
    glm::vec2 toPoint = glm::normalize(worldPos - playerPos);
    float angle = std::acos(glm::clamp(glm::dot(toPoint, playerDir), -1.0f, 1.0f));
    return angle <= visionAngle * 0.5f;
}

