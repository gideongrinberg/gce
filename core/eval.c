#include "core.h"

#include <string.h>

// change PIECE_WHITE/PIECE_BLACK (8/16) to 0/1 for indexing piece tables
#define COLOR_INDEX(color) (((color) >> 4) & 1)
#define FLIP(sq) ((sq) ^ 56)
// Based on Sunfish's tables.
static const int piece_values[7] = {
    [PAWN] = 100, [KNIGHT] = 280, [BISHOP] = 320,
    [ROOK] = 479, [QUEEN] = 929,  [KING] = 60000};

// these piece tables are for white. they are flipped and negated at runtime
// to form the black tables.
static const int raw_piece_tables[7][64] = {
    [PAWN] = {0,   0,   0,  0,   0,   0,   0,   0,  78,  83,  86, 73,  102,
              82,  85,  90, 7,   29,  21,  44,  40, 31,  44,  7,  -17, 16,
              -2,  15,  14, 0,   15,  -13, -26, 3,  10,  9,   6,  1,   0,
              -23, -22, 9,  5,   -11, -10, -2,  3,  -19, -31, 8,  -7,  -37,
              -36, -14, 3,  -31, 0,   0,   0,   0,  0,   0,   0,  0},
    [KNIGHT] = {-66, -53, -75, -75, -10, -55, -58, -70, -3,  -6,  100, -36, 4,
                62,  -4,  -14, 10,  67,  1,   74,  73,  27,  62,  -2,  24,  24,
                45,  37,  33,  41,  25,  17,  -1,  5,   31,  21,  22,  35,  2,
                0,   -18, 10,  13,  22,  18,  15,  11,  -14, -23, -15, 2,   0,
                2,   0,   -23, -20, -74, -23, -26, -24, -19, -35, -22, -69},
    [BISHOP] = {-59, -78, -82, -76, -23, -107, -37, -50, -11, 20, 35,
                -42, -39, 31,  2,   -22, -9,   39,  -32, 41,  52, -10,
                28,  -14, 25,  17,  20,  34,   26,  25,  15,  10, 13,
                10,  17,  23,  17,  16,  0,    7,   14,  25,  24, 15,
                8,   25,  20,  15,  19,  20,   11,  6,   7,   6,  20,
                16,  -7,  2,   -15, -12, -14,  -15, -10, -10},
    [ROOK] = {35,  29,  33,  4,   37,  33,  56,  50,  55,  29,  56,  67,  55,
              62,  34,  60,  19,  35,  28,  33,  45,  27,  25,  15,  0,   5,
              16,  13,  18,  -4,  -9,  -6,  -28, -35, -16, -21, -13, -29, -46,
              -30, -42, -28, -42, -25, -25, -35, -26, -46, -53, -38, -31, -26,
              -29, -43, -44, -53, -30, -24, -18, 5,   -2,  -18, -31, -32},
    [QUEEN] = {6,   1,   -8,  -104, 69,  24,  88,  26,  14,  32,  60,  -10, 20,
               76,  57,  24,  -2,   43,  32,  60,  72,  63,  43,  2,   1,   -16,
               22,  17,  25,  20,   -13, -6,  -14, -15, -2,  -5,  -1,  -10, -20,
               -22, -30, -6,  -13,  -11, -16, -11, -16, -27, -36, -18, 0,   -19,
               -15, -15, -21, -38,  -39, -30, -31, -13, -31, -36, -34, -42},
    [KING] = {4,   54,  47,  -99, -99, 60,  83,  -62, -32, 10,  55,  56,  56,
              55,  10,  3,   -62, 12,  -57, 44,  -67, 28,  37,  -31, -55, 50,
              11,  -4,  -19, 13,  0,   -49, -55, -43, -52, -28, -51, -47, -8,
              -50, -47, -42, -43, -79, -64, -32, -29, -32, -4,  3,   -14, -50,
              -57, -18, 13,  4,   17,  30,  -3,  -14, 6,   -1,  40,  18}};

int piece_tables[2][7][64];
static bool initialized = false;

void init_tables() {
    for (int i = 0; i < 7; i++) {
        memcpy(piece_tables[0][i], raw_piece_tables[i], sizeof(int) * 64);
        for (int j = 0; j < 64; j++) {
            piece_tables[1][i][FLIP(j)] = 0 - piece_tables[0][i][j];
        }
    }
}

int eval_board(Board *board) {
    if (!initialized)
        init_tables();

    int pos = 0;
    int mat = 0;
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int index = BOARD_INDEX(rank, file);
            uint8_t piece = board->board[index];
            if (GET_TYPE(piece) == EMPTY)
                continue;

            pos += piece_tables[COLOR_INDEX(GET_COLOR(piece))][GET_TYPE(piece)]
                               [rank * 8 + file];

            int sign = 1;
            if (GET_COLOR(piece) == PIECE_BLACK)
                sign = -1;

            mat += sign * piece_values[GET_TYPE(piece)];
        }
    }

    return mat + pos;
}