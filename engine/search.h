#ifndef SEARCH_H
#define SEARCH_H
#include "position.h"
#include <stdbool.h>

bool init_tt();
void free_tt();

Move get_best_move(Position *pos, int depth);

/**
 * Search for the best move using iterative deepening within the provided
 * restrictions. Each parameter corresponds to a UCI-standard argument. Provide
 * pointers to return variables for each parameter prefixed with result_.
 *
 * time_available: wtime/btime for the moving side
 *
 * increment: winc/binc for the moving side
 *
 * moves_to_go: moves to go until the next time control, corresponds to UCI
 * movestogo.
 *
 * depth: searches x number of plies, corresponds to UCI depth.
 *
 * move_time: search for exactly x ms, corresponds to UCI movetime.
 *
 * Exclude a parameter using the value -1.
 * todo: support ponder, stop
 */
void get_best_move_ex(Position *pos, float time_available, float increment,
                      int moves_to_go, int depth, float move_time,
                      Move *result_move, int *result_depth, int *result_eval);
#endif // SEARCH_H
