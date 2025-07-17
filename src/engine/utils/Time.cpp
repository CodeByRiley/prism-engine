#include "Time.h"

std::chrono::high_resolution_clock::time_point Time::lastTime = std::chrono::high_resolution_clock::now();
float Time::deltaTime = 0.0f;
float Time::totalTime = 0.0f;
int Time::fps = 0;

void Time::Tick() {
    auto now = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;
}

float Time::DeltaTime() { return deltaTime; }
float Time::TotalTime() { return totalTime; }
int Time::FPS() { return fps; }