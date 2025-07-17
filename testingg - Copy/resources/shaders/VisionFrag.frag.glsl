#version 400 core
in vec2 vLocalPos;   // [-0.5, 0.5] quad local space
in vec2 vWorldPos;   // world position (pixels)
in vec2 vTexCoord;   // texture coordinates
out vec4 FragColor;

// Vision parameters
uniform vec2 uPlayerPos;        // player position (pixels, world space)
uniform vec2 uPlayerDirection;  // player facing direction (normalized)
uniform float uVisionRange;     // maximum vision distance (pixels)
uniform float uVisionAngle;     // vision cone angle in radians (e.g., PI/3 for 60 degrees)
uniform vec4 uDarkColor;        // color for areas outside vision

// Obstacle parameters
uniform int uObstacleCount;     // number of obstacles
uniform vec2 uObstacles[32];    // obstacle positions (up to 32)
uniform vec2 uObstacleSizes[32]; // obstacle sizes

// Shadow parameters
uniform float uShadowLength;    // how far shadows extend
uniform float uShadowSoftness;  // edge softness for shadows

// Helper function to calculate vision cone fade (0.0 = outside, 1.0 = center)
float getVisionConeFade(vec2 worldPos, vec2 playerPos, vec2 playerDir, float visionAngle) {
    vec2 toPoint = normalize(worldPos - playerPos);
    float angle = acos(clamp(dot(toPoint, playerDir), -1.0, 1.0));
    
    float halfAngle = visionAngle * 0.5;
    float fadeZone = halfAngle * 0.15; // 15% of the cone angle for fade zone
    
    if (angle <= halfAngle - fadeZone) {
        return 1.0; // Full visibility in center
    } else if (angle <= halfAngle) {
        // Smooth fade at edges
        return 1.0 - smoothstep(halfAngle - fadeZone, halfAngle, angle);
    } else {
        return 0.0; // Outside cone
    }
}

// Helper function to calculate distance fade (0.0 = max range, 1.0 = close)
float getDistanceFade(float distance, float maxRange) {
    float fadeStart = maxRange * 0.7; // Start fading at 70% of max range
    return 1.0 - smoothstep(fadeStart, maxRange, distance);
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

// Check line of sight from player to a point
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
                // Obstacle blocks the view with soft edges
                float shadowFactor = 1.0 - smoothstep(hitDistance, hitDistance + uShadowSoftness * 20.0, rayLength);
                visibility *= shadowFactor;
                
                // If mostly blocked, break early
                if (visibility <= 0.05) {
                    visibility = 0.0;
                    break;
                }
            }
        }
    }
    
    return visibility;
}

// Calculate shadow intensity from obstacles
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
            // Calculate distance from obstacle edge
            vec2 obstacleMin = obstaclePos - obstacleSize * 0.5;
            vec2 obstacleMax = obstaclePos + obstacleSize * 0.5;
            
            // Check if in shadow region
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
    
    // Start with full visibility
    float visibility = 1.0;
    
    // 1. Apply vision cone fade (soft edges)
    float coneFade = getVisionConeFade(vWorldPos, uPlayerPos, uPlayerDirection, uVisionAngle);
    visibility *= coneFade;
    
    // 2. Apply distance fade (soft range limit)
    float distanceFade = getDistanceFade(distanceToPlayer, uVisionRange);
    visibility *= distanceFade;
    
    // 3. Check line of sight (raycasting against obstacles)
    if (visibility > 0.0) {
        visibility *= calculateLineOfSight(vWorldPos, uPlayerPos);
    }
    
    // 4. Apply shadow casting
    if (visibility > 0.0) {
        float shadowIntensity = calculateShadowIntensity(vWorldPos);
        visibility *= (1.0 - shadowIntensity);
    }
    
    // Calculate final darkness
    float darkness = 1.0 - visibility;
    
    // Output the darkness overlay with smooth transitions
    FragColor = vec4(uDarkColor.rgb, uDarkColor.a * darkness);
    
    // DEBUG: Show player position as a blue dot
    if (distance(vWorldPos, uPlayerPos) < 8.0) {
        FragColor = vec4(0.0, 0.0, 1.0, 1.0); // Blue dot at player position
    }
    
    // DEBUG: Show player direction as a line
    vec2 directionEnd = uPlayerPos + uPlayerDirection * 50.0;
    float distToLine = abs(dot(vWorldPos - uPlayerPos, vec2(-uPlayerDirection.y, uPlayerDirection.x)));
    if (distToLine < 3.0 && dot(vWorldPos - uPlayerPos, uPlayerDirection) > 0.0 && 
        distance(vWorldPos, uPlayerPos) < 50.0) {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Green line showing direction
    }
} 