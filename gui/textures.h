#ifndef TEXTURES_H
#define TEXTURES_H

#include "raylib.h"
#include <stdint.h>

extern Texture2D piece_textures[2][7];

void load_piece_textures(void);
Texture2D get_piece_texture(uint8_t piece);
void unload_piece_textures(void);

#endif // TEXTURES_H
