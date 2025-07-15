#include "board.hpp"

#include "textures.h"

#include <iostream>

#define SELECTED_COLOR (Color){72, 118, 255, 180}
#define LEGAL_COLOR (Color){255, 0, 255, 128}

void Board::draw() const {
    BeginTextureMode(renderTexture);
    ClearBackground(RAYWHITE);
    int tileSize = renderTexture.texture.height / 8;
    // draw squares
    for (int sq = 0; sq < 64; sq++) {
        Color color = ((sq / 8 + sq % 8) % 2 == 1) ? DARKBROWN : BEIGE;
        Vector2 pos = squareToScreen(sq, tileSize);
        DrawRectangle(pos.x, pos.y, tileSize, tileSize, color);
    }

    // highlight selected sq
    if (selectedSq != -1) {
        Vector2 pos = squareToScreen(selectedSq, tileSize);
        DrawRectangle(pos.x, pos.y, tileSize, tileSize, SELECTED_COLOR);
    }

    FOREACH_SET_BIT(legalMoves, sq) {
        Vector2 pos = squareToScreen(sq, tileSize);
        DrawRectangle(pos.x, pos.y, tileSize, tileSize, LEGAL_COLOR);
    }

    // draw pieces
    const std::array<uint8_t, 2> colors = {PIECE_WHITE, PIECE_BLACK};
    const std::array<uint8_t, 6> pieces = {PIECE_PAWN,   PIECE_KNIGHT,
                                           PIECE_BISHOP, PIECE_ROOK,
                                           PIECE_QUEEN,  PIECE_KING};

    for (auto color : colors) {
        for (auto piece : pieces) {
            FOREACH_SET_BIT(position.bitboards[color | piece], sq) {
                Vector2 pos = squareToScreen(sq, tileSize);
                Texture2D tex = getPieceTexture(color | piece);
                Rectangle src = {0, 0, static_cast<float>(tex.width),
                                 static_cast<float>(tex.height)};
                Rectangle dst = {pos.x, pos.y, static_cast<float>(tileSize),
                                 static_cast<float>(tileSize)};

                Vector2 origin = {0.0f, 0.0f};
                DrawTexturePro(tex, src, dst, origin, 0.0f, WHITE);
            }
        }
    }

    EndTextureMode();
}

void Board::update() {
    int clicked = getClicked();
    if (ImGui::BeginPopupModal("Promo", &pendingPromo.display,
                               ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoDocking)) {
        std::cout << "displaying modal" << std::endl;

        ImGui::Text("Choose promotion piece:");
        static int promotionChoice = 0;
        if (ImGui::RadioButton("Knight", promotionChoice == 1))
            promotionChoice = 1;
        if (ImGui::RadioButton("Bishop", promotionChoice == 2))
            promotionChoice = 2;
        if (ImGui::RadioButton("Rook", promotionChoice == 3))
            promotionChoice = 3;
        if (ImGui::RadioButton("Queen", promotionChoice == 4))
            promotionChoice = 4;
        if (ImGui::Button("Ok")) {
            execute_move(&position,
                         ENCODE_MOVE(pendingPromo.from, pendingPromo.to,
                                     promotionChoice));
            pendingPromo.display = false;
            selectedSq = -1;
            legalMoves = 0;
            promoMoves = 0;
        }
        ImGui::EndPopup();
    }

    if (clicked != -1) {
        int sideToMove = position.moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
        // if piece selected
        if (GET_COLOR_OCCUPIED(&position, sideToMove) & (1ULL << clicked)) {
            selectedSq = clicked;

            // update legal moves
            legalMoves = 0;
            std::array<Move, 256> moves = {};
            int numMoves = generate_moves(&position, moves.data());
            for (int i = 0; i < numMoves; i++) {
                Move move = moves[i];
                if (MOVE_FROM(move) == selectedSq) {
                    legalMoves |= 1ULL << MOVE_TO(move);
                    if (MOVE_PROMO(move) != 0) {
                        promoMoves |= 1ULL << MOVE_TO(move);
                    }
                }
            }
        } else if (legalMoves & (1ULL << clicked)) {
            std::cout << clicked << std::endl;
            if (promoMoves & (1ULL << clicked)) {
                std::cout << "promo" << std::endl;
                pendingPromo = PendingPromotion{selectedSq, clicked, true};
                ImGui::OpenPopup("Promo");
                std::cout << pendingPromo.display << std::endl;
            } else {
                execute_move(&position, ENCODE_MOVE(selectedSq, clicked, 0));
                legalMoves = 0;
                promoMoves = 0;
                selectedSq = -1;
            }
        }
    }
}

void Board::render() {
    draw();
    ImGui::Begin("Board");
    // Size board to texture
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    const auto textureWidth = static_cast<float>(renderTexture.texture.width);
    const auto textureHeight = static_cast<float>(renderTexture.texture.height);
    const float textureAspect = textureWidth / textureHeight;
    const float availAspect = availableSize.x / availableSize.y;

    ImVec2 drawSize;
    if (availAspect > textureAspect) {
        drawSize.y = availableSize.y;
        drawSize.x = drawSize.y * textureAspect;
    } else {
        drawSize.x = availableSize.x;
        drawSize.y = drawSize.x / textureAspect;
    }

    // Center texture in window
    const ImVec2 cursorPos = ImGui::GetCursorPos();
    const ImVec2 centeredPos = {
        cursorPos.x + (availableSize.x - drawSize.x) * 0.5f,
        cursorPos.y + (availableSize.y - drawSize.y) * 0.5f};
    ImGui::SetCursorPos(centeredPos);

    // Convert to ImTexture and render
    const auto texId = static_cast<ImTextureID>(
        static_cast<uintptr_t>(renderTexture.texture.id));
    ImGui::Image(texId, drawSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    update();
    ImGui::End();
}

int Board::getClicked() {
    const ImVec2 mousePos = ImGui::GetMousePos();
    const ImVec2 boardTopLeft = ImGui::GetItemRectMin();
    const ImVec2 boardSize = ImGui::GetItemRectSize();

    const float relX = mousePos.x - boardTopLeft.x;
    const float relY = mousePos.y - boardTopLeft.y;

    if (relX >= 0 && relX < boardSize.x && relY >= 0 && relY < boardSize.y) {

        float scale = boardSize.x / 8.0f;

        int file = static_cast<int>(relX / scale);
        int rank = 7 - static_cast<int>(relY / scale);

        int square = rank * 8 + file;

        if (ImGui::IsMouseClicked(0, false)) {
            return square;
        }
    }

    return -1;
}