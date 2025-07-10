#define RAYLIB_NUKLEAR_IMPLEMENTATION
#include "raylib-nuklear.h"

#include "assets.h"
#include "game.h"
#include "raylib.h"
#include "textures.h"

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Gideon's Chess Engine v0.1.0 (GUI Client)");
    SetTargetFPS(60);
    Font font = LoadFontFromMemory(".ttf", roboto_ttf, (int)roboto_ttf_len, 18,
                                   NULL, 0);
    struct nk_context *ctx = InitNuklearEx(font, 18);
    load_piece_textures();
    Game *game = new_game();

    while (!WindowShouldClose()) {
        game_update(game);
        BeginDrawing();
        ClearBackground(ColorFromNuklear(ctx->style.window.background));
        game_draw(game);
        game_draw_gui(ctx, game);
        UpdateNuklear(ctx);
        DrawNuklear(ctx);
        EndDrawing();
    };

    free_game(game);
    unload_piece_textures();
    UnloadFont(font);
    UnloadNuklear(ctx);
    CloseWindow();
    return 0;
}
