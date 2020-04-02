#include "gfx/theme.h"

const static ColorSet colorSetLight = { 
    .colorSetId             = ColorSetId_Light,
    .backgroundColor        = RGBA8(0xeb, 0xeb, 0xeb, 0xff),
    .backgroundColor2       = RGBA8(0xf0, 0xf0, 0xf0, 0xff),
    .backgroundColor3       = RGBA8(0xfd, 0xfd, 0xfd, 0xff),
    .foregroundColor        = RGBA8(0x2d, 0x2d, 0x2d, 0xff),
    .foregroundColor2       = RGBA8(0x80, 0x80, 0x80, 0xff),
    .highlightColor         = RGBA8(0x32, 0x50, 0xee, 0xff),
    .dialogOverlayColor     = RGBA8(0x07, 0x11, 0x1e, 0xbf),
    .dialogBackgroundColor  = RGBA8(0xf0, 0xf0, 0xf0, 0xff),
    .dialogSeparatorColor   = RGBA8(0x67, 0x67, 0x67, 0xff),
    .selectionColor         = RGBA8(0xfd, 0xfd, 0xfd, 0xff),
    .glowColor              = RGBA8(0x66, 0xdc, 0xee, 0xff),
};

const static ColorSet colorSetDark = { 
    .colorSetId             = ColorSetId_Dark,
    .backgroundColor        = RGBA8(0x2d, 0x2d, 0x2d, 0xff),
    .backgroundColor2       = RGBA8(0x32, 0x32, 0x32, 0xff),
    .backgroundColor3       = RGBA8(0x3a, 0x3d, 0x42, 0xff),
    .foregroundColor        = RGBA8(0xff, 0xff, 0xff, 0xff),
    .foregroundColor2       = RGBA8(0xa0, 0xa0, 0xa0, 0xa0),
    .highlightColor         = RGBA8(0x01, 0xff, 0xc9, 0xff),
    .dialogOverlayColor     = RGBA8(0x07, 0x11, 0x1e, 0xbf),
    .dialogBackgroundColor  = RGBA8(0x46, 0x46, 0x46, 0xff),
    .dialogSeparatorColor   = RGBA8(0x67, 0x67, 0x67, 0xff),
    .selectionColor         = RGBA8(0x21, 0x22, 0x27, 0xff),
    .glowColor              = RGBA8(0x66, 0xdc, 0xee, 0xff),
};

const ColorSet *getCurrentColorSet(void) {
    ColorSetId id;
    setsysGetColorSetId(&id);

    switch (id) {
        case ColorSetId_Light:
            return &colorSetLight;
        default:
            return &colorSetDark;
    }
}
