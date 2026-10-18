/* Drawing toolkit shared by every screen. Everything is drawn straight to the display; each element
 * repaints its own area, so a screen can update one part without clearing the rest (no flicker). */
#ifndef YARROW_UI_H
#define YARROW_UI_H

#include "font.h"
#include "theme.h"
#include <stdbool.h>
#include <stdint.h>

extern const theme_t *ui_theme;

/* layout constants for the 240 x 320 portrait display */
#define UI_W 240
#define UI_H 320
#define UI_MARGIN 12
#define UI_STATUS_H 26
#define UI_FOOTER_H 31

/* ---- sprites: rows of characters, '.' is transparent ---- */
typedef struct {
    const char *const *rows;
    uint8_t w, h;
} sprite_t;

/* palette slots a sprite character can use */
typedef enum { PAL_INK, PAL_DIM, PAL_OK, PAL_WARN, PAL_HOT, PAL_BAD, PAL_ACC, PAL_PETAL, PAL_LEAF, PAL_BG } pal_t;

uint16_t ui_pal(pal_t p);

void ui_sprite(int x, int y, const sprite_t *s, uint16_t color, int scale);
/* sprite where character 'a'..'i' picks the palette slot (a = PAL_INK, b = PAL_DIM, ...), 'y' uses color */
void ui_sprite_multi(int x, int y, const sprite_t *s, int scale);
void ui_trefoil(int x, int y, uint16_t color, int scale); /* generated radiation symbol, 13 x 13 cells */
void ui_hazard(int x, int y, uint16_t color, int scale);  /* warning triangle, 17 x 14 cells */

extern const sprite_t spr_bolt, spr_magnet, spr_house, spr_drop, spr_cross, spr_radio, spr_battery, spr_plug,
    spr_disk, spr_check, spr_book, spr_tube, spr_signal, spr_flask, spr_gear, spr_list, spr_flower, spr_trefoil, spr_atom, spr_mushroom, spr_tower, spr_skull;

/* ---- text ---- */
void ui_text(const font_t *f, int x, int y, const char *s, uint16_t fg, uint16_t bg);
void ui_text_right(const font_t *f, int right, int y, const char *s, uint16_t fg, uint16_t bg);
void ui_text_center(const font_t *f, int cx, int y, const char *s, uint16_t fg, uint16_t bg);
/* text in a box that is cleared first, so changing text leaves nothing behind. align: 0 left, 1 right, 2 centre */
void ui_label(const font_t *f, int x, int y, int w, const char *s, uint16_t fg, uint16_t bg, int align);
/* word wrapped text in a box, returns the y after the last line */
int ui_text_wrap(const font_t *f, int x, int y, int w, const char *s, uint16_t fg, uint16_t bg, int line_gap);

/* ---- shapes ---- */
void ui_clear(void);
void ui_panel(int x, int y, int w, int h, uint16_t color);  /* corners notched by 4 pixels */
void ui_notched(int x, int y, int w, int h, uint16_t color, uint16_t behind, int notch);
int ui_chip(int x, int y, const char *s, uint16_t bg);      /* returns the chip width */
void ui_divider(int y);                                     /* dotted line across the screen */

/* 20 block meter. zone_colors: blocks 0-9 ok, 10-14 warn, 15-17 hot, 18-19 bad; mono uses one color */
void ui_meter(int x, int y, int w, int lit, bool zones, uint16_t mono);

/* pixel columns: values scaled so that `max` fills the height; threshold < 0 hides the dashed line */
void ui_columns(int x, int y, int w, int h, const uint32_t *vals, int n, uint32_t max, int bs, int col_w, int gap,
                uint16_t color, bool auto_color, int32_t threshold);

/* ---- compound elements ---- */
void ui_statusbar(const char *left, int battery_percent, bool charging);
void ui_header(const sprite_t *icon, uint16_t icon_color, const char *title);
void ui_footer(const char *left, const char *right);
void ui_stat_panel(int x, int y, int w, const char *label, const char *value, const char *unit);
void ui_battery_icon(int x, int y, int percent, uint16_t color);

/* a menu or settings row: label on the left, value on the right; selected rows are highlighted */
void ui_row_icon(int y, const sprite_t *icon, const char *label, bool selected);
void ui_row(int y, const char *label, const char *value, bool selected);

#endif
