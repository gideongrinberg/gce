#ifndef BOARD_HPP
#define BOARD_HPP

#include "engine.h"
#include "imgui.h"
#include "raylib.h"
#include "window.hpp"

#include <array>
#include <memory>

struct PendingPromotion {
    int from;
    int to;
    bool display;
};

class Board : public GuiWindow {
  public:
    RenderTexture renderTexture{};

    explicit Board(Position &pos)
        : GuiWindow(pos), selectedSq(-1),
          renderTexture(LoadRenderTexture(2048, 2048)) {
        pendingPromo.display = false;
    }

    void draw() const;
    void update();
    void render();

  private:
    std::array<RenderTexture2D, MAX_PIECE> pieceTextures;
    int selectedSq;
    uint64_t legalMoves;
    uint64_t promoMoves;
    PendingPromotion pendingPromo;
    int getClicked();
};

inline Vector2 squareToScreen(int square, int tileSize) {
    int file = square % 8;
    int rank = 7 - (square / 8); // Flip Y for rendering
    return {static_cast<float>(file * tileSize),
            static_cast<float>(rank * tileSize)};
}

#endif // BOARD_HPP
