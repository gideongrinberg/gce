#ifndef GUI_H
#define GUI_H
#include "core.h"
#include <stdbool.h>

#include "raylib-nuklear.h"

#define BOARD_SIZE 8
#ifndef EMSCRIPTEN
#define SELECTED_COLOR (Color){72, 118, 255, 180}  // Light blue
#define POSSIBLE_COLOR (Color){255, 255, 102, 160} // Yellow
#else
#define SELECTED_COLOR (Color){72, 118, 255, 255}  // Light blue
#define POSSIBLE_COLOR (Color){255, 255, 102, 255} // Yellow
#endif

struct nk_context;
typedef struct {
    int rank;
    int file;
} Square;

typedef enum { NEW_GAME, IN_PROGRESS, GAME_OVER } GameState;
typedef struct {
    Outcome outcome;
    int winner;
} GameOutcome;

typedef struct {
    GameState state;
    int player_color;
    GameOutcome outcome;
    Board *board;
    Square selected_square;
    // bitboard of squares the current piece can move to
    uint64_t possible_squares;
} Game;

Game *new_game();
void free_game(Game *game);

void game_update(Game *game);
void game_draw(Game *game);
void game_draw_gui(struct nk_context *ctx, Game *game);

void draw_board(Game *game);
void draw_debug_text(Game *game);

bool get_mouse_square(float mouseX, float mouseY, int screenWidth,
                      int screenHeight, int *outRank, int *outFile,
                      bool reverse);
#endif // GUI_H
