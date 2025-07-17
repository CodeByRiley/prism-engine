#include <iostream>
#include "engine/utils/Logger.h"
#include "game/Game.h"
int main() {
    Logger::Initialize("Deez");
    Logger::Info("Yuh");

    Game game(1280,720,"Prism");
    game.Run();
    Logger::Info("Game Closing");
    return 0;
}
