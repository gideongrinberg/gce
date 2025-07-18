#include "game.hpp"
#include "raylib.h"
#include "textures.h"

#include <memory>
int main() {
    auto game = std::make_unique<Game>();
    while (!WindowShouldClose()) {
        game->render();
    }
    unloadPieceTextures();
}