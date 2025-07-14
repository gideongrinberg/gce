#ifndef INFO_HPP
#define INFO_HPP
#include "imgui.h"
#include "position.h"
#include "window.hpp"
#include <cstring>

class InfoWindow : public GuiWindow {
  public:
    explicit InfoWindow(Position &pos) : GuiWindow(pos) {
        std::strcpy(fenBuffer,
                    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    };

    void render() override {
        ImGui::Begin("Info");
        ImGui::InputText("FEN", fenBuffer, sizeof(fenBuffer));
        if (ImGui::Button("Load FEN")) {
            if (Position *newPos = position_from_fen(fenBuffer)) {
                position = *newPos;
            }
        }
        ImGui::End();
    }

  private:
    char fenBuffer[256]{};
};
#endif // INFO_HPP
