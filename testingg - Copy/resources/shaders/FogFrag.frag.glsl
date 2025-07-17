#version 400 core
in vec2 vLocalPos;   // [-0.5, 0.5] quad local space
in vec2 vWorldPos;   // world position (pixels)
in vec2 vTexCoord;   // texture coordinates
out vec4 FragColor;

// Vision parameters
uniform vec2 uPlayerPos;        // player position (pixels, world space)
uniform float uVisionRange;     // maximum vision distance (pixels)
uniform vec4 uFogColor;         // RGBA for fog color (dark areas)

// Obstacle parameters
uniform int uObstacleCount;     // number of obstacles
uniform vec2 uObstacles[32];    // obstacle positions (up to 32)
uniform vec2 uObstacleSizes[32]; // obstacle sizes

// Shadow parameters
uniform float uShadowSoftness;  // edge softness for shadows

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

// Check line of sight from player to a point (omnidirectional)
float calculateLineOfSight(vec2 worldPos, vec2 playerPos) {
    vec2 rayDir = worldPos - playerPos;
    float rayLength = length(rayDir);
    
    if (rayLength == 0.0) return 1.0; // At player position
    
    rayDir = normalize(rayDir);
    
    float visibility = 1.0;
    
    // Check against all obstacles
    for (int i = 0; i < uObstacleCount && i < 32; i++) {
        float hitDistance;
        if (rayIntersectsBox(playerPos, rayDir, uObstacles[i], uObstacleSizes[i], hitDistance)) {
            if (hitDistance < rayLength) {
                // Obstacle blocks the view completely
                visibility = 0.0;
                break;
            }
        }
    }
    
    return visibility;
}

// Calculate shadow intensity from obstacles (for soft shadows behind obstacles)
float calculateShadowIntensity(vec2 worldPos) {
    float shadow = 0.0;
    
    for (int i = 0; i < uObstacleCount && i < 32; i++) {
        vec2 obstaclePos = uObstacles[i];
        vec2 obstacleSize = uObstacleSizes[i];
        
        // Vector from obstacle to current fragment
        vec2 obstacleToFrag = worldPos - obstaclePos;
        // Vector from player to obstacle
        vec2 playerToObstacle = obstaclePos - uPlayerPos;
        
        // Check if fragment is behind the obstacle relative to player
        if (dot(obstacleToFrag, playerToObstacle) > 0.0) {
            // Calculate distance from player and obstacle
            float distFromPlayer = length(worldPos - uPlayerPos);
            float obstacleDistFromPlayer = length(playerToObstacle);
            
            if (distFromPlayer > obstacleDistFromPlayer) {
                // Cast shadow ray
                vec2 shadowDir = normalize(playerToObstacle);
                vec2 shadowRay = uPlayerPos + shadowDir * distFromPlayer;
                
                // Check if current fragment is in shadow
                vec2 perpendicular = vec2(-shadowDir.y, shadowDir.x);
                vec2 toFrag = worldPos - shadowRay;
                float perpDist = abs(dot(toFrag, perpendicular));
                
                // Shadow width based on obstacle size and distance
                float shadowWidth = max(obstacleSize.x, obstacleSize.y) * 0.5;
                shadowWidth *= (distFromPlayer / obstacleDistFromPlayer); // Shadow grows with distance
                shadowWidth *= 1.5; // Make shadows a bit wider for better visibility
                
                if (perpDist < shadowWidth) {
                    float shadowIntensity = 1.0 - smoothstep(0.0, shadowWidth * uShadowSoftness, perpDist);
                    shadow = max(shadow, shadowIntensity);
                }
            }
        }
    }
    
    return shadow;
}

void main() {
    vec2 toFragment = vWorldPos - uPlayerPos;
    float distanceToPlayer = length(toFragment);
    
    // Base visibility (start with full visibility)
    float visibility = 1.0;
    
    // 1. Check if within vision range
    if (distanceToPlayer > uVisionRange) {
        visibility = 0.0;
    }
    
    // 2. Check line of sight (raycasting against obstacles) - omnidirectional
    if (visibility > 0.0) {
        visibility *= calculateLineOfSight(vWorldPos, uPlayerPos);
    }
    
    // 3. Apply shadow casting for softer shadows behind obstacles
    if (visibility > 0.0) {
        float shadowIntensity = calculateShadowIntensity(vWorldPos);
        visibility *= (1.0 - shadowIntensity);
    }
    
    // 4. Apply distance-based falloff for more realistic visibility
    if (visibility > 0.0) {
        float falloff = 1.0 - smoothstep(uVisionRange * 0.7, uVisionRange, distanceToPlayer);
        visibility *= falloff;
    }
    
    // Calculate final darkness (inverted visibility)
    // Where visibility = 1.0 (can see) -> show no fog (bright)
    // Where visibility = 0.0 (blocked) -> show full fog (dark)
    float darkness = 1.0 - visibility;
    
    // Output the darkness overlay
    FragColor = vec4(uFogColor.rgb, uFogColor.a * darkness);
    
    // DEBUG: Show player position as a bright dot
    if (distance(vWorldPos, uPlayerPos) < 8.0) {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Bright yellow dot at player position
    }
}