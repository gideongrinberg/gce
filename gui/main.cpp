#define Vector2Ones ((Vector2){1.0f, 1.0f})
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