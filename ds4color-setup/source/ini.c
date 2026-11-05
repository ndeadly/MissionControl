/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
#include "ini.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

#define KEY_BRIGHT "dualshock4_lightbar_brightness"
#define KEY_COLOUR "dualshock4_lightbar_colour"
#define KEY_MAP    "dualshock4_map_"

/* ------------------------------------------------------------------ tables */
/* Keep these in step with the sysmodule (mcmitm_config.cpp): same key names, same defaults. */
static const struct { const char *key; const char *label; int def; } g_inputs[MC_NUM_INPUTS] = {
    [MC_IN_CROSS]       = { "cross",           "Cross",      MC_BIT(MC_BTN_B) },
    [MC_IN_CIRCLE]      = { "circle",          "Circle",     MC_BIT(MC_BTN_A) },
    [MC_IN_SQUARE]      = { "square",          "Square",     MC_BIT(MC_BTN_Y) },
    [MC_IN_TRIANGLE]    = { "triangle",        "Triangle",   MC_BIT(MC_BTN_X) },
    [MC_IN_L1]          = { "l1",              "L1",         MC_BIT(MC_BTN_L) },
    [MC_IN_R1]          = { "r1",              "R1",         MC_BIT(MC_BTN_R) },
    [MC_IN_L2]          = { "l2",              "L2",         MC_BIT(MC_BTN_ZL) },
    [MC_IN_R2]          = { "r2",              "R2",         MC_BIT(MC_BTN_ZR) },
    [MC_IN_L3]          = { "l3",              "L3",         MC_BIT(MC_BTN_STICKL) },
    [MC_IN_R3]          = { "r3",              "R3",         MC_BIT(MC_BTN_STICKR) },
    [MC_IN_SHARE]       = { "share",           "Share",      MC_BIT(MC_BTN_MINUS) },
    [MC_IN_OPTIONS]     = { "options",         "Options",    MC_BIT(MC_BTN_PLUS) },
    [MC_IN_PS]          = { "ps",              "PS",         MC_BIT(MC_BTN_HOME) },
    [MC_IN_DPAD_UP]     = { "dpad_up",         "D-Up",       MC_BIT(MC_BTN_UP) },
    [MC_IN_DPAD_DOWN]   = { "dpad_down",       "D-Down",     MC_BIT(MC_BTN_DOWN) },
    [MC_IN_DPAD_LEFT]   = { "dpad_left",       "D-Left",     MC_BIT(MC_BTN_LEFT) },
    [MC_IN_DPAD_RIGHT]  = { "dpad_right",      "D-Right",    MC_BIT(MC_BTN_RIGHT) },
    [MC_IN_PAD_LEFT]    = { "touchpad_left",   "Pad Left",   MC_BIT(MC_BTN_MINUS) },
    [MC_IN_PAD_CENTRE]  = { "touchpad_centre", "Pad Centre", MC_BIT(MC_BTN_CAPTURE) },
    [MC_IN_PAD_RIGHT]   = { "touchpad_right",  "Pad Right",  MC_BIT(MC_BTN_PLUS) },
};

static const struct { const char *key; const char *label; } g_buttons[MC_NUM_BUTTONS] = {
    [MC_BTN_A] = { "a", "A" },           [MC_BTN_B] = { "b", "B" },
    [MC_BTN_X] = { "x", "X" },           [MC_BTN_Y] = { "y", "Y" },
    [MC_BTN_L] = { "l", "L" },           [MC_BTN_R] = { "r", "R" },
    [MC_BTN_ZL] = { "zl", "ZL" },        [MC_BTN_ZR] = { "zr", "ZR" },
    [MC_BTN_PLUS] = { "plus", "Plus" },  [MC_BTN_MINUS] = { "minus", "Minus" },
    [MC_BTN_HOME] = { "home", "Home" },  [MC_BTN_CAPTURE] = { "capture", "Capture" },
    [MC_BTN_STICKL] = { "stickl", "L Stick" }, [MC_BTN_STICKR] = { "stickr", "R Stick" },
    [MC_BTN_UP] = { "up", "Up" },        [MC_BTN_DOWN] = { "down", "Down" },
    [MC_BTN_LEFT] = { "left", "Left" },  [MC_BTN_RIGHT] = { "right", "Right" },
};

const char *mc_input_key(int i)     { return g_inputs[i].key; }
const char *mc_input_label(int i)   { return g_inputs[i].label; }
int         mc_input_default(int i) { return g_inputs[i].def; }
const char *mc_button_key(int b)    { return g_buttons[b].key; }
const char *mc_button_label(int b)  { return g_buttons[b].label; }

int mc_binding(const McConfig *cfg, int input) {
    return cfg->map[input] >= 0 ? cfg->map[input] : g_inputs[input].def;
}

void mc_format_binding(int mask, char *out, size_t n) {
    size_t o = 0;
    if (n == 0) return;
    out[0] = '\0';
    if (mask == 0) { snprintf(out, n, "None"); return; }
    for (int b = 0; b < MC_NUM_BUTTONS; b++) {
        if (!(mask & MC_BIT(b))) continue;
        int w = snprintf(out + o, n - o, "%s%s", o ? "+" : "", g_buttons[b].label);
        if (w < 0 || (size_t)w >= n - o) { out[n - 1] = '\0'; return; }
        o += (size_t)w;
    }
}

int mc_parse_binding(const char *text, int *mask) {
    int m = 0;
    const char *tok = text;
    for (;;) {
        const char *plus = strchr(tok, '+');
        size_t len = plus ? (size_t)(plus - tok) : strlen(tok);
        int found = 0;
        if (len == 4 && strncasecmp(tok, "none", 4) == 0) {
            found = 1;
        } else {
            for (int b = 0; b < MC_NUM_BUTTONS && !found; b++)
                if (strlen(g_buttons[b].key) == len && strncasecmp(tok, g_buttons[b].key, len) == 0) {
                    m |= MC_BIT(b);
                    found = 1;
                }
        }
        if (!found) return -1;
        if (!plus) break;
        tok = plus + 1;
    }
    *mask = m;
    return 0;
}

/* ------------------------------------------------------------------ config */
void mc_config_defaults(McConfig *cfg) {
    for (int i = 0; i < 9; i++) cfg->colour[i] = -1;
    cfg->brightness = -1;
    for (int i = 0; i < MC_NUM_INPUTS; i++) cfg->map[i] = -1;
}

static char *slurp(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = '\0';
    *len = got;
    return buf;
}

/* Classifies one line. Returns 0 for blank/comment/unparseable, 1 for [section], 2 for key=value.
 * name receives the section or key (trimmed), value the trimmed value (cut at whitespace/';'). */
static int parse_line(const char *s, size_t n, char *name, size_t nsz, char *value, size_t vsz) {
    size_t i = 0;
    name[0] = value[0] = '\0';
    while (i < n && (s[i] == ' ' || s[i] == '\t')) i++;
    if (i >= n || s[i] == ';' || s[i] == '#' || s[i] == '\r' || s[i] == '\n') return 0;

    if (s[i] == '[') {
        size_t j = ++i;
        while (j < n && s[j] != ']' && s[j] != '\n' && s[j] != '\r') j++;
        if (j >= n || s[j] != ']') return 0;
        size_t len = j - i;
        if (len >= nsz) len = nsz - 1;
        memcpy(name, s + i, len);
        name[len] = '\0';
        return 1;
    }

    size_t eq = i;
    while (eq < n && s[eq] != '=' && s[eq] != '\n') eq++;
    if (eq >= n || s[eq] != '=') return 0;
    size_t kend = eq;
    while (kend > i && (s[kend - 1] == ' ' || s[kend - 1] == '\t')) kend--;
    size_t klen = kend - i;
    if (klen == 0 || klen >= nsz) return 0;
    memcpy(name, s + i, klen);
    name[klen] = '\0';

    size_t v = eq + 1;
    while (v < n && (s[v] == ' ' || s[v] == '\t')) v++;
    size_t vend = v;
    while (vend < n && s[vend] != '\n' && s[vend] != '\r' && s[vend] != ';' && s[vend] != ' ' && s[vend] != '\t') vend++;
    size_t vlen = vend - v;
    if (vlen >= vsz) vlen = vsz - 1;
    memcpy(value, s + v, vlen);
    value[vlen] = '\0';
    return 2;
}

#define SLOT_NONE   (-1)
#define SLOT_BRIGHT 100
#define SLOT_MAP    200   /* + input index */

/* Which managed setting does this key belong to? SLOT_NONE if it isn't ours; 0..8 colour; SLOT_BRIGHT; SLOT_MAP+input */
static int managed_slot(const char *key) {
    if (strcasecmp(key, KEY_BRIGHT) == 0) return SLOT_BRIGHT;
    if (strcasecmp(key, KEY_COLOUR) == 0) return 0;
    size_t pl = strlen(KEY_COLOUR "_");
    if (strncasecmp(key, KEY_COLOUR "_", pl) == 0 && key[pl] >= '1' && key[pl] <= '8' && key[pl + 1] == '\0')
        return key[pl] - '0';
    size_t ml = strlen(KEY_MAP);
    if (strncasecmp(key, KEY_MAP, ml) == 0) {
        const char *rest = key + ml;
        for (int i = 0; i < MC_NUM_INPUTS; i++)
            if (strcasecmp(rest, g_inputs[i].key) == 0) return SLOT_MAP + i;
        if (strcasecmp(rest, "touchpad_center") == 0) return SLOT_MAP + MC_IN_PAD_CENTRE;   /* spelling the sysmodule also accepts */
    }
    return SLOT_NONE;
}

static int parse_colour(const char *v) {
    if (*v == '#') v++;
    if (strlen(v) != 6) return -1;
    for (int i = 0; i < 6; i++) if (!isxdigit((unsigned char)v[i])) return -1;
    return (int)strtoul(v, NULL, 16);
}

static const char *next_line(const char *p, const char *end, size_t *linelen) {
    const char *e = p;
    while (e < end && *e != '\n') e++;
    *linelen = (size_t)(e - p);
    return (e < end) ? e + 1 : e;
}

int mc_load(const char *path, McConfig *cfg) {
    mc_config_defaults(cfg);
    size_t len = 0;
    char *buf = slurp(path, &len);
    if (!buf) return 1;

    int in_misc = 0;
    const char *p = buf, *end = buf + len;
    char name[64], value[160];
    while (p < end) {
        size_t ll;
        const char *line = p;
        p = next_line(p, end, &ll);
        int kind = parse_line(line, ll, name, sizeof name, value, sizeof value);
        if (kind == 1) {
            in_misc = (strcasecmp(name, "misc") == 0);
        } else if (kind == 2 && in_misc) {
            int slot = managed_slot(name);
            if (slot == SLOT_BRIGHT) {
                char *e; long b = strtol(value, &e, 10);
                if (e != value && b >= 0 && b <= 9) cfg->brightness = (int)b;
            } else if (slot >= SLOT_MAP) {
                int m;
                if (mc_parse_binding(value, &m) == 0) cfg->map[slot - SLOT_MAP] = m;
            } else if (slot >= 0) {
                int c = parse_colour(value);
                if (c >= 0) cfg->colour[slot] = c;
            }
        }
    }
    free(buf);
    return 0;
}

static int write_file(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return MC_ERR_IO;
    size_t w = fwrite(data, 1, len, f);
    int rc = fclose(f);
    return (w == len && rc == 0) ? 0 : MC_ERR_IO;
}

#define LINES_CAP 8192   /* worst case is ~4 KiB: 20 bindings of up to 18 buttons each */

static size_t managed_lines(const McConfig *cfg, char *out, size_t cap) {
    size_t n = 0;
    char val[160];
#define EMIT(...) do { int w_ = snprintf(out + n, cap - n, __VA_ARGS__); if (w_ > 0 && (size_t)w_ < cap - n) n += (size_t)w_; } while (0)
    if (cfg->brightness >= 0)
        EMIT(KEY_BRIGHT "=%d\n", cfg->brightness);
    if (cfg->colour[0] >= 0)
        EMIT(KEY_COLOUR "=%06x\n", cfg->colour[0] & 0xffffff);
    for (int i = 1; i <= 8; i++)
        if (cfg->colour[i] >= 0)
            EMIT(KEY_COLOUR "_%d=%06x\n", i, cfg->colour[i] & 0xffffff);

    /* Only bindings that differ from the default are written, so the file stays small and readable */
    for (int i = 0; i < MC_NUM_INPUTS; i++) {
        int m = cfg->map[i];
        if (m < 0 || m == g_inputs[i].def) continue;
        val[0] = '\0';
        if (m == 0) {
            snprintf(val, sizeof val, "none");
        } else {
            size_t o = 0;
            for (int b = 0; b < MC_NUM_BUTTONS; b++)
                if (m & MC_BIT(b)) {
                    int w = snprintf(val + o, sizeof val - o, "%s%s", o ? "+" : "", g_buttons[b].key);
                    if (w > 0 && (size_t)w < sizeof val - o) o += (size_t)w;
                }
        }
        EMIT(KEY_MAP "%s=%s\n", g_inputs[i].key, val);
    }
#undef EMIT
    return n;
}

int mc_save(const char *path, const McConfig *cfg) {
    size_t len = 0;
    char *orig = slurp(path, &len);   /* NULL if the file doesn't exist yet */

    char *lines = malloc(LINES_CAP);
    if (!lines) { free(orig); return MC_ERR_NOMEM; }
    size_t lines_len = managed_lines(cfg, lines, LINES_CAP);

    char *out = malloc(len + lines_len + 64);
    if (!out) { free(orig); free(lines); return MC_ERR_NOMEM; }
    size_t o = 0;
    int saw_misc = 0, in_misc = 0;

    if (orig) {
        const char *p = orig, *end = orig + len;
        char name[64], value[160];
        while (p < end) {
            size_t ll;
            const char *line = p;
            p = next_line(p, end, &ll);
            int has_nl = (line + ll < end);
            int kind = parse_line(line, ll, name, sizeof name, value, sizeof value);

            if (kind == 2 && in_misc && managed_slot(name) != SLOT_NONE) continue; /* drop old managed line */

            memcpy(out + o, line, ll); o += ll;
            if (has_nl || kind == 1) out[o++] = '\n';
            else if (ll > 0) out[o++] = '\n';

            if (kind == 1) {
                in_misc = (strcasecmp(name, "misc") == 0);
                if (in_misc && !saw_misc) {
                    saw_misc = 1;
                    memcpy(out + o, lines, lines_len); o += lines_len;
                }
            }
        }
    }
    if (!saw_misc) {
        if (o > 0) out[o++] = '\n';
        memcpy(out + o, "[misc]\n", 7); o += 7;
        memcpy(out + o, lines, lines_len); o += lines_len;
    }

    int rc = 0;
    if (orig) {
        char bak[512];
        snprintf(bak, sizeof bak, "%s.bak", path);
        FILE *probe = fopen(bak, "rb");
        if (probe) fclose(probe);
        else write_file(bak, orig, len);   /* best-effort one-time backup */
    }
    rc = write_file(path, out, o);
    free(out);
    free(orig);
    free(lines);
    return rc;
}
