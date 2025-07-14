#ifndef TEXTURES_H
#define TEXTURES_H
#ifdef __cplusplus
extern "C" {
#endif
#include "raylib.h"
#include <stdlib.h>

void loadPieceTextures(void);
Texture2D getPieceTexture(uint8_t piece);
void unloadPieceTextures(void);

#ifdef __cplusplus
}
#endif
#endif // TEXTURES_H