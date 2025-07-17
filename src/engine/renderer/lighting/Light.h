#pragma once
#include <glm/glm.hpp>
#include <vector>

enum class LightType {
    POINT_LIGHT,        // Omnidirectional light (like a light bulb)
    DIRECTIONAL_LIGHT,  // Parallel light rays (like sunlight)
    SPOT_LIGHT         // Cone-shaped light (like a flashlight)
};

// Structure to represent a light source
struct Light {
    LightType type;
    glm::vec2 position;            // Light position (not used for directional lights)
    glm::vec2 direction;           // Light direction (for directional and spot lights)
    glm::vec3 color;               // Light color (RGB)
    float intensity;               // Light brightness (0.0 to 2.0)
    float range;                   // Light radius/distance (not used for directional lights)
    float innerAngle;              // Inner cone angle in radians (for spot lights)
    float outerAngle;              // Outer cone angle in radians (for spot lights)
    float bloom;                   // Bloom effect intensity
    
    // Deprecated field - kept for compatibility but use type instead
    bool isDirectional;

    // Constructor for Point Light (omnidirectional)
    Light(const glm::vec2& pos, float lightRange, const glm::vec3& lightColor = glm::vec3(1.0f), 
          float lightIntensity = 1.0f, float lightBloom = 0.0f)
        : type(LightType::POINT_LIGHT), position(pos), direction(0.0f, -1.0f), 
          color(lightColor), intensity(lightIntensity), range(lightRange), 
          innerAngle(0.0f), outerAngle(2.0f * 3.14159f), bloom(lightBloom), isDirectional(false) {}
    
    // Constructor for Spot Light (cone-shaped)
    Light(const glm::vec2& pos, const glm::vec2& dir, float lightRange, float coneAngle, 
          const glm::vec3& lightColor = glm::vec3(1.0f), float lightIntensity = 1.0f, float lightBloom = 0.0f)
        : type(LightType::SPOT_LIGHT), position(pos), direction(glm::normalize(dir)), 
          color(lightColor), intensity(lightIntensity), range(lightRange), 
          innerAngle(coneAngle * 0.8f), outerAngle(coneAngle), bloom(lightBloom), isDirectional(true) {}
    
    // Constructor for Directional Light (parallel rays like sunlight)
    static Light CreateDirectionalLight(const glm::vec2& dir, const glm::vec3& lightColor = glm::vec3(1.0f), 
                                       float lightIntensity = 1.0f, float lightBloom = 0.0f) {
        Light light;
        light.type = LightType::DIRECTIONAL_LIGHT;
        light.position = glm::vec2(0.0f); // Not used for directional lights
        light.direction = glm::normalize(dir);
        light.color = lightColor;
        light.intensity = lightIntensity;
        light.range = 0.0f; // Not used for directional lights
        light.innerAngle = 0.0f;
        light.outerAngle = 0.0f;
        light.bloom = lightBloom;
        light.isDirectional = true;
        return light;
    }
    
    // Constructor for Spot Light with separate inner and outer angles
    static Light CreateSpotLight(const glm::vec2& pos, const glm::vec2& dir, float lightRange, 
                                float innerConeAngle, float outerConeAngle, 
                                const glm::vec3& lightColor = glm::vec3(1.0f), 
                                float lightIntensity = 1.0f, float lightBloom = 0.0f) {
        Light light;
        light.type = LightType::SPOT_LIGHT;
        light.position = pos;
        light.direction = glm::normalize(dir);
        light.color = lightColor;
        light.intensity = lightIntensity;
        light.range = lightRange;
        light.innerAngle = innerConeAngle;
        light.outerAngle = outerConeAngle;
        light.bloom = lightBloom;
        light.isDirectional = true;
        return light;
    }

private:
    // Private default constructor for static factory methods
    Light() = default;
};

// Structure to hold lighting parameters
struct LightConfig {
    float shadowSoftness = 0.5f;                      // Edge softness for shadows
    float ambientLight = 0.1f;                        // Ambient light level (0.0 to 1.0)
    glm::vec3 ambientColor = glm::vec3(0.2f, 0.2f, 0.3f); // Ambient light color
    float shadowLength = 1000.0f;                     // How far shadows extend
    bool enableShadows = true;                        // Enable/disable shadow casting
    float bloom = 0.0f;                               // Global bloom effect intensity (0.0 to 1.0)
    LightType lightType = LightType::POINT_LIGHT;     // Default light type for new lights
}; 