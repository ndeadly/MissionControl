/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
#pragma once
#include <stddef.h>

/* Reads/edits only the keys this app manages in Mission Control's missioncontrol.ini:
 *   dualshock4_lightbar_brightness, dualshock4_lightbar_colour[_1..8], dualshock4_map_<input>
 * Everything else in the file (other settings, comments, ordering) is preserved. */

/* ---- Dualshock 4 inputs that can be remapped (same order as the sysmodule's Dualshock4Input enum) ---- */
enum {
    MC_IN_CROSS, MC_IN_CIRCLE, MC_IN_SQUARE, MC_IN_TRIANGLE,
    MC_IN_L1, MC_IN_R1, MC_IN_L2, MC_IN_R2, MC_IN_L3, MC_IN_R3,
    MC_IN_SHARE, MC_IN_OPTIONS, MC_IN_PS,
    MC_IN_DPAD_UP, MC_IN_DPAD_DOWN, MC_IN_DPAD_LEFT, MC_IN_DPAD_RIGHT,
    MC_IN_PAD_LEFT, MC_IN_PAD_CENTRE, MC_IN_PAD_RIGHT,
    MC_NUM_INPUTS
};

/* ---- Switch buttons an input can be assigned to. A binding is a bitmask of MC_BIT(MC_BTN_*); 0 = unbound ---- */
enum {
    MC_BTN_A, MC_BTN_B, MC_BTN_X, MC_BTN_Y,
    MC_BTN_L, MC_BTN_R, MC_BTN_ZL, MC_BTN_ZR,
    MC_BTN_PLUS, MC_BTN_MINUS, MC_BTN_HOME, MC_BTN_CAPTURE,
    MC_BTN_STICKL, MC_BTN_STICKR,
    MC_BTN_UP, MC_BTN_DOWN, MC_BTN_LEFT, MC_BTN_RIGHT,
    MC_NUM_BUTTONS
};
#define MC_BIT(b) (1 << (b))

typedef struct {
    int colour[9];   /* 0xRRGGBB per target, -1 = unset. [0] = all players, [1..8] = player N */
    int brightness;  /* 0-9, or -1 = not present in file (Mission Control default is 5) */
    int map[MC_NUM_INPUTS]; /* binding per DS4 input, -1 = not in the file (Mission Control uses the default) */
} McConfig;

#define MC_ERR_IO      (-1)
#define MC_ERR_NOMEM   (-2)

void mc_config_defaults(McConfig *cfg);

/* Returns 0 on success, 1 if the file doesn't exist (cfg left at defaults), <0 on error. */
int mc_load(const char *path, McConfig *cfg);

/* Rewrites the managed keys in the [misc] section. Backs up the original to <path>.bak once.
 * Returns 0 on success, <0 on error. */
int mc_save(const char *path, const McConfig *cfg);

/* ---- tables ---- */
const char *mc_input_key(int input);      /* ini key suffix, e.g. "touchpad_centre" */
const char *mc_input_label(int input);    /* display name, e.g. "Pad Centre" */
int         mc_input_default(int input);  /* default binding bitmask */
const char *mc_button_key(int button);    /* ini name, e.g. "stickl" */
const char *mc_button_label(int button);  /* display name, e.g. "L Stick" */

/* The binding Mission Control will use for this input (the file's value, or the default). */
int  mc_binding(const McConfig *cfg, int input);

/* Formats a binding for display ("A", "ZL+L", "None"). */
void mc_format_binding(int mask, char *out, size_t n);

/* Parses "a", "zl+l" or "none" (case-insensitive). Returns 0 on success, -1 on bad input. */
int  mc_parse_binding(const char *text, int *mask);
