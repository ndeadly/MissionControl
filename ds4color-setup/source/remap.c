/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
/* Buttons page: pick a DS4 control (on the drawn controller or in the grid), then pick the Switch button it should press. */
#include "pages.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ layout */
#define DG_X 40          /* controller diagram panel */
#define DG_Y 84
#define DG_W 600
#define DG_H 386

#define CH_X0 40         /* grid of all 20 inputs and their current binding */
#define CH_Y0 484
#define CH_W 116
#define CH_H 38
#define CH_SX 121
#define CH_SY 44
#define CH_COLS 5
#define CH_ROWS 4

#define RP_X 660         /* right panel: the Switch button picker */
#define RP_Y 84
#define RP_W 580
#define RP_H 460

#define PK_X 676
#define PK_Y 212
#define PK_W 104
#define PK_H 46
#define PK_SX 112
#define PK_SY 54

#define COMBO_X 676
#define COMBO_Y 452
#define COMBO_W 200
#define COMBO_H 36

#define BTN_Y 556
#define BTN_H 54
#define BTN_W 186
static const int btn_x[3] = { 660, 857, 1054 };   /* Save, Reset input, Reset all */

/* Which input sits in which grid cell (5 columns x 4 rows) - also the D-pad navigation order */
static const int grid_order[CH_COLS * CH_ROWS] = {
    MC_IN_L2,       MC_IN_L1,        MC_IN_R1,         MC_IN_R2,         MC_IN_PS,
    MC_IN_DPAD_UP,  MC_IN_DPAD_DOWN, MC_IN_DPAD_LEFT,  MC_IN_DPAD_RIGHT, MC_IN_SHARE,
    MC_IN_TRIANGLE, MC_IN_CIRCLE,    MC_IN_CROSS,      MC_IN_SQUARE,     MC_IN_OPTIONS,
    MC_IN_L3,       MC_IN_R3,        MC_IN_PAD_LEFT,   MC_IN_PAD_CENTRE, MC_IN_PAD_RIGHT,
};

/* Picker grid (5x4): a Switch button per slot */
#define PK_NONE 100
#define PK_EMPTY (-1)
static const int pk_slots[CH_COLS * CH_ROWS] = {
    MC_BTN_A,      MC_BTN_B,      MC_BTN_X,       MC_BTN_Y,     MC_BTN_PLUS,
    MC_BTN_L,      MC_BTN_R,      MC_BTN_ZL,      MC_BTN_ZR,    MC_BTN_MINUS,
    MC_BTN_UP,     MC_BTN_DOWN,   MC_BTN_LEFT,    MC_BTN_RIGHT, MC_BTN_HOME,
    MC_BTN_STICKL, MC_BTN_STICKR, MC_BTN_CAPTURE, PK_NONE,      PK_EMPTY,
};

/* ------------------------------------------------------------------ controller diagram geometry (panel-local) */
enum { HK_PILL, HK_ARM_UP, HK_ARM_DOWN, HK_ARM_LEFT, HK_ARM_RIGHT,
       HK_TRI, HK_CIRCLE, HK_CROSS, HK_SQUARE, HK_STICK, HK_PS, HK_ZONE };

typedef struct { int input, kind, x, y, w, h, pad; const char *txt; } Hot;

/* Touchpad zones come first so they win over neighbouring controls' touch padding */
static const Hot hots[MC_NUM_INPUTS] = {
    { MC_IN_PAD_LEFT,    HK_ZONE,   170,  78,  39, 120, 0, "L" },
    { MC_IN_PAD_CENTRE,  HK_ZONE,   209,  78, 182, 120, 0, "TOUCHPAD" },
    { MC_IN_PAD_RIGHT,   HK_ZONE,   391,  78,  39, 120, 0, "R" },
    { MC_IN_L2,          HK_PILL,    70,  14, 130,  30, 4, "L2" },
    { MC_IN_R2,          HK_PILL,   400,  14, 130,  30, 4, "R2" },
    { MC_IN_L1,          HK_PILL,    58,  48, 150,  22, 4, "L1" },
    { MC_IN_R1,          HK_PILL,   392,  48, 150,  22, 4, "R1" },
    { MC_IN_SHARE,       HK_PILL,   100,  80,  52,  22, 8, "SHARE" },
    { MC_IN_OPTIONS,     HK_PILL,   432,  80,  68,  22, 8, "OPTIONS" },
    { MC_IN_DPAD_UP,     HK_ARM_UP,    86, 118, 32, 38, 3, "" },
    { MC_IN_DPAD_DOWN,   HK_ARM_DOWN,  86, 188, 32, 38, 3, "" },
    { MC_IN_DPAD_LEFT,   HK_ARM_LEFT,  48, 156, 38, 32, 3, "" },
    { MC_IN_DPAD_RIGHT,  HK_ARM_RIGHT, 118, 156, 38, 32, 3, "" },
    { MC_IN_TRIANGLE,    HK_TRI,    479, 109,  38,  38, 4, "" },
    { MC_IN_CIRCLE,      HK_CIRCLE, 523, 153,  38,  38, 4, "" },
    { MC_IN_CROSS,       HK_CROSS,  479, 197,  38,  38, 4, "" },
    { MC_IN_SQUARE,      HK_SQUARE, 435, 153,  38,  38, 4, "" },
    { MC_IN_L3,          HK_STICK,  180, 228,  68,  68, 2, "L3" },
    { MC_IN_R3,          HK_STICK,  352, 228,  68,  68, 2, "R3" },
    { MC_IN_PS,          HK_PS,     287, 255,  26,  26, 8, "PS" },
};

#define COL_PART     0x4a5266   /* buttons on the drawn controller */
#define COL_PART_DK  0x1d2027   /* touchpad, sticks */
#define COL_BODY     0x3a4050
#define COL_BODY_OUT 0x14161c

static int grid_pos(int input) {
    for (int i = 0; i < CH_COLS * CH_ROWS; i++) if (grid_order[i] == input) return i;
    return 0;
}

static int slot_of_button(int b) {
    for (int i = 0; i < CH_COLS * CH_ROWS; i++) if (pk_slots[i] == b) return i;
    return 0;
}

/* Where the picker cursor starts for an input: its first bound button, or "None" */
static int pick_start(const App *a) {
    int m = mc_binding(&a->cfg, a->sel_in);
    for (int b = 0; b < MC_NUM_BUTTONS; b++) if (m & MC_BIT(b)) return slot_of_button(b);
    return slot_of_button(PK_NONE);
}

static int is_custom(const App *a, int input) {
    return mc_binding(&a->cfg, input) != mc_input_default(input);
}

/* ------------------------------------------------------------------ editing */
static void set_binding(App *a, int input, int mask) {
    a->cfg.map[input] = (mask == mc_input_default(input)) ? -1 : mask;
    a->dirty = 1;
}

static void assign_slot(App *a, int slot) {
    int s = pk_slots[slot];
    if (s == PK_EMPTY) return;
    int cur = mc_binding(&a->cfg, a->sel_in);
    int m;
    if (s == PK_NONE) m = 0;
    else if (a->combo) m = cur ^ MC_BIT(s);
    else m = MC_BIT(s);
    set_binding(a, a->sel_in, m);
}

static void reset_input(App *a) {
    a->cfg.map[a->sel_in] = -1;
    a->dirty = 1;
    char buf[96];
    snprintf(buf, sizeof buf, "%s reset to its default.", mc_input_label(a->sel_in));
    ui_set_status(a, 0, buf);
}

static void reset_all(App *a) {
    for (int i = 0; i < MC_NUM_INPUTS; i++) a->cfg.map[i] = -1;
    a->dirty = 1;
    ui_set_status(a, 0, "All buttons reset to their defaults.");
}

static void move_input(App *a, int dx, int dy) {
    int gi = grid_pos(a->sel_in);
    int r = gi / CH_COLS, c = gi % CH_COLS;
    c = (c + dx + CH_COLS) % CH_COLS;
    r = (r + dy + CH_ROWS) % CH_ROWS;
    a->sel_in = grid_order[r * CH_COLS + c];
}

static void move_pick(App *a, int dx, int dy) {
    int r = a->pick / CH_COLS, c = a->pick % CH_COLS;
    do {
        c = (c + dx + CH_COLS) % CH_COLS;
        r = (r + dy + CH_ROWS) % CH_ROWS;
    } while (pk_slots[r * CH_COLS + c] == PK_EMPTY);
    a->pick = r * CH_COLS + c;
}

/* Which input is under this screen position on the drawn controller, or -1 */
static int hot_at(int x, int y) {
    x -= DG_X; y -= DG_Y;
    for (int i = 0; i < MC_NUM_INPUTS; i++) {
        const Hot *h = &hots[i];
        if (ui_in_rect(x, y, h->x - h->pad, h->y - h->pad, h->w + 2 * h->pad, h->h + 2 * h->pad)) return h->input;
    }
    return -1;
}

/* ------------------------------------------------------------------ page: init / update */
void remap_init(App *a) {
    a->sel_in = MC_IN_CROSS;
    a->focus = FOCUS_INPUTS;
    a->pick = 0;
    a->combo = 0;
}

int remap_update(App *a, const UiInput *in) {
    int used_b = 0;
    int dx = ((in->down & UI_RIGHT) ? 1 : 0) - ((in->down & UI_LEFT) ? 1 : 0);
    int dy = ((in->down & UI_DOWN) ? 1 : 0) - ((in->down & UI_UP) ? 1 : 0);

    if (a->focus == FOCUS_INPUTS) {
        if (dx || dy) move_input(a, dx, dy);
        if (in->down & UI_A) { a->focus = FOCUS_PICKER; a->pick = pick_start(a); }
    } else {
        if (dx || dy) move_pick(a, dx, dy);
        if (in->down & UI_A) {
            assign_slot(a, a->pick);
            if (!a->combo || pk_slots[a->pick] == PK_NONE) a->focus = FOCUS_INPUTS;
        }
        if (in->down & UI_B) { a->focus = FOCUS_INPUTS; used_b = 1; }
    }

    if (in->down & UI_X) {
        if (in->held & UI_ZL) reset_all(a); else reset_input(a);
    }
    if (in->down & UI_Y) a->combo = !a->combo;

    if (in->touch_started) {
        int x = in->tx, y = in->ty;

        int h = hot_at(x, y);
        if (h >= 0) { a->sel_in = h; a->focus = FOCUS_INPUTS; }

        for (int i = 0; i < CH_COLS * CH_ROWS; i++) {
            int cx = CH_X0 + (i % CH_COLS) * CH_SX, cy = CH_Y0 + (i / CH_COLS) * CH_SY;
            if (ui_in_rect(x, y, cx, cy, CH_W, CH_H)) { a->sel_in = grid_order[i]; a->focus = FOCUS_INPUTS; }
        }

        for (int i = 0; i < CH_COLS * CH_ROWS; i++) {
            int px = PK_X + (i % CH_COLS) * PK_SX, py = PK_Y + (i / CH_COLS) * PK_SY;
            if (pk_slots[i] != PK_EMPTY && ui_in_rect(x, y, px, py, PK_W, PK_H)) {
                assign_slot(a, i);
                a->focus = FOCUS_INPUTS;
            }
        }

        if (ui_in_rect(x, y, COMBO_X, COMBO_Y, COMBO_W, COMBO_H)) a->combo = !a->combo;
        if (ui_in_rect(x, y, btn_x[0], BTN_Y, BTN_W, BTN_H)) ui_save(a);
        if (ui_in_rect(x, y, btn_x[1], BTN_Y, BTN_W, BTN_H)) reset_input(a);
        if (ui_in_rect(x, y, btn_x[2], BTN_Y, BTN_W, BTN_H)) reset_all(a);
    }
    return used_b;
}

/* ------------------------------------------------------------------ drawing */
static void chevron(unsigned char *fb, int stride, int cx, int cy, int kind, int col) {
    int d = 7;
    switch (kind) {
    case HK_ARM_UP:    gfx_line(fb, stride, cx - d, cy + 4, cx, cy - 4, 3, col); gfx_line(fb, stride, cx, cy - 4, cx + d, cy + 4, 3, col); break;
    case HK_ARM_DOWN:  gfx_line(fb, stride, cx - d, cy - 4, cx, cy + 4, 3, col); gfx_line(fb, stride, cx, cy + 4, cx + d, cy - 4, 3, col); break;
    case HK_ARM_LEFT:  gfx_line(fb, stride, cx + 4, cy - d, cx - 4, cy, 3, col); gfx_line(fb, stride, cx - 4, cy, cx + 4, cy + d, 3, col); break;
    default:           gfx_line(fb, stride, cx - 4, cy - d, cx + 4, cy, 3, col); gfx_line(fb, stride, cx + 4, cy, cx - 4, cy + d, 3, col); break;
    }
}

static void draw_symbol(unsigned char *fb, int stride, int cx, int cy, int kind, int col) {
    switch (kind) {
    case HK_TRI:
        gfx_line(fb, stride, cx, cy - 9, cx - 9, cy + 7, 3, col);
        gfx_line(fb, stride, cx, cy - 9, cx + 9, cy + 7, 3, col);
        gfx_line(fb, stride, cx - 9, cy + 7, cx + 9, cy + 7, 3, col);
        break;
    case HK_CIRCLE: gfx_ring(fb, stride, cx, cy, 9, 3, col); break;
    case HK_CROSS:
        gfx_line(fb, stride, cx - 8, cy - 8, cx + 8, cy + 8, 3, col);
        gfx_line(fb, stride, cx - 8, cy + 8, cx + 8, cy - 8, 3, col);
        break;
    default: gfx_frame(fb, stride, cx - 8, cy - 8, 16, 16, 3, col); break;
    }
}

static int symbol_colour(int kind) {
    switch (kind) {
    case HK_TRI:    return 0x4fd1a5;
    case HK_CIRCLE: return 0xff6b7a;
    case HK_CROSS:  return 0x6aa8ff;
    default:        return 0xe38ad6;
    }
}

static void draw_hot(const App *a, unsigned char *fb, int stride, const Hot *h) {
    int sel = (h->input == a->sel_in);
    int dim_sel = sel && a->focus == FOCUS_PICKER;     /* still selected, but the picker has the focus */
    int x = DG_X + h->x, y = DG_Y + h->y, w = h->w, hh = h->h;
    int cx = x + w / 2, cy = y + hh / 2;
    int hi = sel ? COL_ACCENT : 0;
    if (dim_sel) hi = gfx_mix(COL_ACCENT, COL_BODY, 110);
    int fg = sel ? COL_ONACC : COL_TEXT;

    switch (h->kind) {
    case HK_PILL:
        gfx_rrect(fb, stride, x, y, w, hh, hh / 2 > 12 ? 12 : hh / 2, sel ? hi : COL_PART);
        gfx_text_center(fb, stride, cx, y + (hh - 16) / 2, 1, fg, h->txt);
        break;
    case HK_ARM_UP: case HK_ARM_DOWN: case HK_ARM_LEFT: case HK_ARM_RIGHT:
        gfx_rrect(fb, stride, x, y, w, hh, 6, sel ? hi : COL_PART);
        chevron(fb, stride, cx, cy, h->kind, sel ? COL_ONACC : 0xaeb6c8);
        break;
    case HK_TRI: case HK_CIRCLE: case HK_CROSS: case HK_SQUARE:
        gfx_circle(fb, stride, cx, cy, 19, sel ? hi : COL_PART_DK);
        draw_symbol(fb, stride, cx, cy, h->kind, sel ? COL_ONACC : symbol_colour(h->kind));
        break;
    case HK_STICK:
        gfx_circle(fb, stride, cx, cy, 34, sel ? hi : COL_PART_DK);
        gfx_circle(fb, stride, cx, cy, 24, sel ? gfx_mix(hi, 0xffffff, 40) : COL_PART);
        gfx_text_center(fb, stride, cx, cy - 8, 1, fg, h->txt);
        break;
    case HK_PS:
        gfx_circle(fb, stride, cx, cy, 13, sel ? hi : COL_PART_DK);
        gfx_text_center(fb, stride, cx, cy - 8, 1, sel ? COL_ONACC : 0xaeb6c8, h->txt);
        break;
    case HK_ZONE:
        if (sel) {
            gfx_rect(fb, stride, x + 3, y + 3, w - 6, hh - 6, gfx_mix(COL_PART_DK, COL_ACCENT, dim_sel ? 70 : 130));
            gfx_frame(fb, stride, x + 3, y + 3, w - 6, hh - 6, 2, hi);
        }
        gfx_text_center(fb, stride, cx, cy - 8, 1, sel ? COL_TEXT : COL_DIM, h->txt);
        break;
    }

    /* amber dot: this control has been rebound */
    if (is_custom(a, h->input)) {
        int dx = h->kind == HK_ZONE ? x + w - 9 : x + w - 3;
        int dy = h->kind == HK_ZONE ? y + 10 : y + 3;
        gfx_circle(fb, stride, dx, dy, 5, COL_BG);
        gfx_circle(fb, stride, dx, dy, 4, COL_AMBER);
    }
}

static void draw_controller(const App *a, unsigned char *fb, int stride) {
    gfx_rrect(fb, stride, DG_X, DG_Y, DG_W, DG_H, 10, COL_PANEL);

    /* silhouette: an outline pass (every shape grown by 3px), then the fill */
    static const int shapes[4][5] = {         /* x, y, w, h, radius */
        {  80,  58, 440, 176, 60 },           /* top of the body */
        {  34, 112, 150, 238, 72 },           /* left grip */
        { 416, 112, 150, 238, 72 },           /* right grip */
        { 120, 150, 360, 150, 60 },           /* middle, under the sticks */
    };
    for (int pass = 0; pass < 2; pass++) {
        int g = pass == 0 ? 3 : 0;
        for (int i = 0; i < 4; i++)
            gfx_rrect(fb, stride, DG_X + shapes[i][0] - g, DG_Y + shapes[i][1] - g,
                      shapes[i][2] + 2 * g, shapes[i][3] + 2 * g, shapes[i][4] + g, pass == 0 ? COL_BODY_OUT : COL_BODY);
    }

    /* lightbar strip, in the colour player 1 gets */
    int lc = a->cfg.colour[1] >= 0 ? a->cfg.colour[1] : (a->cfg.colour[0] >= 0 ? a->cfg.colour[0] : 0x2040ff);
    gfx_rrect(fb, stride, DG_X + 200, DG_Y + 62, 200, 8, 4, lc);

    /* touchpad body and zone dividers */
    gfx_rrect(fb, stride, DG_X + 170, DG_Y + 78, 260, 120, 6, COL_PART_DK);
    gfx_rrect_frame(fb, stride, DG_X + 170, DG_Y + 78, 260, 120, 6, 2, 0x5a6276);

    for (int i = 0; i < MC_NUM_INPUTS; i++) draw_hot(a, fb, stride, &hots[i]);

    gfx_line(fb, stride, DG_X + 209, DG_Y + 84, DG_X + 209, DG_Y + 192, 1, 0x5a6276);
    gfx_line(fb, stride, DG_X + 391, DG_Y + 84, DG_X + 391, DG_Y + 192, 1, 0x5a6276);

    gfx_text_center(fb, stride, DG_X + DG_W / 2, DG_Y + DG_H - 26, 1, COL_DIM, "Tap a control, or move with the D-pad");
}

static void draw_inputs_grid(const App *a, unsigned char *fb, int stride) {
    char buf[64];
    for (int gi = 0; gi < CH_COLS * CH_ROWS; gi++) {
        int input = grid_order[gi];
        int x = CH_X0 + (gi % CH_COLS) * CH_SX, y = CH_Y0 + (gi / CH_COLS) * CH_SY;
        int sel = (input == a->sel_in);
        int custom = is_custom(a, input);

        gfx_rrect(fb, stride, x, y, CH_W, CH_H, 8, sel ? COL_PANEL2 : COL_PANEL);
        if (sel)
            gfx_rrect_frame(fb, stride, x, y, CH_W, CH_H, 8, 3,
                            a->focus == FOCUS_INPUTS ? COL_ACCENT : gfx_mix(COL_ACCENT, COL_PANEL, 110));
        gfx_text(fb, stride, x + 9, y + 3, 1, COL_DIM, mc_input_label(input));
        mc_format_binding(mc_binding(&a->cfg, input), buf, sizeof buf);
        gfx_text_clip(fb, stride, x + 9, y + 19, 1, custom ? COL_AMBER : COL_TEXT, buf, CH_W - 14);
        if (custom) gfx_circle(fb, stride, x + CH_W - 10, y + 9, 3, COL_AMBER);
    }
}

static void draw_picker(const App *a, unsigned char *fb, int stride) {
    char buf[96], cur[64], def[64];
    int mask = mc_binding(&a->cfg, a->sel_in);
    int defmask = mc_input_default(a->sel_in);

    gfx_rrect(fb, stride, RP_X, RP_Y, RP_W, RP_H, 10, COL_PANEL);
    gfx_text(fb, stride, PK_X, 98, 1, COL_DIM, "Assign a Switch button to");
    gfx_text(fb, stride, PK_X, 120, 3, COL_ACCENT, mc_input_label(a->sel_in));

    mc_format_binding(mask, cur, sizeof cur);
    mc_format_binding(defmask, def, sizeof def);
    gfx_text(fb, stride, PK_X, 178, 1, COL_DIM, "Currently");
    gfx_text_clip(fb, stride, PK_X + 80, 178, 1, is_custom(a, a->sel_in) ? COL_AMBER : COL_TEXT, cur, 200);
    gfx_text(fb, stride, PK_X + 300, 178, 1, COL_DIM, "Default");
    gfx_text_clip(fb, stride, PK_X + 372, 178, 1, COL_TEXT, def, 170);

    for (int i = 0; i < CH_COLS * CH_ROWS; i++) {
        int s = pk_slots[i];
        if (s == PK_EMPTY) continue;
        int x = PK_X + (i % CH_COLS) * PK_SX, y = PK_Y + (i / CH_COLS) * PK_SY;
        int on = (s == PK_NONE) ? (mask == 0) : ((mask & MC_BIT(s)) != 0);
        int is_def = (s != PK_NONE) && ((defmask & MC_BIT(s)) != 0);

        gfx_rrect(fb, stride, x, y, PK_W, PK_H, 8, on ? (s == PK_NONE ? COL_ERR : COL_ACCENT) : COL_PANEL2);
        gfx_text_center(fb, stride, x + PK_W / 2, y + (PK_H - 16) / 2, 1, on ? COL_ONACC : COL_TEXT,
                        s == PK_NONE ? "None" : mc_button_label(s));
        if (is_def) gfx_circle(fb, stride, x + 11, y + 11, 4, COL_OK);
        if (a->focus == FOCUS_PICKER && a->pick == i)
            gfx_rrect_frame(fb, stride, x - 3, y - 3, PK_W + 6, PK_H + 6, 10, 3, 0xffffff);
    }
    gfx_circle(fb, stride, PK_X + 4, 433, 4, COL_OK);
    gfx_text(fb, stride, PK_X + 16, 425, 1, COL_DIM, "= default for this input");

    /* combo toggle */
    gfx_rrect(fb, stride, COMBO_X, COMBO_Y, COMBO_W, COMBO_H, 8, a->combo ? COL_AMBER : COL_PANEL2);
    gfx_text_center(fb, stride, COMBO_X + COMBO_W / 2, COMBO_Y + 10, 1, a->combo ? COL_ONACC : COL_TEXT,
                    a->combo ? "Combo (Y): ON" : "Combo (Y): OFF");
    gfx_text_clip(fb, stride, COMBO_X + COMBO_W + 14, COMBO_Y + 10, 1, COL_DIM,
                  a->combo ? "tap several: they are pressed together" : "one button per input", RP_X + RP_W - COMBO_X - COMBO_W - 26);

    snprintf(buf, sizeof buf, "%s", a->live ? "Live apply is on: changes reach the controller within a second."
                                             : "Live apply is off: press + or Save to send your changes.");
    gfx_text_clip(fb, stride, PK_X, 504, 1, COL_DIM, buf, RP_W - 32);
    gfx_text_clip(fb, stride, PK_X, 524, 1, COL_DIM, "Pad Left/Centre/Right fire when the touchpad is clicked.", RP_W - 32);
}

void remap_draw(const App *a, unsigned char *fb, int stride) {
    draw_controller(a, fb, stride);
    draw_inputs_grid(a, fb, stride);
    draw_picker(a, fb, stride);

    ui_button(fb, stride, btn_x[0], BTN_Y, BTN_W, BTN_H, "Save", 2, 1);
    ui_button(fb, stride, btn_x[1], BTN_Y, BTN_W, BTN_H, "Reset input", 1, 0);
    ui_button(fb, stride, btn_x[2], BTN_Y, BTN_W, BTN_H, "Reset all", 1, 0);

    if (a->status_frames > 0)
        gfx_text_clip(fb, stride, RP_X, 628, 1, a->status_error ? COL_ERR : COL_OK, a->status, RP_W);
}
