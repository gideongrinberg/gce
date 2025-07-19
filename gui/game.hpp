#ifndef GAME_HPP
#define GAME_HPP

#include "engine.h"
#include "raylib.h"
#include "window.hpp"

#include <memory>
#include <vector>

class Game {
  public:
    Game();

    void setup();
    void render();

    Position position;
#ifndef NDEBUG
    int showPst;
#endif

  private:
    void drawViewport();

    std::vector<std::unique_ptr<GuiWindow>> windows;
    Color bgColor;
};

#endif // GAME_HPP
