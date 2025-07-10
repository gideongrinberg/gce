#include "core.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

FILE *log_file = NULL;
#define LOG(fmt, ...)                                                          \
    do {                                                                       \
        time_t now = time(NULL);                                               \
        struct tm *t = localtime(&now);                                        \
        char buf[32];                                                          \
        strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", t);                  \
        fprintf(log_file, "%s ", buf);                                         \
        fprintf(log_file, fmt, ##__VA_ARGS__);                                 \
        fprintf(log_file, "\n");                                               \
        fflush(log_file);                                                      \
    } while (0)

#define SEND_MESSAGE(msg)                                                      \
    do {                                                                       \
        LOG("Sent reply: %s", msg);                                            \
        puts(msg);                                                             \
        fflush(stdout);                                                        \
    } while (0)

int main(void) {
    srand(time(NULL));
    const char *home = getenv("HOME");
    const char filename[1024];
    snprintf(filename, sizeof(filename), "%s/gce-uci.log", home);
    log_file = fopen(filename, "w");
    if (!log_file) {
        perror("Could not open log file");
        return 1;
    }

    LOG("Started session");
    Board *board = NULL;
    char *buffer = NULL;
    size_t size = 0;
    ssize_t len;

    while ((len = getline(&buffer, &size, stdin)) != -1) {
        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        LOG("Got command '%s'", buffer);
        if (strcmp(buffer, "quit") == 0) {
            LOG("Goodbye.");
            break;
        }

        if (strcmp(buffer, "uci") == 0) {
            SEND_MESSAGE("id name Gideon's Chess Engine");
            SEND_MESSAGE("id author Gideon Grinberg");
            SEND_MESSAGE("uciok");
            continue;
        }

        if (strcmp(buffer, "isready") == 0) {
            SEND_MESSAGE("readyok");
            continue;
        }

        if (strncmp(buffer, "go", 2) == 0) {
            char output[64];
            uint32_t move = best_move(board);
            if (move == 0)
                LOG("No move found");
            else
                LOG(move_to_string(move));

            Board *board_copy = copy_board(board);
            execute_move(board_copy, move);
            sprintf(output, "info depth 10 score cp %d",
                    eval_board(board_copy));
            SEND_MESSAGE(output);
            sprintf(output, "bestmove %s", move_to_string(move));
            SEND_MESSAGE(output);
        }

        if (strcmp(buffer, "fen") == 0) {
            if (board != NULL) {
                LOG("Board is not null");
                char *fen = board_to_fen(board);
                SEND_MESSAGE(fen);
                free(fen);
            } else {
                LOG("Board is null");
                SEND_MESSAGE("Board not initialized.");
            }
            continue;
        }

        if (strncmp(buffer, "position", 8) == 0 &&
            (buffer[8] == ' ' || buffer[8] == '\0')) {
            LOG("Executing position command.");
            char *copy = strdup(buffer);
            char *saveptr = NULL;
            char *token = strtok_r(copy, " ", &saveptr); // "position"
            token = strtok_r(NULL, " ", &saveptr); // next: "startpos" or "fen"

            if (!token) {
                LOG("No position type given.");
                free(copy);
                continue;
            }

            if (strcmp(token, "startpos") == 0) {
                LOG("Initializing with startpos.");
                if (board)
                    free(board);
                board = board_from_startpos();
            } else if (strcmp(token, "fen") == 0) {
                LOG("Initializing with fen.");
                char fen_buf[256] = {0};
                char *fen_parts[6] = {0};

                for (int i = 0; i < 6; i++) {
                    fen_parts[i] = strtok_r(NULL, " ", &saveptr);
                    if (!fen_parts[i])
                        break;
                    strcat(fen_buf, fen_parts[i]);
                    if (i < 5)
                        strcat(fen_buf, " ");
                }

                if (fen_parts[5]) {
                    if (board)
                        free(board);
                    board = board_from_fen(fen_buf);
                } else {
                    LOG("Incomplete FEN string.");
                }
            } else {
                LOG("Unknown position subcommand.");
                free(copy);
                continue;
            }

            // look for "moves"
            token = strtok_r(NULL, " ", &saveptr);
            if (token && strcmp(token, "moves") == 0) {
                LOG("Executing moves.");
                char *move;
                while ((move = strtok_r(NULL, " ", &saveptr)) != NULL) {
                    LOG(move);
                    if (board)
                        execute_move(board, move_from_string(move));
                }
            }

            free(copy);
        }
    }

    free(buffer);
    if (board)
        free(board);
    fclose(log);
    return 0;
}
