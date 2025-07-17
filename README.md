# Prism Engine

Passion Project for learning OpenGL and C++.

If you want to use it, you can, but don't expect anything to be good.
However I'd much recommend you use something like [Raylib](https://www.raylib.com/) or [SFML](https://www.sfml-dev.org/) instead.
As these are serious projects and are updated regularly.

Initially cloned from [GLFWCMakeSetup](https://github.com/meemknight/GLFWCMakeSetup)

## ✨ Features

### 🎨 Rendering
- **Modern OpenGL**: Hardware-accelerated 2D Rendering
- **Batch Rendering**: Optimized quad batching for 2D sprites
- **Lighting System**: Dynamic 2D lighting with customizable light sources
- **Shader Support**: Custom GLSL shader pipeline

### 🎮 Core Systems
- **Entity-Component-Scene**: Flexible scene management
- **Input Handling**: Keyboard, mouse, and gamepad support
- **Resource Management**: Efficient asset loading and caching
- **Logging System**: Multi-level logging system / File logging

[TODO: Add more features](TODO.md)

### 🛠️ Developer Tools
- **ImGui Integration**: Real-time debugging and editor tools
- **Comprehensive Logging**: Multi-level logging system
- **Hot Reloading**: Shader and asset hot reloading during development
- **Cross-Platform**: Windows, Linux, and macOS support

## 📋 Requirements

### Minimum Requirements
- **OS**: Windows 10, Ubuntu 18.04+, or macOS 10.14+
- **GPU**: OpenGL 3.3+ compatible graphics card
- **RAM**: 2GB minimum, 4GB recommended
- **Storage**: 500MB for engine + assets

### Development Requirements
- **C++17** compatible compiler
- **CMake 3.16+**
- **Git** for version control

## 🚀 Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/CodeByRiley/prismengine.git
cd prismengine
```

### 2. Build the Engine
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 3. Run the Demo
```bash
# Windows
./Release/prismengine.exe

# Linux/macOS
./prismengine
```

## 🏗️ Project Structure

```
PrismEngine/
├── src/
│   ├── engine/
│   │   ├── core/               # Core engine systems
|   |   |   ├── physics/        # Physics Systems
│   │   |   ├── networking/     # Networking system [ENet](https://github.com/lsalzman/enet)
│   │   |   ├── audio/          # Audio system [raudio](https://github.com/raysan5/raudio)
│   │   |   ├── input/          # Input handling [GLFW](https://github.com/glfw/glfw)
│   │   ├── renderer/           # Rendering pipeline [OpenGL](https://www.opengl.org/)
|   |   |   ├── ui/             # UI System [ImGui](https://github.com/ocornut/imgui)
│   │   ├── scene/              # Scene management
|   |   |   ├── serialization/  # Serialization [YAML-CPP](https://github.com/jbeder/yaml-cpp)
|   |   |   ├── entity/         # Entity Component System
│   │   ├── math/               # Math utilities
│   │   ├── resource/           # Asset management
│   │   └── utils/              # Utility functions
│   ├── game/                   # Game-specific code
│   └── main.cpp                # Application entry point
├── resources/
│   ├── shaders/                # GLSL shaders
│   └── textures/               # Game textures
└── thirdparty/                 # External dependencies
```

## 💻 Usage Example

```cpp
#include "engine/core/Engine.h"
#include "engine/renderer/Renderer2D.h"

class MyGame : public Engine {
public:
    void Initialize() override {
        // Initialize your game
        renderer = new Renderer2D(width, height);
    }
    
    void Update(float deltaTime) override {
        // Update game logic
        player.Update(deltaTime);
    }
    
    void Render() override {
        // Render your game
        renderer->BeginBatch(renderer->GetBaseShader());
        // {Vector2 position, Vector2 size, Vector4 color}
        renderer->DrawRect({100, 100}, {50, 50}, {1, 0, 0, 1});
        renderer->EndBatch();
    }
    
private:
    Renderer2D* renderer;
    Player player;
};

int main() {
    MyGame game;
    game.Run();
    game.OnShutdown();
}
```

## 🎯 Key Components

### Renderer2D
High-performance 2D rendering with automatic batching:
```cpp
renderer->DrawRect(position, size, color);
renderer->DrawRectRot(position, size, rotation, color);
```

### Lighting System
Dynamic 2D lighting effects:
```cpp
LightRenderer2D lightRenderer;
Light pointLight(position, radius, color, intensity);
lightRenderer.AddLight(pointLight);
```

### Input System
Easy input handling:
```cpp
if (Input::IsKeyPressed(KEY_SPACE)) {
    player.Jump();
}

Vector2 mousePos = Input::GetMousePosition();
```

## 🔧 Building from Source

### Prerequisites
The engine automatically manages the following dependencies:
- **GLFW** - Windowing and input
- **GLAD** - OpenGL loading
- **GLM** - Mathematics library
- **ImGui** - Immediate mode GUI
- **stb_image** - Image loading
- **ENet** - Networking
- **raudio** - Audio system

### Build Configuration
For development builds:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

For production builds:
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DPRODUCTION_BUILD=ON ..
```
## 🤝 Contributing

contributions are welcome!

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](resources/License.txt) file for details.

## 🙏 Acknowledgments

- **GLFW Team** - Windowing and input handling
- **ImGui** - Immediate mode GUI framework
- **OpenGL** - Graphics API
- **stb Libraries** - Image and font loading
- **GLM** - Mathematics library