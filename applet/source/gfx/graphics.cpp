#include "gfx/graphics.hpp"
//#include "byteswap.h"

namespace mc::gfx {

    namespace {

        static SDL_Window *g_window;
        static SDL_Renderer *g_renderer;

    }

    void Initialise(void) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            return;
        }

        g_window = SDL_CreateWindow("sdl2_gles2", 0, 0, SCREEN_W, SCREEN_H, 0);
        if (!g_window) {
            SDL_Quit();
        }

        g_renderer = SDL_CreateRenderer(g_window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!g_renderer) {
            SDL_Quit();
        }

        /* Initialise fonts */
        TTF_Init();

        /* Initialise image formats */
        IMG_Init(IMG_INIT_PNG);
    }

    void Finalise(void) {
        SDL_RenderClear(g_renderer);
        SDL_DestroyRenderer(g_renderer);
        SDL_DestroyWindow(g_window);

        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
    }

    SDL_Renderer *GetRenderer(void) {
        return g_renderer;
    }

    void Clear(void) {
        SDL_RenderClear(g_renderer);
    }

    void Present(void) {
        SDL_RenderPresent(g_renderer);
    }

    SDL_Texture *LoadTexture(const char *path) {
        return IMG_LoadTexture(g_renderer, path);
    }

    void DestroyTexture(SDL_Texture *texture) {
        SDL_DestroyTexture(texture);
    }

    void DrawHLine(int16_t x1, int16_t x2, int16_t y, Color color) {
        hlineColor(g_renderer, x1, x2, y, color.abgr);
    }

    void DrawVLine(int16_t x, int16_t y1, int16_t y2, Color color) {
        vlineColor(g_renderer, x, y1, y2, color.abgr);
    }

    void DrawRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) {
        boxColor(g_renderer, x1, y1, x2, y2, color.abgr);
    }

    void DrawRoundedRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, Color color) {
        roundedBoxColor(g_renderer, x1, y1, x2, y2, r, color.abgr);
    }

    void DrawCircleAA(int16_t x, int16_t y, int16_t r, Color color) {
        aacircleColor(g_renderer, x, y, r, color.abgr);
    }

    void DrawTexture(int16_t x, int16_t y, SDL_Texture *tex, SDL_Rect* clip) {
        int w, h;
        SDL_QueryTexture(tex, NULL, NULL, &w, &h);

        SDL_Rect r = {x, y, w, h}; 
        if (clip != NULL) {
            r.w = clip->w;
            r.h = clip->h;
        }

        SDL_RenderCopy(g_renderer, tex, clip, &r);
    }

    void DrawText(int16_t x, int16_t y, TTF_Font *font, Color color, const char *text, ...) {
        char buff[256];
        va_list argv;
        va_start(argv, text);
        vsnprintf(buff, sizeof(buff), text, argv);
        va_end(argv);

        SDL_Surface *surf = TTF_RenderUTF8_Blended_Wrapped(font, buff, color.sdl, SCREEN_W);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(g_renderer, surf);
        SDL_Rect pos = {x, y, surf->w, surf->h};

        SDL_RenderCopy(g_renderer, tex, NULL, &pos);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }

    void DrawGlyph(int16_t x, int16_t y, TTF_Font *font, Color color, uint16_t glyph) {
        SDL_Surface *surf = TTF_RenderGlyph_Blended(font, glyph, color.sdl);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(g_renderer, surf);
        SDL_Rect pos = {x, y, surf->w, surf->h};

        SDL_RenderCopy(g_renderer, tex, NULL, &pos);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }

}
