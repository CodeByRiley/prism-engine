#pragma once

#include <chrono>

class Time {
public:
    // Get the time between the current and last frame in seconds
    static float DeltaTime();
    
    // Get the time between the current and last frame in seconds (double precision)
    static double DeltaTimeDouble();
    
    // Update the time system - should be called once per frame
    static void Tick();
    
    // Get the total time since the application started in seconds
    static float TotalTime();
    
    // Get the total time since the application started in seconds (double precision)
    static double TotalTimeDouble();
    
    // Get the current frame count
    static uint64_t FrameCount();
    
    // Get the current frames per second
    static float FPS();

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;
    
    static TimePoint s_startTime;
    static TimePoint s_lastFrameTime;
    static TimePoint s_currentFrameTime;
    static double s_deltaTime;
    static uint64_t s_frameCount;
    static double s_fpsUpdateTimer;
    static float s_currentFPS;
    
    // Initialize the time system
    static void Initialize();
    static bool s_initialized;
}; 