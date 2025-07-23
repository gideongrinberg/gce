#ifndef GAME_HPP
#define GAME_HPP

#include "engine.h"
#include "polyglot.hpp"
#include "raylib.h"
#include "window.hpp"

#include <memory>
#include <string>
#include <vector>

enum GameState : int { IN_PROGRESS, NEW_GAME, GAME_OVER };
enum GameMode : int { LOCAL_MP, ENGINE };
class Game {
  public:
    Game(int width, int height);
    ~Game();

    void setup();
    void render();

    PolyglotBook book;
    bool onBook;
    std::string moves;
    GameMode mode;
    GameState state;
    Position position;

    int playerColor;
#ifndef NDEBUG
    int showPst;
#endif

  private:
    int width, height;
    void drawViewport();

    std::vector<std::unique_ptr<GuiWindow>> windows;
    Color bgColor;
};

#endif // GAME_HPP
