#pragma once

class Engine {
public:
    Engine(int width, int height, const char* title);
    virtual ~Engine();

    // Main loop
    void Run();

protected:
    // Lifecycle hooks
    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnDraw();
    virtual void OnShutdown();
    virtual void OnResize(int width, int height) {}  // Empty default implementation

    // Window and context
    struct GLFWwindow* m_Window;
    int m_Width, m_Height;
    bool m_Running;

private:
    void PollEvents();
    float GetDeltaTime();
};