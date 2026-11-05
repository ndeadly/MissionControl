/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
#pragma once
/* Internal: shared theme/helpers and the interface between the app shell (ui.c) and its two pages. */
#include "ui.h"

#define COL_BG      0x181a20
#define COL_PANEL   0x242832
#define COL_PANEL2  0x2f3440
#define COL_ACCENT  0x5aa0ff
#define COL_TEXT    0xe8eaf0
#define COL_DIM     0x8b92a3
#define COL_OK      0x62d98a
#define COL_ERR     0xff6b6b
#define COL_AMBER   0xffc857
#define COL_ONACC   0x0b1220   /* text on an accent-coloured background */

/* Shared helpers (ui.c) */
void ui_set_status(App *a, int err, const char *msg);
int  ui_in_rect(int x, int y, int rx, int ry, int rw, int rh);
void ui_save(App *a);
/* Standard button: filled rounded rect with centred label */
void ui_button(unsigned char *fb, int stride, int x, int y, int w, int h, const char *label, int scale, int accent);

/* Lightbar page (ui.c) */
void light_init(App *a);
void light_update(App *a, const UiInput *in);
void light_draw(const App *a, unsigned char *fb, int stride);

/* Buttons page (remap.c) */
void remap_init(App *a);
/* Returns 1 if it used the B press (so the shell must not treat it as "quit") */
int  remap_update(App *a, const UiInput *in);
void remap_draw(const App *a, unsigned char *fb, int stride);
