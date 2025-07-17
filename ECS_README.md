# Entity Component System (ECS) Documentation

## Overview

This is a modern, modular, and performant Entity Component System with built-in YAML serialization support. The system is designed to be:

- **Modular**: Components and systems are independent and can be easily combined
- **Fast**: Uses component pools and efficient lookups for optimal performance
- **Clean**: Modern C++ design with clear separation of concerns
- **Serializable**: Full YAML serialization/deserialization support

## Architecture

### Core Components

1. **ComponentManager**: Manages component storage and retrieval using component pools
2. **EntityManager**: Handles entity creation, destruction, and relationships
3. **SystemManager**: Manages and updates all systems
4. **Scene**: High-level container that orchestrates all ECS components

### Key Features

- **Type-safe component system** with compile-time type checking
- **Component pools** for efficient memory management and cache-friendly access
- **System registration** with automatic dependency injection
- **Parent-child entity relationships** for hierarchical scene graphs
- **YAML serialization** for scenes, entities, and components
- **Query system** for finding entities with specific component combinations

## Basic Usage

### 1. Creating a Scene

```cpp
#include "Scene.h"

// Create a new scene
Scene scene("MyScene", 1);
```

### 2. Registering Systems

```cpp
#include "system/CommonSystems.h"

// Register built-in systems
auto* physicsSystem = scene.RegisterSystem<PhysicsSystem>();
auto* renderSystem = scene.RegisterSystem<RenderSystem>();
auto* cameraSystem = scene.RegisterSystem<CameraSystem>();

// Configure systems
physicsSystem->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
```

### 3. Creating Entities and Adding Components

```cpp
#include "component/CommonComponents.h"

// Create an entity
Entity player = scene.CreateEntity("Player");

// Add components
player.AddComponent<TransformComponent>(glm::vec3(0.0f, 5.0f, 0.0f));
player.AddComponent<RenderableComponent>("player_mesh", "player_material");
player.AddComponent<PhysicsComponent>(70.0f); // 70kg mass

// Configure components
auto* renderable = player.GetComponent<RenderableComponent>();
if (renderable) {
    renderable->color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
}
```

### 4. Updating the Scene

```cpp
// In your game loop
float deltaTime = 1.0f / 60.0f; // 60 FPS
scene.Update(deltaTime);
```

### 5. Querying Entities

```cpp
// Find all entities with specific components
auto renderableEntities = scene.GetEntitiesWith<TransformComponent, RenderableComponent>();

for (const Entity& entity : renderableEntities) {
    auto* transform = entity.GetComponent<TransformComponent>();
    auto* renderable = entity.GetComponent<RenderableComponent>();
    
    // Process entity...
}
```

### 6. Serialization

```cpp
// Save scene to YAML file
scene.SaveToFile("my_scene.yaml");

// Load scene from YAML file
Scene newScene("LoadedScene");
newScene.RegisterSystem<PhysicsSystem>(); // Register systems before loading
newScene.LoadFromFile("my_scene.yaml");
```

## Creating Custom Components

```cpp
#include "component/Component.h"

class HealthComponent : public Component {
public:
    float maxHealth = 100.0f;
    float currentHealth = 100.0f;
    bool isDead = false;

    COMPONENT_TYPE(HealthComponent)

    HealthComponent() = default;
    HealthComponent(float max) : maxHealth(max), currentHealth(max) {}

    void TakeDamage(float damage) {
        currentHealth = std::max(0.0f, currentHealth - damage);
        isDead = currentHealth <= 0.0f;
    }

    // Serialization
    YAML::Node Serialize() const override {
        YAML::Node node;
        node["maxHealth"] = maxHealth;
        node["currentHealth"] = currentHealth;
        node["isDead"] = isDead;
        return node;
    }

    void Deserialize(const YAML::Node& node) override {
        maxHealth = node["maxHealth"].as<float>(100.0f);
        currentHealth = node["currentHealth"].as<float>(100.0f);
        isDead = node["isDead"].as<bool>(false);
    }
};
```

## Creating Custom Systems

```cpp
#include "system/System.h"

class HealthSystem : public ECSSystem {
public:
    SYSTEM_TYPE(HealthSystem)

    void Update(float deltaTime) override {
        // Get all entities with health components
        auto entities = GetEntitiesWith<HealthComponent>();
        
        for (EntityID entityID : entities) {
            auto* health = GetComponent<HealthComponent>(entityID);
            if (!health) continue;
            
            // Handle death
            if (health->isDead) {
                // Could trigger death animation, drop items, etc.
                HandleEntityDeath(entityID);
            }
        }
    }

private:
    void HandleEntityDeath(EntityID entityID) {
        // Custom death handling logic
        if (m_entityManager) {
            m_entityManager->DestroyEntity(entityID);
        }
    }
};
```

## Built-in Components

- **TransformComponent**: Position, rotation, scale
- **RenderableComponent**: Mesh, material, color, visibility
- **PhysicsComponent**: Velocity, acceleration, mass, gravity
- **CameraComponent**: FOV, near/far planes, projection settings
- **AudioComponent**: Audio clips, 3D positioning, volume
- **TagComponent**: String tags for categorization
- **LifetimeComponent**: Automatic entity destruction after time

## Built-in Systems

- **PhysicsSystem**: Updates physics simulation
- **RenderSystem**: Manages render queue and sorting
- **CameraSystem**: Handles camera transformations
- **AudioSystem**: Manages audio playback and 3D positioning
- **LifetimeSystem**: Destroys entities after their lifetime expires

## Performance Considerations

### Component Storage
- Components are stored in pools for cache-friendly access
- No dynamic casting during component retrieval
- Efficient component lookup using type IDs

### Entity Management
- Entity IDs are reused to minimize memory fragmentation
- Fast entity validation and lookup
- Efficient parent-child relationship management

### System Updates
- Systems only process entities that have required components
- Batch processing for optimal cache usage
- Systems can be enabled/disabled individually

## Advanced Features

### Entity Relationships
```cpp
Entity parent = scene.CreateEntity("Parent");
Entity child = scene.CreateEntity("Child");

// Set up parent-child relationship
child.SetParent(parent);

// Access relationships
auto children = parent.GetChildren();
Entity parentEntity = child.GetParent();
```

### Component Queries
```cpp
// Get entities with multiple specific components
auto physicsEntities = scene.GetEntitiesWith<TransformComponent, PhysicsComponent>();

// Check if entity has component
if (entity.HasComponent<HealthComponent>()) {
    auto* health = entity.GetComponent<HealthComponent>();
    // Use health component...
}
```

### System Management
```cpp
// Get system reference
auto* physics = scene.GetSystem<PhysicsSystem>();

// Enable/disable systems
scene.SetSystemEnabled<PhysicsSystem>(false);

// Remove systems
scene.RemoveSystem<PhysicsSystem>();
```

## Example Scene YAML Output

```yaml
name: ExampleScene
id: 1
active: true
entities:
  - id: 1
    name: Player
    active: true
    components:
      - type: TransformComponent
        enabled: true
        data:
          position: {x: 0, y: 5, z: 0}
          rotation: {x: 0, y: 0, z: 0}
          scale: {x: 1, y: 1, z: 1}
      - type: RenderableComponent
        enabled: true
        data:
          meshName: player_mesh
          materialName: player_material
          color: {r: 1, g: 0, b: 0, a: 1}
          visible: true
          renderLayer: 0
```

## Best Practices

1. **Component Design**: Keep components as data containers, avoid logic
2. **System Design**: Put all logic in systems, keep them focused on specific concerns
3. **Performance**: Use component queries efficiently, avoid unnecessary lookups
4. **Serialization**: Always implement Serialize/Deserialize for custom components
5. **Memory**: Use the COMPONENT_TYPE macro for type safety and performance

## Integration with Existing Code

The ECS system is designed to integrate with your existing renderer, physics engine, and audio system. Simply:

1. Update your renderer to consume data from the RenderSystem
2. Integrate PhysicsSystem with your physics engine
3. Connect AudioSystem to your audio engine
4. Use CameraSystem matrices for rendering

This ECS provides a solid foundation for building complex game systems while maintaining performance and modularity. 