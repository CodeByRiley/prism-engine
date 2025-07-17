#include "VisionRenderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <engine/utils/Logger.h>
#include <algorithm>
#include <cmath>

VisionRenderer2D::VisionRenderer2D(int windowWidth, int windowHeight)
    : m_WindowWidth(windowWidth), m_WindowHeight(windowHeight), m_DebugMode(false)
{
    m_VisionShader = new Shader("shaders/VisionVertex.vert.glsl", "shaders/VisionFrag.frag.glsl");
    m_QuadBatch = new QuadBatch();
    Logger::Info("Vision shader created with ID: " + std::to_string(m_VisionShader->GetID()));
}

VisionRenderer2D::~VisionRenderer2D() {
    delete m_QuadBatch;
    delete m_VisionShader;
}

void VisionRenderer2D::DrawVisionOverlay(const glm::vec2& playerPos, const glm::vec2& playerDirection, 
                                        const VisionConfig& config) {
    // Start the quad batch with our vision shader
    m_QuadBatch->Begin(m_VisionShader);
    
    // Create the projection matrix for the current window size
    glm::mat4 projection = glm::ortho(0.0f, (float)m_WindowWidth, (float)m_WindowHeight, 0.0f);
    
    // Set basic uniforms
    m_VisionShader->SetMat4("uProjection", projection);
    
    // Update all shader uniforms
    UpdateShaderUniforms(playerPos, playerDirection, config);
    
    // Create a quad that covers the entire screen
    QuadInstance visionInstance;
    visionInstance.position = glm::vec2(m_WindowWidth * 0.5f, m_WindowHeight * 0.5f);
    visionInstance.size = glm::vec2(m_WindowWidth + 100.0f, m_WindowHeight + 100.0f); // Slightly larger for full coverage
    visionInstance.rotation = 0.0f;
    visionInstance.color = glm::vec4(1.0f); // White color (actual color handled in shader)
    visionInstance.texIndex = 0.0f;
    
    m_QuadBatch->Add(visionInstance);
    m_QuadBatch->End();
}

void VisionRenderer2D::AddObstacle(const glm::vec2& position, const glm::vec2& size) {
    m_Obstacles.emplace_back(position, size);
}

void VisionRenderer2D::AddObstacle(const Obstacle& obstacle) {
    m_Obstacles.push_back(obstacle);
}

void VisionRenderer2D::ClearObstacles() {
    m_Obstacles.clear();
}

void VisionRenderer2D::RemoveObstacle(size_t index) {
    if (index < m_Obstacles.size()) {
        m_Obstacles.erase(m_Obstacles.begin() + index);
    }
}

void VisionRenderer2D::SetVisionConfig(const VisionConfig& config) {
    m_Config = config;
}

const VisionConfig& VisionRenderer2D::GetVisionConfig() const {
    return m_Config;
}

void VisionRenderer2D::SetWindowSize(int width, int height) {
    m_WindowWidth = width;
    m_WindowHeight = height;
}

bool VisionRenderer2D::IsPositionVisible(const glm::vec2& position, const glm::vec2& playerPos, 
                                        const glm::vec2& playerDirection, const VisionConfig& config) const {
    return GetVisibilityAtPosition(position, playerPos, playerDirection, config) > 0.1f;
}

float VisionRenderer2D::GetVisibilityAtPosition(const glm::vec2& position, const glm::vec2& playerPos, 
                                               const glm::vec2& playerDirection, const VisionConfig& config) const {
    // 1. Check distance
    float distance = glm::length(position - playerPos);
    if (distance > config.range) return 0.0f;
    
    // 2. Check vision cone
    if (!IsInVisionCone(position, playerPos, playerDirection, config.angle)) return 0.0f;
    
    // 3. Check line of sight
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
    
    // 4. Apply distance falloff
    float falloff = 1.0f - (distance / config.range);
    return falloff;
}

void VisionRenderer2D::SetDebugMode(bool enabled) {
    m_DebugMode = enabled;
}

void VisionRenderer2D::DrawObstaclesDebug() {
    if (!m_DebugMode) return;
    
    // This method can be used with a separate debug renderer if needed
    // For now, we'll rely on the shader's debug visualization
    Logger::Info("Drawing " + std::to_string(m_Obstacles.size()) + " obstacles");
}

void VisionRenderer2D::AddObstacles(const std::vector<Obstacle>& obstacles) {
    m_Obstacles.insert(m_Obstacles.end(), obstacles.begin(), obstacles.end());
}

void VisionRenderer2D::UpdateShaderUniforms(const glm::vec2& playerPos, const glm::vec2& playerDirection, 
                                           const VisionConfig& config) {
    // Vision parameters
    m_VisionShader->SetVec2("uPlayerPos", playerPos);
    m_VisionShader->SetVec2("uPlayerDirection", glm::normalize(playerDirection));
    m_VisionShader->SetFloat("uVisionRange", config.range);
    m_VisionShader->SetFloat("uVisionAngle", config.angle);
    m_VisionShader->SetVec4("uDarkColor", config.darkColor);
    
    // Shadow parameters
    m_VisionShader->SetFloat("uShadowLength", config.shadowLength);
    m_VisionShader->SetFloat("uShadowSoftness", config.shadowSoftness);
    
    // Obstacle parameters
    int obstacleCount = std::min((int)m_Obstacles.size(), 32); // Limit to 32 obstacles
    m_VisionShader->SetInt("uObstacleCount", obstacleCount);
    
    // Set obstacle positions and sizes
    for (int i = 0; i < obstacleCount; i++) {
        std::string posUniform = "uObstacles[" + std::to_string(i) + "]";
        std::string sizeUniform = "uObstacleSizes[" + std::to_string(i) + "]";
        
        m_VisionShader->SetVec2(posUniform, m_Obstacles[i].position);
        m_VisionShader->SetVec2(sizeUniform, m_Obstacles[i].size);
    }
}

bool VisionRenderer2D::RayIntersectsBox(const glm::vec2& rayStart, const glm::vec2& rayDir, 
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

bool VisionRenderer2D::IsInVisionCone(const glm::vec2& worldPos, const glm::vec2& playerPos, 
                                     const glm::vec2& playerDir, float visionAngle) const {
    glm::vec2 toPoint = glm::normalize(worldPos - playerPos);
    float angle = std::acos(glm::clamp(glm::dot(toPoint, playerDir), -1.0f, 1.0f));
    return angle <= visionAngle * 0.5f;
} 