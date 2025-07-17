#pragma once

#include <string>
#include <cstdint>
#include <yaml-cpp/yaml.h>

// Type ID generator for components
class ComponentTypeID {
private:
    static std::size_t s_counter;
public:
    template<typename T>
    static std::size_t GetID() {
        static std::size_t id = s_counter++;
        return id;
    }
};

// Base component class
class Component {
public:
    Component() = default;
    virtual ~Component() = default;

    // Lifecycle methods
    virtual void OnCreate() {}
    virtual void OnDestroy() {}
    virtual void OnUpdate(float deltaTime) {}

    // Serialization support
    virtual YAML::Node Serialize() const { return YAML::Node(); }
    virtual void Deserialize(const YAML::Node& node) {}
    
    // Get component type name for serialization
    virtual std::string GetTypeName() const = 0;
    
    // Enable/disable component
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

private:
    bool m_enabled = true;
};

// Helper macro to automatically implement GetTypeName
#define COMPONENT_TYPE(ClassName) \
    std::string GetTypeName() const override { return #ClassName; } \
    static std::size_t GetStaticTypeID() { return ComponentTypeID::GetID<ClassName>(); } \
    std::size_t GetTypeID() const { return GetStaticTypeID(); }
