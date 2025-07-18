#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "position.h"
#include "tables.h"

#include <string.h>

// Masks to isolate a specific rank or file of a bitboard
#define RANK_2 0x000000000000FF00ULL
#define RANK_3 0x0000000000FF0000ULL
#define RANK_4 0x00000000FF000000ULL
#define RANK_5 0x000000FF00000000ULL
#define RANK_6 0x0000FF0000000000ULL
#define RANK_7 0x00FF000000000000ULL

#define FILE_A 0x0101010101010101ULL
#define FILE_H 0x8080808080808080ULL

#define SQUARE_INDEX(file_char, rank_char)                                     \
    (((rank_char - '1') * 8) + (file_char - 'a'))

Position *position_from_fen(const char *fen_string) {
    char fen_str[256];
    strcpy(fen_str, fen_string);

    Position *p = calloc(1, sizeof(*p));
    const char *fen = strtok(fen_str, " ");
    int rank = 7, file = 0;
    for (int i = 0; fen[i] != '\0'; i++) {
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
    }

    // skip color to move (implied by move count)
    const char *side_to_move = strtok(NULL, " ");
    fen = strtok(NULL, " ");
    if (fen == "-") {
        p->castling_rights = 0;
    } else {
        for (int i = 0; fen[i] != '\0'; i++) {
            switch (fen[i]) {
            case 'K':
                p->castling_rights |= WHITE_KINGSIDE;
                break;
            case 'Q':
                p->castling_rights |= WHITE_QUEENSIDE;
                break;
            case 'k':
                p->castling_rights |= BLACK_KINGSIDE;
                break;
            case 'q':
                p->castling_rights |= BLACK_QUEENSIDE;
                break;
            }
        }
    }

    fen = strtok(NULL, " ");
    // todo: en passant

    fen = strtok(NULL, " ");
    int halfmove = atoi(fen);
    p->halfmoves = halfmove;

    fen = strtok(NULL, " ");
    int moves = atoi(fen);
    p->moves = (moves - 1) * 2 + (strcmp(side_to_move, "b") == 0 ? 1 : 0);
    p->en_passant = 0;
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

void print_bitboard(uint64_t bb) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            printf("%c ", (bb & (1ULL << sq)) ? '1' : '.');
        }
        printf("  %d\n", rank + 1);
    }
    printf("a b c d e f g h\n");
}

/**
 * Since the bishop and rook attack generation are essentially the same, they
 * can be generated with a macro.
 */
#define DEFINE_SLIDER_ATTACK_FN(NAME)                                          \
    uint64_t get_##NAME##_attacks(uint64_t occupancy, int sq) {                \
        uint64_t blockers = occupancy & NAME##_blocker_masks[sq];              \
        uint64_t magic = NAME##_magic_numbers[sq];                             \
        int shift = 64 - NAME##_rel_bits[sq];                                  \
        uint64_t index = (blockers * magic) >> shift;                          \
        return NAME##_attack_tables[sq][index];                                \
    }

DEFINE_SLIDER_ATTACK_FN(rook)
DEFINE_SLIDER_ATTACK_FN(bishop)
uint64_t get_queen_attacks(uint64_t occupancy, int sq) {
    // a queen is just a bishop and a rook combined
    return get_bishop_attacks(occupancy, sq) | get_rook_attacks(occupancy, sq);
}

uint64_t generate_attacks(Position *p, int color) {
    uint64_t attacks = 0;

    // Generate pawn attacks
    int shift_left = (color == PIECE_WHITE) ? 7 : -9;
    int shift_right = (color == PIECE_WHITE) ? 9 : -7;
    uint64_t mask_left = ~FILE_A;
    uint64_t mask_right = ~FILE_H;
    uint64_t pawns = p->bitboards[color | PIECE_PAWN];

    uint64_t left_captures = shift_left >= 0
                                 ? ((pawns & mask_left) << shift_left)
                                 : ((pawns & mask_left) >> -shift_left);

    uint64_t right_captures = shift_right >= 0
                                  ? ((pawns & mask_right) << shift_right)
                                  : ((pawns & mask_right) >> -shift_right);

    attacks |= left_captures | right_captures;

    // Knights and kings
    FOREACH_SET_BIT(p->bitboards[color | PIECE_KNIGHT], from) {
        attacks |= knight_moves[from];
    }

    FOREACH_SET_BIT(p->bitboards[color | PIECE_KING], from) {
        attacks |= king_moves[from];
    }

    // Sliders
    uint64_t occupied = GET_OCCUPIED(p);
    FOREACH_SET_BIT(p->bitboards[color | PIECE_QUEEN], queen) {
        attacks |= get_queen_attacks(occupied, queen);
    }

    FOREACH_SET_BIT(p->bitboards[color | PIECE_BISHOP], bishop) {
        attacks |= get_bishop_attacks(occupied, bishop);
    }

    FOREACH_SET_BIT(p->bitboards[color | PIECE_ROOK], rook) {
        attacks |= get_rook_attacks(occupied, rook);
    }

    return attacks;
}

static void add_pawn_moves(uint64_t bb, int shift, Move *arr,
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

// A macro for getting the attacks for every sliding piece of a
// specific type and adding them as legal moves.
#define ADD_SLIDER_MOVES(piece_bb, get_attacks_fn)                             \
    FOREACH_SET_BIT(piece_bb, from) {                                          \
        uint64_t attacks = get_attacks_fn(occupancy, from) & ~own_pieces;      \
        if (pinned_pieces & (1ULL << from)) {                                  \
            attacks &= pin_rays[from];                                         \
        }                                                                      \
        FOREACH_SET_BIT(attacks, to) {                                         \
            arr[moves_count++] = ENCODE_MOVE(from, to, 0);                     \
        }                                                                      \
    }

static bool are_aligned(int king_sq, int slider_sq, int piece_type) {
    switch (piece_type) {
    case PIECE_BISHOP:
        return get_bishop_attacks(0ULL, slider_sq) & (1ULL << king_sq);
    case PIECE_ROOK:
        return get_rook_attacks(0ULL, slider_sq) & (1ULL << king_sq);
    case PIECE_QUEEN:
        return get_queen_attacks(0ULL, slider_sq) & (1ULL << king_sq);
    default:
        return false;
    }
}

#define DETECT_PINS(piece_type, enemy_bb, piece_name)                          \
    FOREACH_SET_BIT(enemy_bb, piece_name) {                                    \
        if (are_aligned(king, piece_name, piece_type)) {                       \
            uint64_t between = squares_between[king][piece_name];              \
            uint64_t blockers = between & GET_OCCUPIED(p);                     \
            if (__builtin_popcountll(blockers) == 1) {                         \
                pinned_pieces |= blockers;                                     \
                pin_rays[__builtin_ctzll(blockers)] =                          \
                    (1ULL << piece_name) | between;                            \
            }                                                                  \
        }                                                                      \
    }

uint64_t generate_attacks_xray(Position *p, int color, uint64_t king_bb) {
    int opp = color ^ 8;
    uint64_t opponent_attacks = 0;

    FOREACH_SET_BIT(p->bitboards[opp | PIECE_KNIGHT], sq) {
        opponent_attacks |= knight_moves[sq];
    }
    FOREACH_SET_BIT(p->bitboards[opp | PIECE_KING], sq) {
        opponent_attacks |= king_moves[sq];
    }

    // Pawn attacks (directional)
    uint64_t pawns = p->bitboards[opp | PIECE_PAWN];
    if (opp == PIECE_WHITE) {
        opponent_attacks |= ((pawns & ~FILE_A) << 7);
        opponent_attacks |= ((pawns & ~FILE_H) << 9);
    } else {
        opponent_attacks |= ((pawns & ~FILE_H) >> 7);
        opponent_attacks |= ((pawns & ~FILE_A) >> 9);
    }

    // Sliding attacks with x-ray
    uint64_t occupied = GET_OCCUPIED(p);
    FOREACH_SET_BIT(p->bitboards[opp | PIECE_ROOK], sq) {
        opponent_attacks |= get_rook_attacks(occupied ^ king_bb, sq);
    }
    FOREACH_SET_BIT(p->bitboards[opp | PIECE_BISHOP], sq) {
        opponent_attacks |= get_bishop_attacks(occupied ^ king_bb, sq);
    }
    FOREACH_SET_BIT(p->bitboards[opp | PIECE_QUEEN], sq) {
        opponent_attacks |= get_queen_attacks(occupied ^ king_bb, sq);
    }

    return opponent_attacks;
}

int generate_evasive_moves(Position *p, Move *arr) {
    int moves_count = 0;
    int color = p->moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
    int opp = color ^ 8;
    uint64_t king_bb = p->bitboards[color | PIECE_KING];
    int king_sq = __builtin_ctzll(king_bb);
    int king = king_sq;
    int attackers = 0;
    int attacker_sq = 0;

    uint64_t own_pieces = GET_COLOR_OCCUPIED(p, color);
    uint64_t opponent_pieces = GET_COLOR_OCCUPIED(p, color ^ 8);
    uint64_t opponent_attacks = generate_attacks_xray(p, color, king_bb);
    uint64_t pinned_pieces = 0;
    uint64_t pin_rays[64] = {0};

    // build a bitboad of attacked squares

    // identify attackers
    FOREACH_SET_BIT(p->bitboards[opp | PIECE_QUEEN], queen) {
        if (get_queen_attacks(GET_OCCUPIED(p), queen) & (1ULL << king_sq)) {
            attackers++;
            attacker_sq = queen;
        }
    }

    FOREACH_SET_BIT(p->bitboards[opp | PIECE_ROOK], rook) {
        if (get_rook_attacks(GET_OCCUPIED(p), rook) & (1ULL << king_sq)) {
            attackers++;
            attacker_sq = rook;
        }
    }

    FOREACH_SET_BIT(p->bitboards[opp | PIECE_BISHOP], bishop) {
        if (get_bishop_attacks(GET_OCCUPIED(p), bishop) & (1ULL << king_sq)) {
            attackers++;
            attacker_sq = bishop;
        }
    }

    FOREACH_SET_BIT(p->bitboards[opp | PIECE_KNIGHT], knight) {
        if (knight_moves[knight] & (1ULL << king_sq)) {
            attackers++;
            attacker_sq = knight;
        }
    }

    uint64_t pawn_attackers;
    if (color == PIECE_WHITE) {
        pawn_attackers =
            ((king_bb & ~FILE_A) << 7) | ((king_bb & ~FILE_H) << 9);
    } else {
        pawn_attackers =
            ((king_bb & ~FILE_H) >> 7) | ((king_bb & ~FILE_A) >> 9);
    }
    int num_pawn_attackers =
        __builtin_popcountll(pawn_attackers & p->bitboards[opp | PIECE_PAWN]);
    if (num_pawn_attackers > 0) {
        attackers += num_pawn_attackers;
        attacker_sq =
            __builtin_ctzll(pawn_attackers & p->bitboards[opp | PIECE_PAWN]);
    }

    uint64_t attacker_bb = 1ULL << attacker_sq;

    // king moves are always valid
    uint64_t king_squares =
        king_moves[king_sq] & ~own_pieces & ~opponent_attacks;
    FOREACH_SET_BIT(king_squares, to) {
        arr[moves_count++] = ENCODE_MOVE(king_sq, to, 0);
    }

    // if the king is attacked more than once, only it can move
    if (attackers > 1) {
        return moves_count;
    }

    // squares that can block the attacker.
    uint64_t block_bb = squares_between[king_sq][attacker_sq] | attacker_bb;
    // printf("%i\n", attacker_sq);

    // Generate pins
    DETECT_PINS(PIECE_BISHOP, p->bitboards[opp | PIECE_BISHOP], bishop)
    DETECT_PINS(PIECE_ROOK, p->bitboards[opp | PIECE_ROOK], rook)
    DETECT_PINS(PIECE_QUEEN, p->bitboards[opp | PIECE_QUEEN], queen)

    uint64_t occupancy = own_pieces | opponent_pieces;
    FOREACH_SET_BIT(p->bitboards[color | PIECE_BISHOP], from) {
        uint64_t moves = get_bishop_attacks(occupancy, from);
        if (pinned_pieces & (1ULL << from)) {
            moves &= pin_rays[from];
        }

        moves &= block_bb;
        FOREACH_SET_BIT(moves, to) {
            arr[moves_count++] = ENCODE_MOVE(from, to, 0);
        }
    }

    FOREACH_SET_BIT(p->bitboards[color | PIECE_ROOK], from) {
        uint64_t moves = get_rook_attacks(occupancy, from);
        if (pinned_pieces & (1ULL << from)) {
            moves &= pin_rays[from];
        }

        moves &= block_bb;
        FOREACH_SET_BIT(moves, to) {
            arr[moves_count++] = ENCODE_MOVE(from, to, 0);
        }
    }

    FOREACH_SET_BIT(p->bitboards[color | PIECE_QUEEN], from) {
        uint64_t moves = get_queen_attacks(occupancy, from);
        if (pinned_pieces & (1ULL << from)) {
            moves &= pin_rays[from];
        }

        moves &= block_bb;
        FOREACH_SET_BIT(moves, to) {
            arr[moves_count++] = ENCODE_MOVE(from, to, 0);
        }
    }

    FOREACH_SET_BIT(p->bitboards[color | PIECE_KNIGHT], from) {
        if (!(pinned_pieces & (1ULL << from))) {
            uint64_t attacks = knight_moves[from] & block_bb;
            attacks &= ~own_pieces;
            FOREACH_SET_BIT(attacks, to) {
                arr[moves_count++] = ENCODE_MOVE(from, to, 0);
            }
        }
    }

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

    // Generate pawn captures
    int shift_left = (color == PIECE_WHITE) ? 7 : -9;
    int shift_right = (color == PIECE_WHITE) ? 9 : -7;
    uint64_t mask_left = ~FILE_A;
    uint64_t mask_right = ~FILE_H;
    pawns = p->bitboards[color | PIECE_PAWN];

    uint64_t capture_mask = opponent_pieces | p->en_passant;
    uint64_t left_captures =
        shift_left >= 0 ? ((pawns & mask_left) << shift_left) & capture_mask
                        : ((pawns & mask_left) >> -shift_left) & capture_mask;

    uint64_t right_captures =
        shift_right >= 0
            ? ((pawns & mask_right) << shift_right) & capture_mask
            : ((pawns & mask_right) >> -shift_right) & capture_mask;

    single_push &= block_bb;
    double_push &= block_bb;
    left_captures &= block_bb;
    right_captures &= block_bb;

    // Filter out pinned pawn moves
    FOREACH_SET_BIT(pawns & pinned_pieces, pawn_sq) {
        uint64_t pin_ray = pin_rays[pawn_sq];
        uint64_t push_dest = 1ULL << (pawn_sq + push_dir);
        if (!(pin_ray & push_dest))
            single_push &= ~push_dest;

        uint64_t double_dest = 1ULL << (pawn_sq + (2 * push_dir));
        if (!(pin_ray & double_dest))
            double_push &= ~double_dest;

        uint64_t left_dest = 1ULL << (pawn_sq + shift_left);
        if (!(pin_ray & left_dest))
            left_captures &= ~left_dest;

        uint64_t right_dest = 1ULL << (pawn_sq + shift_right);
        if (!(pin_ray & right_dest))
            right_captures &= ~right_dest;
    }

    add_pawn_moves(single_push, push_dir, arr, &moves_count);
    add_pawn_moves(double_push, 2 * push_dir, arr, &moves_count);
    add_pawn_moves(left_captures, shift_left, arr, &moves_count);
    add_pawn_moves(right_captures, shift_right, arr, &moves_count);

    return moves_count;
}

int generate_moves(Position *p, Move *arr) {
    int moves_count = 0;
    int color = p->moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
    int opp = color ^ 8;
    uint64_t own_pieces = GET_COLOR_OCCUPIED(p, color);
    uint64_t opponent_pieces = GET_COLOR_OCCUPIED(p, color ^ 8);
    uint64_t opponent_attacks = generate_attacks(p, color ^ 8);
    uint64_t pinned_pieces = 0;
    uint64_t pin_rays[64] = {0};

    // Handle check
    int king = __builtin_ctzll(p->bitboards[color | PIECE_KING]);
    if (opponent_attacks & p->bitboards[color | PIECE_KING])
        return generate_evasive_moves(p, arr);

    // Generate pins
    DETECT_PINS(PIECE_BISHOP, p->bitboards[opp | PIECE_BISHOP], bishop)
    DETECT_PINS(PIECE_ROOK, p->bitboards[opp | PIECE_ROOK], rook)
    DETECT_PINS(PIECE_QUEEN, p->bitboards[opp | PIECE_QUEEN], queen)

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

    // Generate pawn captures
    int shift_left = (color == PIECE_WHITE) ? 7 : -9;
    int shift_right = (color == PIECE_WHITE) ? 9 : -7;
    uint64_t mask_left = ~FILE_A;
    uint64_t mask_right = ~FILE_H;
    pawns = p->bitboards[color | PIECE_PAWN];

    uint64_t capture_mask = opponent_pieces | p->en_passant;
    uint64_t left_captures =
        shift_left >= 0 ? ((pawns & mask_left) << shift_left) & capture_mask
                        : ((pawns & mask_left) >> -shift_left) & capture_mask;

    uint64_t right_captures =
        shift_right >= 0
            ? ((pawns & mask_right) << shift_right) & capture_mask
            : ((pawns & mask_right) >> -shift_right) & capture_mask;

    // Filter out illegal pawn moves
    FOREACH_SET_BIT(pawns & pinned_pieces, pawn_sq) {
        uint64_t pin_ray = pin_rays[pawn_sq];
        uint64_t push_dest = 1ULL << (pawn_sq + push_dir);
        if (!(pin_ray & push_dest))
            single_push &= ~push_dest;

        uint64_t double_dest = 1ULL << (pawn_sq + (2 * push_dir));
        if (!(pin_ray & double_dest))
            double_push &= ~double_dest;

        uint64_t left_dest = 1ULL << (pawn_sq + shift_left);
        if (!(pin_ray & left_dest))
            left_captures &= ~left_dest;

        uint64_t right_dest = 1ULL << (pawn_sq + shift_right);
        if (!(pin_ray & right_dest))
            right_captures &= ~right_dest;
    }

    add_pawn_moves(single_push, push_dir, arr, &moves_count);
    add_pawn_moves(double_push, 2 * push_dir, arr, &moves_count);
    add_pawn_moves(left_captures, shift_left, arr, &moves_count);
    add_pawn_moves(right_captures, shift_right, arr, &moves_count);

    uint64_t knights = p->bitboards[MAKE_PIECE(color, PIECE_KNIGHT)];
    FOREACH_SET_BIT(knights, from) {
        if (!(pinned_pieces & (1ULL << from))) {
            uint64_t attacks = knight_moves[from] & ~(own_pieces);
            FOREACH_SET_BIT(attacks, to) {
                arr[moves_count++] = ENCODE_MOVE(from, to, 0);
            }
        }
    }

    uint64_t king_squares = king_moves[king] & ~own_pieces & ~opponent_attacks;
    FOREACH_SET_BIT(king_squares, to) {
        arr[moves_count++] = ENCODE_MOVE(king, to, 0);
    }

    uint64_t castle_mask = own_pieces | opponent_pieces | opponent_attacks;
    if (color == PIECE_WHITE && !(opponent_attacks & (1ULL << king))) {
        if ((p->castling_rights & WHITE_KINGSIDE) &&
            !(castle_mask & ((1ULL << 5) | (1ULL << 6)))) {
            arr[moves_count++] = ENCODE_MOVE(king, SQUARE_INDEX('g', '1'), 0);
        }

        if ((p->castling_rights & WHITE_QUEENSIDE) &&
            !(castle_mask & ((1ULL << 2) | (1ULL << 3)))) {
            arr[moves_count++] = ENCODE_MOVE(king, SQUARE_INDEX('c', '1'), 0);
        }
    } else if (color == PIECE_BLACK && !(opponent_attacks & (1ULL << king))) {
        if ((p->castling_rights & BLACK_KINGSIDE) &&
            !(castle_mask & ((1ULL << 61) | (1ULL << 62)))) {
            arr[moves_count++] = ENCODE_MOVE(king, SQUARE_INDEX('g', '8'), 0);
        }

        if ((p->castling_rights & BLACK_QUEENSIDE) &&
            !(castle_mask & ((1ULL << 58) | (1ULL << 59)))) {
            arr[moves_count++] = ENCODE_MOVE(king, SQUARE_INDEX('c', '8'), 0);
        }
    }

    uint64_t occupancy = own_pieces | opponent_pieces;
    ADD_SLIDER_MOVES(p->bitboards[color | PIECE_BISHOP], get_bishop_attacks)
    ADD_SLIDER_MOVES(p->bitboards[color | PIECE_ROOK], get_rook_attacks)
    ADD_SLIDER_MOVES(p->bitboards[color | PIECE_QUEEN], get_queen_attacks)

    return moves_count;
}

void execute_move(Position *p, Move move) {
    int color = (GET_COLOR_OCCUPIED(p, PIECE_WHITE) & (1ULL << MOVE_FROM(move)))
                    ? PIECE_WHITE
                    : PIECE_BLACK;

    int opp = color ^ 8;
    int moving_piece = -1;

    uint64_t from_bb = 1ULL << MOVE_FROM(move);
    uint64_t to_bb = 1ULL << MOVE_TO(move);
    // handle castling
    if (p->bitboards[color | PIECE_KING] & from_bb) {
        moving_piece = PIECE_KING;
        switch (color) {
        case PIECE_WHITE:
            switch (move) {
            case ENCODE_MOVE(4, 6, 0): // e1 -> g1
                p->bitboards[PIECE_WHITE | PIECE_ROOK] &= ~(1ULL << 7);
                p->bitboards[PIECE_WHITE | PIECE_ROOK] |= (1ULL << 5);
                p->bitboards[PIECE_WHITE | PIECE_KING] = (1ULL << 6);
                goto end;
            case ENCODE_MOVE(4, 2, 0):
                p->bitboards[PIECE_WHITE | PIECE_ROOK] &= ~(1ULL << 0);
                p->bitboards[PIECE_WHITE | PIECE_ROOK] |= (1ULL << 3);
                p->bitboards[PIECE_WHITE | PIECE_KING] = (1ULL << 2);
                goto end;
            }
        case PIECE_BLACK:
            switch (move) {
            case ENCODE_MOVE(60, 58, 0):
                p->bitboards[PIECE_BLACK | PIECE_ROOK] &= ~(1ULL << 56);
                p->bitboards[PIECE_BLACK | PIECE_ROOK] |= (1ULL << 59);
                p->bitboards[PIECE_BLACK | PIECE_KING] = (1ULL << 58);
                goto end;
            case ENCODE_MOVE(60, 62, 0):
                p->bitboards[PIECE_BLACK | PIECE_ROOK] &= ~(1ULL << 63);
                p->bitboards[PIECE_BLACK | PIECE_ROOK] |= (1ULL << 61);
                p->bitboards[PIECE_BLACK | PIECE_KING] = (1ULL << 62);
                goto end;
            }
        }
    }

    // find the piece
    for (int i = 0; i < 6; i++) {
        if (p->bitboards[color | i] & from_bb) {
            moving_piece = i;
            break;
        }
    }

    if (moving_piece == -1) {
        return;
    }

    p->bitboards[moving_piece | color] &= ~from_bb;
    if (MOVE_PROMO(move) == 0) {
        p->bitboards[moving_piece | color] |= to_bb;
    } else {
        p->bitboards[MOVE_PROMO(move) | color] |= to_bb;
    }

    // handle capture
    bool capture = false;
    // ep capture
    if (moving_piece == PIECE_PAWN && to_bb == p->en_passant) {
        capture = true;
        int capture_sq =
            (color == PIECE_WHITE) ? MOVE_TO(move) - 8 : MOVE_TO(move) + 8;
        p->bitboards[opp | PIECE_PAWN] &= ~(1ULL << capture_sq);
    } else { // regular capture
        for (int i = 0; i < 6; i++) {
            if (p->bitboards[opp | i] & to_bb) {
                p->bitboards[opp | i] &= ~to_bb;
                capture = true;
                // update castling rights when rook is captured
                if (i == PIECE_ROOK) {
                    switch (MOVE_TO(move)) {
                    case SQUARE_INDEX('a', '1'):
                        p->castling_rights &= ~WHITE_QUEENSIDE;
                        break;
                    case SQUARE_INDEX('h', '1'):
                        p->castling_rights &= ~WHITE_KINGSIDE;
                        break;
                    case SQUARE_INDEX('h', '8'):
                        p->castling_rights &= ~BLACK_KINGSIDE;
                        break;
                    case SQUARE_INDEX('a', '8'):
                        p->castling_rights &= ~BLACK_QUEENSIDE;
                        break;
                    }
                }
                break;
            }
        }
    }

end: // cleanup
    // update en passant
    if (moving_piece == PIECE_PAWN &&
        abs(MOVE_TO(move) - MOVE_FROM(move)) == 16) {
        p->en_passant = 1ULL << ((MOVE_FROM(move) + MOVE_TO(move)) / 2);
    } else {
        p->en_passant = 0;
    }

    // update castling rights
    if (moving_piece == PIECE_KING) {
        switch (color) {
        case PIECE_WHITE:
            p->castling_rights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
            break;
        case PIECE_BLACK:
            p->castling_rights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
            break;
        }
    } else if (moving_piece == PIECE_ROOK) {
        switch (MOVE_FROM(move)) {
        case SQUARE_INDEX('a', '1'):
            p->castling_rights &= ~WHITE_QUEENSIDE;
            break;
        case SQUARE_INDEX('h', '1'):
            p->castling_rights &= ~WHITE_KINGSIDE;
            break;
        case SQUARE_INDEX('h', '8'):
            p->castling_rights &= ~BLACK_KINGSIDE;
            break;
        case SQUARE_INDEX('a', '8'):
            p->castling_rights &= ~BLACK_QUEENSIDE;
            break;
        }
    }

    p->moves++;
    if (!capture && moving_piece != PIECE_PAWN) {
        p->halfmoves++;
    } else {
        p->halfmoves = 0;
    }
}