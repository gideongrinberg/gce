#ifndef WINDOW_HPP
#define WINDOW_HPP

class Game;
class GuiWindow {
  public:
    explicit GuiWindow(Game &game) : game(game) {}
    virtual ~GuiWindow() = default;

    virtual void render() = 0;

  protected:
    Game &game;
};

#endif // WINDOW_HPP
