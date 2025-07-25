#ifndef INFO_HPP
#define INFO_HPP
#include "assets.h"
#include "engine.h"
#include "imgui.h"
#include "nlohmann/json.hpp"
#include "window.hpp"
#include <cstdarg>
#include <cstring>
#include <iostream>
#include <string>

class InfoWindow : public GuiWindow {
  public:
    explicit InfoWindow(Game &game) : GuiWindow(game) {
        std::strcpy(fenBuffer,
                    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        std::string json_str(reinterpret_cast<const char *>(openings_json),
                             openings_json_len);
        openings = nlohmann::json::parse(json_str);
    };

    void render() override {
        ImGui::Begin("Info");

        if (!game.moves.empty() && openings.contains(game.moves)) {
            currentOpening = openings[game.moves];
        }

        if (!currentOpening.empty()) {
            ImGui::TextWrapped("%s (%s book)", currentOpening.c_str(),
                               game.onBook ? "on" : "off");
        }

        ImGui::InputText("##", fenBuffer, sizeof(fenBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Load FEN")) {
            if (Position *newPos = position_from_fen(fenBuffer)) {
                game.position = *newPos;
            }
        }
        ImGui::Text("Moves: %d", game.position.moves);
        ImGui::Text("Half moves: %d", game.position.halfmoves);
        std::string castling_rights = "";
        if (game.position.castling_rights & WHITE_KINGSIDE)
            castling_rights += "K";
        if (game.position.castling_rights & WHITE_QUEENSIDE)
            castling_rights += "Q";
        if (game.position.castling_rights & BLACK_KINGSIDE)
            castling_rights += "k";
        if (game.position.castling_rights & BLACK_QUEENSIDE)
            castling_rights += "q";
        if (castling_rights.empty())
            castling_rights = "-";
        ImGui::Text("Castling rights: %s", castling_rights.c_str());
        ImGui::End();
    }

  private:
    nlohmann::json openings;
    std::string currentOpening{};
    char fenBuffer[256]{};
};
#endif // INFO_HPP
