#pragma once

#include <switch.h>
#include <SDL.h>

typedef union 
{
    uint32_t abgr;

    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    SDL_Color sdl;
} Color;

typedef struct {
    ColorSetId  colorSetId;
    Color backgroundColor;      // main background
    Color backgroundColor2;     // sidebar/dialog background
    Color backgroundColor3;     // selected button background
    Color foregroundColor;      // text, separators
    Color foregroundColor2;     // greyed text
    Color highlightColor;
    Color dialogOverlayColor;
    Color dialogBackgroundColor;
    Color dialogSeparatorColor;
    Color selectionColor;
    Color glowColor;
} ColorSet;

#ifdef __cplusplus
extern "C" {
#endif

const ColorSet *getCurrentColorSet(void);

#ifdef __cplusplus
}
#endif
