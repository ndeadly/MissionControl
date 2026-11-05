/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
#pragma once
/* Tiny software renderer for an RGBA8888 framebuffer (1280x720). No dependencies beyond libm. */

#define UI_W 1280
#define UI_H 720

/* Colours are 0xRRGGBB */
void gfx_px(unsigned char *fb, int stride, int x, int y, int rgb);
void gfx_rect(unsigned char *fb, int stride, int x, int y, int w, int h, int rgb);
void gfx_frame(unsigned char *fb, int stride, int x, int y, int w, int h, int t, int rgb);
void gfx_rrect(unsigned char *fb, int stride, int x, int y, int w, int h, int r, int rgb);
void gfx_rrect_frame(unsigned char *fb, int stride, int x, int y, int w, int h, int r, int t, int rgb);
void gfx_circle(unsigned char *fb, int stride, int cx, int cy, int r, int rgb);
void gfx_ring(unsigned char *fb, int stride, int cx, int cy, int r, int t, int rgb);
void gfx_line(unsigned char *fb, int stride, int x0, int y0, int x1, int y1, int t, int rgb);

/* 8x16 font scaled by an integer factor */
void gfx_text(unsigned char *fb, int stride, int x, int y, int scale, int rgb, const char *s);
void gfx_text_center(unsigned char *fb, int stride, int cx, int y, int scale, int rgb, const char *s);
/* Draws at most max_w pixels of text, ending in ".." if it had to be cut. */
void gfx_text_clip(unsigned char *fb, int stride, int x, int y, int scale, int rgb, const char *s, int max_w);
int  gfx_text_w(const char *s, int scale);

/* Mix of a and b: alpha 0 = a, 255 = b */
int  gfx_mix(int a, int b, int alpha);
