#include "LightRenderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <engine/utils/Logger.h>
#include <algorithm>
#include <cmath>
#include "../vision/VisionRenderer2D.h" // For Obstacle struct

LightRenderer2D::LightRenderer2D(int windowWidth, int windowHeight)
    : m_WindowWidth(windowWidth), m_WindowHeight(windowHeight), m_DebugMode(false)
{
    m_LightShader = new Shader("shaders/LightVertex.vert.glsl", "shaders/LightFrag.frag.glsl");
    m_QuadBatch = new QuadBatch();
    Logger::Info("Light shader created with ID: " + std::to_string(m_LightShader->GetID()));
}

LightRenderer2D::~LightRenderer2D() {
    delete m_QuadBatch;
    delete m_LightShader;
}

void LightRenderer2D::DrawLightingOverlay(const std::vector<Light>& lights, const LightConfig& config) {
    // Start the quad batch with our light shader
    m_QuadBatch->Begin(m_LightShader);
    
    // Create the projection matrix for the current window size
    glm::mat4 projection = glm::ortho(0.0f, (float)m_WindowWidth, (float)m_WindowHeight, 0.0f);
    
    // Set basic uniforms
    m_LightShader->SetMat4("uProjection", projection);
    
    // Update all shader uniforms
    UpdateShaderUniforms(lights, config);
    
    // Create a quad that covers the entire screen
    QuadInstance lightInstance;
    lightInstance.position = glm::vec2(m_WindowWidth * 0.5f, m_WindowHeight * 0.5f);
    lightInstance.size = glm::vec2(m_WindowWidth + 100.0f, m_WindowHeight + 100.0f); // Slightly larger for full coverage
    lightInstance.rotation = 0.0f;
    lightInstance.color = glm::vec4(1.0f); // White color (actual lighting handled in shader)
    lightInstance.texIndex = 0.0f;
    
    m_QuadBatch->Add(lightInstance);
    m_QuadBatch->End();
}

void LightRenderer2D::AddLight(const Light& light) {
    m_Lights.push_back(light);
}

void LightRenderer2D::AddLights(const std::vector<Light>& lights) {
    m_Lights.insert(m_Lights.end(), lights.begin(), lights.end());
}

void LightRenderer2D::ClearLights() {
    m_Lights.clear();
}

void LightRenderer2D::RemoveLight(size_t index) {
    if (index < m_Lights.size()) {
        m_Lights.erase(m_Lights.begin() + index);
    }
}

void LightRenderer2D::UpdateLight(size_t index, const Light& light) {
    if (index < m_Lights.size()) {
        m_Lights[index] = light;
    }
}

void LightRenderer2D::AddObstacle(const glm::vec2& position, const glm::vec2& size) {
    m_Obstacles.emplace_back(position, size);
}

void LightRenderer2D::AddObstacles(const std::vector<Obstacle>& obstacles) {
    m_Obstacles.insert(m_Obstacles.end(), obstacles.begin(), obstacles.end());
}

void LightRenderer2D::ClearObstacles() {
    m_Obstacles.clear();
}

void LightRenderer2D::RemoveObstacle(size_t index) {
    if (index < m_Obstacles.size()) {
        m_Obstacles.erase(m_Obstacles.begin() + index);
    }
}

void LightRenderer2D::SetLightConfig(const LightConfig& config) {
    m_Config = config;
}

const LightConfig& LightRenderer2D::GetLightConfig() const {
    return m_Config;
}

void LightRenderer2D::SetWindowSize(int width, int height) {
    m_WindowWidth = width;
    m_WindowHeight = height;
}

bool LightRenderer2D::IsPositionLit(const glm::vec2& position, float threshold) const {
    return GetLightIntensityAtPosition(position) > threshold;
}

float LightRenderer2D::GetLightIntensityAtPosition(const glm::vec2& position) const {
    float totalIntensity = m_Config.ambientLight;
    
    for (const auto& light : m_Lights) {
        totalIntensity += CalculateLightContribution(light, position);
    }
    
    return std::min(totalIntensity, 2.0f); // Clamp to reasonable values
}

glm::vec3 LightRenderer2D::GetLightColorAtPosition(const glm::vec2& position) const {
    glm::vec3 totalColor = m_Config.ambientColor * m_Config.ambientLight;
    
    for (const auto& light : m_Lights) {
        float contribution = CalculateLightContribution(light, position);
        totalColor += light.color * contribution;
    }
    
    return glm::clamp(totalColor, glm::vec3(0.0f), glm::vec3(2.0f));
}

void LightRenderer2D::SetDebugMode(bool enabled) {
    m_DebugMode = enabled;
}

void LightRenderer2D::DrawLightsDebug() {
    if (!m_DebugMode) return;
    
    Logger::Info("Drawing " + std::to_string(m_Lights.size()) + " lights and " + 
                std::to_string(m_Obstacles.size()) + " obstacles");
}

void LightRenderer2D::UpdateShaderUniforms(const std::vector<Light>& lights, const LightConfig& config) {
    // Global lighting parameters
    m_LightShader->SetFloat("uShadowSoftness", config.shadowSoftness);
    m_LightShader->SetFloat("uAmbientLight", config.ambientLight);
    m_LightShader->SetVec3("uAmbientColor", config.ambientColor);
    m_LightShader->SetFloat("uShadowLength", config.shadowLength);
    m_LightShader->SetBool("uEnableShadows", config.enableShadows);
    
    // Light parameters
    int lightCount = std::min((int)lights.size(), 16); // Limit to 16 lights
    m_LightShader->SetInt("uLightCount", lightCount);
    
    // Set light properties
    for (int i = 0; i < lightCount; i++) {
        const Light& light = lights[i];
        
        std::string posUniform = "uLightPositions[" + std::to_string(i) + "]";
        std::string dirUniform = "uLightDirections[" + std::to_string(i) + "]";
        std::string rangeUniform = "uLightRanges[" + std::to_string(i) + "]";
        std::string innerAngleUniform = "uLightInnerAngles[" + std::to_string(i) + "]";
        std::string outerAngleUniform = "uLightOuterAngles[" + std::to_string(i) + "]";
        std::string intensityUniform = "uLightIntensities[" + std::to_string(i) + "]";
        std::string colorUniform = "uLightColors[" + std::to_string(i) + "]";
        std::string typeUniform = "uLightTypes[" + std::to_string(i) + "]";

        m_LightShader->SetVec2(posUniform, light.position);
        m_LightShader->SetVec2(dirUniform, light.direction);
        m_LightShader->SetFloat(rangeUniform, light.range);
        m_LightShader->SetFloat(innerAngleUniform, light.innerAngle);
        m_LightShader->SetFloat(outerAngleUniform, light.outerAngle);
        m_LightShader->SetFloat(intensityUniform, light.intensity);
        m_LightShader->SetVec3(colorUniform, light.color);
        m_LightShader->SetInt(typeUniform, static_cast<int>(light.type));
    }
    
    // Obstacle parameters
    int obstacleCount = std::min((int)m_Obstacles.size(), 32); // Limit to 32 obstacles
    m_LightShader->SetInt("uObstacleCount", obstacleCount);
    
    // Set obstacle positions and sizes
    for (int i = 0; i < obstacleCount; i++) {
        std::string posUniform = "uObstacles[" + std::to_string(i) + "]";
        std::string sizeUniform = "uObstacleSizes[" + std::to_string(i) + "]";
        
        m_LightShader->SetVec2(posUniform, m_Obstacles[i].position);
        m_LightShader->SetVec2(sizeUniform, m_Obstacles[i].size);
    }
}

bool LightRenderer2D::RayIntersectsBox(const glm::vec2& rayStart, const glm::vec2& rayDir, 
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

bool LightRenderer2D::IsInLightCone(const glm::vec2& worldPos, const glm::vec2& lightPos, 
                                   const glm::vec2& lightDir, float lightAngle, bool isDirectional) const {
    if (!isDirectional) return true; // Omnidirectional light
    
    glm::vec2 toPoint = glm::normalize(worldPos - lightPos);
    float angle = std::acos(glm::clamp(glm::dot(toPoint, lightDir), -1.0f, 1.0f));
    return angle <= lightAngle * 0.5f;
}

float LightRenderer2D::CalculateLightContribution(const Light& light, const glm::vec2& position) const {
    float attenuation = 1.0f;
    float spotAttenuation = 1.0f;
    
    // Calculate attenuation based on light type
    if (light.type == LightType::POINT_LIGHT) {
        // Point light: distance-based attenuation with smooth radial falloff
        float distance = glm::length(position - light.position);
        if (distance > light.range) return 0.0f;
        
        // Gentler attenuation for good light dispersion (matches shader)
        float normalizedDistance = distance / light.range;
        
        // Smoother falloff curve - not too aggressive
        attenuation = 1.0f / (1.0f + 0.05f * distance + 0.01f * distance * distance);
        
        // Smooth edge falloff
        attenuation *= (1.0f - glm::smoothstep(light.range * 0.7f, light.range, distance));
        
        // Gentle power curve for natural light dispersion (less aggressive than 1.5)
        attenuation = std::pow(attenuation, 0.8f);
        
    } else if (light.type == LightType::DIRECTIONAL_LIGHT) {
        // Directional light: no distance attenuation (like sunlight)
        attenuation = 1.0f;
        
    } else if (light.type == LightType::SPOT_LIGHT) {
        // Spot light: distance + cone attenuation
        float distance = glm::length(position - light.position);
        if (distance > light.range) return 0.0f;
        
        // Similar gentler attenuation for spot lights (matches shader)
        float normalizedDistance = distance / light.range;
        attenuation = 1.0f / (1.0f + 0.05f * distance + 0.01f * distance * distance);
        attenuation *= (1.0f - glm::smoothstep(light.range * 0.7f, light.range, distance));
        attenuation = std::pow(attenuation, 0.8f);
        
        // Cone attenuation
        glm::vec2 toPoint = glm::normalize(position - light.position);
        float angle = std::acos(glm::clamp(glm::dot(toPoint, light.direction), -1.0f, 1.0f));
        
        // Inside inner cone - full intensity
        if (angle <= light.innerAngle * 0.5f) {
            spotAttenuation = 1.0f;
        }
        // Outside outer cone - no light
        else if (angle > light.outerAngle * 0.5f) {
            return 0.0f;
        }
        // Between inner and outer cone - smooth falloff
        else {
            float falloffStart = light.innerAngle * 0.5f;
            float falloffEnd = light.outerAngle * 0.5f;
            spotAttenuation = 1.0f - glm::smoothstep(falloffStart, falloffEnd, angle);
        }
    }
    
    // Check line of sight (simplified for CPU calculation)
    if (m_Config.enableShadows) {
        glm::vec2 rayStart, rayDir;
        float rayLength;
        
        if (light.type == LightType::DIRECTIONAL_LIGHT) {
            // For directional lights, ray goes from position towards light direction
            rayStart = position;
            rayDir = -light.direction;
            rayLength = m_Config.shadowLength;
        } else {
            // For point and spot lights, ray goes from light to position
            rayStart = light.position;
            rayDir = glm::normalize(position - light.position);
            rayLength = glm::length(position - light.position);
        }
        
        for (const auto& obstacle : m_Obstacles) {
            float hitDistance;
            if (RayIntersectsBox(rayStart, rayDir, obstacle.position, obstacle.size, hitDistance)) {
                if (light.type == LightType::DIRECTIONAL_LIGHT) {
                    // For directional lights, any intersection blocks the light
                    if (hitDistance >= 0.0f && hitDistance < rayLength) {
                        attenuation = 0.0f;
                        break;
                    }
                } else {
                    // For point and spot lights, check if obstacle is between light and point
                    if (hitDistance < rayLength) {
                        attenuation *= 0.1f; // Heavily attenuate if blocked
                        break;
                    }
                }
            }
        }
    }
    
    return light.intensity * attenuation * spotAttenuation;
}

// Convenience methods for creating specific light types
void LightRenderer2D::AddPointLight(const glm::vec2& position, float range, const glm::vec3& color, float intensity) {
    m_Lights.emplace_back(position, range, color, intensity);
}

void LightRenderer2D::AddSpotLight(const glm::vec2& position, const glm::vec2& direction, float range, float angle, 
                                  const glm::vec3& color, float intensity) {
    m_Lights.emplace_back(position, direction, range, angle, color, intensity);
}

void LightRenderer2D::AddDirectionalLight(const glm::vec2& direction, const glm::vec3& color, float intensity) {
    m_Lights.push_back(Light::CreateDirectionalLight(direction, color, intensity));
}

void LightRenderer2D::AddAdvancedSpotLight(const glm::vec2& position, const glm::vec2& direction, float range, 
                                          float innerAngle, float outerAngle, const glm::vec3& color, float intensity) {
    m_Lights.push_back(Light::CreateSpotLight(position, direction, range, innerAngle, outerAngle, color, intensity));
} 