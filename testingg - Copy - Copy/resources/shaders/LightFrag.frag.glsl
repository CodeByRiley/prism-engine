#version 400 core
in vec2 vLocalPos;   // [-0.5, 0.5] quad local space
in vec2 vWorldPos;   // world position (pixels)
in vec2 vTexCoord;   // texture coordinates
out vec4 FragColor;

// Light parameters
uniform int uLightCount;               // number of lights
uniform vec2 uLightPositions[16];      // light positions (up to 16)
uniform vec2 uLightDirections[16];     // light directions (for directional and spot lights)
uniform float uLightRanges[16];        // light ranges (not used for directional lights)
uniform float uLightInnerAngles[16];   // light inner cone angles (for spot lights)
uniform float uLightOuterAngles[16];   // light outer cone angles (for spot lights)
uniform float uLightIntensities[16];   // light intensities
uniform vec3 uLightColors[16];         // light colors
uniform int uLightTypes[16];           // light types (0=point, 1=directional, 2=spot)

// Obstacle parameters
uniform int uObstacleCount;            // number of obstacles
uniform vec2 uObstacles[32];           // obstacle positions (up to 32)
uniform vec2 uObstacleSizes[32];       // obstacle sizes

// Global lighting parameters
uniform float uShadowSoftness;         // edge softness for shadows
uniform float uAmbientLight;           // ambient light level
uniform vec3 uAmbientColor;            // ambient light color
uniform float uShadowLength;           // how far shadows extend
uniform int uEnableShadows;            // enable/disable shadows (0 or 1)

// Light type constants
const int POINT_LIGHT = 0;
const int DIRECTIONAL_LIGHT = 1;
const int SPOT_LIGHT = 2;

// Helper function to check if a point is in a spot light cone
float getSpotLightAttenuation(vec2 worldPos, vec2 lightPos, vec2 lightDir, float innerAngle, float outerAngle) {
    vec2 toPoint = normalize(worldPos - lightPos);
    float angle = acos(clamp(dot(toPoint, lightDir), -1.0, 1.0));
    
    // Inside inner cone - full intensity
    if (angle <= innerAngle * 0.5) {
        return 1.0;
    }
    // Outside outer cone - no light
    else if (angle > outerAngle * 0.5) {
        return 0.0;
    }
    // Between inner and outer cone - smooth falloff
    else {
        float falloffStart = innerAngle * 0.5;
        float falloffEnd = outerAngle * 0.5;
        return 1.0 - smoothstep(falloffStart, falloffEnd, angle);
    }
}

// Ray-box intersection for obstacles
bool rayIntersectsBox(vec2 rayStart, vec2 rayDir, vec2 boxCenter, vec2 boxSize, out float hitDistance) {
    vec2 boxMin = boxCenter - boxSize * 0.5;
    vec2 boxMax = boxCenter + boxSize * 0.5;
    
    vec2 invDir = 1.0 / rayDir;
    vec2 t1 = (boxMin - rayStart) * invDir;
    vec2 t2 = (boxMax - rayStart) * invDir;
    
    vec2 tMin = min(t1, t2);
    vec2 tMax = max(t1, t2);
    
    float tNear = max(tMin.x, tMin.y);
    float tFar = min(tMax.x, tMax.y);
    
    hitDistance = tNear;
    return tNear >= 0.0 && tNear <= tFar;
}

// Check line of sight from light to a point
float calculateLineOfSight(vec2 worldPos, vec2 lightPos, int lightType) {
    if (uEnableShadows == 0) return 1.0;
    
    // For directional lights, use the opposite direction to cast shadows
    vec2 rayStart, rayDir;
    float rayLength;
    
    if (lightType == DIRECTIONAL_LIGHT) {
        // For directional lights, the ray goes from the point towards the light direction
        rayStart = worldPos;
        rayDir = -uLightDirections[0]; // Assuming we're checking the first directional light
        rayLength = uShadowLength; // Use a large distance for directional lights
    } else {
        // For point and spot lights, ray goes from light to point
        rayStart = lightPos;
        rayDir = worldPos - lightPos;
        rayLength = length(rayDir);
        
        if (rayLength == 0.0) return 1.0; // At light position
        rayDir = normalize(rayDir);
    }
    
    float visibility = 1.0;
    
    // Check against all obstacles
    for (int i = 0; i < uObstacleCount && i < 32; i++) {
        float hitDistance;
        if (rayIntersectsBox(rayStart, rayDir, uObstacles[i], uObstacleSizes[i], hitDistance)) {
            if (lightType == DIRECTIONAL_LIGHT) {
                // For directional lights, any intersection blocks the light
                if (hitDistance >= 0.0 && hitDistance < rayLength) {
                    visibility = 0.0;
                    break;
                }
            } else {
                // For point and spot lights, check if obstacle is between light and point
                if (hitDistance < rayLength) {
                    float shadowFactor = 1.0 - smoothstep(hitDistance, hitDistance + uShadowSoftness * 10.0, rayLength);
                    visibility *= shadowFactor;
                    
                    if (visibility <= 0.01) {
                        visibility = 0.0;
                        break;
                    }
                }
            }
        }
    }
    
    return visibility;
}

// Calculate lighting contribution from a single light
vec3 calculateLightContribution(int lightIndex, vec2 worldPos) {
    vec2 lightPos = uLightPositions[lightIndex];
    vec2 lightDir = uLightDirections[lightIndex];
    float lightRange = uLightRanges[lightIndex];
    float lightInnerAngle = uLightInnerAngles[lightIndex];
    float lightOuterAngle = uLightOuterAngles[lightIndex];
    float lightIntensity = uLightIntensities[lightIndex];
    vec3 lightColor = uLightColors[lightIndex];
    int lightType = uLightTypes[lightIndex];
    
    float attenuation = 1.0;
    float spotAttenuation = 1.0;
    
    // Calculate attenuation based on light type
    if (lightType == POINT_LIGHT) {
        // Point light: distance-based attenuation with smooth radial falloff
        float distance = length(worldPos - lightPos);
        if (distance > lightRange) return vec3(0.0);
        
        // Gentler attenuation for good light dispersion
        float normalizedDistance = distance / lightRange;
        
        // Smoother falloff curve - not too aggressive
        attenuation = 1.0 / (1.0 + 0.05 * distance + 0.01 * distance * distance);
        
        // Smooth edge falloff
        attenuation *= (1.0 - smoothstep(lightRange * 0.7, lightRange, distance));
        
        // Gentle power curve for natural light dispersion (less aggressive than 1.5)
        attenuation = pow(attenuation, 0.8);
        
    } else if (lightType == DIRECTIONAL_LIGHT) {
        // Directional light: no distance attenuation (like sunlight)
        attenuation = 1.0;
        
    } else if (lightType == SPOT_LIGHT) {
        // Spot light: distance + cone attenuation
        float distance = length(worldPos - lightPos);
        if (distance > lightRange) return vec3(0.0);
        
        // Similar gentler attenuation for spot lights
        float normalizedDistance = distance / lightRange;
        attenuation = 1.0 / (1.0 + 0.05 * distance + 0.01 * distance * distance);
        attenuation *= (1.0 - smoothstep(lightRange * 0.7, lightRange, distance));
        attenuation = pow(attenuation, 0.8);
        
        // Cone attenuation
        spotAttenuation = getSpotLightAttenuation(worldPos, lightPos, lightDir, lightInnerAngle, lightOuterAngle);
        if (spotAttenuation == 0.0) return vec3(0.0);
    }
    
    // Check line of sight (shadows)
    float visibility = calculateLineOfSight(worldPos, lightPos, lightType);
    
    // Calculate final light contribution
    float finalIntensity = lightIntensity * attenuation * spotAttenuation * visibility;
    return lightColor * finalIntensity;
}

void main() {
    // Start with ambient lighting
    vec3 finalColor = uAmbientColor * uAmbientLight;
    
    // Add contribution from all lights
    for (int i = 0; i < uLightCount && i < 16; i++) {
        finalColor += calculateLightContribution(i, vWorldPos);
    }
    
    // Clamp to reasonable values
    finalColor = clamp(finalColor, 0.0, 2.0);
    
    // Output the final lit color
    FragColor = vec4(finalColor, 1.0);
    
    // DEBUG: Show light positions as colored dots
    for (int i = 0; i < uLightCount && i < 16; i++) {
        if (uLightTypes[i] != DIRECTIONAL_LIGHT && distance(vWorldPos, uLightPositions[i]) < 8.0) {
            FragColor = vec4(uLightColors[i], 1.0); // Show light position
        }
    }
} 