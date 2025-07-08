#include "textures.h"
#include "core.h"
#include "images.h"
#include <stddef.h>
Texture2D piece_textures[2][7];

void load_piece_textures(void) {
    struct {
        const unsigned char *data;
        unsigned int len;
    } sources[2][7] = {{{NULL, 0},
                        {wpawn_png, wpawn_png_len},
                        {wknight_png, wknight_png_len},
                        {wbishop_png, wbishop_png_len},
                        {wrook_png, wrook_png_len},
                        {wqueen_png, wqueen_png_len},
                        {wking_png, wking_png_len}},
                       {{NULL, 0},
                        {bpawn_png, bpawn_png_len},
                        {bknight_png, bknight_png_len},
                        {bbishop_png, bbishop_png_len},
                        {brook_png, brook_png_len},
                        {bqueen_png, bqueen_png_len},
                        {bking_png, bking_png_len}}};

    for (int color = 0; color < 2; color++) {
        for (int type = 1; type <= 6; type++) {
            Image img = LoadImageFromMemory(".png", sources[color][type].data,
                                            sources[color][type].len);
            piece_textures[color][type] = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    }
}

Texture2D get_piece_texture(uint8_t piece) {
    int type = GET_TYPE(piece);
    int color = GET_COLOR(piece);
    if (type == EMPTY)
        return (Texture2D){0};
    int color_idx = (color == PIECE_BLACK) ? 1 : 0;
    return piece_textures[color_idx][type];
}

void unload_piece_textures(void) {
    for (int color = 0; color < 2; color++) {
        for (int type = 1; type <= 6; type++) {
            UnloadTexture(piece_textures[color][type]);
        }
    }
}
