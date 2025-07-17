#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

// Transform Component - Position, rotation, scale
class TransformComponent : public Component {
public:
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f}; // Euler angles in radians
    glm::vec3 scale{1.0f};

    COMPONENT_TYPE(TransformComponent)

    TransformComponent() = default;
    TransformComponent(const glm::vec3& pos) : position(pos) {}
    TransformComponent(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl)
        : position(pos), rotation(rot), scale(scl) {}

    glm::mat4 GetTransformMatrix() const {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1, 0, 0));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0, 1, 0));
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0, 0, 1));
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        
        return translation * rotationZ * rotationY * rotationX * scaleMatrix;
    }

    // Serialization
    YAML::Node Serialize() const override {
        YAML::Node node;
        node["position"] = YAML::Node();
        node["position"]["x"] = position.x;
        node["position"]["y"] = position.y;
        node["position"]["z"] = position.z;
        
        node["rotation"] = YAML::Node();
        node["rotation"]["x"] = rotation.x;
        node["rotation"]["y"] = rotation.y;
        node["rotation"]["z"] = rotation.z;
        
        node["scale"] = YAML::Node();
        node["scale"]["x"] = scale.x;
        node["scale"]["y"] = scale.y;
        node["scale"]["z"] = scale.z;
        
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        if (node["position"]) {
            const auto& pos = node["position"];
            position.x = pos["x"].as<float>(0.0f);
            position.y = pos["y"].as<float>(0.0f);
            position.z = pos["z"].as<float>(0.0f);
        }
        
        if (node["rotation"]) {
            const auto& rot = node["rotation"];
            rotation.x = rot["x"].as<float>(0.0f);
            rotation.y = rot["y"].as<float>(0.0f);
            rotation.z = rot["z"].as<float>(0.0f);
        }
        
        if (node["scale"]) {
            const auto& scl = node["scale"];
            scale.x = scl["x"].as<float>(1.0f);
            scale.y = scl["y"].as<float>(1.0f);
            scale.z = scl["z"].as<float>(1.0f);
        }
    }
};

// Renderable Component - For 2D rendering
class RenderableComponent : public Component {
public:
    std::string meshName;
    std::string materialName;
    glm::vec4 color{1.0f}; // RGBA
    bool visible = true;
    int renderLayer = 0;

    COMPONENT_TYPE(RenderableComponent)

    RenderableComponent() = default;
    RenderableComponent(const std::string& mesh, const std::string& material = "")
        : meshName(mesh), materialName(material) {}

    YAML::Node Serialize() const override {
        YAML::Node node;
        node["meshName"] = meshName;
        node["materialName"] = materialName;
        node["color"] = YAML::Node();
        node["color"]["r"] = color.r;
        node["color"]["g"] = color.g;
        node["color"]["b"] = color.b;
        node["color"]["a"] = color.a;
        node["visible"] = visible;
        node["renderLayer"] = renderLayer;
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        meshName = node["meshName"].as<std::string>("");
        materialName = node["materialName"].as<std::string>("");
        
        if (node["color"]) {
            const auto& col = node["color"];
            color.r = col["r"].as<float>(1.0f);
            color.g = col["g"].as<float>(1.0f);
            color.b = col["b"].as<float>(1.0f);
            color.a = col["a"].as<float>(1.0f);
        }
        
        visible = node["visible"].as<bool>(true);
        renderLayer = node["renderLayer"].as<int>(0);
    }
};

// Physics Component - For physics simulation
class PhysicsComponent : public Component {
public:
    glm::vec3 velocity{0.0f};
    glm::vec3 acceleration{0.0f};
    float mass = 1.0f;
    float drag = 0.01f;
    bool isStatic = false;
    bool useGravity = true;

    COMPONENT_TYPE(PhysicsComponent)

    PhysicsComponent() = default;
    PhysicsComponent(float m) : mass(m) {}

    void ApplyForce(const glm::vec3& force) {
        if (!isStatic && mass > 0.0f) {
            acceleration += force / mass;
        }
    }

    YAML::Node Serialize() const override {
        YAML::Node node;
        
        node["velocity"] = YAML::Node();
        node["velocity"]["x"] = velocity.x;
        node["velocity"]["y"] = velocity.y;
        node["velocity"]["z"] = velocity.z;
        
        node["acceleration"] = YAML::Node();
        node["acceleration"]["x"] = acceleration.x;
        node["acceleration"]["y"] = acceleration.y;
        node["acceleration"]["z"] = acceleration.z;
        
        node["mass"] = mass;
        node["drag"] = drag;
        node["isStatic"] = isStatic;
        node["useGravity"] = useGravity;
        
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        if (node["velocity"]) {
            const auto& vel = node["velocity"];
            velocity.x = vel["x"].as<float>(0.0f);
            velocity.y = vel["y"].as<float>(0.0f);
            velocity.z = vel["z"].as<float>(0.0f);
        }
        
        if (node["acceleration"]) {
            const auto& acc = node["acceleration"];
            acceleration.x = acc["x"].as<float>(0.0f);
            acceleration.y = acc["y"].as<float>(0.0f);
            acceleration.z = acc["z"].as<float>(0.0f);
        }
        
        mass = node["mass"].as<float>(1.0f);
        drag = node["drag"].as<float>(0.01f);
        isStatic = node["isStatic"].as<bool>(false);
        useGravity = node["useGravity"].as<bool>(true);
    }
};

// Tag Component - Simple string tag for categorization
class TagComponent : public Component {
public:
    std::string tag;

    COMPONENT_TYPE(TagComponent)

    TagComponent() = default;
    TagComponent(const std::string& t) : tag(t) {}

    YAML::Node Serialize() const override {
        YAML::Node node;
        node["tag"] = tag;
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        tag = node["tag"].as<std::string>("");
    }
};

// Camera Component - For rendering viewpoints
class CameraComponent : public Component {
public:
    float fov = 45.0f; // Field of view in degrees
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;
    bool isPrimary = false;
    bool isOrthographic = false;
    float orthographicSize = 10.0f;

    COMPONENT_TYPE(CameraComponent)

    CameraComponent() = default;

    glm::mat4 GetProjectionMatrix() const {
        if (isOrthographic) {
            float halfSize = orthographicSize * 0.5f;
            float halfWidth = halfSize * aspectRatio;
            return glm::ortho(-halfWidth, halfWidth, -halfSize, halfSize, nearPlane, farPlane);
        } else {
            return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        }
    }

    YAML::Node Serialize() const override {
        YAML::Node node;
        node["fov"] = fov;
        node["nearPlane"] = nearPlane;
        node["farPlane"] = farPlane;
        node["aspectRatio"] = aspectRatio;
        node["isPrimary"] = isPrimary;
        node["isOrthographic"] = isOrthographic;
        node["orthographicSize"] = orthographicSize;
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        fov = node["fov"].as<float>(45.0f);
        nearPlane = node["nearPlane"].as<float>(0.1f);
        farPlane = node["farPlane"].as<float>(1000.0f);
        aspectRatio = node["aspectRatio"].as<float>(16.0f / 9.0f);
        isPrimary = node["isPrimary"].as<bool>(false);
        isOrthographic = node["isOrthographic"].as<bool>(false);
        orthographicSize = node["orthographicSize"].as<float>(10.0f);
    }
};

// Audio Component - For playing sounds
class AudioComponent : public Component {
public:
    std::string audioClipName;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool isLooping = false;
    bool playOnCreate = false;
    bool is3D = true;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;

    COMPONENT_TYPE(AudioComponent)

    AudioComponent() = default;
    AudioComponent(const std::string& clipName) : audioClipName(clipName) {}

    YAML::Node Serialize() const override {
        YAML::Node node;
        node["audioClipName"] = audioClipName;
        node["volume"] = volume;
        node["pitch"] = pitch;
        node["isLooping"] = isLooping;
        node["playOnCreate"] = playOnCreate;
        node["is3D"] = is3D;
        node["minDistance"] = minDistance;
        node["maxDistance"] = maxDistance;
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        audioClipName = node["audioClipName"].as<std::string>("");
        volume = node["volume"].as<float>(1.0f);
        pitch = node["pitch"].as<float>(1.0f);
        isLooping = node["isLooping"].as<bool>(false);
        playOnCreate = node["playOnCreate"].as<bool>(false);
        is3D = node["is3D"].as<bool>(true);
        minDistance = node["minDistance"].as<float>(1.0f);
        maxDistance = node["maxDistance"].as<float>(100.0f);
    }
}; 