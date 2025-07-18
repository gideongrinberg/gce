#ifndef WINDOW_HPP
#define WINDOW_HPP
#include "engine.h"

class GuiWindow {
  public:
    explicit GuiWindow(Position &positionRef) : position(positionRef) {}
    virtual ~GuiWindow() = default;

    virtual void render() = 0;

  protected:
    Position &position;
};

#endif // WINDOW_HPP
