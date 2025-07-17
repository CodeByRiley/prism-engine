#include "Engine.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "engine/core/input/Input.h"
#include "engine/utils/Time.h"
#include "engine/utils/ResourcePath.h"

Engine::Engine(int width, int height, const char* title)
    : m_Width(width), m_Height(height), m_Running(true)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Initialize resources
    ResourcePath::SetBasePath("resources/");


    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    glfwMakeContextCurrent(m_Window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1); // Enable vsync

    Input::Initialize(m_Window);
}

Engine::~Engine() {
    OnShutdown();
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Engine::Run() {
    // Initialize the game before starting the main loop
    OnInit();
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (m_Running && !glfwWindowShouldClose(m_Window)) {
        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
        glClear(GL_COLOR_BUFFER_BIT);

        PollEvents();
        OnUpdate();
        Input::Update();
        Time::Tick();
        OnDraw();
        glfwSwapBuffers(m_Window);
    }
}

void Engine::PollEvents() {
    glfwPollEvents();
    if (glfwWindowShouldClose(m_Window))
        m_Running = false;
}

void Engine::OnInit()    {}
void Engine::OnUpdate() {}
void Engine::OnDraw()    {}
void Engine::OnShutdown() {}