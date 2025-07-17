#include "Time.h"
#include <algorithm>

// Initialize static members
Time::TimePoint Time::s_startTime = Time::Clock::now();
Time::TimePoint Time::s_lastFrameTime = Time::Clock::now();
Time::TimePoint Time::s_currentFrameTime = Time::Clock::now();
double Time::s_deltaTime = 0.0;
uint64_t Time::s_frameCount = 0;
double Time::s_fpsUpdateTimer = 0.0;
float Time::s_currentFPS = 0.0f;
bool Time::s_initialized = false;

void Time::Initialize() {
    if (s_initialized) return;
    
    s_startTime = Clock::now();
    s_lastFrameTime = s_startTime;
    s_currentFrameTime = s_startTime;
    s_deltaTime = 0.0;
    s_frameCount = 0;
    s_fpsUpdateTimer = 0.0;
    s_currentFPS = 0.0f;
    s_initialized = true;
}

void Time::Tick() {
    if (!s_initialized) {
        Initialize();
    }
    
    // Update time points
    s_lastFrameTime = s_currentFrameTime;
    s_currentFrameTime = Clock::now();
    
    // Calculate delta time
    Duration delta = s_currentFrameTime - s_lastFrameTime;
    s_deltaTime = delta.count();
    
    // Clamp delta time to prevent huge jumps (useful when debugging or alt-tabbing)
    s_deltaTime = std::min(s_deltaTime, 1.0 / 20.0); // Max 50ms (20 FPS minimum)
    
    // Update frame count
    s_frameCount++;
    
    // Update FPS calculation (every 0.5 seconds)
    s_fpsUpdateTimer += s_deltaTime;
    if (s_fpsUpdateTimer >= 0.5) {
        s_currentFPS = static_cast<float>(1.0 / s_deltaTime);
        s_fpsUpdateTimer = 0.0;
    }
}

float Time::DeltaTime() {
    if (!s_initialized) {
        Initialize();
    }
    return static_cast<float>(s_deltaTime);
}

double Time::DeltaTimeDouble() {
    if (!s_initialized) {
        Initialize();
    }
    return s_deltaTime;
}

float Time::TotalTime() {
    if (!s_initialized) {
        Initialize();
    }
    Duration total = s_currentFrameTime - s_startTime;
    return static_cast<float>(total.count());
}

double Time::TotalTimeDouble() {
    if (!s_initialized) {
        Initialize();
    }
    Duration total = s_currentFrameTime - s_startTime;
    return total.count();
}

uint64_t Time::FrameCount() {
    return s_frameCount;
}

float Time::FPS() {
    return s_currentFPS;
} 