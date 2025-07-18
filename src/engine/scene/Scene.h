#pragma once

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include "entity/Entity.h"
#include "entity/EntityManager.h"
#include "component/ComponentManager.h"
#include "system/System.h"
#include "..\utils\Logger.h"

class Scene {
private:
    std::unique_ptr<ComponentManager> m_componentManager;
    std::unique_ptr<EntityManager> m_entityManager;
    std::unique_ptr<SystemManager> m_systemManager;
    
    std::string m_name;
    uint32_t m_id;
    bool m_active = true;

public:
    Scene(const std::string& name = "Scene", uint32_t id = 0) 
        : m_name(name), m_id(id) {
        Initialize();
    }

    ~Scene() = default;

    // Move constructor and assignment (for managing unique_ptrs)
    Scene(Scene&& other) noexcept = default;
    Scene& operator=(Scene&& other) noexcept = default;

    // Disable copy (scenes should be unique)
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Initialization
    void Initialize() {
        m_componentManager = std::make_unique<ComponentManager>();
        m_entityManager = std::make_unique<EntityManager>(m_componentManager.get());
        m_systemManager = std::make_unique<SystemManager>(m_entityManager.get(), m_componentManager.get());
    }

    // Scene properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    
    uint32_t GetId() const { return m_id; }
    void SetId(uint32_t id) { m_id = id; }
    
    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }

    // Entity management
    Entity CreateEntity(const std::string& name = "Entity") {
        EntityID id = m_entityManager->CreateEntity(name);
        return Entity(id, m_entityManager.get(), m_componentManager.get());
    }

    void DestroyEntity(const Entity& entity) {
        if (entity.IsValid()) {
            m_entityManager->DestroyEntity(entity.GetID());
        }
    }

    void DestroyEntity(EntityID entityID) {
        m_entityManager->DestroyEntity(entityID);
    }

    Entity GetEntity(EntityID entityID) {
        if (m_entityManager->IsValid(entityID)) {
            return Entity(entityID, m_entityManager.get(), m_componentManager.get());
        }
        return Entity::Invalid();
    }

    std::vector<Entity> GetAllEntities() {
        std::vector<Entity> entities;
        const auto& allEntities = m_entityManager->GetAllEntities();
        entities.reserve(allEntities.size());
        
        for (const auto& [entityID, info] : allEntities) {
            entities.emplace_back(entityID, m_entityManager.get(), m_componentManager.get());
        }
        
        return entities;
    }

    // Get entities with specific components
    template<typename... ComponentTypes>
    std::vector<Entity> GetEntitiesWith() {
        std::vector<Entity> entities;
        auto entityIDs = m_entityManager->GetEntitiesWith<ComponentTypes...>();
        entities.reserve(entityIDs.size());
        
        for (EntityID id : entityIDs) {
            entities.emplace_back(id, m_entityManager.get(), m_componentManager.get());
        }
        
        return entities;
    }

    // System management
    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args) {
        return m_systemManager->RegisterSystem<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* GetSystem() {
        return m_systemManager->GetSystem<T>();
    }

    template<typename T>
    void RemoveSystem() {
        m_systemManager->RemoveSystem<T>();
    }

    template<typename T>
    void SetSystemEnabled(bool enabled) {
        m_systemManager->SetSystemEnabled<T>(enabled);
    }

    // Scene lifecycle
    void Update(float deltaTime) {
        if (m_active && m_systemManager) {
            m_systemManager->UpdateSystems(deltaTime);
        }
    }

    void Clear() {
        if (m_entityManager) {
            m_entityManager->Clear();
        }
    }

    // Direct access to managers (for advanced use)
    EntityManager* GetEntityManager() const { return m_entityManager.get(); }
    ComponentManager* GetComponentManager() const { return m_componentManager.get(); }
    SystemManager* GetSystemManager() const { return m_systemManager.get(); }

    // Serialization
    YAML::Node Serialize() const {
        YAML::Node sceneNode;
        sceneNode["name"] = m_name;
        sceneNode["id"] = m_id;
        sceneNode["active"] = m_active;
        
        // Serialize all entities
        YAML::Node entitiesNode;
        const auto& allEntities = m_entityManager->GetAllEntities();
        
        for (const auto& [entityID, info] : allEntities) {
            YAML::Node entityNode = m_entityManager->SerializeEntity(entityID);
            if (!entityNode.IsNull()) {
                entitiesNode.push_back(entityNode);
            }
        }
        
        sceneNode["entities"] = entitiesNode;
        return sceneNode;
    }

    void Deserialize(const YAML::Node& sceneNode) {
        if (sceneNode["name"]) {
            m_name = sceneNode["name"].as<std::string>();
        }
        Logger::Info("Deserializing scene: " + m_name);


        if (sceneNode["id"]) {
            m_id = sceneNode["id"].as<uint32_t>();
        }
        
        if (sceneNode["active"]) {
            m_active = sceneNode["active"].as<bool>();
        }
        
        // Clear existing entities
        Clear();
        
        // Deserialize entities
        if (sceneNode["entities"] && sceneNode["entities"].IsSequence()) {
            // First pass: Create all entities
            std::unordered_map<EntityID, YAML::Node> entityNodes;
            
            for (const auto& entityNodeIterator : sceneNode["entities"]) {
                YAML::Node entityNode = entityNodeIterator;
                EntityID entityID = m_entityManager->DeserializeEntity(entityNode);
                if (entityID != INVALID_ENTITY_ID) {
                    entityNodes[entityID] = entityNode;
                }
            }
            
            // Second pass: Set up relationships and components
            for (const auto& [entityID, entityNode] : entityNodes) {
                m_entityManager->DeserializeEntityRelationships(entityID, entityNode);
            }
        }
    }

    // Save/Load scene to/from file
    bool SaveToFile(const std::string& filepath) const {
        try {
            YAML::Node sceneNode = Serialize();
            std::ofstream outFile(filepath);
            outFile << sceneNode;
            return true;
        } catch (const std::exception& e) {
            Logger::Error<Scene>("Failed to save scene to file: " + std::string(e.what()), this);
            return false;
        }
    }

    bool LoadFromFile(const std::string& filepath) {
        try {
            YAML::Node sceneNode = YAML::LoadFile(filepath);
            Deserialize(sceneNode);
            return true;
        } catch (const std::exception& e) {
            Logger::Error<Scene>("Failed to load scene from file: " + std::string(e.what()), this);
            return false;
        }
    }
};