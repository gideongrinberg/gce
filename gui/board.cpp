#include "board.hpp"
#include "game.hpp"
#include "textures.h"
#include <iostream>
#include <optional>
#include <algorithm>
#include <cmath>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#else
#include <thread>
#endif

#define SELECTED_COLOR (Color){72, 118, 255, 180}
#define LEGAL_COLOR (Color){255, 0, 255, 128}

// -1 = no, 0 = started, 1 = done
static std::atomic engineStatus = -1;
static std::optional<Move> bestMove;

Color hslToRgb(float h, float s, float l, unsigned char alpha = 255);
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

    // draw PST values (for debug)
#ifndef NDEBUG
    if (game.showPst != -1) {
        bool black = PIECE_COLOR(game.showPst) == PIECE_BLACK;
        int piece = PIECE_TYPE(game.showPst);
        const int *pst = piece_tables[piece];
        auto [min, max] = std::minmax_element(pst, pst + 64);
        for (int sq = 0; sq < 64; sq++) {
            int sqFlipped = black ? sq ^ 56 : sq;
            int sqValue = pst[sqFlipped];

            float norm = 0.5f;
            if (*max - *min != 0) {
                norm = static_cast<float>(sqValue - *min) / (*max - *min);
                std::cout << max - min << std::endl;
            }
            Vector2 pos = squareToScreen(sqFlipped, tileSize);
            float hue = (1.0f - norm) * 120.0f;
            Color c = hslToRgb(hue, 1.0f, 0.5f, 200);
            DrawRectangle(pos.x, pos.y, tileSize, tileSize, c);
        }

    } else {
#endif
        // highlight selected sq
        if (selectedSq != -1) {
            auto [x, y] = squareToScreen(selectedSq, tileSize);
            DrawRectangle(x, y, tileSize, tileSize, SELECTED_COLOR);
        }

        FOREACH_SET_BIT(legalMoves, sq) {
            auto [x, y] = squareToScreen(sq, tileSize);
            DrawRectangle(x, y, tileSize, tileSize, LEGAL_COLOR);
        }

#ifndef NDEBUG
    }
#endif

    // draw pieces
    constexpr std::array<uint8_t, 2> colors = {PIECE_WHITE, PIECE_BLACK};
    constexpr std::array<uint8_t, 6> pieces = {PIECE_PAWN,   PIECE_KNIGHT,
                                               PIECE_BISHOP, PIECE_ROOK,
                                               PIECE_QUEEN,  PIECE_KING};

    for (auto color : colors) {
        for (auto piece : pieces) {
            FOREACH_SET_BIT(game.position.bitboards[color | piece], sq) {
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

void Board::handleInput(const ImVec2 &boardTopLeft, const ImVec2 &boardSize) {
    int clicked = getClicked(boardTopLeft, boardSize);
    int sideToMove = game.position.moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
    // promotion modal
    if (ImGui::BeginPopupModal("Promo", &pendingPromo.display,
                               ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoDocking)) {

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
            executeMove(ENCODE_MOVE(pendingPromo.from, pendingPromo.to,
                                    promotionChoice));
            pendingPromo.display = false;
            selectedSq = -1;
            legalMoves = 0;
            promoMoves = 0;
        }
        ImGui::EndPopup();
    }

    if (clicked != -1) {
        // if piece selected
        if (GET_COLOR_OCCUPIED(&game.position, sideToMove) &
            (1ULL << clicked)) {
            selectedSq = clicked;

            // update legal moves
            legalMoves = 0;
            std::array<Move, 256> moves = {};
            int numMoves = generate_moves(&game.position, moves.data());
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
            if (promoMoves & (1ULL << clicked)) {
                pendingPromo = PendingPromotion{selectedSq, clicked, true};
                ImGui::OpenPopup("Promo");
            } else {
                executeMove(ENCODE_MOVE(selectedSq, clicked, 0));
                legalMoves = 0;
                promoMoves = 0;
                selectedSq = -1;
            }
        }
    }
}

void getBestMove(void *arg) {
    auto game = static_cast<Game *>(arg);
    if (game->onBook) {
        Move move = game->book.getMove(&game->position);
        if (move == 0) {
            game->onBook = false;
        } else {
            bestMove = move;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            engineStatus = 1;
            return;
        }
    }

    bestMove = get_best_move(&game->position, 5);
    engineStatus = 1;
}

std::string moveToString(Move move) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int promo = MOVE_PROMO(move);

    auto sqToStr = [](int sq) {
        char file = 'a' + (sq % 8);
        char rank = '1' + (sq / 8);
        return std::string{file, rank};
    };

    std::string uci = sqToStr(from) + sqToStr(to);

    if (promo != 0) {
        char promoChar = 'q'; // fallback/default
        switch (promo) {
        case 1:
            promoChar = 'n';
            break;
        case 2:
            promoChar = 'b';
            break;
        case 3:
            promoChar = 'r';
            break;
        case 4:
            promoChar = 'q';
            break;
        }

        uci += promoChar;
    }

    return uci;
}

void Board::executeMove(Move move) {
    execute_move(&game.position, move);
    if (game.moves.empty()) {
        game.moves = moveToString(move);
    } else {
        game.moves += " " + moveToString(move);
    }
}

void Board::update() {
    int sideToMove = game.position.moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
    // make engine move
    if (game.mode == ENGINE && sideToMove != game.playerColor) {
        if (engineStatus == -1) {
            engineStatus = 0;
#ifdef EMSCRIPTEN
            emscripten_async_call(getBestMove, &game, 0);
#else
            std::thread([this] { getBestMove(&game); }).detach();
#endif
        } else if (engineStatus == 1 && bestMove.has_value()) {
            executeMove(bestMove.value());
            bestMove = std::nullopt;
            engineStatus = -1;
            // lastEngineMove = game.position.moves + 1;
        }
    }

    if (position_outcome(&game.position) != ONGOING) {
        game.state = GAME_OVER;
    }
}

void Board::render() {
    if (game.state == IN_PROGRESS)
        update();
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
    ImVec2 boardTopLeft = ImGui::GetItemRectMin();
    ImVec2 boardSize = ImGui::GetItemRectSize();
    handleInput(boardTopLeft, boardSize);
    ImGui::End();
}

int Board::getClicked(const ImVec2 &boardTopLeft, const ImVec2 &boardSize) {
    const ImVec2 mousePos = ImGui::GetMousePos();
    float relX = mousePos.x - boardTopLeft.x;
    float relY = mousePos.y - boardTopLeft.y;

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

Color hslToRgb(float h, float s, float l, unsigned char alpha) {
    float c = (1.0f - std::fabs(2.0f * l - 1.0f)) * s;
    float h_prime = h / 60.0f;
    float x = c * (1.0f - std::fabs(fmod(h_prime, 2.0f) - 1.0f));

    float r = 0, g = 0, b = 0;
    if (0 <= h_prime && h_prime < 1) {
        r = c;
        g = x;
        b = 0;
    } else if (1 <= h_prime && h_prime < 2) {
        r = x;
        g = c;
        b = 0;
    } else if (2 <= h_prime && h_prime < 3) {
        r = 0;
        g = c;
        b = x;
    } else if (3 <= h_prime && h_prime < 4) {
        r = 0;
        g = x;
        b = c;
    } else if (4 <= h_prime && h_prime < 5) {
        r = x;
        g = 0;
        b = c;
    } else if (5 <= h_prime && h_prime < 6) {
        r = c;
        g = 0;
        b = x;
    }

    float m = l - c / 2.0f;
    r += m;
    g += m;
    b += m;

    return {static_cast<unsigned char>(r * 255.0f),
            static_cast<unsigned char>(g * 255.0f),
            static_cast<unsigned char>(b * 255.0f), alpha};
}
