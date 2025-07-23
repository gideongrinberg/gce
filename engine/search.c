#include "search.h"
#include "eval.h"
#include "zobrist.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

typedef enum { UPPER, LOWER, EXACT } BoundType;

typedef struct {
    uint64_t hash;
    int depth;
    int eval;
    Move best_move;
    BoundType bound_type;
} TableEntry;

const int TT_SIZE = 1 << 24; // 2^24 entries * 16 bytes = 256 MiB
const int TT_MASK = TT_SIZE - 1;

TableEntry *transposition_table = NULL;

bool init_tt() {
    if (transposition_table != NULL) {
        return true;
    }

    transposition_table = calloc(TT_SIZE, sizeof(TableEntry));
    if (!transposition_table) {
        return false;
    }

    return true;
}

void free_tt() {
    if (transposition_table != NULL) {
        free(transposition_table);
        transposition_table = NULL;
    }
}

int search(Position *pos, int depth, int alpha, int beta, bool maximizing,
           bool moving, Move *best_move) {
    if (depth == 0) {
        return eval_position(pos);
    }

    uint64_t hash = position_zobrist(pos);
    TableEntry *entry = &transposition_table[hash & TT_MASK];
    if (entry->hash == hash && entry->depth >= depth) {
        bool hit = entry->bound_type == EXACT ||
                   (entry->bound_type == LOWER && entry->eval >= beta) ||
                   (entry->bound_type == UPPER && entry->eval <= alpha);

        if (hit) {
            if (moving)
                *best_move = entry->best_move;
            return entry->eval;
        }
    }

    Move moves[256];
    int num_moves = generate_moves(pos, moves);
    int value;
    Move best_move_buf = moves[0];

    if (maximizing) {
        value = -INF;
        for (int i = 0; i < num_moves; i++) {
            Position copy = *pos;
            execute_move(&copy, moves[i]);

            int new_value =
                search(&copy, depth - 1, alpha, beta, false, false, NULL);

            if (new_value > value) {
                value = new_value;
                best_move_buf = moves[i];
            }

            if (new_value > alpha)
                alpha = new_value;
            if (alpha >= beta)
                break; // Beta cut-off
        }
    } else {
        value = INF;
        for (int i = 0; i < num_moves; i++) {
            Position copy = *pos;
            execute_move(&copy, moves[i]);

            int new_value =
                search(&copy, depth - 1, alpha, beta, true, false, NULL);

            if (new_value < value) {
                value = new_value;
                best_move_buf = moves[i];
            }

            if (new_value < beta)
                beta = new_value;
            if (beta <= alpha)
                break;
        }
    }

    if (moving) {
        *best_move = best_move_buf;
    }

    entry->hash = hash;
    entry->depth = depth;
    entry->eval = value;
    entry->best_move = best_move_buf;

    if (value <= alpha) {
        entry->bound_type = UPPER;
    } else if (value >= beta) {
        entry->bound_type = LOWER;
    } else {
        entry->bound_type = EXACT;
    }

    return value;
}

Move get_best_move(Position *pos, int depth) {
    Move best_move = 0;
    search(pos, depth, -INF, INF, pos->moves % 2 == 0, true, &best_move);
    return best_move;
}

double now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

void get_best_move_ex(Position *pos, float time_available, float increment,
                      int moves_to_go, int depth, float move_time,
                      Move *result_move, int *result_depth, int *result_eval) {
    bool maximizing = pos->moves % 2 == 0;

    if (depth != -1) {
        *result_eval =
            search(pos, depth, -INF, INF, maximizing, true, result_move);
        *result_depth = depth;
        return;
    }

    double target;
    if (move_time != -1) {
        target = move_time;
    } else if (moves_to_go > 0) {
        target = (time_available / moves_to_go) + (increment / 2);
    } else {
        target = (time_available / 20) + (increment / 2);
    }

    target /= 1000; // convert to seconds

    const double start = now();
    int curr_depth = 1;
    int curr_eval = 0;
    Move best_move;
    do {
        // todo: break on mate or 1 legal move
        curr_eval =
            search(pos, curr_depth, -INF, INF, maximizing, true, &best_move);
        curr_depth++;
    } while (now() - start < target);

    *result_depth = curr_depth - 1;
    *result_eval = curr_eval;
    *result_move = best_move;
}