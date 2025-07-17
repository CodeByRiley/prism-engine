#pragma once
#include <glm/glm.hpp>
#include "../engine/renderer/Texture2D.h"

struct Player {
    // Transform properties
    glm::vec2 position;
    glm::vec2 direction;
    glm::vec2 lastMoveDirection;
    glm::vec2 size;
    float rotation;
    glm::vec4 color;
    
    // Movement properties
    float speed;
    
    // Texture properties
    Texture2D* texture;

    // Constructor
    Player(const glm::vec2& startPos, const glm::vec4& playerColor, float playerSpeed = 700.0f) 
        : position(startPos),
          direction(0.0f, -1.0f),           // Start facing up
          lastMoveDirection(0.0f, -1.0f),
          size(100.0f, 100.0f),             // Default player size
          rotation(0.0f),
          speed(playerSpeed),
          texture(nullptr),
          color(playerColor)
    {}
    
    // Utility methods
    glm::vec2 GetCenter() const {
        return position;
    }
    
    glm::vec2 GetMinBounds() const {
        return position - size * 0.5f;
    }

    glm::vec2 GetMaxBounds() const {
        return position + size * 0.5f;
    }
    
    // Update player facing direction based on mouse position
    void UpdateDirectionFromMouse(const glm::vec2& mousePos) {
        direction = glm::normalize(mousePos - position);
    }
    
    // Update player facing direction based on movement
    void UpdateDirectionFromMovement(const glm::vec2& moveDirection) {
        if (glm::length(moveDirection) > 0.0f) {
            lastMoveDirection = glm::normalize(moveDirection);
            direction = lastMoveDirection;
        }
    }
    
    // Get direction indicator position for rendering
    glm::vec2 GetDirectionIndicatorPos(float distance = 60.0f) const {
        return position + direction * distance;
    }
}; 