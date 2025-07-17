#pragma once

#include <queue>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <yaml-cpp/yaml.h>
#include "../component/ComponentManager.h"

using EntityID = std::uint32_t;
const EntityID INVALID_ENTITY_ID = 0;

struct EntityInfo {
    EntityID id = INVALID_ENTITY_ID;
    std::string name;
    bool active = true;
    EntityID parent = INVALID_ENTITY_ID;
    std::vector<EntityID> children;
    
    EntityInfo() = default;
    EntityInfo(EntityID entityID, const std::string& entityName) 
        : id(entityID), name(entityName) {}
};

class EntityManager {
private:
    std::queue<EntityID> m_availableIDs;
    std::unordered_map<EntityID, EntityInfo> m_entities;
    EntityID m_nextID = 1; // Start from 1, reserve 0 for invalid
    ComponentManager* m_componentManager;

public:
    EntityManager(ComponentManager* componentManager) 
        : m_componentManager(componentManager) {}

    // Entity lifecycle
    EntityID CreateEntity(const std::string& name = "Entity") {
        EntityID id;
        
        if (!m_availableIDs.empty()) {
            id = m_availableIDs.front();
            m_availableIDs.pop();
        } else {
            id = m_nextID++;
        }

        m_entities[id] = EntityInfo(id, name);
        return id;
    }

    void DestroyEntity(EntityID entityID) {
        if (!IsValid(entityID)) return;

        auto& entity = m_entities[entityID];
        
        // Remove from parent's children list
        if (entity.parent != INVALID_ENTITY_ID) {
            RemoveChild(entity.parent, entityID);
        }
        
        // Destroy all children
        auto children = entity.children; // Copy to avoid iterator invalidation
        for (EntityID child : children) {
            DestroyEntity(child);
        }
        
        // Remove all components
        m_componentManager->RemoveAllComponents(entityID);
        
        // Remove entity
        m_entities.erase(entityID);
        m_availableIDs.push(entityID);
    }

    // Entity validation
    bool IsValid(EntityID entityID) const {
        return entityID != INVALID_ENTITY_ID && 
               m_entities.find(entityID) != m_entities.end();
    }

    // Entity properties
    const EntityInfo* GetEntityInfo(EntityID entityID) const {
        auto it = m_entities.find(entityID);
        return (it != m_entities.end()) ? &it->second : nullptr;
    }

    EntityInfo* GetEntityInfo(EntityID entityID) {
        auto it = m_entities.find(entityID);
        return (it != m_entities.end()) ? &it->second : nullptr;
    }

    void SetEntityName(EntityID entityID, const std::string& name) {
        if (auto* info = GetEntityInfo(entityID)) {
            info->name = name;
        }
    }

    std::string GetEntityName(EntityID entityID) const {
        if (const auto* info = GetEntityInfo(entityID)) {
            return info->name;
        }
        return "";
    }

    void SetEntityActive(EntityID entityID, bool active) {
        if (auto* info = GetEntityInfo(entityID)) {
            info->active = active;
        }
    }

    bool IsEntityActive(EntityID entityID) const {
        if (const auto* info = GetEntityInfo(entityID)) {
            return info->active;
        }
        return false;
    }

    // Parent-child relationships
    void SetParent(EntityID childID, EntityID parentID) {
        if (!IsValid(childID)) return;
        
        auto* child = GetEntityInfo(childID);
        if (!child) return;

        // Remove from old parent
        if (child->parent != INVALID_ENTITY_ID) {
            RemoveChild(child->parent, childID);
        }

        // Set new parent
        child->parent = parentID;
        
        // Add to new parent's children
        if (parentID != INVALID_ENTITY_ID && IsValid(parentID)) {
            auto* parent = GetEntityInfo(parentID);
            if (parent) {
                parent->children.push_back(childID);
            }
        }
    }

    void RemoveChild(EntityID parentID, EntityID childID) {
        auto* parent = GetEntityInfo(parentID);
        if (!parent) return;

        auto& children = parent->children;
        children.erase(std::remove(children.begin(), children.end(), childID), children.end());
    }

    EntityID GetParent(EntityID entityID) const {
        if (const auto* info = GetEntityInfo(entityID)) {
            return info->parent;
        }
        return INVALID_ENTITY_ID;
    }

    const std::vector<EntityID>& GetChildren(EntityID entityID) const {
        if (const auto* info = GetEntityInfo(entityID)) {
            return info->children;
        }
        static const std::vector<EntityID> empty;
        return empty;
    }

    // Component management (delegated to ComponentManager)
    template<typename T, typename... Args>
    T* AddComponent(EntityID entityID, Args&&... args) {
        if (!IsValid(entityID)) return nullptr;
        return m_componentManager->AddComponent<T>(entityID, std::forward<Args>(args)...);
    }

    template<typename T>
    void RemoveComponent(EntityID entityID) {
        if (IsValid(entityID)) {
            m_componentManager->RemoveComponent<T>(entityID);
        }
    }

    template<typename T>
    T* GetComponent(EntityID entityID) {
        if (!IsValid(entityID)) return nullptr;
        return m_componentManager->GetComponent<T>(entityID);
    }

    template<typename T>
    bool HasComponent(EntityID entityID) const {
        if (!IsValid(entityID)) return false;
        return m_componentManager->HasComponent<T>(entityID);
    }

    // Get all entities (for systems)
    const std::unordered_map<EntityID, EntityInfo>& GetAllEntities() const {
        return m_entities;
    }

    // Get entities with specific components
    template<typename... ComponentTypes>
    std::vector<EntityID> GetEntitiesWith() const {
        std::vector<EntityID> result;
        for (const auto& [entityID, info] : m_entities) {
            if (info.active && (m_componentManager->HasComponent<ComponentTypes>(entityID) && ...)) {
                result.push_back(entityID);
            }
        }
        return result;
    }

    // Serialization
    YAML::Node SerializeEntity(EntityID entityID) const {
        if (!IsValid(entityID)) return YAML::Node();
        
        const auto* info = GetEntityInfo(entityID);
        YAML::Node entityNode;
        
        entityNode["id"] = entityID;
        entityNode["name"] = info->name;
        entityNode["active"] = info->active;
        
        if (info->parent != INVALID_ENTITY_ID) {
            entityNode["parent"] = info->parent;
        }
        
        if (!info->children.empty()) {
            entityNode["children"] = info->children;
        }
        
        // Serialize components
        YAML::Node componentsNode = m_componentManager->SerializeEntity(entityID);
        if (!componentsNode.IsNull()) {
            entityNode["components"] = componentsNode["components"];
        }
        
        return entityNode;
    }

    EntityID DeserializeEntity(const YAML::Node& entityNode) {
        if (!entityNode["name"]) return INVALID_ENTITY_ID;
        
        std::string name = entityNode["name"].as<std::string>();
        EntityID entityID = CreateEntity(name);
        
        if (entityNode["active"]) {
            SetEntityActive(entityID, entityNode["active"].as<bool>());
        }
        
        // Note: Parent-child relationships and components should be handled
        // after all entities are created to avoid dependency issues
        
        return entityID;
    }

    void DeserializeEntityRelationships(EntityID entityID, const YAML::Node& entityNode) {
        if (entityNode["parent"]) {
            EntityID parentID = entityNode["parent"].as<EntityID>();
            SetParent(entityID, parentID);
        }
        
        if (entityNode["components"]) {
            m_componentManager->DeserializeEntity(entityID, entityNode);
        }
    }

    // Clear all entities
    void Clear() {
        auto entities = m_entities; // Copy to avoid iterator invalidation
        for (const auto& [entityID, info] : entities) {
            DestroyEntity(entityID);
        }
    }
}; 