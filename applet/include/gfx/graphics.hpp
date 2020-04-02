#pragma once

#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include <SDL_image.h>

#include "gfx/theme.h"
#include "gfx/font.hpp"

#define SCREEN_W 1280
#define SCREEN_H 720

namespace mc::gfx {

    void Initialise(void);
    void Finalise(void);

    SDL_Renderer *GetRenderer(void);
    void Clear(void);
    void Present(void);

    SDL_Texture *LoadTexture(const char *path);
    void DestroyTexture(SDL_Texture *texture);

    void DrawHLine(int16_t x1, int16_t x2, int16_t y, Color color);
    void DrawVLine(int16_t x, int16_t y1, int16_t y2, Color color);
    void DrawRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color);
    void DrawRoundedRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, Color color);
    void DrawCircleAA(int16_t x, int16_t y, int16_t r, Color color);
    void DrawTexture(int16_t x, int16_t y, SDL_Texture *tex, SDL_Rect* clip);
    void DrawText(int16_t x, int16_t y, TTF_Font *font, Color color, const char *text, ...);
    void DrawGlyph(int16_t x, int16_t y, TTF_Font *font, Color color, uint16_t glyph);

}
