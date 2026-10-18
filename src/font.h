#ifndef YARROW_FONT_H
#define YARROW_FONT_H

#include <stdint.h>

typedef struct {
    uint16_t cp;   /* Unicode code point */
    uint8_t adv;   /* advance width */
    uint8_t w, h;  /* bitmap size */
    int8_t xoff;   /* bitmap left edge relative to the pen */
    int8_t yoff;   /* bitmap top edge above the baseline */
    uint16_t off;  /* byte offset into the bitmap blob */
} glyph_t;

typedef struct {
    const glyph_t *glyphs; /* sorted by code point */
    uint16_t count;
    const uint8_t *bits;   /* 2 bits per pixel, 4 pixels per byte, MSB first */
    uint8_t ascent;
    uint8_t height;
} font_t;

extern const font_t font_12_regular;
extern const font_t font_12_medium;
extern const font_t font_14_medium;
extern const font_t font_20_medium;
extern const font_t font_28_medium;
extern const font_t font_34_medium;
extern const font_t font_48_medium;

/* Pixel width of a UTF-8 string with extra spacing between glyphs. */
int font_width(const font_t *f, const char *text, int spacing);

/* Draws text with its top at y over a flat background colour. */
void font_draw(const font_t *f, int x, int y, const char *text, uint16_t fg, uint16_t bg, int spacing);

/* Draws a text line in a w-wide box without flicker; align 0 left, 1 right, 2 centre. */
void font_label(const font_t *f, int x, int y, int w, const char *text, uint16_t fg, uint16_t bg, int align);

/* Draws text centred on the display width. */
void font_draw_centered(const font_t *f, int y, const char *text, uint16_t fg, uint16_t bg, int spacing);

#endif
