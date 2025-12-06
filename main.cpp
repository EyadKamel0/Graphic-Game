#include "Game.h"
#include <iostream>

int main() {
    Game game;
    
    if (!game.init()) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return -1;
    }
    
    game.run();
    game.cleanup();
    
    return 0;
}
