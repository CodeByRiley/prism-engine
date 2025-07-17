#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <typeinfo>
#include <type_traits>
#include "../entity/EntityManager.h"
#include "../component/ComponentManager.h"

// Base system interface
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void OnCreate() {}
    virtual void OnDestroy() {}
    virtual void Update(float deltaTime) = 0;
    virtual std::string GetSystemName() const = 0;
    
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

protected:
    bool m_enabled = true;
};

// Templated system base class for type safety
template<typename Derived>
class System : public ISystem {
public:
    std::string GetSystemName() const override {
        return typeid(Derived).name(); // Could be improved with demangling
    }
    
    static std::size_t GetStaticTypeID() {
        static std::size_t id = s_counter++;
        return id;
    }
    
    std::size_t GetTypeID() const {
        return GetStaticTypeID();
    }

private:
    static std::size_t s_counter;
};

template<typename Derived>
std::size_t System<Derived>::s_counter = 0;

// Helper macro for system type identification
#define SYSTEM_TYPE(ClassName) \
    std::string GetSystemName() const override { return #ClassName; }

// SFINAE helpers for detecting methods
template<typename T, typename = void>
struct has_set_entity_manager : std::false_type {};

template<typename T>
struct has_set_entity_manager<T, std::void_t<decltype(std::declval<T*>()->SetEntityManager(std::declval<EntityManager*>()))>> : std::true_type {};

template<typename T, typename = void>
struct has_set_component_manager : std::false_type {};

template<typename T>
struct has_set_component_manager<T, std::void_t<decltype(std::declval<T*>()->SetComponentManager(std::declval<ComponentManager*>()))>> : std::true_type {};

// System Manager - Manages all systems
class SystemManager {
private:
    std::vector<std::unique_ptr<ISystem>> m_systems;
    std::unordered_map<std::size_t, ISystem*> m_systemMap;
    EntityManager* m_entityManager;
    ComponentManager* m_componentManager;

public:
    SystemManager(EntityManager* entityManager, ComponentManager* componentManager)
        : m_entityManager(entityManager), m_componentManager(componentManager) {}

    ~SystemManager() {
        for (auto& system : m_systems) {
            system->OnDestroy();
        }
    }

    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* rawPtr = system.get();
        
        // Set up system with managers
        if constexpr (has_set_entity_manager<T>::value) {
            rawPtr->SetEntityManager(m_entityManager);
        }
        if constexpr (has_set_component_manager<T>::value) {
            rawPtr->SetComponentManager(m_componentManager);
        }
        
        system->OnCreate();
        m_systemMap[T::GetStaticTypeID()] = rawPtr;
        m_systems.push_back(std::move(system));
        
        return rawPtr;
    }

    template<typename T>
    T* GetSystem() {
        auto it = m_systemMap.find(T::GetStaticTypeID());
        return (it != m_systemMap.end()) ? static_cast<T*>(it->second) : nullptr;
    }

    template<typename T>
    void RemoveSystem() {
        std::size_t typeID = T::GetStaticTypeID();
        auto it = m_systemMap.find(typeID);
        if (it != m_systemMap.end()) {
            // Find and remove from systems vector
            auto systemIt = std::find_if(m_systems.begin(), m_systems.end(),
                [&](const std::unique_ptr<ISystem>& system) {
                    return system.get() == it->second;
                });
            
            if (systemIt != m_systems.end()) {
                (*systemIt)->OnDestroy();
                m_systems.erase(systemIt);
            }
            
            m_systemMap.erase(it);
        }
    }

    void UpdateSystems(float deltaTime) {
        for (auto& system : m_systems) {
            if (system->IsEnabled()) {
                system->Update(deltaTime);
            }
        }
    }

    void SetSystemEnabled(std::size_t typeID, bool enabled) {
        auto it = m_systemMap.find(typeID);
        if (it != m_systemMap.end()) {
            it->second->SetEnabled(enabled);
        }
    }

    template<typename T>
    void SetSystemEnabled(bool enabled) {
        SetSystemEnabled(T::GetStaticTypeID(), enabled);
    }

    const std::vector<std::unique_ptr<ISystem>>& GetAllSystems() const {
        return m_systems;
    }
};

// Base class for systems that need access to ECS managers
template<typename Derived>
class ECSSystem : public System<Derived> {
protected:
    EntityManager* m_entityManager = nullptr;
    ComponentManager* m_componentManager = nullptr;

public:
    void SetEntityManager(EntityManager* entityManager) {
        m_entityManager = entityManager;
    }

    void SetComponentManager(ComponentManager* componentManager) {
        m_componentManager = componentManager;
    }

    // Helper method to get entities with specific components
    template<typename... ComponentTypes>
    std::vector<EntityID> GetEntitiesWith() const {
        if (m_entityManager) {
            return m_entityManager->GetEntitiesWith<ComponentTypes...>();
        }
        return {};
    }

    // Helper methods for component access
    template<typename T>
    T* GetComponent(EntityID entityID) const {
        return m_componentManager ? m_componentManager->GetComponent<T>(entityID) : nullptr;
    }

    template<typename T>
    bool HasComponent(EntityID entityID) const {
        return m_componentManager ? m_componentManager->HasComponent<T>(entityID) : false;
    }
}; 