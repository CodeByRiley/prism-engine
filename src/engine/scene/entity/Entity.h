#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <functional>
#include "EntityManager.h"

// Forward declaration
class Scene;

// Lightweight Entity wrapper
class Entity {
private:
    EntityID m_id = INVALID_ENTITY_ID;
    EntityManager* m_entityManager = nullptr;
    ComponentManager* m_componentManager = nullptr;

public:
    // Constructors
    Entity() = default;
    Entity(EntityID id, EntityManager* entityManager, ComponentManager* componentManager)
        : m_id(id), m_entityManager(entityManager), m_componentManager(componentManager) {}

    // Copy constructor and assignment
    Entity(const Entity& other) = default;
    Entity& operator=(const Entity& other) = default;

    // Move constructor and assignment
    Entity(Entity&& other) noexcept = default;
    Entity& operator=(Entity&& other) noexcept = default;

    // Validity check
    bool IsValid() const {
        return m_id != INVALID_ENTITY_ID && 
               m_entityManager && 
               m_entityManager->IsValid(m_id);
    }

    // Implicit conversion to EntityID for use with managers
    operator EntityID() const { return m_id; }

    // Comparison operators
    bool operator==(const Entity& other) const { return m_id == other.m_id; }
    bool operator!=(const Entity& other) const { return m_id != other.m_id; }
    bool operator<(const Entity& other) const { return m_id < other.m_id; }

    // Entity ID access
    EntityID GetID() const { return m_id; }

    // Entity properties
    std::string GetName() const {
        return m_entityManager ? m_entityManager->GetEntityName(m_id) : "";
    }

    void SetName(const std::string& name) {
        if (m_entityManager) {
            m_entityManager->SetEntityName(m_id, name);
        }
    }

    bool IsActive() const {
        return m_entityManager ? m_entityManager->IsEntityActive(m_id) : false;
    }

    void SetActive(bool active) {
        if (m_entityManager) {
            m_entityManager->SetEntityActive(m_id, active);
        }
    }

    // Component management
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        if (!IsValid() || !m_componentManager) return nullptr;
        return m_componentManager->AddComponent<T>(m_id, std::forward<Args>(args)...);
    }

    template<typename T>
    T* GetComponent() {
        if (!IsValid() || !m_componentManager) return nullptr;
        return m_componentManager->GetComponent<T>(m_id);
    }

    template<typename T>
    const T* GetComponent() const {
        if (!IsValid() || !m_componentManager) return nullptr;
        return m_componentManager->GetComponent<T>(m_id);
    }

    template<typename T>
    bool HasComponent() const {
        if (!IsValid() || !m_componentManager) return false;
        return m_componentManager->HasComponent<T>(m_id);
    }

    template<typename T>
    void RemoveComponent() {
        if (IsValid() && m_componentManager) {
            m_componentManager->RemoveComponent<T>(m_id);
        }
    }

    // Parent-child relationships
    Entity GetParent() const {
        if (!IsValid() || !m_entityManager) return Entity();
        
        EntityID parentID = m_entityManager->GetParent(m_id);
        if (parentID != INVALID_ENTITY_ID) {
            return Entity(parentID, m_entityManager, m_componentManager);
        }
        return Entity();
    }

    void SetParent(const Entity& parent) {
        if (IsValid() && m_entityManager && parent.IsValid()) {
            m_entityManager->SetParent(m_id, parent.GetID());
        }
    }

    void SetParent(EntityID parentID) {
        if (IsValid() && m_entityManager) {
            m_entityManager->SetParent(m_id, parentID);
        }
    }

    std::vector<Entity> GetChildren() const {
        std::vector<Entity> children;
        if (!IsValid() || !m_entityManager) return children;

        const auto& childIDs = m_entityManager->GetChildren(m_id);
        children.reserve(childIDs.size());
        
        for (EntityID childID : childIDs) {
            children.emplace_back(childID, m_entityManager, m_componentManager);
        }
        
        return children;
    }

    void AddChild(Entity& child) {
        if (child.IsValid()) {
            child.SetParent(*this);
        }
    }

    void RemoveChild(Entity& child) {
        if (child.IsValid() && child.GetParent() == *this) {
            child.SetParent(INVALID_ENTITY_ID);
        }
    }

    // Destroy this entity
    void Destroy() {
        if (IsValid() && m_entityManager) {
            m_entityManager->DestroyEntity(m_id);
            m_id = INVALID_ENTITY_ID;
        }
    }

    // Internal access for Scene and other systems
    EntityManager* GetEntityManager() const { return m_entityManager; }
    ComponentManager* GetComponentManager() const { return m_componentManager; }

    // Create an invalid entity
    static Entity Invalid() {
        return Entity();
    }
};

// Hash specialization for std::unordered_map
namespace std {
    template<>
    struct hash<Entity> {
        std::size_t operator()(const Entity& entity) const noexcept {
            return std::hash<EntityID>{}(entity.GetID());
        }
    };
}