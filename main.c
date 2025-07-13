#include "tables.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint64_t uint64;

/*
 * The engine represents pieces as 8-bit integers where the
 * lowest 3 bits are the piece type and the 4th bit is the color.
 * So, since black is 0b1000 (8) and a knight is 0b0001, a black
 * knight is 0b1001.
 *
 * The color value can be "flipped" (black->white/white->black)
 * using color^8.
 */
typedef uint8_t Piece;
enum Pieces {
    PIECE_WHITE = 0,
    PIECE_BLACK = 8,
    PIECE_PAWN = 0,
    PIECE_KNIGHT = 1,
    PIECE_BISHOP = 2,
    PIECE_ROOK = 3,
    PIECE_QUEEN = 4,
    PIECE_KING = 5,
    NUM_PIECE = 12,
    MAX_PIECE = (1 << 4)
};

#define MAKE_PIECE(color, kind) (color | kind)
#define PIECE_TYPE(p) ((p) & 7)
#define PIECE_COLOR(p) ((p) & 8)

/**
 * We encode moves as 16-bit integers with 6 bits for
 * from and to and 4 bits for promotion (1-4 for N-Q)
 */
typedef uint16_t Move;
#define ENCODE_MOVE(from, to, promo)                                           \
    (((from) & 0x3F) | (((to) & 0x3F) << 6) | (((promo) & 0xF) << 12))

#define MOVE_FROM(move) ((move) & 0x3F)
#define MOVE_TO(move) (((move) >> 6) & 0x3F)
#define MOVE_PROMO(move) (((move) >> 12) & 0xF)

/**
 * We encode castling rights as a bitflag.
 */
typedef uint8_t CastlingRights;
enum CastlingRights {
    CASTLE_WHITE_KING = 1,
    CASTLE_WHITE_QUEEN = 2,
    CASTLE_BLACK_KING = 4,
    CASTLE_BLACK_QUEEN = 8,

    CASTLE_ALL = 1 | 2 | 4 | 8,
    CASTLE_NONE = 0
};

// Masks to isolate a specific rank or file of a bitboard
#define RANK_2 0x000000000000FF00ULL
#define RANK_3 0x0000000000FF0000ULL
#define RANK_4 0x00000000FF000000ULL
#define RANK_5 0x000000FF00000000ULL
#define RANK_6 0x0000FF0000000000ULL
#define RANK_7 0x00FF000000000000ULL

#define FILE_A 0x0101010101010101ULL
#define FILE_H 0x8080808080808080ULL

typedef struct {
    uint64 bitboards[MAX_PIECE];
    CastlingRights castling_rights;
} Position;

// Combines all the bitboards of the given color.
#define GET_COLOR_OCCUPIED(p, color)                                           \
    ((p)->bitboards[MAKE_PIECE(color, PIECE_PAWN)] |                           \
     (p)->bitboards[MAKE_PIECE(color, PIECE_KNIGHT)] |                         \
     (p)->bitboards[MAKE_PIECE(color, PIECE_BISHOP)] |                         \
     (p)->bitboards[MAKE_PIECE(color, PIECE_ROOK)] |                           \
     (p)->bitboards[MAKE_PIECE(color, PIECE_QUEEN)] |                          \
     (p)->bitboards[MAKE_PIECE(color, PIECE_KING)])

#define GET_OCCUPIED(p)                                                        \
    (GET_COLOR_OCCUPIED((p), PIECE_WHITE) |                                    \
     GET_COLOR_OCCUPIED((p), PIECE_BLACK))

/**
 * This macro iterates over each set bit in a uint64_t. It is the
 * equivalent of doing the following:
 *     while (bitboard) {
 *      int idx = __builtin_ctzll(bitboard);
 *      // do something with idx
 *      bitboard &= bitboard - 1;
 *     }
 */
#define FOREACH_SET_BIT(bb, sq)                                                \
    for (uint64_t _bb = (bb); _bb; _bb &= _bb - 1)                             \
        for (int sq = __builtin_ctzll(_bb), _once = 1; _once; _once = 0)

Position *position_from_fen(char *fen) {
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

uint64 generate_attacks(Position *p, int color) {
    uint64 attacks = 0;

    int shift_left = (color == PIECE_WHITE) ? 7 : -9;
    int shift_right = (color == PIECE_WHITE) ? 9 : -7;
    uint64 mask_left = (color == PIECE_WHITE) ? ~FILE_H : ~FILE_A;
    uint64 mask_right = (color == PIECE_WHITE) ? ~FILE_A : ~FILE_H;
    uint64 pawns = p->bitboards[color | PIECE_PAWN];

    uint64 left_captures = shift_left >= 0
                               ? ((pawns & mask_left) << shift_left)
                               : ((pawns & mask_left) >> -shift_left);

    uint64 right_captures = shift_right >= 0
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
    uint64 own_pieces = GET_COLOR_OCCUPIED(p, color);
    uint64 opponent_pieces = GET_COLOR_OCCUPIED(p, color ^ 8);

    // Generate pawn moves
    uint64 pawns = p->bitboards[color | PIECE_PAWN];
    uint64 empty = ~GET_OCCUPIED(p);
    uint64 single_push =
        (color == PIECE_WHITE) ? (pawns << 8) & empty : (pawns >> 8) & empty;
    uint64 rank = (color == PIECE_WHITE) ? RANK_3 : RANK_6;
    uint64 double_push = (color == PIECE_WHITE)
                             ? ((single_push & rank) << 8) & empty
                             : ((single_push & rank) >> 8) & empty;

    int push_dir = (color == PIECE_WHITE) ? 8 : -8;
    add_pawn_moves(single_push, push_dir, arr, &moves_count);
    add_pawn_moves(single_push, 2 * push_dir, arr, &moves_count);

    // Generate pawn captures
    int shift_left = (color == PIECE_WHITE) ? 7 : -9;
    int shift_right = (color == PIECE_WHITE) ? 9 : -7;
    uint64 mask_left = (color == PIECE_WHITE) ? ~FILE_H : ~FILE_A;
    uint64 mask_right = (color == PIECE_WHITE) ? ~FILE_A : ~FILE_H;
    pawns = p->bitboards[color | PIECE_PAWN];

    uint64 left_captures =
        shift_left >= 0
            ? ((pawns & mask_left) << shift_left) & opponent_pieces
            : ((pawns & mask_left) >> -shift_left) & opponent_pieces;

    uint64 right_captures =
        shift_right >= 0
            ? ((pawns & mask_right) << shift_right) & opponent_pieces
            : ((pawns & mask_right) >> -shift_right) & opponent_pieces;

    add_pawn_moves(left_captures, shift_left, arr, &moves_count);
    add_pawn_moves(right_captures, shift_right, arr, &moves_count);

    uint64_t knights = p->bitboards[MAKE_PIECE(color, PIECE_KNIGHT)];
    FOREACH_SET_BIT(knights, from) {
        uint64 attacks = knight_moves[from] & ~(own_pieces);
        FOREACH_SET_BIT(attacks, to) {
            arr[moves_count++] = ENCODE_MOVE(from, to, 0);
        }
    }

    return moves_count;
}

int main(void) {
    Position *p = pos_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR ");
    Move moves[256];
    printf("%d", generate_moves(p, PIECE_WHITE, moves));
    return 0;
}