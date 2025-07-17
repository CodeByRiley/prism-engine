#include "Input.h"
#include <GLFW/glfw3.h>
#include <array>

namespace {
    constexpr int KEY_COUNT = 350; // GLFW_KEY_LAST + 1
    constexpr int MOUSE_BUTTON_COUNT = 8;

    std::array<bool, KEY_COUNT> currentKeys{};
    std::array<bool, KEY_COUNT> previousKeys{};

    std::array<bool, MOUSE_BUTTON_COUNT> currentMouse{};
    std::array<bool, MOUSE_BUTTON_COUNT> previousMouse{};

    double mouseX = 0.0, mouseY = 0.0;
    GLFWwindow* g_window = nullptr;

    void KeyCallback(GLFWwindow*, int key, int, int action, int) {
        if (key >= 0 && key < KEY_COUNT) {
            currentKeys[key] = (action != GLFW_RELEASE);
        }
    }

    void MouseButtonCallback(GLFWwindow*, int button, int action, int) {
        if (button >= 0 && button < MOUSE_BUTTON_COUNT) {
            currentMouse[button] = (action != GLFW_RELEASE);
        }
    }

    void CursorPosCallback(GLFWwindow*, double xpos, double ypos) {
        mouseX = xpos;
        mouseY = ypos;
    }
}

namespace Input {

    void Initialize(GLFWwindow* window) {
        g_window = window;
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetCursorPosCallback(window, CursorPosCallback);
    }

    void Update() {
        previousKeys = currentKeys;
        previousMouse = currentMouse;
        // Mouse position updated via callback
    }

    // Keyboard
    bool IsKeyPressed(int key) {
        return currentKeys[key] && !previousKeys[key];
    }
    bool IsKeyHeld(int key) {
        return currentKeys[key];
    }
    bool IsKeyReleased(int key) {
        return !currentKeys[key] && previousKeys[key];
    }
    bool IsKeyUp(int key) {
        return !currentKeys[key];
    }

    // Mouse
    bool IsMousePressed(int button) {
        return currentMouse[button] && !previousMouse[button];
    }
    bool IsMouseHeld(int button) {
        return currentMouse[button];
    }
    bool IsMouseReleased(int button) {
        return !currentMouse[button] && previousMouse[button];
    }
    bool IsMouseUp(int button) {
        return !currentMouse[button];
    }

    double GetMouseX() { return mouseX; }
    double GetMouseY() { return mouseY; }
}