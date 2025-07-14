#include "textures.h"
#include "assets.h"
#include "engine.h"

#include <assert.h>
#include <stdio.h>

Texture2D pieceTextures[2][6];

void loadPieceTextures(void) {
    struct {
        const unsigned char *data;
        unsigned int len;
    } sources[2][6] = {{{wpawn_png, wpawn_png_len},
                        {wknight_png, wknight_png_len},
                        {wbishop_png, wbishop_png_len},
                        {wrook_png, wrook_png_len},
                        {wqueen_png, wqueen_png_len},
                        {wking_png, wking_png_len}},
                       {{bpawn_png, bpawn_png_len},
                        {bknight_png, bknight_png_len},
                        {bbishop_png, bbishop_png_len},
                        {brook_png, brook_png_len},
                        {bqueen_png, bqueen_png_len},
                        {bking_png, bking_png_len}}};

    for (int color = 0; color < 2; color++) {
        for (int type = 0; type < 6; type++) {
            printf("Loading texture: color %d, type %d\n", color, type);
            Image img = LoadImageFromMemory(".png", sources[color][type].data,
                                            sources[color][type].len);
            pieceTextures[color][type] = LoadTextureFromImage(img);
            SetTextureFilter(pieceTextures[color][type],
                             TEXTURE_FILTER_BILINEAR);
            UnloadImage(img);
        }
    }
}

Texture2D getPieceTexture(uint8_t piece) {
    int type = PIECE_TYPE(piece);
    int color = PIECE_COLOR(piece);
    int color_idx = (color == PIECE_BLACK) ? 1 : 0;
    printf("type: %d, color: %d, color_idx: %d\n", type, color, color_idx);
    assert(type >= 0 && type < 6);
    assert(color_idx == 0 || color_idx == 1);
    return pieceTextures[color_idx][type];
}

void unloadPieceTextures(void) {
    for (int color = 0; color < 2; color++) {
        for (int type = 1; type < 6; type++) {
            UnloadTexture(pieceTextures[color][type]);
        }
    }
}
