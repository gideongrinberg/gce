#include "game.h"
#include <stdlib.h>

#include "raylib.h"
#include "textures.h"

#include <stdio.h>
#include <string.h>

#define NK_LINE_TEXT(align, msg, ...)                                          \
    {                                                                          \
        sprintf(buffer, msg, __VA_ARGS__);                                     \
        nk_layout_row_dynamic(ctx, 30, 1);                                     \
        nk_label(ctx, buffer, NK_TEXT_ALIGN_##align);                          \
    }

Game *new_game() {
    Game *game = malloc(sizeof(Game));
    game->player_color = PIECE_WHITE;
    game->board = board_from_startpos();
    game->selected_square = (Square){-1, -1};
    game->state = IN_PROGRESS;
    game->outcome = (GameOutcome){Ongoing, -1};
    game->possible_squares = 0;
    return game;
}

void free_game(Game *game) {
    free(game->board);
    free(game);
}

void game_update(Game *game) {
    int rank, file;
    if (game->state == IN_PROGRESS && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        float mouse_x, mouse_y;
        Vector2 mouse_pos = GetMousePosition();
        mouse_x = mouse_pos.x;
        mouse_y = mouse_pos.y;
        if (get_mouse_square(mouse_x, mouse_y, GetScreenWidth(),
                             GetScreenHeight(), &rank, &file,
                             game->player_color != PIECE_WHITE)) {
            uint8_t selected_piece =
                game->board->board[BOARD_INDEX(rank, file)];
            uint8_t color =
                game->board->moves % 2 == 0 ? PIECE_WHITE : PIECE_BLACK;
            if (GET_TYPE(selected_piece) != EMPTY &&
                GET_COLOR(selected_piece) == color) {
                game->selected_square = (Square){rank, file};

                // update potential moves by storing all possible moves
                // from the selected square in a bitboard
                uint32_t moves[256];
                int num_moves = get_legal_moves(game->board, moves);
                game->possible_squares = 0;
                if (game->selected_square.rank != -1 &&
                    game->selected_square.file != -1) {
                    for (int i = 0; i < num_moves; i++) {
                        if (MOVE_FROM(moves[i]) ==
                            BOARD_INDEX(game->selected_square.rank,
                                        game->selected_square.file)) {
                            int target = MOVE_TO(moves[i]);
                            int t_rank = SQ_RANK(target),
                                t_file = SQ_FILE(target);
                            game->possible_squares |=
                                (1ULL << (t_rank * 8 + t_file));
                        }
                    }
                }
            } else if (game->selected_square.rank != -1 &&
                       game->selected_square.file != -1 &&
                       (game->possible_squares & 1ULL << (rank * 8 + file))) {
                // todo: handle promotions
                uint32_t move =
                    ENCODE_MOVE(BOARD_INDEX(game->selected_square.rank,
                                            game->selected_square.file),
                                BOARD_INDEX(rank, file), PROMO_NONE);
                char *move_str = move_to_string(move);
                printf("%s ", move_str);
                free(move_str);
                fflush(stdout);
                execute_move(game->board, move);
                game->selected_square = (Square){-1, -1};
                game->possible_squares = 0;
            }
        } else {
            game->selected_square = (Square){-1, -1};
        }
    }

    if (game->state == IN_PROGRESS) {
        Outcome outcome = board_outcome(game->board);
        if (outcome == Checkmate) {
            game->outcome = (GameOutcome){
                Checkmate,
                (game->board->moves % 2 == 0) ? PIECE_BLACK : PIECE_WHITE};
            game->state = GAME_OVER;
        } else if (outcome == Draw || outcome == Stalemate) {
            game->outcome = (GameOutcome){outcome, -1};
            game->state = GAME_OVER;
        }
    }
}

void game_draw(Game *game) { draw_board(game); }

int draw_start_modal(struct nk_context *ctx) {
    int selected = -1;
    struct nk_rect bounds =
        nk_rect(((float)GetScreenWidth() - 300) / 2,
                ((float)GetScreenHeight() - 180) / 2, 300, 180);
    if (nk_begin(ctx, "Select your color", bounds, NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_button_label(ctx, "Play as White")) {
            selected = PIECE_WHITE;
        }

        if (nk_button_label(ctx, "Play as Black")) {
            selected = PIECE_BLACK;
        }

        nk_end(ctx);
    }

    return selected;
}

bool draw_game_over_modal(Game *game, struct nk_context *ctx) {
    bool r = false;
    struct nk_rect bounds =
        nk_rect(((float)GetScreenWidth() - 300) / 2,
                ((float)GetScreenHeight() - 180) / 2, 300, 180);
    if (nk_begin(ctx, "Game over", bounds, NK_WINDOW_TITLE)) {
        char outcome[128];
        if (game->outcome.outcome == Checkmate) {
            sprintf(outcome, "%s won by checkmate",
                    game->outcome.winner == PIECE_WHITE ? "White" : "Black");
        } else if (game->outcome.outcome == Draw) {
            sprintf(outcome, "Game ended in draw");
        } else if (game->outcome.outcome == Stalemate) {
            sprintf(outcome, "Game ended in stalemate");
        } else {
            puts("Outcome not found");
        }

        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, outcome, NK_TEXT_CENTERED);
        if (nk_button_label(ctx, "Play again")) {
            r = true;
        }
        nk_end(ctx);
    }

    return r;
}

void game_draw_gui(struct nk_context *ctx, Game *game) {
    bool horizontal = GetScreenWidth() < GetScreenHeight();
    int boardPixels = (horizontal ? GetScreenWidth() : GetScreenHeight());
    int tileSize = boardPixels / BOARD_SIZE;
    int marginX = (GetScreenWidth() - tileSize * BOARD_SIZE) / 2;
    int marginY = (GetScreenHeight() - tileSize * BOARD_SIZE) / 2;
    int width, height;
    if (horizontal) {
        width = GetScreenWidth();
        height = marginY;
    } else {
        width = marginX;
        height = GetScreenHeight();
    }

    struct nk_rect bounds = nk_rect(0, 0, width, height);
    if (nk_begin(ctx, "Game details", bounds, NK_WINDOW_TITLE)) {
        char buffer[128];

        NK_LINE_TEXT(LEFT, "Move count: %d", game->board->halfmoves);

        // display the FEN in an editable (selectable) box.
        nk_layout_row(ctx, NK_DYNAMIC, 30, 2, (float[]){0.25f, 0.75f});
        nk_label(ctx, "FEN: ", NK_TEXT_LEFT);
        char *fen = board_to_fen(game->board);
        int len = strlen(fen);
        nk_edit_string(
            ctx, NK_EDIT_AUTO_SELECT | NK_EDIT_CLIPBOARD | NK_EDIT_SELECTABLE,
            fen, &len, len, nk_filter_ascii);

        nk_layout_row_dynamic(ctx, 30, 1);
        if (nk_button_label(ctx, "New game")) {
            game->state = NEW_GAME;
        }
        // nk_end(ctx);
        // struct nk_rect bounds;
        // if (horizontal) {
        //     bounds = nk_rect(0, GetScreenHeight() - height, GetScreenWidth(),
        //                      height);
        // } else {
        //     bounds =
        //         nk_rect(GetScreenWidth() - width, 0, width,
        //         GetScreenHeight());
        // }
        // nk_begin(ctx, "padding", bounds, NK_WINDOW_NOT_INTERACTIVE);
        nk_end(ctx);
    }
    if (game->state == NEW_GAME) {
        int selected = draw_start_modal(ctx);
        if (selected != -1) {
            free(game->board);
            game->board = board_from_startpos();

            if (game->board == NULL) {
                exit(-1);
            }
            game->selected_square = (Square){-1, -1};
            game->possible_squares = 0;
            game->player_color = selected;
            game->state = IN_PROGRESS;
            game->outcome = (GameOutcome){Ongoing, -1};
        }
    } else if (game->state == GAME_OVER) {
        if (draw_game_over_modal(game, ctx)) {
            game->state = NEW_GAME;
        }
    }
}

void draw_board(Game *game) {
    Board *board = game->board;
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int boardPixels = (screenWidth < screenHeight ? screenWidth : screenHeight);
    int tileSize = boardPixels / BOARD_SIZE;

    int offsetX = (screenWidth - tileSize * BOARD_SIZE) / 2;
    int offsetY = (screenHeight - tileSize * BOARD_SIZE) / 2;

    for (int visual_rank = 0; visual_rank < BOARD_SIZE; visual_rank++) {
        for (int visual_file = 0; visual_file < BOARD_SIZE; visual_file++) {
            int rank = (game->player_color == PIECE_WHITE)
                           ? visual_rank
                           : (BOARD_SIZE - 1 - visual_rank);
            int file = (game->player_color == PIECE_WHITE)
                           ? visual_file
                           : (BOARD_SIZE - 1 - visual_file);

            int x = offsetX + visual_file * tileSize;
            int y = offsetY + (7 - visual_rank) * tileSize;

            bool isDark = (rank + file) % 2 == 1;
            Color tileColor = isDark ? DARKBROWN : BEIGE;
            if (game->selected_square.rank == rank &&
                game->selected_square.file == file) {
                tileColor = SELECTED_COLOR;
            } else if (game->possible_squares & (1ULL << (rank * 8 + file))) {
                tileColor = POSSIBLE_COLOR;
            }

            DrawRectangle(x, y, tileSize, tileSize, tileColor);

            uint8_t piece = board->board[BOARD_INDEX(rank, file)];
            if (GET_TYPE(piece) != EMPTY) {
                // Draw the piece
                Texture2D tex = get_piece_texture(piece);
                float scale = (float)tileSize / tex.width;
                float heightScaled = tex.height * scale;
                float yOffset = (tileSize - heightScaled) / 2.0f;

                DrawTextureEx(tex, (Vector2){x, y + yOffset}, 0.0f, scale,
                              WHITE);
            }
        }
    }
}

bool get_mouse_square(float mouseX, float mouseY, int screenWidth,
                      int screenHeight, int *outRank, int *outFile,
                      bool reverse) {
    int boardPixels = (screenWidth < screenHeight ? screenWidth : screenHeight);
    int tileSize = boardPixels / BOARD_SIZE;
    int offsetX = (screenWidth - tileSize * BOARD_SIZE) / 2;
    int offsetY = (screenHeight - tileSize * BOARD_SIZE) / 2;

    int file = (mouseX - offsetX) / tileSize;
    int rank = (mouseY - offsetY) / tileSize;

    if (file >= 0 && file < BOARD_SIZE && rank >= 0 && rank < BOARD_SIZE) {
        if (reverse) {
            rank = (BOARD_SIZE - 1 - rank);
            file = (BOARD_SIZE - 1 - file);
        }

        *outFile = file;
        *outRank = 7 - rank;

        return true;
    }

    return false;
}
