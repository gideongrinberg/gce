#include "modals.hpp"

#include "game.hpp"
#include "imgui.h"

#include <iostream>

void newGameModal(Game *game) {
    static bool modalOpen = false;
    static bool openSettings = false;
    if (game->state == NEW_GAME && !modalOpen) {
        ImGui::OpenPopup("Start a new game");
        modalOpen = true;
    }

    if (openSettings) {
        openSettings = false;
        ImGui::OpenPopup("Game settings");
    }

    if (ImGui::BeginPopupModal("Start a new game")) {
        static int mode = 1;
        ImGui::Text("Game mode:");
        ImGui::RadioButton("Pass-and-play", &mode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Play against a bot", &mode, 2);
        if (mode == 1) {
            if (ImGui::Button("Start")) {
                game->state = IN_PROGRESS;
                game->mode = LOCAL_MP;
                game->position = *position_from_fen(
                    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                modalOpen = false;
                ImGui::CloseCurrentPopup();
            }
        } else {
            if (ImGui::Button("Next")) {
                ImGui::CloseCurrentPopup();
                openSettings = true;
            }
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Game settings")) {
        static int color = 0;
        ImGui::RadioButton("White", &color, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Black", &color, 8);
        if (ImGui::Button("Start")) {
            game->state = IN_PROGRESS;
            game->mode = ENGINE;
            game->position = *position_from_fen(
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            game->playerColor = color;
            modalOpen = false;
            openSettings = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void gameOverModal(Game *game) {
    static bool modalOpen = false;
    static bool restartRequested = false;
    if (ImGui::BeginPopupModal("Game over")) {
        switch (position_outcome(&game->position)) {
        case STALEMATE:
            ImGui::Text("Draw by stalemate");
            break;
        case DRAW:
            ImGui::Text("Draw by 50 move rule");
            break;
        case CHECKMATE:
            if (game->position.moves % 2 ==
                0) { // if it's white's turn to move, black won
                ImGui::Text("Black wins by checkmate");
            } else {
                ImGui::Text("White wins by checkmate");
            }
        }

        if (ImGui::Button("Play again")) {
            game->state = NEW_GAME;
            ImGui::CloseCurrentPopup();
            restartRequested = true;
        }

        ImGui::EndPopup();
    }
    if (!modalOpen) {
        ImGui::OpenPopup("Game over");
        modalOpen = true;
    }

    if (restartRequested) {
        restartRequested = false;
        modalOpen = false;
        game->state = NEW_GAME;
    }
}

void GameModal::render() {
    switch (game.state) {
    case NEW_GAME:
        newGameModal(&game);
        break;
    case GAME_OVER:
        gameOverModal(&game);
        break;
    case IN_PROGRESS:
        break;
    }
}
