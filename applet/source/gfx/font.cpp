#include "gfx/font.hpp"
#include <switch.h>

namespace mc::font {

    TTF_Font *Small;
    TTF_Font *Medium;
    TTF_Font *Large;
    TTF_Font *ExtSmall;
    TTF_Font *ExtMedium;
    TTF_Font *ExtLarge;

    void Initialise(void) {
        PlFontData font;
        PlFontData ext_font;
        plGetSharedFontByType(&font, PlSharedFontType_Standard);
        plGetSharedFontByType(&ext_font, PlSharedFontType_NintendoExt);

        Small       = TTF_OpenFontRW(SDL_RWFromMem(font.address, font.size), 1, 22);
        Medium      = TTF_OpenFontRW(SDL_RWFromMem(font.address, font.size), 1, 24);
        Large       = TTF_OpenFontRW(SDL_RWFromMem(font.address, font.size), 1, 28);
        ExtSmall    = TTF_OpenFontRW(SDL_RWFromMem(ext_font.address, ext_font.size), 1, 22);
        ExtMedium   = TTF_OpenFontRW(SDL_RWFromMem(ext_font.address, ext_font.size), 1, 24);
        ExtLarge    = TTF_OpenFontRW(SDL_RWFromMem(ext_font.address, ext_font.size), 1, 28);
    }

    void Finalise(void) {
        TTF_CloseFont(Small);
        TTF_CloseFont(Medium);
        TTF_CloseFont(Large);
        TTF_CloseFont(ExtSmall);
        TTF_CloseFont(ExtMedium);
        TTF_CloseFont(ExtLarge);
    }

}
