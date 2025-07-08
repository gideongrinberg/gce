#include "core.h"
#include "raylib.h"
#include "game.h"

int main(const int argc, const char **argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Gideon's Chess Engine v0.1.0 (GUI Client)");
    SetTargetFPS(60);

    Game* game = new_game();
    while (!WindowShouldClose()) {
        game_update(game);
        BeginDrawing();
        ClearBackground(RAYWHITE);
        game_draw(game);
        EndDrawing();
    };
    free_game(game);
    CloseWindow();
    return 0;
}
