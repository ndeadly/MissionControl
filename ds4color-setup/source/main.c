/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
/* DS4 Controller Setup - companion app for the patched Mission Control sysmodule.
 * Edits /config/MissionControl/missioncontrol.ini. */
#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "ui.h"

#define INI_DIR1 "sdmc:/config"
#define INI_DIR2 "sdmc:/config/MissionControl"
#define INI_PATH "sdmc:/config/MissionControl/missioncontrol.ini"

static unsigned map_buttons(u64 k) {
    unsigned m = 0;
    if (k & HidNpadButton_A)     m |= UI_A;
    if (k & HidNpadButton_B)     m |= UI_B;
    if (k & HidNpadButton_X)     m |= UI_X;
    if (k & HidNpadButton_Y)     m |= UI_Y;
    if (k & HidNpadButton_L)     m |= UI_L;
    if (k & HidNpadButton_R)     m |= UI_R;
    if (k & HidNpadButton_ZL)    m |= UI_ZL;
    if (k & HidNpadButton_ZR)    m |= UI_ZR;
    if (k & (HidNpadButton_Up    | HidNpadButton_StickLUp))    m |= UI_UP;
    if (k & (HidNpadButton_Down  | HidNpadButton_StickLDown))  m |= UI_DOWN;
    if (k & (HidNpadButton_Left  | HidNpadButton_StickLLeft))  m |= UI_LEFT;
    if (k & (HidNpadButton_Right | HidNpadButton_StickLRight)) m |= UI_RIGHT;
    if (k & HidNpadButton_Plus)  m |= UI_PLUS;
    if (k & HidNpadButton_Minus) m |= UI_MINUS;
    return m;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    mkdir(INI_DIR1, 0777);
    mkdir(INI_DIR2, 0777);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    hidInitializeTouchScreen();

    NWindow *win = nwindowGetDefault();
    Framebuffer fb;
    framebufferCreate(&fb, win, UI_W, UI_H, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    static App app;   /* large-ish; keep off the stack */
    app_init(&app, INI_PATH);

    int was_touching = 0;
    while (appletMainLoop()) {
        padUpdate(&pad);

        UiInput in;
        memset(&in, 0, sizeof in);
        in.down = map_buttons(padGetButtonsDown(&pad));
        in.held = map_buttons(padGetButtons(&pad));

        HidTouchScreenState ts = {0};
        if (hidGetTouchScreenStates(&ts, 1) && ts.count > 0) {
            in.touching = 1;
            in.tx = (int)ts.touches[0].x;
            in.ty = (int)ts.touches[0].y;
        }
        in.touch_started = in.touching && !was_touching;
        was_touching = in.touching;

        if (app_update(&app, &in)) break;

        u32 stride;
        u8 *buf = (u8 *)framebufferBegin(&fb, &stride);
        app_draw(&app, buf, (int)stride);
        framebufferEnd(&fb);
    }

    framebufferClose(&fb);
    return 0;
}
