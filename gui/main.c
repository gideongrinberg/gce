#include "game.h"
#include "raylib.h"
#include "textures.h"

int main(const int argc, const char **argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Gideon's Chess Engine v0.1.0 (GUI Client)");
    SetTargetFPS(60);
    load_piece_textures();
    Game *game = new_game();
    while (!WindowShouldClose()) {
        game_update(game);
        BeginDrawing();
        ClearBackground(RAYWHITE);
        game_draw(game);
        EndDrawing();
    };
    free_game(game);
    unload_piece_textures();
    CloseWindow();
    return 0;
}
