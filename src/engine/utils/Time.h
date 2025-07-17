#pragma once
#include <chrono>

class Time {
public:
    static void Tick();
    static float DeltaTime();
    static float TotalTime();
    static int FPS();

private:
    static std::chrono::high_resolution_clock::time_point lastTime;
    static float deltaTime;
    static float totalTime;
    static int fps;
};