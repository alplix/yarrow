#include "font.h"
#include "display.h"

static uint32_t next_cp(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    uint32_t cp = *p++;
    if (cp >= 0xF0)      { cp = ((cp & 7u) << 18) | ((p[0] & 63u) << 12) | ((p[1] & 63u) << 6) | (p[2] & 63u); p += 3; }
    else if (cp >= 0xE0) { cp = ((cp & 15u) << 12) | ((p[0] & 63u) << 6) | (p[1] & 63u); p += 2; }
    else if (cp >= 0xC0) { cp = ((cp & 31u) << 6) | (p[0] & 63u); p += 1; }
    *s = (const char *)p;
    return cp;
}

static const glyph_t *find(const font_t *f, uint32_t cp)
{
    int lo = 0, hi = (int)f->count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (f->glyphs[mid].cp == cp)
            return &f->glyphs[mid];
        if (f->glyphs[mid].cp < cp) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}

int font_width(const font_t *f, const char *text, int spacing)
{
    int w = 0, n = 0;
    while (*text) {
        const glyph_t *g = find(f, next_cp(&text));
        if (g) { w += g->adv; n++; }
    }
    return n ? w + spacing * (n - 1) : 0;
}

static uint16_t mix(uint16_t a, uint16_t b, int t) /* t = 0..255 weight of b */
{
    int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
    int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
    int r = ar + (br - ar) * t / 255;
    int g = ag + (bg - ag) * t / 255;
    int bl = ab + (bb - ab) * t / 255;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

static void blit(const font_t *f, const glyph_t *g, int x, int y, uint16_t fg, uint16_t bg)
{
    uint16_t pal[4] = {bg, mix(bg, fg, 85), mix(bg, fg, 170), fg};
    const uint8_t *bits = f->bits + g->off;
    display_window(x, y, g->w, g->h);
    for (int i = 0, n = g->w * g->h; i < n; i++)
        display_pixel(pal[(bits[i >> 2] >> (6 - 2 * (i & 3))) & 3]);
    display_end();
}

void font_draw(const font_t *f, int x, int y, const char *text, uint16_t fg, uint16_t bg, int spacing)
{
    const char *whole = text;
    int tw = font_width(f, text, spacing);
    if (x + tw > DISPLAY_W - 2)
        layout_warn("text past the edge", text, x + tw, DISPLAY_W - 2);
    display_group_begin(x - 4, y - 4, tw + 12, f->height + 8); /* generous box: glyphs may overhang the advance */
    int baseline = y + f->ascent;
    while (*text) {
        uint32_t cp = next_cp(&text);
        const glyph_t *g = find(f, cp);
        if (!g) {
            layout_warn("missing glyph", whole, (int)cp, f->height);
            continue;
        }
        if (g->w && g->h)
            blit(f, g, x + g->xoff, baseline - g->yoff, fg, bg);
        x += g->adv + spacing;
    }
    display_group_end();
}

void font_draw_centered(const font_t *f, int y, const char *text, uint16_t fg, uint16_t bg, int spacing)
{
    font_draw(f, (DISPLAY_W - font_width(f, text, spacing)) / 2, y, text, fg, bg, spacing);
}

/* Draws a text line inside a w-wide box in one pass, row by row, so every pixel is written exactly once and nothing
 * flashes (clearing the box first and drawing the glyphs on top shows as flicker when a value changes often). */
void font_label(const font_t *f, int x, int y, int w, const char *text, uint16_t fg, uint16_t bg, int align)
{
    static uint16_t row[DISPLAY_W];
    if (x < 0 || y < 0 || w <= 0 || w > DISPLAY_W || x + w > DISPLAY_W || y + f->height > DISPLAY_H)
        return;

    int tw = font_width(f, text, 0);
    if (tw > w)
        layout_warn("label", text, tw, w);
    display_group_begin(x, y, w, f->height);
    int pen0 = align == 1 ? w - tw : align == 2 ? (w - tw) / 2 : 0;
    uint16_t pal[4] = {bg, mix(bg, fg, 85), mix(bg, fg, 170), fg};

    for (int ry = 0; ry < f->height; ry++) {
        for (int i = 0; i < w; i++)
            row[i] = bg;
        int pen = pen0;
        for (const char *p = text; *p;) {
            const glyph_t *g = find(f, next_cp(&p));
            if (!g)
                continue;
            int gr = ry - (f->ascent - g->yoff); /* row inside the glyph bitmap */
            if (g->w && g->h && gr >= 0 && gr < g->h) {
                const uint8_t *bits = f->bits + g->off;
                for (int gx = 0; gx < g->w; gx++) {
                    int px = pen + g->xoff + gx;
                    if (px >= 0 && px < w) {
                        int idx = gr * g->w + gx;
                        row[px] = pal[(bits[idx >> 2] >> (6 - 2 * (idx & 3))) & 3];
                    }
                }
            }
            pen += g->adv;
        }
        display_window(x, y + ry, w, 1);
        for (int i = 0; i < w; i++)
            display_pixel(row[i]);
        display_end();
    }
    display_group_end();
}
