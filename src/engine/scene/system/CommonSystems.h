#pragma once

#include "System.h"
#include "../component/CommonComponents.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <vector>

// Physics System - Updates physics components
class PhysicsSystem : public ECSSystem<PhysicsSystem> {
private:
    glm::vec3 m_gravity{0.0f, -9.81f, 0.0f};
    
public:
    SYSTEM_TYPE(PhysicsSystem)

    void OnCreate() override {
        // Initialize physics system
    }

    void OnDestroy() override {
        // Cleanup physics system
    }

    void Update(float deltaTime) override {
        // Get all entities with Transform and Physics components
        auto entities = GetEntitiesWith<TransformComponent, PhysicsComponent>();
        
        for (EntityID entityID : entities) {
            auto* transform = GetComponent<TransformComponent>(entityID);
            auto* physics = GetComponent<PhysicsComponent>(entityID);
            
            if (!transform || !physics || physics->isStatic) continue;
            
            // Apply gravity
            if (physics->useGravity) {
                physics->ApplyForce(m_gravity * physics->mass);
            }
            
            // Update velocity based on acceleration
            physics->velocity += physics->acceleration * deltaTime;
            
            // Apply drag
            physics->velocity *= (1.0f - physics->drag);
            
            // Update position based on velocity
            transform->position += physics->velocity * deltaTime;
            
            // Reset acceleration for next frame
            physics->acceleration = glm::vec3(0.0f);
        }
    }

    void SetGravity(const glm::vec3& gravity) {
        m_gravity = gravity;
    }

    const glm::vec3& GetGravity() const {
        return m_gravity;
    }
};

// Render System - Processes renderable entities
class RenderSystem : public ECSSystem<RenderSystem> {
private:
    struct RenderData {
        EntityID entityID;
        glm::mat4 transform;
        RenderableComponent* renderable;
        int layer;
    };
    
    std::vector<RenderData> m_renderQueue;

public:
    SYSTEM_TYPE(RenderSystem)

    void OnCreate() override {
        // Initialize rendering resources
    }

    void OnDestroy() override {
        // Cleanup rendering resources
    }

    void Update(float deltaTime) override {
        // Clear previous frame's render queue
        m_renderQueue.clear();
        
        // Get all entities with Transform and Renderable components
        auto entities = GetEntitiesWith<TransformComponent, RenderableComponent>();
        
        // Populate render queue
        for (EntityID entityID : entities) {
            auto* transform = GetComponent<TransformComponent>(entityID);
            auto* renderable = GetComponent<RenderableComponent>(entityID);
            
            if (!transform || !renderable || !renderable->visible) continue;
            
            RenderData data;
            data.entityID = entityID;
            data.transform = transform->GetTransformMatrix();
            data.renderable = renderable;
            data.layer = renderable->renderLayer;
            
            m_renderQueue.push_back(data);
        }
        
        // Sort by render layer
        std::sort(m_renderQueue.begin(), m_renderQueue.end(),
            [](const RenderData& a, const RenderData& b) {
                return a.layer < b.layer;
            });
        
        // Render all objects (this would integrate with your actual renderer)
        Render();
    }

private:
    void Render() {
        // This is where you'd integrate with your actual rendering pipeline
        // For now, this is just a placeholder
        for (const auto& data : m_renderQueue) {
            // Example: Submit to renderer
            // renderer->DrawMesh(data.renderable->meshName, data.transform, data.renderable->color);
        }
    }

public:
    const std::vector<RenderData>& GetRenderQueue() const {
        return m_renderQueue;
    }
};

// Camera System - Manages camera transformations
class CameraSystem : public ECSSystem<CameraSystem> {
private:
    EntityID m_primaryCameraEntity = INVALID_ENTITY_ID;
    glm::mat4 m_viewMatrix{1.0f};
    glm::mat4 m_projectionMatrix{1.0f};

public:
    SYSTEM_TYPE(CameraSystem)

    void Update(float deltaTime) override {
        // Find primary camera
        auto cameraEntities = GetEntitiesWith<TransformComponent, CameraComponent>();
        
        EntityID newPrimaryCamera = INVALID_ENTITY_ID;
        for (EntityID entityID : cameraEntities) {
            auto* camera = GetComponent<CameraComponent>(entityID);
            if (camera && camera->isPrimary) {
                newPrimaryCamera = entityID;
                break;
            }
        }
        
        // If no primary camera found, use the first available camera
        if (newPrimaryCamera == INVALID_ENTITY_ID && !cameraEntities.empty()) {
            newPrimaryCamera = cameraEntities[0];
            auto* camera = GetComponent<CameraComponent>(newPrimaryCamera);
            if (camera) {
                camera->isPrimary = true;
            }
        }
        
        m_primaryCameraEntity = newPrimaryCamera;
        
        // Update view and projection matrices
        if (m_primaryCameraEntity != INVALID_ENTITY_ID) {
            auto* transform = GetComponent<TransformComponent>(m_primaryCameraEntity);
            auto* camera = GetComponent<CameraComponent>(m_primaryCameraEntity);
            
            if (transform && camera) {
                // Calculate view matrix (inverse of camera transform)
                m_viewMatrix = glm::inverse(transform->GetTransformMatrix());
                m_projectionMatrix = camera->GetProjectionMatrix();
            }
        }
    }

    EntityID GetPrimaryCameraEntity() const {
        return m_primaryCameraEntity;
    }

    const glm::mat4& GetViewMatrix() const {
        return m_viewMatrix;
    }

    const glm::mat4& GetProjectionMatrix() const {
        return m_projectionMatrix;
    }

    glm::mat4 GetViewProjectionMatrix() const {
        return m_projectionMatrix * m_viewMatrix;
    }
};

// Audio System - Manages audio playback
class AudioSystem : public ECSSystem<AudioSystem> {
public:
    SYSTEM_TYPE(AudioSystem)

    void OnCreate() override {
        // Initialize audio system
    }

    void OnDestroy() override {
        // Cleanup audio resources
    }

    void Update(float deltaTime) override {
        // Get all entities with audio components
        auto entities = GetEntitiesWith<AudioComponent>();
        
        for (EntityID entityID : entities) {
            auto* audio = GetComponent<AudioComponent>(entityID);
            if (!audio) continue;
            
            // Handle play on create
            if (audio->playOnCreate) {
                PlayAudio(entityID);
                audio->playOnCreate = false; // Prevent playing every frame
            }
            
            // Update 3D audio positioning if needed
            if (audio->is3D) {
                auto* transform = GetComponent<TransformComponent>(entityID);
                if (transform) {
                    Update3DAudio(entityID, transform->position);
                }
            }
        }
    }

private:
    void PlayAudio(EntityID entityID) {
        auto* audio = GetComponent<AudioComponent>(entityID);
        if (!audio) return;
        
        // This would integrate with your audio system
        // audioEngine->PlaySound(audio->audioClipName, audio->volume, audio->pitch, audio->isLooping);
    }

    void Update3DAudio(EntityID entityID, const glm::vec3& position) {
        auto* audio = GetComponent<AudioComponent>(entityID);
        if (!audio) return;
        
        // This would update 3D audio positioning
        // audioEngine->SetSoundPosition(entityID, position, audio->minDistance, audio->maxDistance);
    }
};

// Lifetime System - Manages entity lifecycles (optional system for timed entities)
class LifetimeComponent : public Component {
public:
    float lifetime = 5.0f; // Seconds
    float elapsed = 0.0f;
    bool destroyOnTimeout = true;

    COMPONENT_TYPE(LifetimeComponent)

    LifetimeComponent() = default;
    LifetimeComponent(float life) : lifetime(life) {}

    YAML::Node Serialize() const override {
        YAML::Node node;
        node["lifetime"] = lifetime;
        node["elapsed"] = elapsed;
        node["destroyOnTimeout"] = destroyOnTimeout;
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        lifetime = node["lifetime"].as<float>(5.0f);
        elapsed = node["elapsed"].as<float>(0.0f);
        destroyOnTimeout = node["destroyOnTimeout"].as<bool>(true);
    }
};

class LifetimeSystem : public ECSSystem<LifetimeSystem> {
public:
    SYSTEM_TYPE(LifetimeSystem)

    void Update(float deltaTime) override {
        auto entities = GetEntitiesWith<LifetimeComponent>();
        std::vector<EntityID> entitiesToDestroy;
        
        for (EntityID entityID : entities) {
            auto* lifetime = GetComponent<LifetimeComponent>(entityID);
            if (!lifetime) continue;
            
            lifetime->elapsed += deltaTime;
            
            if (lifetime->elapsed >= lifetime->lifetime && lifetime->destroyOnTimeout) {
                entitiesToDestroy.push_back(entityID);
            }
        }
        
        // Destroy expired entities
        for (EntityID entityID : entitiesToDestroy) {
            if (m_entityManager) {
                m_entityManager->DestroyEntity(entityID);
            }
        }
    }
}; 