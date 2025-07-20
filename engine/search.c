#include "search.h"
#include "eval.h"
#include <stdbool.h>

int minimax(Position *pos, int depth, bool maximizing, bool moving,
            Move *best_move) {
    if (depth == 0)
        return eval_position(pos);

    int best_score = (maximizing ? -1 : 1) * 1000000000;
    Move moves[256];
    int num_moves = generate_moves(pos, moves);
    for (int i = 0; i < num_moves; i++) {
        Position copy = *pos;
        execute_move(&copy, moves[i]);
        int score = minimax(&copy, depth - 1, !maximizing, false, best_move);
        if ((maximizing && score > best_score) ||
            (!maximizing && score < best_score)) {
            best_score = score;
            if (moving) {
                *best_move = moves[i];
            }
        }
    }

    return best_score;
}

Move get_best_move(Position *pos, int depth) {
    Move best_move;
    minimax(pos, depth, pos->moves % 2 == 0, true, &best_move);
    return best_move;
}