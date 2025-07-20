#ifndef MODALS_HPP
#define MODALS_HPP
#include "window.hpp"

class GameModal : public GuiWindow {
  public:
    explicit GameModal(Game &game) : GuiWindow(game) {};
    void render() override;
};
#endif // MODALS_HPP
