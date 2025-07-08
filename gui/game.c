#include "game.h"
#include <stdlib.h>

#include "raylib.h"

Game* new_game() {
    Game* game = malloc(sizeof(Game));
    game->board = board_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    game->selected_square = (Square) {-1, -1};

    return game;
}

void free_game(Game* game) {
    free(game->board);
    free(game);
}

void game_update(Game* game) {
    int rank, file;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (get_mouse_square(GetMouseX(), GetMouseY(), GetScreenWidth(), GetScreenHeight(), &rank, &file)) {
            uint8_t selected_piece = game->board->board[BOARD_INDEX(rank, file)];
            uint8_t color = game->board->moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
            if (GET_TYPE(selected_piece) != EMPTY && GET_COLOR(selected_piece) == color) {
                game->selected_square = (Square) {rank, file};

                // update potential moves
                uint32_t moves[256];
                int num_moves = get_legal_moves(game->board, moves);
                game->possible_squares = 0;
                if (game->selected_square.rank != -1 && game->selected_square.file != -1) {
                    for (int i = 0; i < num_moves; i++) {
                        if (MOVE_FROM(moves[i]) == BOARD_INDEX(game->selected_square.rank, game->selected_square.file)) {
                            int target = MOVE_TO(moves[i]);
                            int t_rank = SQ_RANK(target), t_file = SQ_FILE(target);
                            game->possible_squares |= (1ULL << (t_rank*8 + t_file));
                        }
                    }
                }
            } else if (game->selected_square.rank != -1 && game->selected_square.file != -1 && (game->possible_squares & 1ULL << (rank*8 + file))) {
                // todo: handle promotions
                uint32_t move = ENCODE_MOVE(BOARD_INDEX(game->selected_square.rank, game->selected_square.file), BOARD_INDEX(rank, file), PROMO_NONE);
                execute_move(game->board, move);
                game->selected_square = (Square) {-1, -1};
                game->possible_squares = 0;
            }
        } else {
            game->selected_square = (Square) {-1, -1};
        }
    }
}

void game_draw(Game* game) {
    draw_board(game);
}

void draw_board(Game* game) {
    Board* board = game->board;
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int boardPixels = (screenWidth < screenHeight ? screenWidth : screenHeight);
    int tileSize = boardPixels / BOARD_SIZE;

    int offsetX = (screenWidth - tileSize * BOARD_SIZE) / 2;
    int offsetY = (screenHeight - tileSize * BOARD_SIZE) / 2;

    for (int rank = 0; rank < BOARD_SIZE; rank++) {
        for (int file = 0; file < BOARD_SIZE; file++) {
            bool isDark = (rank + file) % 2 == 1;
            Color tileColor = isDark ? DARKBROWN : BEIGE;
            if (game->selected_square.rank == rank && game->selected_square.file == file) {
                tileColor = SELECTED_COLOR;
            } else if (game->possible_squares & (1ULL << (rank*8 + file))) {
                tileColor = POSSIBLE_COLOR;
            }

            int x = offsetX + file * tileSize;
            int y = offsetY + (7 - rank) * tileSize;
            DrawRectangle(x, y, tileSize, tileSize, tileColor);

            uint8_t piece = board->board[BOARD_INDEX(rank, file)];
            if (GET_TYPE(piece) != EMPTY) {
                // Draw the piece
                DrawText(TextFormat("%d", GET_TYPE(piece)), x + 5, y + 5, 20, (GET_COLOR(piece) == PIECE_WHITE) ? WHITE : BLACK);
            }
        }
    }
}

bool get_mouse_square(int mouseX, int mouseY, int screenWidth, int screenHeight, int* outRank, int* outFile) {
    int boardPixels = (screenWidth < screenHeight ? screenWidth : screenHeight);
    int tileSize = boardPixels / BOARD_SIZE;
    int offsetX = (screenWidth - tileSize * BOARD_SIZE) / 2;
    int offsetY = (screenHeight - tileSize * BOARD_SIZE) / 2;

    int file = (mouseX - offsetX) / tileSize;
    int rank = (mouseY - offsetY) / tileSize;

    if (file >= 0 && file < BOARD_SIZE && rank >= 0 && rank < BOARD_SIZE) {
        *outFile = file;
        *outRank = 7 - rank;
        return true;
    }

    return false;
}
