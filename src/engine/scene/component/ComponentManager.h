#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <typeinfo>
#include <cassert>
#include "Component.h"

using EntityID = std::uint32_t;

// Component pool interface
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void RemoveComponent(EntityID entityID) = 0;
    virtual bool HasComponent(EntityID entityID) const = 0;
    virtual Component* GetComponent(EntityID entityID) = 0;
    virtual YAML::Node SerializeComponent(EntityID entityID) const = 0;
    virtual void DeserializeComponent(EntityID entityID, const YAML::Node& node) = 0;
    virtual std::string GetComponentTypeName() const = 0;
};

// Templated component pool for specific component types
template<typename T>
class ComponentPool : public IComponentPool {
private:
    std::unordered_map<EntityID, std::unique_ptr<T>> m_components;

public:
    T* AddComponent(EntityID entityID) {
        assert(m_components.find(entityID) == m_components.end() && "Component already exists for entity");
        
        auto component = std::make_unique<T>();
        T* rawPtr = component.get();
        m_components[entityID] = std::move(component);
        
        rawPtr->OnCreate();
        return rawPtr;
    }

    template<typename... Args>
    T* AddComponent(EntityID entityID, Args&&... args) {
        assert(m_components.find(entityID) == m_components.end() && "Component already exists for entity");
        
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* rawPtr = component.get();
        m_components[entityID] = std::move(component);
        
        rawPtr->OnCreate();
        return rawPtr;
    }

    void RemoveComponent(EntityID entityID) override {
        auto it = m_components.find(entityID);
        if (it != m_components.end()) {
            it->second->OnDestroy();
            m_components.erase(it);
        }
    }

    T* GetComponent(EntityID entityID) override {
        auto it = m_components.find(entityID);
        return (it != m_components.end()) ? it->second.get() : nullptr;
    }

    bool HasComponent(EntityID entityID) const override {
        return m_components.find(entityID) != m_components.end();
    }

    YAML::Node SerializeComponent(EntityID entityID) const override {
        auto it = m_components.find(entityID);
        if (it != m_components.end()) {
            YAML::Node node;
            node["type"] = it->second->GetTypeName();
            node["enabled"] = it->second->IsEnabled();
            node["data"] = it->second->Serialize();
            return node;
        }
        return YAML::Node();
    }

    void DeserializeComponent(EntityID entityID, const YAML::Node& node) override {
        T* component = GetComponent(entityID);
        if (component && node["data"]) {
            component->Deserialize(node["data"]);
            if (node["enabled"]) {
                component->SetEnabled(node["enabled"].as<bool>());
            }
        }
    }

    std::string GetComponentTypeName() const override {
        return typeid(T).name(); // Could be improved with demangling
    }

    // Get all components for iteration
    const std::unordered_map<EntityID, std::unique_ptr<T>>& GetAllComponents() const {
        return m_components;
    }

    // Update all components
    void UpdateComponents(float deltaTime) {
        for (auto& [entityID, component] : m_components) {
            if (component->IsEnabled()) {
                component->OnUpdate(deltaTime);
            }
        }
    }
};

// Component Manager - Central component management
class ComponentManager {
private:
    std::unordered_map<std::size_t, std::unique_ptr<IComponentPool>> m_componentPools;

    template<typename T>
    ComponentPool<T>* GetPool() {
        std::size_t typeID = ComponentTypeID::GetID<T>();
        auto it = m_componentPools.find(typeID);
        
        if (it == m_componentPools.end()) {
            m_componentPools[typeID] = std::make_unique<ComponentPool<T>>();
        }
        
        return static_cast<ComponentPool<T>*>(m_componentPools[typeID].get());
    }

public:
    template<typename T, typename... Args>
    T* AddComponent(EntityID entityID, Args&&... args) {
        return GetPool<T>()->AddComponent(entityID, std::forward<Args>(args)...);
    }

    template<typename T>
    void RemoveComponent(EntityID entityID) {
        auto pool = GetPool<T>();
        pool->RemoveComponent(entityID);
    }

    template<typename T>
    T* GetComponent(EntityID entityID) {
        return GetPool<T>()->GetComponent(entityID);
    }

    template<typename T>
    bool HasComponent(EntityID entityID) const {
        std::size_t typeID = ComponentTypeID::GetID<T>();
        auto it = m_componentPools.find(typeID);
        return (it != m_componentPools.end()) ? it->second->HasComponent(entityID) : false;
    }

    template<typename T>
    ComponentPool<T>* GetComponentPool() {
        return GetPool<T>();
    }

    void RemoveAllComponents(EntityID entityID) {
        for (auto& [typeID, pool] : m_componentPools) {
            pool->RemoveComponent(entityID);
        }
    }

    // Update all components
    void UpdateAllComponents(float deltaTime) {
        for (auto& [typeID, pool] : m_componentPools) {
            // This would need to be implemented properly for each pool type
            // For now, we'll handle this in the system manager
        }
    }

    // Serialization
    YAML::Node SerializeEntity(EntityID entityID) const {
        YAML::Node entityNode;
        YAML::Node componentsNode;
        
        for (const auto& [typeID, pool] : m_componentPools) {
            if (pool->HasComponent(entityID)) {
                YAML::Node componentNode = pool->SerializeComponent(entityID);
                if (!componentNode.IsNull()) {
                    componentsNode.push_back(componentNode);
                }
            }
        }
        
        entityNode["components"] = componentsNode;
        return entityNode;
    }

    void DeserializeEntity(EntityID entityID, const YAML::Node& entityNode) {
        if (entityNode["components"] && entityNode["components"].IsSequence()) {
            for (const auto& componentNode : entityNode["components"]) {
                if (componentNode["type"]) {
                    std::string typeName = componentNode["type"].as<std::string>();
                    // Component deserialization would need a factory pattern
                    // This is a simplified version - you'd need to register component types
                }
            }
        }
    }
}; 