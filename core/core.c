#include "core.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void execute_move(Board *board, uint32_t move) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int promo = MOVE_PROMO(move);

    uint8_t piece = board->board[from];
    if (GET_TYPE(piece) == PAWN || GET_TYPE(board->board[to]) != EMPTY) {
        board->halfmoves = 0;
    } else {
        board->halfmoves++;
    }
    if (GET_TYPE(piece) == KING) {
        if (castle(board, move, true)) {
            board->en_passant = -1;
            board->moves++;
            return;
        }

        if (GET_COLOR(piece) == PIECE_BLACK)
            board->castling_rights &= ~(CASTLE_BLACK_KING | CASTLE_BLACK_QUEEN);
        else
            board->castling_rights &= ~(CASTLE_WHITE_KING | CASTLE_WHITE_QUEEN);
    } else if (GET_TYPE(piece) == ROOK) {
        switch (MOVE_FROM(move)) {
        case 0x07:
            board->castling_rights &= ~CASTLE_WHITE_KING;
            break;
        case 0x00:
            board->castling_rights &= ~CASTLE_WHITE_QUEEN;
            break;
        case 0x77:
            board->castling_rights &= ~CASTLE_BLACK_KING;
            break;
        case 0x70:
            board->castling_rights &= ~CASTLE_BLACK_QUEEN;
            break;
        }
    }

    uint8_t piece_type = GET_TYPE(piece);
    if (promo != PROMO_NONE) {
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
    int sign = (board->moves % 2) == 0 ? 1 : -1;
    // check for en-passant capture
    if (to == board->en_passant && piece_type == PAWN) {
        board->board[to - (sign * 16)] = EMPTY;
    }

    // update en-passant square
    board->moves++;
    if (piece_type == PAWN && (to - from) == (sign * 32)) {
        board->en_passant = from + sign * 16;
    } else {
        board->en_passant = -1;
    }
}

bool castle(Board *board, uint32_t move, bool make_move) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int rook_from, rook_to, color;
    if (from == 0x04 && to == 0x06 &&
        (board->castling_rights & CASTLE_WHITE_KING)) {
        rook_from = 0x07;
        rook_to = 0x05;
        color = PIECE_WHITE;
    } else if (from == 0x04 && to == 0x02 &&
               (board->castling_rights & CASTLE_WHITE_QUEEN)) {
        rook_from = 0x00;
        rook_to = 0x03;
        color = PIECE_WHITE;
    } else if (from == 0x74 && to == 0x76 &&
               (board->castling_rights & CASTLE_BLACK_KING)) {
        rook_from = 0x77;
        rook_to = 0x75;
        color = PIECE_BLACK;
    } else if (from == 0x74 && to == 0x72 &&
               (board->castling_rights & CASTLE_BLACK_QUEEN)) {
        rook_from = 0x70;
        rook_to = 0x73;
        color = PIECE_BLACK;
    } else {
        return false;
    }

    // King safety check
    uint8_t attacker_color = color == PIECE_WHITE ? PIECE_BLACK : PIECE_WHITE;
    uint8_t transit_square = from + ((to - from) / 2);
    if (is_square_attacked(board, transit_square, attacker_color) ||
        is_square_attacked(board, from, attacker_color) ||
        is_square_attacked(board, to, attacker_color)) {
        return false;
    }

    // check that the piece in the rook spot is actually a rook
    if (board->board[rook_from] != (color | ROOK)) {
        return false;
    }

    // Check no pieces between king and rook.
    int rank = SQ_RANK(from);
    int file1 = SQ_FILE(from);
    int file2 = SQ_FILE(rook_from);
    int min_file = (file1 < file2) ? file1 : file2;
    int max_file = (file1 > file2) ? file1 : file2;

    for (int f = min_file + 1; f < max_file; f++) {
        int idx = BOARD_INDEX(rank, f);
        if (board->board[idx] != EMPTY)
            return false;
    }

    if (make_move) {
        board->board[rook_to] = board->board[rook_from];
        board->board[rook_from] = EMPTY;
        board->board[to] = board->board[from];
        board->board[from] = EMPTY;
        switch (color) {
        case PIECE_WHITE:
            board->castling_rights &= ~(CASTLE_WHITE_KING | CASTLE_WHITE_QUEEN);
            break;
        case PIECE_BLACK:
            board->castling_rights &= ~(CASTLE_BLACK_KING | CASTLE_BLACK_QUEEN);
        };
    }
    return true;
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

    // store side to move
    char *stm = strtok(NULL, " ");

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
    fen = strtok(NULL, " ");
    if (fen[0] == '-') {
        board->en_passant = -1;
    } else {
        int ep_rank = fen[0] - 'a';
        int ep_file = fen[1] - '1';
        board->en_passant = BOARD_INDEX(ep_rank, ep_file);
    }

    char *halfmoves = strtok(NULL, " ");
    if (halfmoves == NULL) {
        fprintf(stderr,
                "FEN does not include half move counter, initializing to 0\n");
        return board;
    }

    board->halfmoves = atoi(halfmoves);
    char *moves = strtok(NULL, " ");
    if (moves == NULL) {
        fprintf(stderr, "FEN does not include half moves, initializing to 0\n");
        return board;
    }

    if (strcmp(stm, "w") == 0) {
        board->moves = (atoi(moves) - 1) * 2;
    } else {
        board->moves = (atoi(moves) - 1) * 2 + 1;
    }
    return board;
}

Board *board_from_startpos() {
    return board_from_fen(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

char *board_to_fen(Board *board) {
    char *fen = malloc(128);
    fen[0] = '\0';
    int empty = 0;
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            uint8_t piece = board->board[BOARD_INDEX(rank, file)];
            if (GET_TYPE(piece) == EMPTY) {
                empty++;
            } else {
                if (empty > 0) {
                    char tmp[2];
                    sprintf(tmp, "%d", empty);
                    strcat(fen, tmp);
                    empty = 0;
                }

                char ch[2];
                switch (GET_TYPE(piece)) {
                case PAWN:
                    ch[0] = 'P';
                    break;
                case KNIGHT:
                    ch[0] = 'N';
                    break;
                case BISHOP:
                    ch[0] = 'B';
                    break;
                case ROOK:
                    ch[0] = 'R';
                    break;
                case QUEEN:
                    ch[0] = 'Q';
                    break;
                case KING:
                    ch[0] = 'K';
                    break;
                }

                ch[1] = '\0';
                if (GET_COLOR(piece) == PIECE_BLACK)
                    ch[0] = tolower(ch[0]);

                strcat(fen, ch);
            }
        }

        if (empty > 0) {
            char tmp[2];
            sprintf(tmp, "%d", empty);
            strcat(fen, tmp);
            empty = 0;
        }

        if (rank != 0)
            strcat(fen, "/");
    }

    sprintf(fen + strlen(fen), " %c ", board->moves % 2 == 0 ? 'w' : 'b');

    char castling[6] = "";
    if (board->castling_rights == 0) {
        strcpy(castling, "-");
    } else {
        if (board->castling_rights & CASTLE_WHITE_KING)
            strcat(castling, "K");
        if (board->castling_rights & CASTLE_WHITE_QUEEN)
            strcat(castling, "Q");
        if (board->castling_rights & CASTLE_BLACK_KING)
            strcat(castling, "k");
        if (board->castling_rights & CASTLE_BLACK_QUEEN)
            strcat(castling, "q");
    }

    strcat(castling, " ");
    strcat(fen, castling);
    if (board->en_passant != -1) {
        char *ep_square = malloc(3);
        sprintf(ep_square, "%c%d", 'a' + SQ_FILE(board->en_passant),
                SQ_RANK(board->en_passant));

        strcat(fen, ep_square);
        free(ep_square);
    } else {
        strcat(fen, "-");
    }

    char move_counts[16];
    sprintf(move_counts, " %d %d", board->halfmoves, (board->moves / 2) + 1);
    strcat(fen, move_counts);

    return fen;
}

Board *copy_board(const Board *original) {
    Board *copy = malloc(sizeof(Board));
    memcpy(copy, original, sizeof(Board));
    return copy;
}

bool is_checkmate(Board *board) {
    int player_color = (board->moves % 2) == 0 ? PIECE_WHITE : PIECE_BLACK;
    int attacker_color = (board->moves % 2) == 0 ? PIECE_BLACK : PIECE_WHITE;
    int king_sq = -1;
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            if (board->board[BOARD_INDEX(rank, file)] ==
                (player_color | KING)) {
                king_sq = BOARD_INDEX(rank, file);
                break;
            }
        }
    }

    if (king_sq == -1)
        return false;
    if (!is_square_attacked(board, king_sq, attacker_color)) {
        return false;
    }

    uint32_t moves[256];
    int num_moves = get_legal_moves(board, moves);
    for (int move = 0; move < num_moves; move++) {
        Board *board_copy = copy_board(board);
        execute_move(board_copy, move);
        if (MOVE_FROM(move) == king_sq &&
            !is_square_attacked(board_copy, MOVE_TO(move), attacker_color)) {
            free(board_copy);
            return false;
        }

        if (!is_square_attacked(board_copy, MOVE_FROM(move), attacker_color)) {
            free(board_copy);
            return false;
        }

        free(board_copy);
    }

    return true;
}

Outcome board_outcome(Board *board) {
    if (board->halfmoves >= 50)
        return Draw;
    if (is_checkmate(board))
        return Checkmate;

    uint32_t moves[256];
    if (get_legal_moves(board, moves) == 0)
        return Draw;

    return Ongoing;
}

char *move_to_string(uint32_t move) {
    int from = MOVE_FROM(move), to = MOVE_TO(move);
    int from_file = SQ_FILE(from), from_rank = SQ_RANK(from),
        to_file = SQ_FILE(to), to_rank = SQ_RANK(to);
    char from_char = 'a' + from_file, to_char = 'a' + to_file;

    char *ret = malloc(6);
    sprintf(ret, "%c%d%c%d", from_char, from_rank + 1, to_char, to_rank + 1);
    if (MOVE_PROMO(move) != PROMO_NONE) {
        const char *promo = "";
        switch (MOVE_PROMO(move)) {
        case PROMO_B:
            promo = "b";
            break;
        case PROMO_N:
            promo = "n";
            break;
        case PROMO_R:
            promo = "r";
            break;
        case PROMO_Q:
            promo = "q";
            break;
        default:
            break;
        }

        strcat(ret, promo);
    }

    return ret;
}

uint32_t move_from_string(const char *move_str) {
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

    return ENCODE_MOVE(from, to, promo);
}
