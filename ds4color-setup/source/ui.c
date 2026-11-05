/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
/* App shell (header, page tabs, save/quit handling, footer) and the Lightbar page. */
#include "pages.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ layout */
/* header */
#define PTAB_Y 16
#define PTAB_W 156
#define PTAB_H 42
static const int ptab_x[NUM_PAGES] = { 480, 644 };
static const char *const ptab_name[NUM_PAGES] = { "Buttons", "Lightbar" };
#define QUIT_X 1150
#define QUIT_W 90
#define LIVE_X 816
#define LIVE_W 164
#define LIVE_FRAMES 15   /* live mode writes at most every 250 ms (60 fps) */

/* lightbar page */
#define TAB_X0 40
#define TAB_Y 76
#define TAB_W 130
#define TAB_H 50
#define TAB_STEP 136

#define TRACK_X 580
#define TRACK_W 620
#define TRACK_H 36
static const int track_y[4] = { 170, 240, 310, 380 };

#define PRESET_X 580
#define PRESET_Y 468
#define PRESET_S 52
#define PRESET_STEP 62
#define NPRESETS 10

#define BTN_Y 556
#define BTN_H 54
#define BTN_W 190
static const int btn_x[2] = { 580, 790 };   /* Save, Clear */

static const int presets[NPRESETS] = {
    0xff0000, 0xff7f00, 0xffff00, 0x00ff00, 0x00ffff,
    0x0000ff, 0x8000ff, 0xff00ff, 0xff69b4, 0xffffff
};

/* Same table and base colours Mission Control uses, so the preview matches the controller */
static const int mult_table[10] = { 0, 1, 4, 8, 12, 16, 20, 24, 28, 32 };
static const int ps4_default[9][3] = {
    {0,0,0}, {0,0,4}, {4,0,0}, {0,4,0}, {2,0,2}, {0,2,2}, {3,1,0}, {2,2,0}, {1,0,3}
};

/* ------------------------------------------------------------------ colour helpers */
static void hsv_to_rgb(float h, float s, float v, int *r, int *g, int *b) {
    float c = v * s;
    float hp = fmodf(h, 360.0f) / 60.0f;
    float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
    float r1 = 0, g1 = 0, b1 = 0;
    if (hp < 1) { r1 = c; g1 = x; }
    else if (hp < 2) { r1 = x; g1 = c; }
    else if (hp < 3) { g1 = c; b1 = x; }
    else if (hp < 4) { g1 = x; b1 = c; }
    else if (hp < 5) { r1 = x; b1 = c; }
    else { r1 = c; b1 = x; }
    float m = v - c;
    *r = (int)lroundf((r1 + m) * 255.0f);
    *g = (int)lroundf((g1 + m) * 255.0f);
    *b = (int)lroundf((b1 + m) * 255.0f);
}

static void rgb_to_hsv(int r, int g, int b, float *h, float *s, float *v) {
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
    float mx = fmaxf(rf, fmaxf(gf, bf)), mn = fminf(rf, fminf(gf, bf));
    float d = mx - mn;
    *v = mx;
    *s = (mx <= 0.0f) ? 0.0f : d / mx;
    if (d <= 0.0f) { *h = 0.0f; return; }
    if (mx == rf) *h = 60.0f * fmodf((gf - bf) / d, 6.0f);
    else if (mx == gf) *h = 60.0f * ((bf - rf) / d + 2.0f);
    else *h = 60.0f * ((rf - gf) / d + 4.0f);
    if (*h < 0) *h += 360.0f;
}

static int pack(int r, int g, int b) { return (r << 16) | (g << 8) | b; }

static int hsv_pack(const float *hsv) {
    int r, g, b;
    hsv_to_rgb(hsv[0], hsv[1], hsv[2], &r, &g, &b);
    return pack(r, g, b);
}

/* ------------------------------------------------------------------ shared helpers */
void ui_set_status(App *a, int err, const char *msg) {
    snprintf(a->status, sizeof a->status, "%s", msg);
    a->status_frames = 60 * 6;
    a->status_error = err;
    a->quit_armed = 0;
}

int ui_in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

void ui_save(App *a) {
    int rc = mc_save(a->path, &a->cfg);
    if (rc == 0) {
        a->dirty = 0;
        ui_set_status(a, 0, "Saved - Mission Control applies it within a second.");
    } else {
        ui_set_status(a, 1, "Could not write missioncontrol.ini (is the SD card writable?)");
    }
}

/* Live mode: quietly write the file, throttled, whenever something changed */
static void autosave(App *a) {
    if (!a->live || !a->dirty || a->live_cooldown > 0) return;
    a->live_cooldown = LIVE_FRAMES;
    if (mc_save(a->path, &a->cfg) == 0) {
        a->dirty = 0;
        ui_set_status(a, 0, "Live: sent to Mission Control");
    } else {
        ui_set_status(a, 1, "Could not write missioncontrol.ini (is the SD card writable?)");
    }
}

void ui_button(unsigned char *fb, int stride, int x, int y, int w, int h, const char *label, int scale, int accent) {
    gfx_rrect(fb, stride, x, y, w, h, 8, accent ? COL_ACCENT : COL_PANEL2);
    gfx_text_center(fb, stride, x + w / 2, y + (h - 16 * scale) / 2, scale, accent ? COL_ONACC : COL_TEXT, label);
}

/* ------------------------------------------------------------------ lightbar page: state helpers */
static const char *target_name(int t, char *buf, size_t n) {
    if (t == 0) return "All players";
    snprintf(buf, n, "Player %d", t);
    return buf;
}

static int brightness_level(const App *a) { return a->cfg.brightness < 0 ? 5 : a->cfg.brightness; }

static void init_hsv(App *a) {
    for (int t = 0; t < 9; t++) {
        int c = a->cfg.colour[t];
        if (c >= 0) rgb_to_hsv((c >> 16) & 255, (c >> 8) & 255, c & 255, &a->hsv[t][0], &a->hsv[t][1], &a->hsv[t][2]);
        else { a->hsv[t][0] = 210.0f; a->hsv[t][1] = 1.0f; a->hsv[t][2] = 1.0f; }
    }
}

static void commit_hsv(App *a) {
    a->cfg.colour[a->target] = hsv_pack(a->hsv[a->target]);
    a->dirty = 1;
}

static void set_colour(App *a, int rgb) {
    float *h = a->hsv[a->target];
    rgb_to_hsv((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, &h[0], &h[1], &h[2]);
    a->cfg.colour[a->target] = rgb;
    a->dirty = 1;
}

static int effective_colour(const App *a, int t, int *r, int *g, int *b);

/* When editing a target that has no colour yet, start from what the preview currently shows */
static void begin_edit(App *a) {
    if (a->cfg.colour[a->target] >= 0) return;
    int r, g, b;
    int src = effective_colour(a, a->target, &r, &g, &b);
    float *h = a->hsv[a->target];
    rgb_to_hsv(r, g, b, &h[0], &h[1], &h[2]);
    if (src == 2) h[2] = 1.0f;   /* PS4 default colours are very dim raw values; start at full value */
}

static void select_target(App *a, int t) {
    a->target = t;
    begin_edit(a);   /* unset targets show sliders matching the preview */
}

/* Adjust slider by an absolute 0..1 position (touch) */
static void slider_set_frac(App *a, int sel, float f) {
    if (f < 0) f = 0;
    if (f > 1) f = 1;
    if (sel < 3) begin_edit(a);
    float *h = a->hsv[a->target];
    if (sel == 0) { h[0] = f * 359.0f; commit_hsv(a); }
    else if (sel == 1) { h[1] = f; commit_hsv(a); }
    else if (sel == 2) { h[2] = f; commit_hsv(a); }
    else { a->cfg.brightness = (int)lroundf(f * 9.0f); a->dirty = 1; }
}

/* Adjust slider by steps (d-pad) */
static void slider_step(App *a, int sel, int dir, int big) {
    if (sel < 3) begin_edit(a);
    float *h = a->hsv[a->target];
    if (sel == 0) {
        h[0] += dir * (big ? 10.0f : 1.0f);
        while (h[0] < 0) h[0] += 360.0f;
        while (h[0] >= 360.0f) h[0] -= 360.0f;
        commit_hsv(a);
    } else if (sel == 1 || sel == 2) {
        float v = h[sel] + dir * (big ? 0.10f : 0.01f);
        h[sel] = v < 0 ? 0 : (v > 1 ? 1 : v);
        commit_hsv(a);
    } else {
        int b = brightness_level(a) + dir;
        a->cfg.brightness = b < 0 ? 0 : (b > 9 ? 9 : b);
        a->dirty = 1;
    }
}

static void do_clear(App *a) {
    a->cfg.colour[a->target] = -1;
    a->dirty = 1;
    begin_edit(a);
    ui_set_status(a, 0, "Cleared. Press Save to write it.");
}

/* what colour does the controller show for this target (before brightness)? returns source: 0 custom, 1 inherited from All, 2 PS4 default */
static int effective_colour(const App *a, int t, int *r, int *g, int *b) {
    int c = a->cfg.colour[t], src = 0;
    if (c < 0 && t != 0) { c = a->cfg.colour[0]; src = 1; }
    if (c >= 0) { *r = (c >> 16) & 255; *g = (c >> 8) & 255; *b = c & 255; return src; }
    int tt = t == 0 ? 1 : t;
    int m = 32; /* base colours are tiny values; show them at the scale MC multiplies them to at full brightness */
    *r = ps4_default[tt][0] * m; *g = ps4_default[tt][1] * m; *b = ps4_default[tt][2] * m;
    return 2;
}

/* ------------------------------------------------------------------ lightbar page: update/draw */
void light_init(App *a) {
    a->drag = -1;
    init_hsv(a);
    begin_edit(a);
}

void light_update(App *a, const UiInput *in) {
    if (in->down & UI_L) select_target(a, (a->target + 8) % 9);
    if (in->down & UI_R) select_target(a, (a->target + 1) % 9);
    if (in->down & UI_UP)   a->sel = (a->sel + 3) % 4;
    if (in->down & UI_DOWN) a->sel = (a->sel + 1) % 4;

    /* left/right with key repeat; ZL/ZR = big steps */
    int big = (in->held & (UI_ZL | UI_ZR)) != 0;
    a->hold_l = (in->held & UI_LEFT)  ? a->hold_l + 1 : 0;
    a->hold_r = (in->held & UI_RIGHT) ? a->hold_r + 1 : 0;
    int fire_l = a->hold_l == 1 || (a->hold_l > 20 && (a->hold_l % 2) == 0);
    int fire_r = a->hold_r == 1 || (a->hold_r > 20 && (a->hold_r % 2) == 0);
    if (a->sel == 3) { fire_l = a->hold_l == 1 || (a->hold_l > 24 && (a->hold_l % 10) == 0); fire_r = a->hold_r == 1 || (a->hold_r > 24 && (a->hold_r % 10) == 0); }
    if (fire_l) slider_step(a, a->sel, -1, big);
    if (fire_r) slider_step(a, a->sel, +1, big);

    if (in->down & UI_Y) {
        set_colour(a, presets[a->preset_idx]);
        a->preset_idx = (a->preset_idx + 1) % NPRESETS;
    }
    if (in->down & UI_X) do_clear(a);

    /* ---- touch ---- */
    if (in->touch_started) {
        int x = in->tx, y = in->ty;
        a->drag = -1;
        for (int t = 0; t < 9; t++)
            if (ui_in_rect(x, y, TAB_X0 + t * TAB_STEP, TAB_Y, TAB_W, TAB_H)) select_target(a, t);
        for (int s = 0; s < 4; s++)
            if (ui_in_rect(x, y, TRACK_X - 10, track_y[s] - 12, TRACK_W + 20, TRACK_H + 24)) { a->sel = s; a->drag = s; }
        for (int p = 0; p < NPRESETS; p++)
            if (ui_in_rect(x, y, PRESET_X + p * PRESET_STEP, PRESET_Y, PRESET_S, PRESET_S)) set_colour(a, presets[p]);
        if (ui_in_rect(x, y, btn_x[0], BTN_Y, BTN_W, BTN_H)) ui_save(a);
        if (ui_in_rect(x, y, btn_x[1], BTN_Y, BTN_W, BTN_H)) do_clear(a);
    }
    if (in->touching && a->drag >= 0) {
        slider_set_frac(a, a->drag, (float)(in->tx - TRACK_X) / (float)(TRACK_W - 1));
    }
    if (!in->touching) a->drag = -1;
}

static void draw_slider(const App *a, unsigned char *fb, int stride, int s) {
    const float *h = a->hsv[a->target];
    int x = TRACK_X, y = track_y[s];
    static const char *names[4] = { "Hue", "Saturation", "Brightness (value)", "Lightbar brightness level" };
    gfx_text(fb, stride, x, y - 26, 1, a->sel == s ? COL_ACCENT : COL_DIM, names[s]);

    for (int i = 0; i < TRACK_W; i++) {
        float f = (float)i / (TRACK_W - 1);
        int r, g, b;
        if (s == 0) hsv_to_rgb(f * 359.0f, 1, 1, &r, &g, &b);
        else if (s == 1) hsv_to_rgb(h[0], f, h[2], &r, &g, &b);
        else if (s == 2) hsv_to_rgb(h[0], h[1], f, &r, &g, &b);
        else { int lvl = (int)(f * 9.999f); int v = 30 + lvl * 24; r = g = b = v; }
        gfx_rect(fb, stride, x + i, y, 1, TRACK_H, pack(r, g, b));
    }
    float f;
    char val[32];
    if (s == 0) { f = h[0] / 359.0f; snprintf(val, sizeof val, "%d", (int)lroundf(h[0])); }
    else if (s == 1) { f = h[1]; snprintf(val, sizeof val, "%d%%", (int)lroundf(h[1] * 100)); }
    else if (s == 2) { f = h[2]; snprintf(val, sizeof val, "%d%%", (int)lroundf(h[2] * 100)); }
    else { f = brightness_level(a) / 9.0f; snprintf(val, sizeof val, "%d / 9%s", brightness_level(a), a->cfg.brightness < 0 ? " (default)" : ""); }
    int hx = x + (int)lroundf(f * (TRACK_W - 1));
    gfx_rect(fb, stride, hx - 4, y - 6, 8, TRACK_H + 12, 0x000000);
    gfx_rect(fb, stride, hx - 2, y - 4, 4, TRACK_H + 8, 0xffffff);
    if (a->sel == s) gfx_frame(fb, stride, x - 4, y - 4, TRACK_W + 8, TRACK_H + 8, 2, COL_ACCENT);
    gfx_text(fb, stride, x + TRACK_W - gfx_text_w(val, 1), y - 26, 1, COL_TEXT, val);
}

void light_draw(const App *a, unsigned char *fb, int stride) {
    char buf[96];

    /* player tabs */
    for (int t = 0; t < 9; t++) {
        int x = TAB_X0 + t * TAB_STEP;
        gfx_rrect(fb, stride, x, TAB_Y, TAB_W, TAB_H, 8, t == a->target ? COL_ACCENT : COL_PANEL);
        const char *lab = t == 0 ? "All" : (snprintf(buf, sizeof buf, "P%d", t), buf);
        gfx_text(fb, stride, x + 14, TAB_Y + 9, 2, t == a->target ? COL_ONACC : COL_TEXT, lab);
        int c = a->cfg.colour[t];
        int chip = c >= 0 ? c : COL_PANEL2;
        gfx_rect(fb, stride, x + TAB_W - 36, TAB_Y + 12, 24, 26, 0x000000);
        gfx_rect(fb, stride, x + TAB_W - 34, TAB_Y + 14, 20, 22, chip);
    }

    /* preview panel */
    gfx_rrect(fb, stride, 40, 150, 480, 460, 10, COL_PANEL);
    int r, g, b;
    int src = effective_colour(a, a->target, &r, &g, &b);
    gfx_rect(fb, stride, 64, 174, 432, 220, pack(r, g, b));
    gfx_frame(fb, stride, 64, 174, 432, 220, 2, 0x000000);
    snprintf(buf, sizeof buf, "#%02X%02X%02X", r, g, b);
    if (src == 2) snprintf(buf, sizeof buf, "PS4 default");
    gfx_text_center(fb, stride, 280, 410, 3, COL_TEXT, buf);
    gfx_text_center(fb, stride, 280, 462, 1, COL_DIM,
                    src == 0 ? "custom colour" : (src == 1 ? "inherits the All players colour" : "no custom colour set"));

    char nb[32];
    snprintf(buf, sizeof buf, "Editing: %s", target_name(a->target, nb, sizeof nb));
    gfx_text(fb, stride, 64, 494, 1, COL_ACCENT, buf);

    int lvl = brightness_level(a);
    int mult = mult_table[lvl];
    int er, eg, eb;
    if (src == 2) { er = (r / 32) * mult; eg = (g / 32) * mult; eb = (b / 32) * mult; }
    else { er = r * mult / 32; eg = g * mult / 32; eb = b * mult / 32; }
    gfx_text(fb, stride, 64, 530, 1, COL_DIM, "Approx. on the controller at");
    snprintf(buf, sizeof buf, "brightness level %d:", lvl);
    gfx_text(fb, stride, 64, 552, 1, COL_DIM, buf);
    gfx_rect(fb, stride, 320, 524, 176, 56, pack(er, eg, eb));
    gfx_frame(fb, stride, 320, 524, 176, 56, 2, 0x000000);

    /* sliders */
    for (int s = 0; s < 4; s++) draw_slider(a, fb, stride, s);

    /* presets */
    gfx_text(fb, stride, PRESET_X, PRESET_Y - 24, 1, COL_DIM, "Presets");
    for (int p = 0; p < NPRESETS; p++) {
        int x = PRESET_X + p * PRESET_STEP;
        gfx_rect(fb, stride, x - 2, PRESET_Y - 2, PRESET_S + 4, PRESET_S + 4, 0x000000);
        gfx_rect(fb, stride, x, PRESET_Y, PRESET_S, PRESET_S, presets[p]);
    }

    /* buttons */
    ui_button(fb, stride, btn_x[0], BTN_Y, BTN_W, BTN_H, "Save", 2, 1);
    ui_button(fb, stride, btn_x[1], BTN_Y, BTN_W, BTN_H, "Clear", 2, 0);

    if (a->status_frames > 0)
        gfx_text_clip(fb, stride, 580, 628, 1, a->status_error ? COL_ERR : COL_OK, a->status, 660);
}

/* ------------------------------------------------------------------ app shell */
void app_init(App *a, const char *ini_path) {
    memset(a, 0, sizeof *a);
    snprintf(a->path, sizeof a->path, "%s", ini_path);
    a->load_state = mc_load(a->path, &a->cfg);
    if (a->load_state < 0) { mc_config_defaults(&a->cfg); }
    a->page = PAGE_BUTTONS;
    a->live = 1;
    light_init(a);
    remap_init(a);
    if (a->load_state == 1) ui_set_status(a, 0, "No missioncontrol.ini yet - one will be created when you save.");
}

/* B / Quit button. With unsaved changes the first request only warns. */
static int request_quit(App *a) {
    if (!a->dirty || (a->quit_armed && a->status_frames > 0)) return 1;
    ui_set_status(a, 1, "Unsaved changes! Press B again to quit without saving, or + to save.");
    a->quit_armed = 1;
    return 0;
}

int app_update(App *a, const UiInput *in) {
    if (a->status_frames > 0) a->status_frames--;
    else a->quit_armed = 0;
    if (a->live_cooldown > 0) a->live_cooldown--;

    if (in->down & UI_PLUS) ui_save(a);
    if (in->down & UI_MINUS) a->page = (a->page + 1) % NUM_PAGES;

    int quit = 0;
    if (in->touch_started) {
        for (int p = 0; p < NUM_PAGES; p++)
            if (ui_in_rect(in->tx, in->ty, ptab_x[p], PTAB_Y, PTAB_W, PTAB_H)) a->page = p;
        if (ui_in_rect(in->tx, in->ty, QUIT_X, PTAB_Y, QUIT_W, PTAB_H)) quit = request_quit(a);
        if (ui_in_rect(in->tx, in->ty, LIVE_X, PTAB_Y, LIVE_W, PTAB_H)) {
            a->live = !a->live;
            ui_set_status(a, 0, a->live ? "Live apply on: changes are sent to Mission Control as you make them."
                                        : "Live apply off: press + or Save to send your changes.");
        }
    }

    int used_b = 0;
    if (a->page == PAGE_BUTTONS) used_b = remap_update(a, in);
    else light_update(a, in);

    if ((in->down & UI_B) && !used_b) quit = request_quit(a) || quit;
    if (!quit) autosave(a);
    return quit;
}

void app_draw(const App *a, unsigned char *fb, int stride) {
    gfx_rect(fb, stride, 0, 0, UI_W, UI_H, COL_BG);

    /* header */
    gfx_text(fb, stride, 40, 22, 2, COL_TEXT, "DS4 Controller");
    gfx_text(fb, stride, 40, 50, 1, COL_DIM, "for Mission Control");
    for (int p = 0; p < NUM_PAGES; p++) {
        int on = (p == a->page);
        gfx_rrect(fb, stride, ptab_x[p], PTAB_Y, PTAB_W, PTAB_H, 10, on ? COL_ACCENT : COL_PANEL);
        gfx_text_center(fb, stride, ptab_x[p] + PTAB_W / 2, PTAB_Y + 10, 2, on ? COL_ONACC : COL_TEXT, ptab_name[p]);
    }
    ui_button(fb, stride, QUIT_X, PTAB_Y, QUIT_W, PTAB_H, "Quit", 1, 0);
    gfx_rrect(fb, stride, LIVE_X, PTAB_Y, LIVE_W, PTAB_H, 10, a->live ? COL_OK : COL_PANEL2);
    gfx_text_center(fb, stride, LIVE_X + LIVE_W / 2, PTAB_Y + 13, 1, a->live ? COL_ONACC : COL_DIM, a->live ? "Live apply: ON" : "Live apply: OFF");
    if (a->dirty) gfx_text(fb, stride, QUIT_X - 20 - gfx_text_w("* unsaved changes", 1), 30, 1, COL_AMBER, "* unsaved changes");

    if (a->page == PAGE_BUTTONS) remap_draw(a, fb, stride);
    else light_draw(a, fb, stride);

    /* footer hints */
    gfx_rect(fb, stride, 0, 664, UI_W, 56, COL_PANEL);
    if (a->page == PAGE_BUTTONS) {
        gfx_text(fb, stride, 40, 674, 1, COL_DIM, "D-pad: choose input   A: pick Switch button   X: reset input (ZL+X: reset all)   Y: combo mode");
        gfx_text(fb, stride, 40, 696, 1, COL_DIM, "+: save   -: Lightbar page   B: back / quit        Touch works too: tap a control, then tap the button it should press.");
    } else {
        gfx_text(fb, stride, 40, 674, 1, COL_DIM, "L/R: player   Up/Down: slider   Left/Right: adjust (hold ZL/ZR = big steps)   Y: next preset   X: clear");
        gfx_text(fb, stride, 40, 696, 1, COL_DIM, "+: save   -: Buttons page   B: quit        Touch works too: tap tabs, drag sliders, tap presets.");
    }
}
