#define RAYLIB_NUKLEAR_IMPLEMENTATION
#include "raylib-nuklear.h"

#include "assets.h"
#include "game.h"
#include "raylib.h"
#include "textures.h"

#include <stdio.h>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

Font font;
struct nk_context *ctx;
Game *game;

// This loop runs every frame
void game_loop() {
    game_update(game);
    BeginDrawing();
    ClearBackground(ColorFromNuklear(ctx->style.window.background));
    game_draw(game);
    UpdateNuklear(ctx);
    game_draw_gui(ctx, game);
    DrawNuklear(ctx);
    EndDrawing();
}

#ifdef EMSCRIPTEN
int width, height;
EM_JS(int, canvas_get_width, (), { return Module.canvas.width; });
EM_JS(int, canvas_get_height, (), { return Module.canvas.height; });

EMSCRIPTEN_KEEPALIVE void on_browser_resize(void) {
    SetWindowSize(canvas_get_width(), canvas_get_height());
}
#else
int width = 800, height = 450;
#endif

int main(void) {
#ifdef EMSCRIPTEN
    width = canvas_get_width(), height = canvas_get_height();
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#endif
    InitWindow(width, height, "Gideon's Chess Engine v0.1.0 (GUI Client)");
    font = LoadFontFromMemory(".ttf", roboto_ttf, (int)roboto_ttf_len, 18, NULL,
                              0);
    ctx = InitNuklearEx(font, 18);
    load_piece_textures();
    game = new_game();

#ifdef EMSCRIPTEN
    emscripten_set_main_loop(game_loop, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        game_loop();
    };
#endif
    free_game(game);
    unload_piece_textures();
    UnloadFont(font);
    UnloadNuklear(ctx);
    CloseWindow();
    return 0;
}
