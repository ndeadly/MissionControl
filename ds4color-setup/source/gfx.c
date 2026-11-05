/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Reedman27
 * Part of the Mission Control fork; licensed under GPL version 2 (see ../LICENSE). */
#include "gfx.h"
#include "font8x16.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void gfx_px(unsigned char *fb, int stride, int x, int y, int rgb) {
    if (x < 0 || y < 0 || x >= UI_W || y >= UI_H) return;
    unsigned char *p = fb + (size_t)y * stride + (size_t)x * 4;
    p[0] = (rgb >> 16) & 0xff; p[1] = (rgb >> 8) & 0xff; p[2] = rgb & 0xff; p[3] = 0xff;
}

void gfx_rect(unsigned char *fb, int stride, int x, int y, int w, int h, int rgb) {
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > UI_W ? UI_W : x + w, y1 = y + h > UI_H ? UI_H : y + h;
    unsigned char r = (rgb >> 16) & 0xff, g = (rgb >> 8) & 0xff, b = rgb & 0xff;
    for (int j = y0; j < y1; j++) {
        unsigned char *p = fb + (size_t)j * stride + (size_t)x0 * 4;
        for (int i = x0; i < x1; i++, p += 4) { p[0] = r; p[1] = g; p[2] = b; p[3] = 0xff; }
    }
}

void gfx_frame(unsigned char *fb, int stride, int x, int y, int w, int h, int t, int rgb) {
    gfx_rect(fb, stride, x, y, w, t, rgb);
    gfx_rect(fb, stride, x, y + h - t, w, t, rgb);
    gfx_rect(fb, stride, x, y, t, h, rgb);
    gfx_rect(fb, stride, x + w - t, y, t, h, rgb);
}

/* How far the rounded outline is pulled in on row j (0..h-1) of a box with corner radius r */
static int corner_inset(int j, int h, int r) {
    int d;
    if (j < r) d = r - j;               /* distance below the top arc's centre line, counted in half rows below */
    else if (j >= h - r) d = j - (h - r) + 1;
    else return 0;
    float dy = (float)d - 0.5f;
    float inset = (float)r - sqrtf((float)r * (float)r - dy * dy);
    return (int)(inset + 0.5f);
}

void gfx_rrect(unsigned char *fb, int stride, int x, int y, int w, int h, int r, int rgb) {
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    if (r < 0) r = 0;
    for (int j = 0; j < h; j++) {
        int in = corner_inset(j, h, r);
        gfx_rect(fb, stride, x + in, y + j, w - 2 * in, 1, rgb);
    }
}

void gfx_rrect_frame(unsigned char *fb, int stride, int x, int y, int w, int h, int r, int t, int rgb) {
    /* Draw the outline as a filled rrect minus an inner one: done row by row so the middle stays untouched */
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    for (int j = 0; j < h; j++) {
        int in = corner_inset(j, h, r);
        int ji = j - t, hi = h - 2 * t;
        int inner_in = (ji >= 0 && ji < hi) ? t + corner_inset(ji, hi, r > t ? r - t : 0) : -1;
        if (inner_in < 0) {
            gfx_rect(fb, stride, x + in, y + j, w - 2 * in, 1, rgb);
        } else {
            int left_w = inner_in - in;
            if (left_w > 0) {
                gfx_rect(fb, stride, x + in, y + j, left_w, 1, rgb);
                gfx_rect(fb, stride, x + w - inner_in, y + j, left_w, 1, rgb);
            }
        }
    }
}

void gfx_circle(unsigned char *fb, int stride, int cx, int cy, int r, int rgb) {
    for (int dy = -r; dy <= r; dy++) {
        float half = sqrtf((float)(r * r) + (float)r - (float)(dy * dy));
        int hw = (int)half;
        if (hw < 0) continue;
        gfx_rect(fb, stride, cx - hw, cy + dy, 2 * hw + 1, 1, rgb);
    }
}

void gfx_ring(unsigned char *fb, int stride, int cx, int cy, int r, int t, int rgb) {
    float ro = (float)r + 0.5f, ri = (float)(r - t) + 0.5f;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            float d2 = (float)(dx * dx + dy * dy);
            if (d2 <= ro * ro && d2 >= ri * ri) gfx_px(fb, stride, cx + dx, cy + dy, rgb);
        }
    }
}

void gfx_line(unsigned char *fb, int stride, int x0, int y0, int x1, int y1, int t, int rgb) {
    int dx = x1 - x0, dy = y1 - y0;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (steps == 0) steps = 1;
    for (int i = 0; i <= steps; i++) {
        int x = x0 + dx * i / steps, y = y0 + dy * i / steps;
        gfx_rect(fb, stride, x - t / 2, y - t / 2, t, t, rgb);
    }
}

int gfx_text_w(const char *s, int scale) { return (int)strlen(s) * 8 * scale; }

void gfx_text(unsigned char *fb, int stride, int x, int y, int scale, int rgb, const char *s) {
    for (; *s; s++, x += 8 * scale) {
        unsigned char c = (unsigned char)*s;
        if (c < 32 || c > 126) continue;
        const unsigned char *g = font8x16[c - 32];
        for (int row = 0; row < 16; row++)
            for (int col = 0; col < 8; col++)
                if (g[row] & (0x80 >> col))
                    gfx_rect(fb, stride, x + col * scale, y + row * scale, scale, scale, rgb);
    }
}

void gfx_text_center(unsigned char *fb, int stride, int cx, int y, int scale, int rgb, const char *s) {
    gfx_text(fb, stride, cx - gfx_text_w(s, scale) / 2, y, scale, rgb, s);
}

void gfx_text_clip(unsigned char *fb, int stride, int x, int y, int scale, int rgb, const char *s, int max_w) {
    int fit = max_w / (8 * scale);
    int len = (int)strlen(s);
    if (len <= fit) { gfx_text(fb, stride, x, y, scale, rgb, s); return; }
    if (fit < 3) return;
    char buf[64];
    if (fit - 2 >= (int)sizeof buf - 3) fit = (int)sizeof buf - 1;
    memcpy(buf, s, (size_t)(fit - 2));
    buf[fit - 2] = '.'; buf[fit - 1] = '.'; buf[fit] = '\0';
    gfx_text(fb, stride, x, y, scale, rgb, buf);
}

int gfx_mix(int a, int b, int alpha) {
    int r = (((a >> 16) & 255) * (255 - alpha) + ((b >> 16) & 255) * alpha) / 255;
    int g = (((a >> 8) & 255) * (255 - alpha) + ((b >> 8) & 255) * alpha) / 255;
    int bl = ((a & 255) * (255 - alpha) + (b & 255) * alpha) / 255;
    return (r << 16) | (g << 8) | bl;
}
