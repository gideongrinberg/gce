#include "search.h"
#include "eval.h"
#include <stdbool.h>
#include <stddef.h>

int search(Position *pos, int depth, int alpha, int beta, bool maximizing,
           bool moving, Move *best_move) {
    if (depth == 0) {
        return eval_position(pos);
    }

    Move moves[256];
    int num_moves = generate_moves(pos, moves);
    if (maximizing) {
        int value = -INF;
        for (int i = 0; i < num_moves; i++) {
            Position copy = *pos;
            execute_move(&copy, moves[i]);

            int new_value =
                search(&copy, depth - 1, alpha, beta, false, false, NULL);

            if (new_value > value) {
                value = new_value;
                if (moving) {
                    *best_move = moves[i];
                }
            }

            if (new_value > alpha)
                alpha = new_value;
            if (alpha >= beta)
                break; // Beta cut-off
        }
        return value;
    } else {
        int value = INF;
        for (int i = 0; i < num_moves; i++) {
            Position copy = *pos;
            execute_move(&copy, moves[i]);

            int new_value =
                search(&copy, depth - 1, alpha, beta, true, false, NULL);

            if (new_value < value) {
                value = new_value;
                if (moving) {
                    *best_move = moves[i];
                }
            }

            if (new_value < beta)
                beta = new_value;
            if (beta <= alpha)
                break;
        }
        return value;
    }
}
Move get_best_move(Position *pos, int depth) {
    Move best_move = 0;
    search(pos, depth, -INF, INF, pos->moves % 2 == 0, true, &best_move);
    return best_move;
}