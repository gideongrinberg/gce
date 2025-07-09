#include "cJSON.h"
#include "core.h"
#include "perft_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_BOLD "\x1b[1m"
#define COLOR_UNDERLINE "\x1b[4m"
#define SC(s) (s ? COLOR_GREEN : COLOR_RED)

#define MAX_DEPTH 10
#define MAX_TESTS 16

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
        free(board_copy);
    }

    return total_nodes;
}

int perft_divide(Board *board, int depth) {
    int total = 0;
    uint32_t moves[256];
    int num_moves = get_legal_moves(board, moves);
    for (int i = 0; i < num_moves; i++) {
        Board *board_copy = copy_board(board);
        execute_move(board_copy, moves[i]);
        int nodes = perft(board_copy, depth - 1);
        total += nodes;
        printf("%s %d\n", move_to_string(moves[i]), nodes);
    }

    return total;
}

bool run_test(const char *name, const char *fen, int depth, int expected[]) {
    bool success = true;
    printf(COLOR_BOLD COLOR_UNDERLINE "Test %s:\n" COLOR_RESET, name);
    printf(COLOR_BOLD "FEN:" COLOR_RESET " %s\n", fen);
    printf(COLOR_BOLD "Max depth: " COLOR_RESET "%d\n", depth);

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

        success &= ts;
        if (!ts)
            break; // can do perft_divide(board, i); for diagnostics.
    }

    total_wall = get_time(CLOCK_MONOTONIC) - total_wall;
    char *final_status = success ? "succeeded" : "failed";
    printf("\n%s%sTest %s %s in %f seconds%s\n", COLOR_BOLD, SC(success), name,
           final_status, total_wall, COLOR_RESET);

    free(board);
    return success;
}

void run_perftree(const int argc, const char **argv) {
    int depth = atoi(argv[1]);
    const char *fen = argv[2];
    char *moves = argv[3];

    Board *board = board_from_fen(fen);
    char *move_str = strtok(moves, " ");
    while (move_str != NULL) {
        const int from_file = move_str[0] - 'a', from_rank = move_str[1] - '1',
                  to_file = move_str[2] - 'a', to_rank = move_str[3] - '1';
        const uint8_t from = BOARD_INDEX(from_rank, from_file),
                      to = BOARD_INDEX(to_rank, to_file);

        int promo = PROMO_NONE;
        if (move_str[4] != '\0') {
            switch (move_str[4]) {
            case 'n':
                promo = PROMO_N;
                break;
            case 'r':
                promo = PROMO_R;
                break;
            case 'b':
                promo = PROMO_B;
                break;
            case 'q':
                promo = PROMO_Q;
                break;
            default:
                break;
            }
        }
        uint32_t move = ENCODE_MOVE(from, to, promo);
        execute_move(board, move);
        move_str = strtok(NULL, " ");
    }

    int total = perft_divide(board, depth);
    printf("\n%d\n", total);
}

typedef struct {
    char name[64];
    char fen[128];
    int depth;
    int expected[MAX_DEPTH];
} Test;

int load_tests(const char *json, Test *tests, int max_tests) {
    cJSON *root = cJSON_Parse(json);
    if (!root || !cJSON_IsArray(root)) {
        fprintf(stderr, "Failed to parse JSON\n");
        return -1;
    }

    int count = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        if (count >= max_tests)
            break;

        Test *test = &tests[count];

        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *fen = cJSON_GetObjectItem(item, "fen");
        cJSON *depth = cJSON_GetObjectItem(item, "depth");
        cJSON *expected = cJSON_GetObjectItem(item, "expected");

        if (!cJSON_IsString(name) || !cJSON_IsString(fen) ||
            !cJSON_IsNumber(depth) || !cJSON_IsArray(expected)) {
            fprintf(stderr, "Invalid format in test #%d\n", count);
            continue;
        }

        strncpy(test->name, name->valuestring, sizeof(test->name) - 1);
        strncpy(test->fen, fen->valuestring, sizeof(test->fen) - 1);
        test->depth = depth->valueint;

        int i = 0;
        cJSON *value;
        cJSON_ArrayForEach(value, expected) {
            if (!cJSON_IsNumber(value))
                break;
            if (i < MAX_DEPTH) {
                test->expected[i++] = value->valueint;
            }
        }

        count++;
    }

    cJSON_Delete(root);
    return count;
}

int main(const int argc, const char **argv) {
    bool success = true;
    Test tests[MAX_TESTS];
    int num_tests = load_tests((const char *)test_cases_json, tests, MAX_TESTS);
    for (int i = 0; i < num_tests; i++) {
        success &= run_test(tests[i].name, tests[i].fen, tests[i].depth,
                            tests[i].expected);
        printf("\n==============================\n\n");
    }

    if (!success)
        return -1;
    return 0;
}