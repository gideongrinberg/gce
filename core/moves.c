#include "core.h"

#include <stdbool.h>
#include <stdlib.h>

const int rook_directions[4] = {-16, 16, -1, 1};
const int bishop_directions[4] = {-15, -17, 15, 17};
const int queen_directions[8] = {-16, 16, -1, 1, -15, -17, 15, 17};
const int king_offsets[8] = {-16, 16, -1, 1, -15, -17, 15, 17};

// Adds a pawn move to the array, handling promotions
static void add_pawn_move(uint32_t *arr, int *moves, int from, int to) {
    if (SQ_RANK(to) == 0 || SQ_RANK(to) == 7) {
        arr[(*moves)++] = ENCODE_MOVE(from, to, PROMO_N);
        arr[(*moves)++] = ENCODE_MOVE(from, to, PROMO_B);
        arr[(*moves)++] = ENCODE_MOVE(from, to, PROMO_R);
        arr[(*moves)++] = ENCODE_MOVE(from, to, PROMO_Q);
    } else {
        arr[(*moves)++] = ENCODE_MOVE(from, to, PROMO_NONE);
    }
}

static void generate_pawn_moves(const Board *board, uint32_t *arr, int *moves,
                                const uint8_t color, const int idx) {
    int sign = color == PIECE_WHITE ? 1 : -1;
    int target = idx + (sign * 16);
    if ((target & 0x88) == 0 && board->board[target] == EMPTY) {
        add_pawn_move(arr, moves, idx, target);
        target = idx + (sign * 32);
        bool can_double_push = (color == PIECE_WHITE && SQ_RANK(idx) == 1) ||
                               (color == PIECE_BLACK && SQ_RANK(idx) == 6);
        if (can_double_push && (target & 0x88) == 0 &&
            board->board[target] == EMPTY) {
            // can't promote so no need to use add_pawn_move
            arr[(*moves)++] = ENCODE_MOVE(idx, target, PROMO_NONE);
        }
    }

    // handle left and right captures
    int capture_offsets[2] = {sign * 15, sign * 17};
    for (int j = 0; j < 2; j++) {
        int capture = idx + capture_offsets[j];
        if ((capture & 0x88) == 0 &&
            ((GET_TYPE(board->board[capture]) != EMPTY &&
              GET_COLOR(board->board[capture]) != color) ||
             capture == board->en_passant)) {
            add_pawn_move(arr, moves, idx, capture);
        }
    }
}

static void generate_knight_moves(const Board *board, uint32_t *arr, int *moves,
                                  const uint8_t color, const int idx) {
    static const int offsets[8] = {33, 18, -14, -31, -33, -18, 14, 31};
    for (int j = 0; j < 8; j++) {
        int target = idx + offsets[j];
        if ((target & 0x88) == 0 &&
            (board->board[target] == EMPTY ||
             GET_COLOR(board->board[target]) != color)) {
            arr[(*moves)++] = ENCODE_MOVE(idx, target, PROMO_NONE);
        }
    }
}

static void generate_sliding_moves(const Board *board, uint32_t *arr,
                                   int *moves, const uint8_t color,
                                   const int idx, const int directions[],
                                   const int num_directions) {
    for (int j = 0; j < num_directions; j++) {
        const int direction = directions[j];
        int target = idx + direction;
        while ((target & 0x88) == 0 &&
               (GET_TYPE(board->board[target]) == EMPTY ||
                GET_COLOR(board->board[target]) != color)) {
            arr[(*moves)++] = ENCODE_MOVE(idx, target, PROMO_NONE);
            // Stop when blocked by a piece
            if (GET_TYPE(board->board[target]) != EMPTY &&
                GET_COLOR(board->board[target]) != color) {
                break;
            }
            target += direction;
        }
    }
}

static void generate_king_moves(const Board *board, uint32_t *arr, int *moves,
                                const uint8_t color, const int idx) {
    for (int j = 0; j < 8; j++) {
        int target = idx + king_offsets[j];
        if ((target & 0x88) == 0 &&
            (GET_TYPE(board->board[target]) == EMPTY ||
             GET_COLOR(board->board[target]) != color)) {
            arr[(*moves)++] = ENCODE_MOVE(idx, target, PROMO_NONE);
        }
    }

    if (SQ_RANK(idx) == 0 || SQ_RANK(idx) == 7) {
        int offsets[2] = {-2, 2};
        for (int j = 0; j < 2; j++) {
            int target = idx + offsets[j];
            uint32_t move = ENCODE_MOVE(idx, target, PROMO_NONE);
            if ((target & 0x88) == 0 && castle(board, move, false)) {
                arr[(*moves)++] = move;
            }
        }
    }
}

/*
 * Gets all pseudolegal moves. In other words,
 * generates a list of all potentially legal moves without checking if they
 * leave the king in check.
 */
int get_pseudolegal_moves(Board *board, uint32_t *arr) {
    int moves = 0;
    uint8_t color = board->moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int idx = BOARD_INDEX(rank, file);
            uint32_t piece = board->board[idx];
            if (GET_COLOR(piece) == EMPTY || GET_COLOR(piece) != color)
                continue;
            switch (GET_TYPE(piece)) {
            case PAWN:
                generate_pawn_moves(board, arr, &moves, color, idx);
                break;

            case KNIGHT:
                generate_knight_moves(board, arr, &moves, color, idx);
                break;
            case BISHOP:
                generate_sliding_moves(board, arr, &moves, color, idx,
                                       bishop_directions, 4);
                break;
            case ROOK:
                generate_sliding_moves(board, arr, &moves, color, idx,
                                       rook_directions, 4);
                break;
            case QUEEN:
                generate_sliding_moves(board, arr, &moves, color, idx,
                                       queen_directions, 8);
                break;
            case KING:
                generate_king_moves(board, arr, &moves, color, idx);
                break;
            }
        }
    }
    return moves;
}

bool is_square_attacked(Board *board, int sq) {
    uint32_t moves[256];
    // increment moves
    int num_moves = get_pseudolegal_moves(board, moves);
    for (int i = 0; i < num_moves; i++) {
        uint32_t move = moves[i];
        if (MOVE_TO(move) == sq) {
            return true;
        }
    }

    return false;
}

void find_kings(const Board *board, int *white_king, int *black_king) {
    *white_king = -1;
    *black_king = -1;

    for (int i = 0; i < 128; i++) {
        if (i & 0x88)
            continue;

        uint8_t piece = board->board[i];
        if (GET_COLOR(piece) == PIECE_WHITE && GET_TYPE(piece) == KING)
            *white_king = i;
        else if (GET_COLOR(piece) == PIECE_BLACK && GET_TYPE(piece) == KING)
            *black_king = i;

        if (*white_king != -1 && *black_king != -1)
            return;
    }
}

int get_legal_moves(Board *board, uint32_t *arr) {
    int moves = 0;
    uint32_t potential_moves[256];
    int num_potential_moves = get_pseudolegal_moves(board, potential_moves);
    // execute each move and check if king is attacked
    for (int i = 0; i < num_potential_moves; i++) {
        uint32_t move = potential_moves[i];

        Board *test_board = copy_board(board);
        execute_move(test_board, move);

        int white_king, black_king;
        find_kings(test_board, &white_king, &black_king);

        int king_sq;
        if (board->moves % 2 == 0) {
            king_sq = white_king;
        } else {
            king_sq = black_king;
        }

        if (!is_square_attacked(test_board, king_sq)) {
            arr[moves++] = move;
        }

        free(test_board);
    }

    return moves;
}
