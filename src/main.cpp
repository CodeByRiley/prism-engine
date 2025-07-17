#include <iostream>
#include "engine/utils/Logger.h"
#include "game/Game.h"

int main() {
    Logger::Initialize("GLFWEng ECS");
    
    std::cout << "=== GLFWEng ECS Game ===" << std::endl;
    
    // Run the ECS-integrated game
    Game game(1280, 720, "GLFWEng ECS Game");
    
    game.Run();
    
    Logger::Info("Game Closing");
    std::cout << "\nGame finished!" << std::endl;
    return 0;
}
