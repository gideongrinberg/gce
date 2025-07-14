#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "position.h"
#include "tables.h"

// Masks to isolate a specific rank or file of a bitboard
#define RANK_2 0x000000000000FF00ULL
#define RANK_3 0x0000000000FF0000ULL
#define RANK_4 0x00000000FF000000ULL
#define RANK_5 0x000000FF00000000ULL
#define RANK_6 0x0000FF0000000000ULL
#define RANK_7 0x00FF000000000000ULL

#define FILE_A 0x0101010101010101ULL
#define FILE_H 0x8080808080808080ULL

Position *position_from_fen(const char *fen) {
    Position *p = calloc(1, sizeof(*p));
    int i = 0;
    int rank = 7, file = 0;
    while (fen[i] != ' ') {
        char c = fen[i];
        if (isalpha(c)) {
            bool white = false;
            Piece piece = 0;
            if (isupper(c)) {
                c = tolower(c);
                white = true;
            }

            switch (c) {
            case 'p':
                piece = PIECE_PAWN;
                break;
            case 'n':
                piece = PIECE_KNIGHT;
                break;
            case 'b':
                piece = PIECE_BISHOP;
                break;
            case 'r':
                piece = PIECE_ROOK;
                break;
            case 'q':
                piece = PIECE_QUEEN;
                break;
            case 'k':
                piece = PIECE_KING;
                break;
            }

            p->bitboards[piece | (white ? PIECE_WHITE : PIECE_BLACK)] |=
                (1ULL << (rank * 8 + file));
            file++;
        } else if (isdigit(c)) {
            file += c - '0';
        } else if (c == '/') {
            rank--;
            file = 0;
        }

        i++;
    }

    return p;
}

char piece_char(int piece) {
    switch (piece) {
    case MAKE_PIECE(PIECE_WHITE, PIECE_PAWN):
        return 'P';
    case MAKE_PIECE(PIECE_WHITE, PIECE_KNIGHT):
        return 'N';
    case MAKE_PIECE(PIECE_WHITE, PIECE_BISHOP):
        return 'B';
    case MAKE_PIECE(PIECE_WHITE, PIECE_ROOK):
        return 'R';
    case MAKE_PIECE(PIECE_WHITE, PIECE_QUEEN):
        return 'Q';
    case MAKE_PIECE(PIECE_WHITE, PIECE_KING):
        return 'K';
    case MAKE_PIECE(PIECE_BLACK, PIECE_PAWN):
        return 'p';
    case MAKE_PIECE(PIECE_BLACK, PIECE_KNIGHT):
        return 'n';
    case MAKE_PIECE(PIECE_BLACK, PIECE_BISHOP):
        return 'b';
    case MAKE_PIECE(PIECE_BLACK, PIECE_ROOK):
        return 'r';
    case MAKE_PIECE(PIECE_BLACK, PIECE_QUEEN):
        return 'q';
    case MAKE_PIECE(PIECE_BLACK, PIECE_KING):
        return 'k';
    default:
        return '.';
    }
}

void print_position(Position *p) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            char c = '.';
            for (int i = 0; i < MAX_PIECE; i++) {
                if (p->bitboards[i] & (1ULL << (rank * 8 + file))) {
                    c = piece_char(i);
                    break;
                }
            }

            printf("%c ", c);
        }
        puts("");
    }
}

uint64_t generate_attacks(Position *p, int color) {
    uint64_t attacks = 0;

    int shift_left = (color == PIECE_WHITE) ? 7 : -9;
    int shift_right = (color == PIECE_WHITE) ? 9 : -7;
    uint64_t mask_left = (color == PIECE_WHITE) ? ~FILE_H : ~FILE_A;
    uint64_t mask_right = (color == PIECE_WHITE) ? ~FILE_A : ~FILE_H;
    uint64_t pawns = p->bitboards[color | PIECE_PAWN];

    uint64_t left_captures = shift_left >= 0
                               ? ((pawns & mask_left) << shift_left)
                               : ((pawns & mask_left) >> -shift_left);

    uint64_t right_captures = shift_right >= 0
                                ? ((pawns & mask_right) << shift_right)
                                : ((pawns & mask_right) >> -shift_right);

    attacks |= left_captures | right_captures;

    uint64_t knights = p->bitboards[MAKE_PIECE(color, PIECE_KNIGHT)];
    FOREACH_SET_BIT(knights, from) { attacks |= knight_moves[from]; }

    return attacks;
}

static inline void add_pawn_moves(uint64_t bb, int shift, Move *arr,
                                  int *moves_count) {
    while (bb) {
        int to = __builtin_ctzll(bb);
        int from = to - shift;
        // Check for promotion
        if ((to <= 7) || (to >= 56)) {
            arr[(*moves_count)++] = ENCODE_MOVE(from, to, PIECE_KNIGHT);
            arr[(*moves_count)++] = ENCODE_MOVE(from, to, PIECE_BISHOP);
            arr[(*moves_count)++] = ENCODE_MOVE(from, to, PIECE_ROOK);
            arr[(*moves_count)++] = ENCODE_MOVE(from, to, PIECE_QUEEN);
        } else {
            arr[(*moves_count)++] = ENCODE_MOVE(from, to, 0);
        }
        bb &= bb - 1;
    }
}

int generate_moves(Position *p, int color, Move *arr) {
    int moves_count = 0;
    uint64_t own_pieces = GET_COLOR_OCCUPIED(p, color);
    uint64_t opponent_pieces = GET_COLOR_OCCUPIED(p, color ^ 8);

    // Generate pawn moves
    uint64_t pawns = p->bitboards[color | PIECE_PAWN];
    uint64_t empty = ~GET_OCCUPIED(p);
    uint64_t single_push =
        (color == PIECE_WHITE) ? (pawns << 8) & empty : (pawns >> 8) & empty;
    uint64_t rank = (color == PIECE_WHITE) ? RANK_3 : RANK_6;
    uint64_t double_push = (color == PIECE_WHITE)
                             ? ((single_push & rank) << 8) & empty
                             : ((single_push & rank) >> 8) & empty;

    int push_dir = (color == PIECE_WHITE) ? 8 : -8;
    add_pawn_moves(single_push, push_dir, arr, &moves_count);
    add_pawn_moves(single_push, 2 * push_dir, arr, &moves_count);

    // Generate pawn captures
    int shift_left = (color == PIECE_WHITE) ? 7 : -9;
    int shift_right = (color == PIECE_WHITE) ? 9 : -7;
    uint64_t mask_left = (color == PIECE_WHITE) ? ~FILE_H : ~FILE_A;
    uint64_t mask_right = (color == PIECE_WHITE) ? ~FILE_A : ~FILE_H;
    pawns = p->bitboards[color | PIECE_PAWN];

    uint64_t left_captures =
        shift_left >= 0
            ? ((pawns & mask_left) << shift_left) & opponent_pieces
            : ((pawns & mask_left) >> -shift_left) & opponent_pieces;

    uint64_t right_captures =
        shift_right >= 0
            ? ((pawns & mask_right) << shift_right) & opponent_pieces
            : ((pawns & mask_right) >> -shift_right) & opponent_pieces;

    add_pawn_moves(left_captures, shift_left, arr, &moves_count);
    add_pawn_moves(right_captures, shift_right, arr, &moves_count);

    uint64_t knights = p->bitboards[MAKE_PIECE(color, PIECE_KNIGHT)];
    FOREACH_SET_BIT(knights, from) {
        uint64_t attacks = knight_moves[from] & ~(own_pieces);
        FOREACH_SET_BIT(attacks, to) {
            arr[moves_count++] = ENCODE_MOVE(from, to, 0);
        }
    }

    return moves_count;
}