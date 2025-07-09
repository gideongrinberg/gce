#ifndef CORE_H
#define CORE_H
#include <stdbool.h>
#include <stdint.h>

// Piece definitions (uint8)
#define EMPTY 0
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

#define PIECE_WHITE 8
#define PIECE_BLACK 16

#define GET_TYPE(p) ((p) & 7)
#define GET_COLOR(p) ((p) & (PIECE_WHITE | PIECE_BLACK))

// Castling rights (uint8)
#define CASTLE_WHITE_KING 1
#define CASTLE_WHITE_QUEEN 2
#define CASTLE_BLACK_KING 4
#define CASTLE_BLACK_QUEEN 8

// Move encoding (uint32)
#define PROMO_NONE 0
#define PROMO_N 1
#define PROMO_B 2
#define PROMO_R 3
#define PROMO_Q 4

#define MOVE_FROM(m) ((m) & 0x7F)
#define MOVE_TO(m) (((m) >> 7) & 0x7F)
#define MOVE_PROMO(m) (((m) >> 14) & 0x7)

#define ENCODE_MOVE(from, to, promo)                                           \
    (((from) & 0x7F) | (((to) & 0x7F) << 7) | ((promo & 0x7) << 14))

// Rank-file/board index conversion
#define BOARD_INDEX(rank, file) (rank << 4) + file
#define SQ_RANK(sq) (sq >> 4)
#define SQ_FILE(sq) (sq & 0xF)

typedef struct {
    uint8_t board[128];
    uint8_t castling_rights;
    int en_passant;
    int moves;
    int halfmoves;
} Board;

void execute_move(Board *board, uint32_t move);

/*
 * Returns whether the provided move is a valid castle and,
 * if `make_move` is true, applies the move to the board.
 */
bool castle(Board *board, uint32_t move, bool make_move);
/*
 * Fills `arr` with all legal moves, encoded as `uint32_t`.
 * `arr` should be `uint32_t[256]`.
 */
int get_legal_moves(Board *board, uint32_t *arr);

uint32_t best_move(Board *board);

/*
 * Creates a `Board` from a FEN string.
 * Does not support en-passant or move counters,
 * but includes castling rights and next-to-move.
 */
Board *board_from_fen(const char *fen_string);
Board *board_from_startpos();
Board *copy_board(const Board *original);

char *board_to_fen(Board *board);
char *move_to_string(uint32_t move);
uint32_t move_from_string(const char *move_str);
bool is_square_attacked(Board *board, int sq, uint8_t attacker_color);
#endif // CORE_H
