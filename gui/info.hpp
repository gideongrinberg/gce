#ifndef INFO_HPP
#define INFO_HPP
#include "imgui.h"
#include "position.h"
#include "window.hpp"
#include <cstring>
#include <string>

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
        ImGui::Text("Moves: %d", position.moves);
        ImGui::Text("Half moves: %d", position.halfmoves);
        std::string castling_rights = "";
        if (position.castling_rights & WHITE_KINGSIDE)
            castling_rights += "K";
        if (position.castling_rights & WHITE_QUEENSIDE)
            castling_rights += "Q";
        if (position.castling_rights & BLACK_KINGSIDE)
            castling_rights += "k";
        if (position.castling_rights & BLACK_QUEENSIDE)
            castling_rights += "q";
        if (castling_rights.empty())
            castling_rights = "-";
        ImGui::Text("Castling rights: %s", castling_rights.c_str());
        ImGui::End();
    }

  private:
    char fenBuffer[256]{};
};
#endif // INFO_HPP
