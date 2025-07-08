#include "core.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void execute_move(Board *board, uint32_t move) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int promo = MOVE_PROMO(move);

    uint8_t piece = board->board[from];
    if (promo != PROMO_NONE) {
        uint8_t piece_type;
        switch (promo) {
        case PROMO_N:
            piece_type = KNIGHT;
            break;
        case PROMO_B:
            piece_type = BISHOP;
            break;
        case PROMO_R:
            piece_type = ROOK;
            break;
        case PROMO_Q:
            piece_type = QUEEN;
            break;
        }

        piece = piece_type | GET_COLOR(piece);
    }

    board->board[from] = EMPTY;
    board->board[to] = piece;
    board->moves++;
}

Board *board_from_fen(const char *fen_string) {
    Board *board = malloc(sizeof(Board));
    if (!board) {
        fprintf(stderr, "Failed to allocate memory for board from fen: %s\n",
                fen_string);
        return NULL;
    }

    memset(board->board, 0, sizeof(board->board));
    char buffer[256];
    strncpy(buffer, fen_string, 256);
    buffer[sizeof(buffer) - 1] = '\0';
    char *fen = strtok(buffer, " ");
    int rank = 7, file = 0;
    for (int i = 0; fen[i] != '\0'; i++) {
        if (isdigit(fen[i])) {
            int digit = fen[i] - '0';
            file += digit;
        } else if (isalpha(fen[i])) {
            char ch = fen[i];
            uint8_t color = islower(ch) ? PIECE_BLACK : PIECE_WHITE;
            if (color == PIECE_BLACK) {
                ch -= 'a' - 'A';
            }

            uint8_t piece;
            switch (ch) {
            case 'P':
                piece = PAWN;
                break;
            case 'N':
                piece = KNIGHT;
                break;
            case 'B':
                piece = BISHOP;
                break;
            case 'R':
                piece = ROOK;
                break;
            case 'Q':
                piece = QUEEN;
                break;
            case 'K':
                piece = KING;
                break;
            default: // this should never happen but clang-tidy complains
                piece = EMPTY;
            }

            board->board[BOARD_INDEX(rank, file)] = color | piece;
            file++;
        } else if (fen[i] == '/') {
            rank--;
            file = 0;
        }
    }

    fen = strtok(NULL, " ");
    if (fen[0] == 'w') {
        board->moves = 0;
    } else {
        board->moves = 1;
    }

    fen = strtok(NULL, " ");
    uint8_t castling_rights = 0;
    for (int i = 0; fen[i] != '\0'; i++) {
        switch (fen[i]) {
        case 'K':
            castling_rights |= CASTLE_WHITE_KING;
            break;
        case 'Q':
            castling_rights |= CASTLE_WHITE_QUEEN;
            break;
        case 'k':
            castling_rights |= CASTLE_BLACK_KING;
            break;
        case 'q':
            castling_rights |= CASTLE_BLACK_QUEEN;
            break;
        case '-':
            break;
        }
    }

    board->castling_rights = castling_rights;
    return board;
}
