#include "position.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int perft(Position *p, int depth) {
    if (depth == 0)
        return 1;

    int total = 0;
    Move moves[256];
    int count = generate_moves(p, moves);

    for (int i = 0; i < count; i++) {
        Position copy = *p;
        execute_move(&copy, moves[i]);
        total += perft(&copy, depth - 1);
    }

    return total;
}

int perftDivide(Position *p, int depth) {
    int total = 0;
    Move moves[256];
    int count = generate_moves(p, moves);

    for (int i = 0; i < count; i++) {
        Position copy = *p;
        execute_move(&copy, moves[i]);
        int nodes = perft(&copy, depth - 1);

        int from = MOVE_FROM(moves[i]);
        int to = MOVE_TO(moves[i]);

        char moveStr[6] = {'a' + (from % 8), '1' + (from / 8), 'a' + (to % 8),
                           '1' + (to / 8), '\0'};

        int promo = MOVE_PROMO(moves[i]);
        if (promo != 0) {
            switch (promo) {
            case PIECE_QUEEN:
                moveStr[4] = 'q';
                break;
            case PIECE_ROOK:
                moveStr[4] = 'r';
                break;
            case PIECE_BISHOP:
                moveStr[4] = 'b';
                break;
            case PIECE_KNIGHT:
                moveStr[4] = 'n';
                break;
            default:
                break;
            }
            moveStr[5] = '\0';
        }

        printf("%s %d\n", moveStr, nodes);
        total += nodes;
    }

    return total;
}

void runPerftree(int argc, char **argv) {
    int depth = atoi(argv[1]);
    const char *fen = argv[2];
    char *movesArg = argv[3];

    Position *p = position_from_fen(fen);

    if (strlen(movesArg) > 0) {
        char buffer[1024];
        strncpy(buffer, movesArg, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        char *token = strtok(buffer, " ");
        while (token != NULL) {
            int fromFile = token[0] - 'a';
            int fromRank = token[1] - '1';
            int toFile = token[2] - 'a';
            int toRank = token[3] - '1';

            // int from = (7 - fromRank) * 8 + fromFile;
            // int to = (7 - toRank) * 8 + toFile;
            int from = (fromRank) * 8 + fromFile;
            int to = (toRank) * 8 + toFile;

            int promo = 0;
            if (strlen(token) == 5) {
                switch (token[4]) {
                case 'q':
                    promo = PIECE_QUEEN;
                    break;
                case 'r':
                    promo = PIECE_ROOK;
                    break;
                case 'b':
                    promo = PIECE_BISHOP;
                    break;
                case 'n':
                    promo = PIECE_KNIGHT;
                    break;
                default:
                    break;
                }
            }

            Move move = ENCODE_MOVE(from, to, promo);
            execute_move(p, move);
            token = strtok(NULL, " ");
        }
    }

    int total = perftDivide(p, depth);
    printf("\n%d", total);
    fflush(stdout);
}

int main(int argc, char **argv) {
    if (argc == 4) {
        runPerftree(argc, argv);
    } else if (argc == 3) {
        char *newArgv[4] = {argv[0], argv[1], argv[2], ""};
        runPerftree(argc, newArgv);
    }

    return 0;
}
