#include "application.hpp"
#include "bluetooth/core.hpp"
#include "gfx/graphics.hpp"
#include "scenes/testcontrollerscene.hpp"

enum ButtonSpriteType {
    Sprite_ButtonA,
    Sprite_ButtonB,
    Sprite_ButtonX,
    Sprite_ButtonY,
    Sprite_ButtonPlus,
    Sprite_ButtonMinus,
    Sprite_ButtonHome,
    Sprite_ButtonCapture,
    Sprite_ButtonL,
    Sprite_ButtonR,
    Sprite_DPad,
    Sprite_LeftStick,
    Sprite_RightStick
};

/* clip rects for button sprites*/
static const SDL_Rect sprites[] = { 
    {564, 123, 52, 52},
    {509, 170, 52, 52},
    {509, 75, 52, 52},
    {454, 123, 52, 52},
    {421, 80, 30, 30},
    {248, 80, 30, 30},
    {384, 133, 30, 30},
    {287, 135, 26, 26},
    {76, 0, 169, 56},
    {453, 0, 169, 56},
    {189, 192, 104, 105},
    {116, 107, 81, 81},
    {402, 204, 81, 81}
};

TestControllerScene::TestControllerScene() {
    m_imgswitchpro = mc::gfx::LoadTexture("romfs:/images/switchpro.png");
}

TestControllerScene::~TestControllerScene() {
    mc::gfx::DestroyTexture(m_imgswitchpro);
}

void TestControllerScene::draw(void) {
    //int ctrl_x_offs = 500;
    //int ctrl_y_offs = 110;

    int m_offsetX = 500;
    int m_offsetY = 110;

    int texWidth = 700;
    int texHeight = 488;

    int highlightOffset = texWidth;
    int themeOffset = mc::app::theme->colorSetId * texHeight;

    // Draw Switch pro Controller background;
    SDL_Rect clip = {0, 0, texWidth, texHeight};
    mc::gfx::DrawTexture(m_offsetX, m_offsetY, m_imgswitchpro, &clip);

    if (this->hasFocus()) {

        /* Scan for user input */
        mc::app::UserInput input;
        mc::app::ScanInput(&input);

        uint16_t r = 75;
        uint16_t x;
        uint16_t y;
        uint16_t dx;
        uint16_t dy;

        /* Draw analog stick coordinates */
        x = 750;
        y = 550;
        mc::gfx::DrawHLine(x-r, x+r, y, mc::app::theme->foregroundColor2);
        mc::gfx::DrawVLine(x, y-r, y+r, mc::app::theme->foregroundColor2);
        mc::gfx::DrawCircleAA(x, y, r, mc::app::theme->foregroundColor2);
        // draw stick coordinates
        dx = x + (int)(r * ((float)input.leftStick.dx / JOYSTICK_MAX));
        dy = y + (int)(r * (-(float)input.leftStick.dy / JOYSTICK_MAX));
        mc::gfx::DrawHLine(dx-3, dx+3, dy, mc::app::theme->highlightColor);
        mc::gfx::DrawVLine(dx, dy-3, dy+3, mc::app::theme->highlightColor);

        x = 950;
        y = 550;
        mc::gfx::DrawHLine(x-r, x+r, y, mc::app::theme->foregroundColor2);
        mc::gfx::DrawVLine(x, y-r, y+r, mc::app::theme->foregroundColor2);
        mc::gfx::DrawCircleAA(x, y, r, mc::app::theme->foregroundColor2);
        // draw stick coordinates
        dx = x + (int)(r * ((float)input.rightStick.dx / JOYSTICK_MAX));
        dy = y + (int)(r * (-(float)input.rightStick.dy / JOYSTICK_MAX));
        mc::gfx::DrawHLine(dx-3, dx+3, dy, mc::app::theme->highlightColor);
        mc::gfx::DrawVLine(dx, dy-3, dy+3, mc::app::theme->highlightColor);

        if (input.kHeld & KEY_A) {
            SDL_Rect clip = sprites[Sprite_ButtonA];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonA].x, 
                                 m_offsetY + sprites[Sprite_ButtonA].y, 
                                 m_imgswitchpro, 
                                 &clip);
        }

        if (input.kHeld & KEY_B) {
            SDL_Rect clip = sprites[Sprite_ButtonB];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonB].x, 
                                 m_offsetY + sprites[Sprite_ButtonB].y, 
                                 m_imgswitchpro, 
                                 &clip);
        }

        if (input.kHeld & KEY_X) {
            SDL_Rect clip = sprites[Sprite_ButtonX];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonX].x, 
                                 m_offsetY + sprites[Sprite_ButtonX].y,
                                 m_imgswitchpro, 
                                 &clip);
        }

        if (input.kHeld & KEY_Y) {
            SDL_Rect clip = sprites[Sprite_ButtonY];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonY].x, 
                                 m_offsetY + sprites[Sprite_ButtonY].y, 
                                 m_imgswitchpro, 
                                 &clip);
        }

        if ( (input.kHeld & KEY_DUP) || (input.kHeld & KEY_DDOWN) || (input.kHeld & KEY_DLEFT) || (input.kHeld & KEY_DRIGHT)) {
            SDL_Rect clip = sprites[Sprite_DPad];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_DPad].x, 
                                 m_offsetY + sprites[Sprite_DPad].y, 
                                 m_imgswitchpro, 
                                 &clip);

            if (input.kHeld & KEY_DUP) {
                mc::gfx::DrawGlyph(m_offsetX + 189 + 41, m_offsetY + 192 + 5, 
                    mc::font::ExtMedium, 
                    mc::app::theme->foregroundColor, 
                    GlyphType_JoyUpButton
                );
            }

            if (input.kHeld & KEY_DDOWN) {
                mc::gfx::DrawGlyph(m_offsetX + 189 + 41, m_offsetY + 192 + 75, 
                    mc::font::ExtMedium, 
                    mc::app::theme->foregroundColor, 
                    GlyphType_JoyDownButton
                );
            }

            if (input.kHeld & KEY_DLEFT) {
                mc::gfx::DrawGlyph(m_offsetX + 189 + 5, m_offsetY + 192 + 40, 
                    mc::font::ExtMedium, 
                    mc::app::theme->foregroundColor, 
                    GlyphType_JoyLeftButton
                );
            }

            if (input.kHeld & KEY_DRIGHT) {
                mc::gfx::DrawGlyph(m_offsetX + 189 + 75, m_offsetY + 192 + 40, 
                    mc::font::ExtMedium, 
                    mc::app::theme->foregroundColor, 
                    GlyphType_JoyRightButton
                );
            }
        }

        if ( (input.kHeld & KEY_L) || (input.kHeld & KEY_ZL) ){
            SDL_Rect clip = sprites[Sprite_ButtonL];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            if (input.kHeld & KEY_L) {
                mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonL].x, 
                                     m_offsetY + sprites[Sprite_ButtonL].y, 
                                     m_imgswitchpro, 
                                     &clip);
                mc::gfx::DrawGlyph(m_offsetX + 50, m_offsetY + 10, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_LButton);
            }
            if (input.kHeld & KEY_ZL) {
                mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonL].x, 
                                     m_offsetY + sprites[Sprite_ButtonL].y, 
                                     m_imgswitchpro, 
                                     &clip);
                mc::gfx::DrawGlyph(m_offsetX + 50, m_offsetY - 10, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_ZLButton);
            }
        }

        if ( (input.kHeld & KEY_R) || (input.kHeld & KEY_ZR) ) {
            SDL_Rect clip = sprites[Sprite_ButtonR];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            if (input.kHeld & KEY_R) {
                mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonR].x, 
                                     m_offsetY + sprites[Sprite_ButtonR].y, 
                                     m_imgswitchpro, 
                                     &clip);
                mc::gfx::DrawGlyph(m_offsetX + 700 - 50 - 20, m_offsetY + 10, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_RButton);
            }
            if (input.kHeld & KEY_ZR) {
                mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonR].x, 
                                     m_offsetY + sprites[Sprite_ButtonR].y, 
                                     m_imgswitchpro, 
                                     &clip);
                mc::gfx::DrawGlyph(m_offsetX + 700 - 50 - 20, m_offsetY - 10, mc::font::ExtMedium, mc::app::theme->foregroundColor, GlyphType_ZRButton);
            }
        }

        if (input.kHeld & KEY_PLUS) {
            SDL_Rect clip = sprites[Sprite_ButtonPlus];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonPlus].x, 
                                 m_offsetY + sprites[Sprite_ButtonPlus].y, 
                                 m_imgswitchpro, 
                                 &clip);
        }

        if (input.kHeld & KEY_MINUS) {
            SDL_Rect clip = sprites[Sprite_ButtonMinus];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_ButtonMinus].x, 
                                 m_offsetY + sprites[Sprite_ButtonMinus].y, 
                                 m_imgswitchpro, 
                                 &clip);
        }

        if ((input.kHeld & KEY_LSTICK) || (hypot(input.leftStick.dx, input.leftStick.dy) > 0)) {
            SDL_Rect clip = sprites[Sprite_LeftStick];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_LeftStick].x, 
                                 m_offsetY + sprites[Sprite_LeftStick].y, 
                                 m_imgswitchpro, 
                                 &clip);
            if (input.kHeld & KEY_LSTICK) {
                mc::gfx::DrawGlyph(m_offsetX + sprites[Sprite_LeftStick].x + 28, 
                                   m_offsetY + sprites[Sprite_LeftStick].y + 22, 
                                   mc::font::ExtMedium, 
                                   mc::app::theme->foregroundColor, 
                                   GlyphType_LeftJoystickPress);
            }
        }

        if ((input.kHeld & KEY_RSTICK) || (hypot(input.rightStick.dx, input.rightStick.dy) > 0)) {
            SDL_Rect clip = sprites[Sprite_RightStick];
            clip.x += highlightOffset;
            clip.y += themeOffset;
            mc::gfx::DrawTexture(m_offsetX + sprites[Sprite_RightStick].x, 
                                 m_offsetY + sprites[Sprite_RightStick].y, 
                                 m_imgswitchpro, 
                                 &clip);
            if (input.kHeld & KEY_RSTICK) {
                mc::gfx::DrawGlyph(m_offsetX + sprites[Sprite_RightStick].x + 28, 
                                   m_offsetY + sprites[Sprite_RightStick].y + 22, 
                                   mc::font::ExtMedium, 
                                   mc::app::theme->foregroundColor, 
                                   GlyphType_RightJoystickPress);
            }
        }
    }
}

void TestControllerScene::handleInput(const mc::app::UserInput *input) {

}