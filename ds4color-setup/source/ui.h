/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
#pragma once
#include "ini.h"
#include "gfx.h"

/* Platform-independent button bits (main.c maps libnx buttons onto these) */
enum {
    UI_A = 1 << 0,  UI_B = 1 << 1,  UI_X = 1 << 2,  UI_Y = 1 << 3,
    UI_L = 1 << 4,  UI_R = 1 << 5,  UI_ZL = 1 << 6, UI_ZR = 1 << 7,
    UI_UP = 1 << 8, UI_DOWN = 1 << 9, UI_LEFT = 1 << 10, UI_RIGHT = 1 << 11,
    UI_PLUS = 1 << 12, UI_MINUS = 1 << 13,
};

typedef struct {
    unsigned down;      /* buttons pressed this frame */
    unsigned held;      /* buttons currently held */
    int touching;       /* finger on screen */
    int touch_started;  /* finger went down this frame */
    int tx, ty;         /* touch position in 1280x720 space */
} UiInput;

enum { PAGE_BUTTONS = 0, PAGE_LIGHTBAR = 1, NUM_PAGES };
enum { FOCUS_INPUTS = 0, FOCUS_PICKER = 1 };

typedef struct {
    McConfig cfg;
    char path[256];
    int page;            /* PAGE_* */
    int dirty;
    int load_state;      /* 0 ok, 1 file missing */
    char status[160];
    int status_frames;
    int status_error;
    int quit_armed;      /* B was pressed once with unsaved changes; press again to leave */
    int live;            /* write the ini a few times a second while editing, so Mission Control applies changes immediately */
    int live_cooldown;   /* frames until live mode may write again */

    /* Lightbar page */
    int target;          /* 0 = all players, 1..8 = player N */
    int sel;             /* selected slider: 0=H 1=S 2=V 3=brightness */
    float hsv[9][3];     /* per-target editing state: h 0..360, s 0..1, v 0..1 */
    int drag;            /* slider being dragged by touch, or -1 */
    int hold_l, hold_r;  /* frames held, for key repeat */
    int preset_idx;

    /* Buttons page */
    int sel_in;          /* selected DS4 input (MC_IN_*) */
    int focus;           /* FOCUS_INPUTS or FOCUS_PICKER */
    int pick;            /* picker cursor: slot index into the Switch button grid */
    int combo;           /* picker toggles buttons on/off (several at once) instead of replacing */
} App;

void app_init(App *a, const char *ini_path);
int  app_update(App *a, const UiInput *in);        /* returns 1 when the app should quit */
void app_draw(const App *a, unsigned char *fb, int stride_bytes);  /* RGBA8888 */
