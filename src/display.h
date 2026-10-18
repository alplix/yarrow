/* Display primitives every target provides (hal/gc03/display.c, sim/display_sim.c). */
#ifndef YARROW_DISPLAY_H
#define YARROW_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_W 240
#define DISPLAY_H 320

/* RGB565 from 8-bit channels, usable in constant expressions */
#define RGB565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define RGB_HEX(h) RGB565((((h) >> 16) & 0xFF), (((h) >> 8) & 0xFF), ((h) & 0xFF))

void display_init(void);
void display_sleep(bool sleep); /* panel off and low power, or back on */

/* What the screens call (src/render.c): draws immediately, or takes part in a change-only redraw, see display_render. */
void display_fill_rect(int x, int y, int w, int h, uint16_t color);

/* Streaming writes: open a window, push w*h pixels row by row, close. */
void display_window(int x, int y, int w, int h);
void display_pixel(uint16_t color);
void display_end(void);

/* Everything drawn between group_begin and group_end counts as one operation covering the given box (a line of text,
 * a sprite). The box must contain every pixel the group writes. */
void display_group_begin(int x, int y, int w, int h);
void display_group_end(void);

/* Calls draw() twice and writes only the parts that differ from the previous frame with the same key. A full redraw
 * (full = true) writes everything. */
void display_render(uint32_t key, bool full, void (*draw)(void *), void *arg);
void display_render_forget(void);

/* Called when a text does not fit its place (kind says where). Does nothing on the device; the simulator lists them so a
 * translation that is too long gets noticed. */
void layout_warn(const char *kind, const char *text, int width, int room);
extern void (*layout_warn_hook)(const char *kind, const char *text, int width, int room);
int display_render_peak(void);
int display_render_last(void);

/* What each target provides: the panel itself. */
void display_raw_fill_rect(int x, int y, int w, int h, uint16_t color);
void display_raw_window(int x, int y, int w, int h);
void display_raw_pixel(uint16_t color);
void display_raw_end(void);

#endif
