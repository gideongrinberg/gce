#include "core.h"

#include <stdio.h>
#include <time.h>

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_BOLD "\x1b[1m"
#define COLOR_UNDERLINE "\x1b[4m"
#define SC(s) (s ? COLOR_GREEN : COLOR_RED)

double get_time(clockid_t clk_id) {
    struct timespec ts;
    clock_gettime(clk_id, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int perft(Board *board, int depth) {
    if (depth == 0) {
        return 1;
    }

    int total_nodes = 0;
    uint32_t moves[256];
    int num_moves = get_legal_moves(board, moves);
    for (int i = 0; i < num_moves; i++) {
        Board *board_copy = copy_board(board);
        execute_move(board_copy, moves[i]);
        total_nodes += perft(board_copy, depth - 1);
    }

    return total_nodes;
}

void perft_divide(Board *board, int depth) {
    uint32_t moves[256];
    int num_moves = get_legal_moves(board, moves);
    for (int i = 0; i < num_moves; i++) {
        Board *board_copy = copy_board(board);
        execute_move(board_copy, moves[i]);
        int nodes = perft(board_copy, depth - 1);
        printf("\t%s: %d\n", move_to_string(moves[i]), nodes);
    }
}

void run_test(const char *name, const char *fen, int depth, int expected[]) {
    bool success = true;
    printf(COLOR_BOLD COLOR_UNDERLINE "Running test %s\n" COLOR_RESET, name);
    double total_wall = get_time(CLOCK_MONOTONIC);
    Board *board = board_from_fen(fen);
    for (int i = 0; i <= depth; i++) {
        double wall_start = get_time(CLOCK_MONOTONIC);
        double thread_start = get_time(CLOCK_PROCESS_CPUTIME_ID);
        int result = perft(board, i);
        double wall_end = get_time(CLOCK_MONOTONIC);
        double thread_end = get_time(CLOCK_PROCESS_CPUTIME_ID);

        const bool ts = result == expected[i];
        char *nodes = result > 1 ? "nodes" : "node";
        printf("%sDepth %d: found %d %s, expected %d (wall: %fs, cpu: "
               "%fs)%s\n",
               SC(ts), i, result, nodes, expected[i], (wall_end - wall_start),
               (thread_end - thread_start), COLOR_RESET);
        if (!ts)
            perft_divide(board, i);

        success &= ts;
    }

    total_wall = get_time(CLOCK_MONOTONIC) - total_wall;
    char *final_status = success ? "succeeded" : "failed";
    printf("\n%s%sTest %s %s in %f seconds%s\n", COLOR_BOLD, SC(success), name,
           final_status, total_wall, COLOR_RESET);
}

int main(const int argc, const char **argv) {
    char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int expected[] = {1, 20, 400, 8902, 197281, 4865609};
    run_test("starting position", fen, 5, expected);
}