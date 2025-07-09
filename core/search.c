#include "core.h"

#include <stdlib.h>
#include <time.h>

uint32_t best_move(Board *board) {
    uint32_t moves[256];
    int num_moves = get_legal_moves(board, moves);
    if (num_moves == 1) {
        return moves[0];
    }

    int r = rand() % (num_moves - 1);
    return moves[r];
}