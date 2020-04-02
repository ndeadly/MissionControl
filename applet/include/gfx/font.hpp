#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

enum GlyphType
{
    GlyphType_SpinnerTopLeft        = 0xe020,
    GlyphType_SpinnerTop            = 0xe021,
    GlyphType_SpinnerTopRight       = 0xe022,
    GlyphType_SpinnerRight          = 0xe023,
    GlyphType_SpinnerBottomRight    = 0xe024,
    GlyphType_SpinnerBottom         = 0xe025,
    GlyphType_SpinnerBottomLeft     = 0xe026,
    GlyphType_SpinnerLeft           = 0xe027,

    GlyphType_AButton               = 0xe0a0,
    GlyphType_BButton               = 0xe0a1,
    GlyphType_XButton               = 0xe0a2,
    GlyphType_YButton               = 0xe0a3,
    GlyphType_LButton               = 0xe0a4,
    GlyphType_RButton               = 0xe0a5,
    GlyphType_ZLButton              = 0xe0a6,
    GlyphType_ZRButton              = 0xe0a7,
    GlyphType_SLButton              = 0xe0a8,
    GlyphType_SRButton              = 0xe0a9,
    GlyphType_JoyDpadReleased       = 0xe0aa,
    GlyphType_JoyDpadRight          = 0xe0ab,
    GlyphType_JoyDpadDown           = 0xe0ac,
    GlyphType_JoyDpadUp             = 0xe0ad,
    GlyphType_JoyDpadLeft           = 0xe0ae,
    GlyphType_JoyUpButton           = 0xe0af,
    GlyphType_JoyDownButton         = 0xe0b0,
    GlyphType_JoyLeftButton         = 0xe0b1,
    GlyphType_JoyRightButton        = 0xe0b2,
    GlyphType_PlusButton            = 0xe0b3,
    GlyphType_MinusButton           = 0xe0b4,

    GlyphType_HomeButton            = 0xe0b9,
    GlyphType_CaptureButton         = 0xe0ba,
    GlyphType_Joystick              = 0xe0c0,
    GlyphType_LeftJoystick          = 0xe0c1,
    GlyphType_RightJoystick         = 0xe0c2,
    GlyphType_JoystickPress         = 0xe0c3,
    GlyphType_LeftJoystickPress     = 0xe0c4,
    GlyphType_RightJoystickPress    = 0xe0c5,

    GlyphType_DpadReleased          = 0xe0d0,
    GlyphType_DpadUp                = 0xe0d1,
    GlyphType_DpadDown              = 0xe0d2,
    GlyphType_DpadLeft              = 0xe0d3,
    GlyphType_DpadRight             = 0xe0d4,

    GlyphType_AButtonInverted       = 0xe0e0,
    GlyphType_BButtonInverted       = 0xe0e1,
    GlyphType_XButtonInverted       = 0xe0e2,
    GlyphType_YButtonInverted       = 0xe0e3,
    GlyphType_LButtonInverted       = 0xe0e4,
    GlyphType_RButtonInverted       = 0xe0e5,
    GlyphType_ZLButtonInverted      = 0xe0e6,
    GlyphType_ZRButtonInverted      = 0xe0e7,
    GlyphType_SLButtonInverted      = 0xe0e8,
    GlyphType_SRButtonInverted      = 0xe0e9,
    GlyphType_JoyDpadReleasedInverted       = 0xe0ea,
    GlyphType_JoyUpButtonInverted           = 0xe0eb,
    GlyphType_JoyDownButtonInverted         = 0xe0ec,
    GlyphType_JoyLeftButtonInverted         = 0xe0ed,
    GlyphType_JoyRightButtonInverted        = 0xe0ee,
    GlyphType_PlusButtonInverted            = 0xe0ef,
    GlyphType_MinusButtonInverted           = 0xe0f0,

    GlyphType_HomeButtonInverted            = 0xe0f4,
    GlyphType_CaptureButtonInverted         = 0xe0f5,
    GlyphType_JoystickInverted              = 0xe100,
    GlyphType_LeftJoystickInverted          = 0xe101,
    GlyphType_RightJoystickInverted         = 0xe102,
    GlyphType_JoystickPressInverted         = 0xe103,
    GlyphType_LeftJoystickPressInverted     = 0xe104,
    GlyphType_RightJoystickPressInverted    = 0xe105,
};

namespace mc::font {

    extern TTF_Font *Small;
    extern TTF_Font *Medium;
    extern TTF_Font *Large;
    extern TTF_Font *ExtSmall;
    extern TTF_Font *ExtMedium;
    extern TTF_Font *ExtLarge;

    void Initialise(void);
    void Finalise(void);
    //void draw_text(TTF_Font *font, int x, int y, SDL_Colour colour, const char *text, ...);
    //void draw_glyph(TTF_Font *font, int x, int y, SDL_Colour colour, uint16_t glyph);
}
