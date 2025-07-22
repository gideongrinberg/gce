#include "eval.h"

const int piece_values[6] = {
    [PIECE_PAWN] = 100, [PIECE_KNIGHT] = 280, [PIECE_BISHOP] = 320,
    [PIECE_ROOK] = 479, [PIECE_QUEEN] = 929,  [PIECE_KING] = 60000};

const int piece_tables[6][64] = {
    // pawn
    {0,   0,  0,  0,   0,   0,  0,  0,   -31, 3,  -14, -36, -37, -7, 8,  -31,
     -19, 3,  -2, -10, -11, 5,  9,  -22, -23, 0,  1,   6,   9,   10, 3,  -26,
     -13, 15, 0,  14,  15,  -2, 16, -17, 7,   44, 31,  40,  44,  21, 29, 7,
     90,  85, 82, 102, 73,  86, 83, 78,  0,   0,  0,   0,   0,   0,  0,  0},

    // knight
    {-69, -22, -35, -19, -24, -26, -23, -74, -20, -23, 0,   2,  0,
     2,   -15, -23, -14, 11,  15,  18,  22,  13,  10,  -18, 0,  2,
     35,  22,  21,  31,  5,   -1,  17,  25,  41,  33,  37,  45, 24,
     24,  -2,  62,  27,  73,  74,  1,   67,  10,  -14, -4,  62, 4,
     -36, 100, -6,  -3,  -70, -58, -55, -10, -75, -75, -53, -66},

    // bishop
    {-10, -10, -15, -14, -12, -15, 2,    -7,  16,  20,  6,   7,  6,
     11,  20,  19,  15,  20,  25,  8,    15,  24,  25,  14,  7,  0,
     16,  17,  23,  17,  10,  13,  10,   15,  25,  26,  34,  20, 17,
     25,  -14, 28,  -10, 52,  41,  -32,  39,  -9,  -22, 2,   31, -39,
     -42, 35,  20,  -11, -50, -37, -107, -23, -76, -82, -78, -59},

    // rook
    {-32, -31, -18, -2,  5,   -18, -24, -30, -53, -44, -43, -29, -26,
     -31, -38, -53, -46, -26, -35, -25, -25, -42, -28, -42, -30, -46,
     -29, -13, -21, -16, -35, -28, -6,  -9,  -4,  18,  13,  16,  5,
     0,   15,  25,  27,  45,  33,  28,  35,  19,  60,  34,  62,  55,
     67,  56,  29,  55,  50,  56,  33,  37,  4,   33,  29,  35},

    // queen
    {-42, -34, -36, -31, -13, -31, -30, -39, -38,  -21, -15, -15, -19,
     0,   -18, -36, -27, -16, -11, -16, -11, -13,  -6,  -30, -22, -20,
     -10, -1,  -5,  -2,  -15, -14, -6,  -13, 20,   25,  17,  22,  -16,
     1,   2,   43,  63,  72,  60,  32,  43,  -2,   24,  57,  76,  20,
     -10, 60,  32,  14,  26,  88,  24,  69,  -104, -8,  1,   6},

    // king
    {18,  40,  -1,  6,   -14, -3,  30,  17,  4,   13,  -18, -57, -50,
     -14, 3,   -4,  -32, -29, -32, -64, -79, -43, -42, -47, -50, -8,
     -47, -51, -28, -52, -43, -55, -49, 0,   13,  -19, -4,  11,  50,
     -55, -31, 37,  28,  -67, 44,  -57, 12,  -62, 3,   10,  55,  56,
     56,  55,  10,  -32, -62, 83,  60,  -99, -99, 47,  54,  4}};

int eval_position(Position *p) {
    int eval = 0;

    int side_to_move = p->moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
    if (generate_attacks(p, side_to_move ^ 8) &
        p->bitboards[PIECE_KING | side_to_move]) {
        Move moves[256];
        if (generate_moves(p, moves) == 0) {
            if (side_to_move == PIECE_WHITE) {
                return -INF + 1;
            } else {
                return INF - 1;
            }
        }
    }

    const int colors[2] = {PIECE_WHITE, PIECE_BLACK};
    for (int piece = PIECE_PAWN; piece <= PIECE_KING; piece++) {
        eval += piece_values[piece] *
                __builtin_popcountll(p->bitboards[piece | PIECE_WHITE]);

        eval += -1 * piece_values[piece] *
                __builtin_popcountll(p->bitboards[piece | PIECE_BLACK]);

        for (int c = 0; c < 2; c++) {
            int color = colors[c];
            int sign = color == PIECE_WHITE ? 1 : -1;
            FOREACH_SET_BIT(p->bitboards[color | piece], sq) {
                int value = color == PIECE_WHITE ? piece_tables[piece][sq]
                                                 : piece_tables[piece][sq ^ 56];
                eval += value * sign;
            }
        }
    }

    return eval;
}