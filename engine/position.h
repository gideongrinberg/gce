#ifndef POSITION_H
#define POSITION_H
#include <stdint.h>

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

typedef struct {
    uint64_t bitboards[MAX_PIECE];
    CastlingRights castling_rights;
} Position;

Position* position_from_fen(char* fen);

void print_position(Position* p);
int generate_moves(Position *p, int color, Move *arr);
#endif // POSITION_H