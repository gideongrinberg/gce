#ifndef BOARD_HPP
#define BOARD_HPP

#include "engine.h"
#include "imgui.h"
#include "raylib.h"
#include "window.hpp"

#include <array>

struct PendingPromotion {
    int from;
    int to;
    bool display;
};

class Board : public GuiWindow {
  public:
    RenderTexture renderTexture{};

    explicit Board(Game &game)
        : GuiWindow(game), selectedSq(-1), legalMoves(0), promoMoves(0),
          renderTexture(LoadRenderTexture(2048, 2048)) {
        pendingPromo.display = false;
    }

    void draw() const;
    void update();
    void handleInput(const ImVec2 &boardTopLeft, const ImVec2 &boardSize);
    void render() override;

  private:
    std::array<RenderTexture2D, MAX_PIECE> pieceTextures;
    int selectedSq;
    uint64_t legalMoves;
    uint64_t promoMoves;
    PendingPromotion pendingPromo;
    static int getClicked(const ImVec2 &boardTopLeft, const ImVec2 &boardSize);
};

inline Vector2 squareToScreen(int square, int tileSize) {
    int file = square % 8;
    int rank = 7 - (square / 8); // Flip Y for rendering
    return {static_cast<float>(file * tileSize),
            static_cast<float>(rank * tileSize)};
}

#endif // BOARD_HPP
