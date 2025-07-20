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
enum {
    WHITE_KINGSIDE = 1,
    WHITE_QUEENSIDE = 2,
    BLACK_KINGSIDE = 4,
    BLACK_QUEENSIDE = 8
};

typedef uint8_t GameOutcome;
enum { ONGOING = 0, CHECKMATE = 1, DRAW = 2, STALEMATE = 3 };

typedef struct {
    uint64_t bitboards[MAX_PIECE];
    uint64_t en_passant;
    CastlingRights castling_rights;
    int moves;
    int halfmoves;
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

Position *position_from_fen(const char *fen_string);

void print_position(Position *p);
int generate_moves(Position *p, Move *arr);
uint64_t generate_attacks(Position *p, int color);
void execute_move(Position *p, Move move);
GameOutcome position_outcome(Position *p);
#endif // POSITION_H