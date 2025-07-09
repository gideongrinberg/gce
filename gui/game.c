#include "game.h"
#include <stdlib.h>

#include "raylib.h"
#include "textures.h"

#include <stdio.h>
#include <string.h>

Game *new_game() {
    Game *game = malloc(sizeof(Game));
    game->board = board_from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/"
                                 "PPPBBPPP/R3K2R w KQkq - 3 10");
    game->selected_square = (Square){-1, -1};

    return game;
}

void free_game(Game *game) {
    free(game->board);
    free(game);
}

void game_update(Game *game) {
    int rank, file;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (get_mouse_square(GetMouseX(), GetMouseY(), GetScreenWidth(),
                             GetScreenHeight(), &rank, &file)) {
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

    puts(board_to_fen(game->board));
}

void game_draw(Game *game) { draw_board(game); }

void draw_board(Game *game) {
    Board *board = game->board;
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
            if (game->selected_square.rank == rank &&
                game->selected_square.file == file) {
                tileColor = SELECTED_COLOR;
            } else if (game->possible_squares & (1ULL << (rank * 8 + file))) {
                tileColor = POSSIBLE_COLOR;
            }

            int x = offsetX + file * tileSize;
            int y = offsetY + (7 - rank) * tileSize;
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

    draw_debug_text(game);
}

void draw_debug_text(Game *game) {
    char *toMove =
        game->board->moves % 2 == 0 ? "White to move" : "Black to move";
    char numMoves[32];
    sprintf(numMoves, "Moves: %d", game->board->moves);

    char castlingRights[5] = "";
    int i = 0;

    if (game->board->castling_rights & CASTLE_WHITE_KING)
        castlingRights[i++] = 'K';

    if (game->board->castling_rights & CASTLE_WHITE_QUEEN)
        castlingRights[i++] = 'Q';

    if (game->board->castling_rights & CASTLE_BLACK_KING)
        castlingRights[i++] = 'k';

    if (game->board->castling_rights & CASTLE_BLACK_QUEEN)
        castlingRights[i++] = 'q';

    castlingRights[i] = '\0';

    char enPassant[27];
    sprintf(enPassant, "EP target: %d", game->board->en_passant);

    const char *debugLines[] = {toMove, numMoves, castlingRights, enPassant};
    int fontSize = 20;
    int lineSpacing = 4;
    int padding = 10;
    int numLines = sizeof(debugLines) / sizeof(debugLines[0]);

    for (int i = 0; i < numLines; i++) {
        int posX = padding;
        int posY = padding + i * (fontSize + lineSpacing);
        DrawText(debugLines[i], posX, posY, fontSize, DARKGRAY);
    }
}

bool get_mouse_square(int mouseX, int mouseY, int screenWidth, int screenHeight,
                      int *outRank, int *outFile) {
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
