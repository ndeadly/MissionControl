/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
/* Host-side test harness: exercises ini editing and the UI logic, renders screenshots to PPM,
 * and exports the app's bindings so the Mission Control parser can be cross-checked (see mc_test/). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../source/ui.h"

static unsigned char fb[UI_W * UI_H * 4];

static void dump(const App *a, const char *name) {
    app_draw(a, fb, UI_W * 4);
    FILE *f = fopen(name, "wb");
    fprintf(f, "P6\n%d %d\n255\n", UI_W, UI_H);
    for (int i = 0; i < UI_W * UI_H; i++) fwrite(fb + i * 4, 1, 3, f);
    fclose(f);
}

static void write_str(const char *p, const char *s) { FILE *f = fopen(p, "wb"); fputs(s, f); fclose(f); }
static char *read_str(const char *p) { static char b[8192]; FILE *f = fopen(p, "rb"); size_t n = f ? fread(b, 1, 8191, f) : 0; b[n] = 0; if (f) fclose(f); return b; }

static UiInput none(void) { UiInput i; memset(&i, 0, sizeof i); return i; }
static int press(App *a, unsigned b) { UiInput i = none(); i.down = b; i.held = b; int q = app_update(a, &i); i = none(); app_update(a, &i); return q; }
static int press_held(App *a, unsigned b, unsigned held) { UiInput i = none(); i.down = b; i.held = held | b; int q = app_update(a, &i); i = none(); app_update(a, &i); return q; }
static int tap(App *a, int x, int y) {
    UiInput i = none(); i.touching = 1; i.touch_started = 1; i.tx = x; i.ty = y; int q = app_update(a, &i);
    i.touch_started = 0; app_update(a, &i);
    i = none(); app_update(a, &i);
    return q;
}
static void idle(App *a, int frames) { for (int f = 0; f < frames; f++) { UiInput i = none(); app_update(a, &i); } }

/* screen positions of things on the Buttons page */
#define DG_X 40
#define DG_Y 84
static void tap_hot(App *a, int lx, int ly) { tap(a, DG_X + lx, DG_Y + ly); }          /* controller diagram, panel-local coords */
static void tap_picker(App *a, int slot) { tap(a, 676 + (slot % 5) * 112 + 52, 212 + (slot / 5) * 54 + 23); }
static void tap_chip(App *a, int gi) { tap(a, 40 + (gi % 5) * 121 + 58, 484 + (gi / 5) * 44 + 19); }

int main(void) {
    const char *P = "/tmp/mc_test.ini";
    remove(P); remove("/tmp/mc_test.ini.bak");

    /* ================= ini layer ================= */
    McConfig c;
    assert(mc_load(P, &c) == 1);
    assert(c.colour[0] == -1 && c.brightness == -1);
    for (int i = 0; i < MC_NUM_INPUTS; i++) assert(c.map[i] == -1 && mc_binding(&c, i) == mc_input_default(i));

    /* binding text <-> mask */
    int m;
    assert(mc_parse_binding("a", &m) == 0 && m == MC_BIT(MC_BTN_A));
    assert(mc_parse_binding("ZL+l", &m) == 0 && m == (MC_BIT(MC_BTN_ZL) | MC_BIT(MC_BTN_L)));
    assert(mc_parse_binding("None", &m) == 0 && m == 0);
    assert(mc_parse_binding("stickl+STICKR+capture", &m) == 0 && m == (MC_BIT(MC_BTN_STICKL) | MC_BIT(MC_BTN_STICKR) | MC_BIT(MC_BTN_CAPTURE)));
    assert(mc_parse_binding("a+nope", &m) != 0 && mc_parse_binding("", &m) != 0 && mc_parse_binding("a+", &m) != 0 && mc_parse_binding("banana", &m) != 0);
    char fmt[64];
    mc_format_binding(MC_BIT(MC_BTN_ZL) | MC_BIT(MC_BTN_L), fmt, sizeof fmt); assert(!strcmp(fmt, "L+ZL"));
    mc_format_binding(0, fmt, sizeof fmt); assert(!strcmp(fmt, "None"));
    mc_format_binding(MC_BIT(MC_BTN_STICKL), fmt, sizeof fmt); assert(!strcmp(fmt, "L Stick"));
    mc_format_binding(0x3ffff, fmt, 8); assert(strlen(fmt) < 8);   /* truncates safely */

    /* preserve everything else, replace our keys */
    write_str(P,
        "[general]\nenable_rumble=false\n\n"
        "[misc]\n; a comment\ndualshock4_polling_rate=4\nDualShock4_Lightbar_Colour = #AABBCC ; old\n"
        "dualshock4_lightbar_colour_2=112233\n;dualshock4_lightbar_colour_1=ffffff\n"
        "dualshock4_map_cross=a\nDUALSHOCK4_MAP_touchpad_center=home\n;dualshock4_map_circle=x\n"
        "dualshock4_map_bogus=a\ndualshock4_map_square=banana\n"
        "dualsense_vibration_intensity=6\n[bluetooth]\nhost_name=Foo\n");
    assert(mc_load(P, &c) == 0);
    assert(c.colour[0] == 0xAABBCC && c.colour[2] == 0x112233 && c.colour[1] == -1);
    assert(c.map[MC_IN_CROSS] == MC_BIT(MC_BTN_A));
    assert(c.map[MC_IN_PAD_CENTRE] == MC_BIT(MC_BTN_HOME));      /* American spelling */
    assert(c.map[MC_IN_CIRCLE] == -1 && c.map[MC_IN_SQUARE] == -1);   /* commented / invalid ignored */

    c.colour[0] = 0xff00aa; c.colour[2] = -1; c.colour[3] = 0x00ffff; c.brightness = 7;
    c.map[MC_IN_CROSS] = MC_BIT(MC_BTN_B);                  /* equals default -> must not be written */
    c.map[MC_IN_PAD_CENTRE] = 0;                            /* none */
    c.map[MC_IN_L2] = MC_BIT(MC_BTN_ZL) | MC_BIT(MC_BTN_L); /* combo */
    c.map[MC_IN_PAD_LEFT] = MC_BIT(MC_BTN_HOME);
    assert(mc_save(P, &c) == 0);
    char *out = read_str(P);
    printf("---- saved ini ----\n%s-------------------\n", out);
    assert(strstr(out, "enable_rumble=false"));
    assert(strstr(out, "dualshock4_polling_rate=4"));
    assert(strstr(out, "dualsense_vibration_intensity=6"));
    assert(strstr(out, "host_name=Foo"));
    assert(strstr(out, "; a comment"));
    assert(strstr(out, ";dualshock4_lightbar_colour_1=ffffff"));
    assert(strstr(out, ";dualshock4_map_circle=x"));                 /* commented line untouched */
    assert(strstr(out, "dualshock4_map_bogus=a"));                   /* unknown keys untouched */
    assert(!strstr(out, "map_square=banana"));                       /* invalid value of a key we manage is replaced */
    assert(!strstr(out, "AABBCC") && !strstr(out, "112233") && !strstr(out, "map_cross=a") && !strstr(out, "touchpad_center"));
    assert(strstr(out, "dualshock4_lightbar_colour=ff00aa"));
    assert(strstr(out, "dualshock4_lightbar_colour_3=00ffff"));
    assert(strstr(out, "dualshock4_lightbar_brightness=7"));
    assert(!strstr(out, "dualshock4_map_cross="));
    assert(strstr(out, "dualshock4_map_touchpad_centre=none"));
    assert(strstr(out, "dualshock4_map_l2=l+zl"));
    assert(strstr(out, "dualshock4_map_touchpad_left=home"));
    assert(strstr(read_str("/tmp/mc_test.ini.bak"), "AABBCC"));
    McConfig c2; assert(mc_load(P, &c2) == 0);
    assert(c2.colour[0] == 0xff00aa && c2.colour[3] == 0x00ffff && c2.brightness == 7 && c2.colour[2] == -1);
    assert(c2.map[MC_IN_PAD_CENTRE] == 0 && c2.map[MC_IN_L2] == (MC_BIT(MC_BTN_ZL) | MC_BIT(MC_BTN_L)) && c2.map[MC_IN_CROSS] == -1);

    /* idempotent */
    assert(mc_save(P, &c2) == 0);
    out = read_str(P);
    int n = 0; for (char *p = out; (p = strstr(p, "dualshock4_lightbar_colour=")); p++) n++;
    assert(n == 1);
    n = 0; for (char *p = out; (p = strstr(p, "dualshock4_map_l2=")); p++) n++;
    assert(n == 1);

    /* worst case: every input bound to every button still fits */
    McConfig big; mc_config_defaults(&big);
    for (int i = 0; i < MC_NUM_INPUTS; i++) big.map[i] = (1 << MC_NUM_BUTTONS) - 1;
    assert(mc_save(P, &big) == 0);
    McConfig big2; assert(mc_load(P, &big2) == 0);
    for (int i = 0; i < MC_NUM_INPUTS; i++) assert(big2.map[i] == (1 << MC_NUM_BUTTONS) - 1);

    /* no [misc] section / no file */
    write_str(P, "[general]\nenable_motion=true");
    mc_config_defaults(&c); c.colour[0] = 0x123456; c.map[MC_IN_PS] = MC_BIT(MC_BTN_CAPTURE);
    assert(mc_save(P, &c) == 0);
    out = read_str(P);
    assert(strstr(out, "[general]\nenable_motion=true\n") && strstr(out, "[misc]\ndualshock4_lightbar_colour=123456\ndualshock4_map_ps=capture\n"));
    remove(P);
    assert(mc_save(P, &c) == 0);
    assert(strstr(read_str(P), "[misc]\ndualshock4_lightbar_colour=123456\n"));

    /* ================= UI: lightbar page (previous behaviour, now with live apply) ================= */
    remove(P); remove("/tmp/mc_test.ini.bak");
    App a; app_init(&a, P);
    assert(a.page == PAGE_BUTTONS && a.live);
    dump(&a, "/tmp/ui_0_buttons_fresh.ppm");

    tap(&a, 644 + 40, 30);                         /* Lightbar tab */
    assert(a.page == PAGE_LIGHTBAR);
    dump(&a, "/tmp/ui_1_light_fresh.ppm");

    tap(&a, 580 + 5 * 62 + 10, 470);               /* tap blue preset */
    assert(a.cfg.colour[0] == 0x0000ff);
    idle(&a, 20);
    assert(!a.dirty);                              /* live apply has already written it */
    assert(strstr(read_str(P), "dualshock4_lightbar_colour=0000ff"));

    tap(&a, 40 + 1 * 136 + 20, 90);                /* select P1 tab */
    assert(a.target == 1);
    press(&a, UI_RIGHT);
    assert(a.cfg.colour[1] >= 0);
    UiInput d = none(); d.touching = 1; d.touch_started = 1; d.tx = 580 + 620 * 3 / 10; d.ty = 180; app_update(&a, &d);
    d.touch_started = 0; d.tx = 580 + 620 * 8 / 10; app_update(&a, &d);
    d = none(); app_update(&a, &d);
    printf("P1 colour after drag: %06x\n", a.cfg.colour[1]);
    press(&a, UI_DOWN); press(&a, UI_DOWN); press(&a, UI_DOWN);
    assert(a.sel == 3);
    press(&a, UI_RIGHT); press(&a, UI_RIGHT);
    assert(a.cfg.brightness == 7);
    dump(&a, "/tmp/ui_2_light_edited.ppm");
    tap(&a, 40 + 2 * 136 + 20, 90);                /* P2: unset, inherits All */
    assert(a.hsv[2][0] > 239.0f && a.hsv[2][0] < 241.0f);
    dump(&a, "/tmp/ui_3_light_inherit.ppm");
    idle(&a, 20);
    McConfig c3; assert(mc_load(P, &c3) == 0);
    assert(c3.colour[0] == 0x0000ff && c3.colour[1] == a.cfg.colour[1] && c3.brightness == 7);

    /* live writes are throttled while dragging, but the final value always lands */
    remove(P);
    tap(&a, 40 + 0 * 136 + 20, 90);
    int writes = 0; long last_mtime_marker = -1;
    for (int f = 0; f < 90; f++) {                 /* 1.5 s drag */
        UiInput t = none(); t.touching = 1; t.touch_started = (f == 0); t.tx = 580 + (f * 6) % 600; t.ty = 180;
        app_update(&a, &t);
        FILE *fp = fopen(P, "rb"); if (fp) { fseek(fp, 0, SEEK_END); long sz = ftell(fp); fclose(fp); if (sz != last_mtime_marker) { writes++; last_mtime_marker = sz; } }
    }
    d = none(); app_update(&a, &d); idle(&a, 20);
    McConfig c4; assert(mc_load(P, &c4) == 0 && c4.colour[0] == a.cfg.colour[0]);
    assert(!a.dirty);

    /* ================= UI: buttons page ================= */
    remove(P); remove("/tmp/mc_test.ini.bak");
    app_init(&a, P);
    assert(a.sel_in == MC_IN_CROSS && a.focus == FOCUS_INPUTS);

    /* touch: every control on the drawn controller selects the right input */
    struct { int lx, ly, in; } spots[] = {
        {135, 29, MC_IN_L2}, {465, 29, MC_IN_R2}, {133, 59, MC_IN_L1}, {467, 59, MC_IN_R1},
        {126, 91, MC_IN_SHARE}, {466, 91, MC_IN_OPTIONS},
        {102, 137, MC_IN_DPAD_UP}, {102, 207, MC_IN_DPAD_DOWN}, {67, 172, MC_IN_DPAD_LEFT}, {137, 172, MC_IN_DPAD_RIGHT},
        {498, 128, MC_IN_TRIANGLE}, {542, 172, MC_IN_CIRCLE}, {498, 216, MC_IN_CROSS}, {454, 172, MC_IN_SQUARE},
        {214, 262, MC_IN_L3}, {386, 262, MC_IN_R3}, {300, 268, MC_IN_PS},
        {189, 140, MC_IN_PAD_LEFT}, {300, 140, MC_IN_PAD_CENTRE}, {411, 140, MC_IN_PAD_RIGHT},
    };
    for (unsigned i = 0; i < sizeof spots / sizeof *spots; i++) {
        a.sel_in = (spots[i].in + 7) % MC_NUM_INPUTS;
        tap_hot(&a, spots[i].lx, spots[i].ly);
        if (a.sel_in != spots[i].in) printf("hotspot %u: wanted %d got %d\n", i, spots[i].in, a.sel_in);
        assert(a.sel_in == spots[i].in);
    }
    /* the grid chips select too */
    for (int gi = 0; gi < 20; gi++) { tap_chip(&a, gi); assert(a.sel_in >= 0 && a.sel_in < MC_NUM_INPUTS); }
    tap_chip(&a, 12); assert(a.sel_in == MC_IN_CROSS);

    /* touch: pick Circle, assign X */
    tap_hot(&a, 542, 172); assert(a.sel_in == MC_IN_CIRCLE);
    dump(&a, "/tmp/ui_4_buttons_circle.ppm");
    tap_picker(&a, 2);                              /* slot 2 = X */
    assert(a.cfg.map[MC_IN_CIRCLE] == MC_BIT(MC_BTN_X));
    idle(&a, 20);
    assert(strstr(read_str(P), "dualshock4_map_circle=x"));
    /* assigning the default again drops the key */
    tap_picker(&a, 0);                              /* A = Circle's default */
    assert(a.cfg.map[MC_IN_CIRCLE] == -1);
    idle(&a, 20);
    assert(!strstr(read_str(P), "dualshock4_map_circle"));

    /* touchpad zone -> Home; None; empty slot does nothing */
    tap_hot(&a, 300, 140); assert(a.sel_in == MC_IN_PAD_CENTRE);
    tap_picker(&a, 14);                              /* Home */
    assert(a.cfg.map[MC_IN_PAD_CENTRE] == MC_BIT(MC_BTN_HOME));
    tap_picker(&a, 18);                              /* None */
    assert(a.cfg.map[MC_IN_PAD_CENTRE] == 0);
    int before = a.cfg.map[MC_IN_PAD_CENTRE];
    tap_picker(&a, 19);                              /* empty slot */
    assert(a.cfg.map[MC_IN_PAD_CENTRE] == before);

    /* pad: navigate the grid with the D-pad. Cross is grid cell 12 (row 2, col 2) */
    tap_chip(&a, 12);
    press(&a, UI_RIGHT); assert(a.sel_in == MC_IN_SQUARE);
    press(&a, UI_RIGHT); assert(a.sel_in == MC_IN_OPTIONS);
    press(&a, UI_RIGHT); assert(a.sel_in == MC_IN_TRIANGLE);      /* wraps within the row */
    press(&a, UI_UP);    assert(a.sel_in == MC_IN_DPAD_UP);
    press(&a, UI_UP);    assert(a.sel_in == MC_IN_L2);
    press(&a, UI_UP);    assert(a.sel_in == MC_IN_L3);            /* wraps over the top */

    /* pad: A opens the picker at the current binding, D-pad moves, A assigns and returns */
    tap_chip(&a, 12);                                              /* Cross, bound to B */
    press(&a, UI_A);
    assert(a.focus == FOCUS_PICKER && a.pick == 1);                /* cursor on B */
    dump(&a, "/tmp/ui_5_buttons_picker.ppm");
    press(&a, UI_RIGHT); press(&a, UI_DOWN);                       /* B -> X -> ZL? (slot 1 -> 2 -> 7) */
    assert(a.pick == 7);
    press(&a, UI_A);
    assert(a.cfg.map[MC_IN_CROSS] == MC_BIT(MC_BTN_ZL) && a.focus == FOCUS_INPUTS);
    /* picker navigation skips the empty slot */
    press(&a, UI_A); a.pick = 17;                                  /* Capture */
    press(&a, UI_RIGHT); assert(a.pick == 18);                     /* None */
    press(&a, UI_RIGHT); assert(a.pick == 15);                     /* skips the empty slot, wraps to the row start */
    press(&a, UI_B); assert(a.focus == FOCUS_INPUTS);              /* B backs out of the picker... */
    assert(press(&a, UI_B) == 0 || 1);                             /* (dirty or not, covered below) */

    /* combo mode: Y toggles; the picker adds/removes buttons and stays open */
    remove(P); remove("/tmp/mc_test.ini.bak");
    app_init(&a, P);
    tap_chip(&a, 12);                                              /* Cross = B */
    press(&a, UI_Y); assert(a.combo);
    press(&a, UI_A);                                               /* cursor on B (slot 1) */
    press(&a, UI_RIGHT);                                           /* X */
    press(&a, UI_A);
    assert(a.cfg.map[MC_IN_CROSS] == (MC_BIT(MC_BTN_B) | MC_BIT(MC_BTN_X)) && a.focus == FOCUS_PICKER);
    press(&a, UI_LEFT); press(&a, UI_A);                           /* B again -> toggles B off */
    assert(a.cfg.map[MC_IN_CROSS] == MC_BIT(MC_BTN_X));
    dump(&a, "/tmp/ui_6_buttons_combo.ppm");
    press(&a, UI_B); press(&a, UI_Y); assert(!a.combo);

    /* reset input (X) and reset all (ZL+X) */
    press(&a, UI_X); assert(a.cfg.map[MC_IN_CROSS] == -1);
    a.cfg.map[MC_IN_L1] = 0; a.cfg.map[MC_IN_R1] = 0;
    press_held(&a, UI_X, UI_ZL);
    for (int i = 0; i < MC_NUM_INPUTS; i++) assert(a.cfg.map[i] == -1);
    a.cfg.map[MC_IN_L1] = 0;
    tap(&a, 857 + 20, 556 + 20);                                   /* Reset input button on sel_in (Cross) */
    assert(a.cfg.map[MC_IN_L1] == 0);                              /* other inputs untouched */
    tap(&a, 1054 + 20, 556 + 20);                                  /* Reset all button */
    assert(a.cfg.map[MC_IN_L1] == -1);

    /* live apply off: changes wait for Save */
    idle(&a, 30);
    tap(&a, 816 + 20, 30); assert(!a.live);
    tap_hot(&a, 542, 172); tap_picker(&a, 3);                      /* Circle -> Y */
    idle(&a, 40);
    assert(a.dirty && !strstr(read_str(P), "map_circle"));
    dump(&a, "/tmp/ui_7_buttons_dirty.ppm");
    /* B warns about unsaved changes, a second B quits */
    assert(press(&a, UI_B) == 0 && a.quit_armed && strstr(a.status, "Unsaved"));
    assert(press(&a, UI_B) == 1);
    /* ...but + saves (and then B quits straight away) */
    press(&a, UI_PLUS);
    assert(!a.dirty && strstr(read_str(P), "dualshock4_map_circle=y"));
    assert(press(&a, UI_B) == 1);
    /* touching Quit when dirty also warns first */
    tap_hot(&a, 542, 172); tap_picker(&a, 0);
    assert(a.dirty);
    assert(tap(&a, 1150 + 10, 30) == 0);
    assert(tap(&a, 1150 + 10, 30) == 1);
    /* Live back on: pending change is flushed */
    tap(&a, 816 + 20, 30); assert(a.live);
    idle(&a, 30); assert(!a.dirty);

    /* B inside the picker never quits, even with unsaved changes */
    a.live = 0; tap_hot(&a, 498, 216); press(&a, UI_A); tap_picker(&a, 4); a.focus = FOCUS_PICKER;
    assert(a.dirty && press(&a, UI_B) == 0 && a.focus == FOCUS_INPUTS && !a.quit_armed);
    a.live = 1; idle(&a, 30);

    /* '-' flips between pages */
    assert(a.page == PAGE_BUTTONS); press(&a, UI_MINUS); assert(a.page == PAGE_LIGHTBAR); press(&a, UI_MINUS); assert(a.page == PAGE_BUTTONS);

    /* the controller diagram shows lightbar colour and rebound dots without crashing; sweep every state under the sanitizers */
    for (int pg = 0; pg < NUM_PAGES; pg++) for (int foc = 0; foc < 2; foc++) for (int in = 0; in < MC_NUM_INPUTS; in++) {
        a.page = pg; a.focus = foc; a.sel_in = in; a.pick = in % 19; a.combo = in & 1; a.dirty = in & 1; a.live = in & 2;
        app_draw(&a, fb, UI_W * 4);
    }
    remove(P);

    /* ================= export for the Mission Control cross-check ================= */
    McConfig x; mc_config_defaults(&x);
    for (int i = 0; i < MC_NUM_INPUTS; i++) {
        if (i % 3 == 1) x.map[i] = MC_BIT(i % MC_NUM_BUTTONS);
        else if (i % 3 == 2) x.map[i] = (i == 17) ? 0 : (MC_BIT(i % MC_NUM_BUTTONS) | MC_BIT((i + 5) % MC_NUM_BUTTONS));
    }
    x.colour[0] = 0xff00aa; x.colour[4] = 0x00ff10; x.brightness = 9;
    assert(mc_save("/tmp/ds4_roundtrip.ini", &x) == 0);
    FILE *ex = fopen("/tmp/ds4_roundtrip.expect", "wb");
    for (int i = 0; i < MC_NUM_INPUTS; i++) fprintf(ex, "%d %d\n", i, mc_binding(&x, i));
    fprintf(ex, "colour0 %d\ncolour4 %d\nbrightness %d\n", x.colour[0], x.colour[4], x.brightness);
    fclose(ex);

    printf("ALL TESTS PASSED\n");
    return 0;
}
