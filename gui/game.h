#ifndef GUI_H
#define GUI_H
#include "core.h"
#include <stdbool.h>

#define BOARD_SIZE 8
#define SELECTED_COLOR (Color){72, 118, 255, 180}  // Light blue
#define POSSIBLE_COLOR (Color){255, 255, 102, 160} // Yellow

typedef struct {
    int rank;
    int file;
} Square;

typedef struct {
    Board *board;
    Square selected_square;
    uint64_t
        possible_squares; // bitboard of squares the current piece can move to
} Game;

Game *new_game();
void free_game(Game *game);

void game_update(Game *game);
void game_draw(Game *game);

void draw_board(Game *game);
bool get_mouse_square(int mouseX, int mouseY, int screenWidth, int screenHeight,
                      int *outRank, int *outFile);
#endif // GUI_H
