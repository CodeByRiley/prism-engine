#pragma once
#include <GLFW/glfw3.h>
#include <array>

namespace Input {
    void Initialize(GLFWwindow* window);
    void Update();

    // Keyboard
    bool IsKeyPressed(int key);
    bool IsKeyHeld(int key);
    bool IsKeyReleased(int key);
    bool IsKeyUp(int key);

    // Mouse
    bool IsMousePressed(int button);
    bool IsMouseHeld(int button);
    bool IsMouseReleased(int button);
    bool IsMouseUp(int button);

    double GetMouseX();
    double GetMouseY();
}