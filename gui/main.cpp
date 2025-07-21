#include "game.hpp"
#include "raylib.h"
#include "textures.h"

#include <memory>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
int width, height;
EM_JS(int, canvas_get_width, (), { return Module.canvas.width; });
EM_JS(int, canvas_get_height, (), { return Module.canvas.height; });

extern "C" {
EMSCRIPTEN_KEEPALIVE void on_browser_resize(void) {
    SetWindowSize(canvas_get_width(), canvas_get_height());
}
}
#else
int width = 800, height = 600;
#endif

static std::unique_ptr<Game> game;
void mainLoop(void *arg) { static_cast<Game *>(arg)->render(); }

int main() {
#ifdef EMSCRIPTEN
    width = canvas_get_width(), height = canvas_get_height();
#endif
    game = std::make_unique<Game>(width, height);
#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(mainLoop, game.get(), 0, 1);
#else
    while (!WindowShouldClose()) {
        mainLoop(game.get());
    }
#endif
    unloadPieceTextures();
}