#ifndef GAME_HPP
#define GAME_HPP

#include "engine.h"
#include "raylib.h"
#include "window.hpp"

#include <memory>
#include <vector>

enum GameState : int { IN_PROGRESS, NEW_GAME, GAME_OVER };
enum GameMode : int { LOCAL_MP, ENGINE };
class Game {
  public:
    Game();

    void setup();
    void render();

    GameMode mode;
    GameState state;
    Position position;

    int playerColor;
#ifndef NDEBUG
    int showPst;
#endif

  private:
    void drawViewport();

    std::vector<std::unique_ptr<GuiWindow>> windows;
    Color bgColor;
};

#endif // GAME_HPP
