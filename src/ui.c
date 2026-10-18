#include "ui.h"
#include "display.h"

const theme_t *ui_theme = &themes[THEME_DARK];

uint16_t ui_pal(pal_t p)
{
    const theme_t *t = ui_theme;
    switch (p) {
    case PAL_INK:   return t->ink;
    case PAL_DIM:   return t->dim;
    case PAL_OK:    return t->ok;
    case PAL_WARN:  return t->warn;
    case PAL_HOT:   return t->hot;
    case PAL_BAD:   return t->bad;
    case PAL_ACC:   return t->acc;
    case PAL_PETAL: return t->petal;
    case PAL_LEAF:  return t->leaf;
    default:        return t->bg;
    }
}

/* ---- sprite data ---- */

#define SPR(name, ...) \
    static const char *const name##_rows[] = {__VA_ARGS__}; \
    const sprite_t name = {name##_rows, 0, sizeof name##_rows / sizeof *name##_rows}

SPR(spr_bolt, "......yy.", ".....yy..", "....yy...", "...yy....", "..yyyyyy.", ".....yy..", "....yy...", "...yy....", "..yy.....", ".yy......");
SPR(spr_magnet, "bbb...bbb", "bbb...bbb", "aaa...aaa", "aaa...aaa", "aaa...aaa", "aaaa.aaaa", ".aaaaaaa.", "..aaaaa..");
SPR(spr_house, "....y....", "...yyy...", "..yyyyy..", ".yyyyyyy.", "yyyyyyyyy", ".yyyyyyy.", ".yyy.yyy.", ".yyy.yyy.", ".yyy.yyy.");
SPR(spr_drop, "....y....", "....y....", "...yyy...", "..yyyyy..", ".yyyyyyy.", "..yyyyy..", ".yyyyyyy.", "yyyyyyyyy", "yy.yyyyyy", "yy.yyyyyy", ".yyyyyyy.", ".yyyyyyy.", "..yyyyy..");
SPR(spr_cross, "...yyy...", "...yyy...", "...yyy...", "yyyyyyyyy", "yyyyyyyyy", "yyyyyyyyy", "...yyy...", "...yyy...", "...yyy...");
SPR(spr_radio, "......y..", ".....y...", "....y....", "yyyyyyyyy", "y.......y", "y.yy..y.y", "y.yy..y.y", "y.......y", "yyyyyyyyy");
SPR(spr_battery, "yyyyyyyyyyyyyyyyy...", "y...............y.yy.", "y...............y.yy.", "y...............y.yy.", "yyyyyyyyyyyyyyyyy....");
SPR(spr_plug, "..yy.yy..", "..yy.yy..", "..yy.yy..", "..yy.yy..", ".yyyyyyy.", "yyyyyyyyy", "yyyyyyyyy", "yyyyyyyyy", ".yyyyyyy.", "..yyyyy..", "..yyyyy..", "...yy....", "...yy....", "...yy....");
SPR(spr_disk, "yyyyyyyyy.", "y.yyyyy.yy", "y.yyyyy.yy", "y.......yy", "y.......yy", "y.yyyyy.yy", "y.yyyyy.yy", "yyyyyyyyyy");
SPR(spr_check, "......y", ".....yy", "y...yy.", "yy.yy..", ".yyy...", "..y....");
SPR(spr_book, "yyyyyyy.", "yy...yy.", "yy.y.yy.", "yy...yy.", "yy.y.yy.", "yy...yy.", "yyyyyyy.");
SPR(spr_tube, "..yyyyyyyyyyyyy..", ".y.............y.", "yy.............yy", "yy.yyyyyyyyyy..yy", "yy.............yy", ".y.............y.", "..yyyyyyyyyyyyy..");
SPR(spr_signal, "...................", "......yyyyyyy......", "....yyy.....yyy....", "...yy....y....yy...", "..y...yyyyyyy...y..", ".....y.......y.....", "....y..yyyyy..y....", "......yy...yy......", "...................", ".........y.........", "........yyy........");
SPR(spr_flask, "..yyyyyyy..", "...y...y...", "...y...y...", "...y...y...", "...y...y...", "...y...y...", "..y.....y..", "..y.....y..", "..yyyyyyy..", ".yyy.yyyyy.", ".yyyyyyyyy.", "yyyyyy.yyyy", "yyyyyyyyyyy");
SPR(spr_gear, "..y.y.y..", ".yyyyyyy.", "yyy...yyy", ".yy...yy.", "yyy...yyy", ".yyyyyyy.", "..y.y.y..");
SPR(spr_list, "yyyyyyyyy", ".........", "yyyyyyyyy", ".........", "yyyyyyyyy");
SPR(spr_flower, "...............", ".......p.......", "...p..pwp..p...", "..pwp..p..pwp..", "...p.......p...", ".....p...p.....", "....pwp.pwp....", ".....p...p.....", ".....l.l.......", "......ll.l.....", ".......ll......", "....ll.l.......", "......ll.ll....", ".......ll......", ".....l.l.......", "......ll.l.....", ".......ll......", ".......l.......");

SPR(spr_atom, "...................", "...................", "..............ww...", "..aaaaa.....aawwa..", "..a...aaa.aaa...a..", "..a.....aaa.....a..", "..aa...aaaaa...aa..", "...a..aa.h.aa..a...", "...aaaa.hhh.aaaa...", "....aa.hhhhh.aa....", "...aaaa.hhh.aaaa...", "...a..aa.h.aa..a...", "..aa...aaaaa...aa..", "..a.....aaa.....a..", "..ww..aaa.aaa.wwa..", "..wwaaa.....aawwa..", "...................", "...................", "...................");
SPR(spr_mushroom, ".....hhhhh.....", "..hhhwwhhhhhh..", ".hhhwhhhhhhhhh.", ".hhwhhhhhhhhhh.", "hhhhhhhhhhhhhhh", ".hhhhhhhhhhhhh.", ".ddddddddddddd.", "......ddd......", "......ddd......", "......ddd......", "......ddd......", ".....ddddd.....", "....ddddddd....", "...ddddddddd...", "..ddddddddddd..");
SPR(spr_tower, ".....dd..ddd...", "....dddd.dd....", "........dd.....", "...ddddddddd...", "....iiiiiid....", "....iiiiiid....", "....iiiiiid....", "....iiiiiid....", "....iiiiiid....", "....iiiiiid....", "...iiiiiiiid...", "..iiiiiiiiiid..", "..iiiiiiiiiid..", "..iiiiiiiiiid..", ".iiiiiiiiiiiid.", "iiiiiiiiiiiiiid");
SPR(spr_skull, "....iii....", "..iiiiiii..", ".iiiiiiiii.", ".iiiiiiiii.", "ii...i...ii", "ii...i...ii", ".i...i...i.", ".iii...iii.", "..iiiiiii..", "..iiiiiii..", "..i.i.i.i..", "..i.i.i.i..");

static int sprite_w(const sprite_t *s)
{
    int w = 0;
    for (int y = 0; y < s->h; y++) {
        int n = 0;
        while (s->rows[y][n])
            n++;
        if (n > w)
            w = n;
    }
    return w;
}

static int sprite_width(const sprite_t *s)
{
    int w = 0;
    for (int r = 0; r < s->h; r++) {
        int c = 0;
        while (s->rows[r][c])
            c++;
        if (c > w)
            w = c;
    }
    return w;
}

void ui_sprite(int x, int y, const sprite_t *s, uint16_t color, int scale)
{
    display_group_begin(x, y, sprite_width(s) * scale, s->h * scale);
    for (int r = 0; r < s->h; r++)
        for (int c = 0; s->rows[r][c]; c++)
            if (s->rows[r][c] != '.')
                display_fill_rect(x + c * scale, y + r * scale, scale, scale, color);
    display_group_end();
}

static uint16_t multi_color(char c)
{
    switch (c) {
    case 'i': return ui_pal(PAL_INK);
    case 'd': return ui_pal(PAL_DIM);
    case 'o': return ui_pal(PAL_OK);
    case 'w': return ui_pal(PAL_WARN);
    case 'h': return ui_pal(PAL_HOT);
    case 'b': return ui_pal(PAL_BAD);
    case 'a': return ui_pal(PAL_ACC);
    case 'p': return ui_pal(PAL_PETAL);
    case 'l': return ui_pal(PAL_LEAF);
    default:  return ui_pal(PAL_INK);
    }
}

void ui_sprite_multi(int x, int y, const sprite_t *s, int scale)
{
    display_group_begin(x, y, sprite_width(s) * scale, s->h * scale);
    for (int r = 0; r < s->h; r++)
        for (int c = 0; s->rows[r][c]; c++)
            if (s->rows[r][c] != '.')
                display_fill_rect(x + c * scale, y + r * scale, scale, scale, multi_color(s->rows[r][c]));
    display_group_end();
}

SPR(spr_trefoil, "....yyyyy....", "...yyyyyyy...", "....yyyyy....", ".....yyy.....", ".............", ".....yyy.....", "yyyy.yyy.yyyy", "yyyy.yyy.yyyy", "yyyyy...yyyyy", ".yyyy...yyyy.", ".yyy.....yyy.", "..yy.....yy..", ".............");

void ui_trefoil(int x, int y, uint16_t color, int scale) { ui_sprite(x, y, &spr_trefoil, color, scale); }

void ui_hazard(int x, int y, uint16_t color, int scale)
{
    display_group_begin(x, y, 17 * scale, 14 * scale);
    for (int row = 0; row < 14; row++) {
        int half = (row * 8 + 6) / 13;
        for (int c = 8 - half; c <= 8 + half; c++) {
            bool mark = c == 8 && ((row >= 4 && row <= 9) || row == 11);
            display_fill_rect(x + c * scale, y + row * scale, scale, scale, mark ? ui_theme->bg : color);
        }
    }
    display_group_end();
}

/* ---- text ---- */

void ui_text(const font_t *f, int x, int y, const char *s, uint16_t fg, uint16_t bg) { font_draw(f, x, y, s, fg, bg, 0); }

void ui_text_right(const font_t *f, int right, int y, const char *s, uint16_t fg, uint16_t bg)
{
    font_draw(f, right - font_width(f, s, 0), y, s, fg, bg, 0);
}

void ui_text_center(const font_t *f, int cx, int y, const char *s, uint16_t fg, uint16_t bg)
{
    font_draw(f, cx - font_width(f, s, 0) / 2, y, s, fg, bg, 0);
}

void ui_label(const font_t *f, int x, int y, int w, const char *s, uint16_t fg, uint16_t bg, int align)
{
    font_label(f, x, y, w, s, fg, bg, align);
}

/* A UTF-8 character that is written without spaces between words (Chinese, Japanese, Korean): the line may break
 * after any of them. */
static bool cjk_lead(unsigned char c) { return c >= 0xE3 && c <= 0xEF; }

/* closing punctuation that must not start a line (full stop, comma, colon, question mark, closing brackets) */
static bool no_line_start(const char *w)
{
    unsigned char a = (unsigned char)w[0], b = (unsigned char)w[1], c = (unsigned char)w[2];
    if (a == 0xE3 && b == 0x80)
        return c == 0x81 || c == 0x82 || c == 0x8D || c == 0x8F || c == 0x89 || c == 0x8B; /* 、。」』〉》 */
    if (a == 0xEF && b == 0xBC)
        return c == 0x8C || c == 0x8E || c == 0x9A || c == 0x9B || c == 0x81 || c == 0x9F || c == 0x89; /* ，．：；！？） */
    return false;
}

int ui_text_wrap(const font_t *f, int x, int y, int w, const char *s, uint16_t fg, uint16_t bg, int line_gap)
{
    char line[160];
    int n = 0;
    bool line_cjk_end = false; /* the line so far ends with such a character */
    const char *p = s;
    while (*p) {
        /* take the next word: a run of letters, or one character of a script without spaces */
        const char *q = p;
        bool cjk = cjk_lead((unsigned char)*p);
        if (cjk) {
            q += 3; /* one three-byte character */
        } else {
            while (*q && *q != ' ' && !cjk_lead((unsigned char)*q))
                q++;
        }
        int wl = (int)(q - p);
        char word[48];
        if (wl > 47)
            wl = 47;
        for (int i = 0; i < wl; i++)
            word[i] = p[i];
        word[wl] = 0;

        /* a space was there between the last word and this one, unless a script without spaces is involved */
        bool space = n && (p > s && p[-1] == ' ');
        char trial[160];
        int tn = 0;
        for (int i = 0; i < n; i++)
            trial[tn++] = line[i];
        if (space && tn < 158)
            trial[tn++] = ' ';
        for (int i = 0; i < wl && tn < 158; i++)
            trial[tn++] = word[i];
        trial[tn] = 0;

        if (n && font_width(f, trial, 0) > w && !no_line_start(word)) {
            line[n] = 0;
            font_draw(f, x, y, line, fg, bg, 0);
            y += f->height + line_gap;
            n = 0;
            for (int i = 0; i < wl && n < 158; i++)
                line[n++] = word[i];
        } else {
            n = tn;
            for (int i = 0; i < tn; i++)
                line[i] = trial[i];
        }
        line_cjk_end = cjk;
        p = q;
        while (*p == ' ')
            p++;
    }
    (void)line_cjk_end;
    if (n) {
        line[n] = 0;
        font_draw(f, x, y, line, fg, bg, 0);
        y += f->height + line_gap;
    }
    if (y > UI_H - UI_FOOTER_H + 2)
        layout_warn("text runs into the footer", s, y, UI_H - UI_FOOTER_H);
    return y;
}

/* ---- shapes ---- */

void ui_clear(void) { display_fill_rect(0, 0, UI_W, UI_H, ui_theme->bg); }

void ui_notched(int x, int y, int w, int h, uint16_t color, uint16_t behind, int n)
{
    (void)behind;
    display_fill_rect(x + n, y, w - 2 * n, h, color);
    display_fill_rect(x, y + n, n, h - 2 * n, color);
    display_fill_rect(x + w - n, y + n, n, h - 2 * n, color);
}

void ui_panel(int x, int y, int w, int h, uint16_t color)
{
    display_group_begin(x, y, w, h);
    /* the four corner squares keep whatever is behind them: clear them to the background first */
    display_fill_rect(x, y, 4, 4, ui_theme->bg);
    display_fill_rect(x + w - 4, y, 4, 4, ui_theme->bg);
    display_fill_rect(x, y + h - 4, 4, 4, ui_theme->bg);
    display_fill_rect(x + w - 4, y + h - 4, 4, 4, ui_theme->bg);
    ui_notched(x, y, w, h, color, ui_theme->bg, 4);
    display_group_end();
}

int ui_chip(int x, int y, const char *s, uint16_t bg)
{
    int w = font_width(&font_12_medium, s, 1) + 18;
    display_group_begin(x, y, w, 18);
    display_fill_rect(x, y, 3, 3, ui_theme->bg);
    display_fill_rect(x + w - 3, y, 3, 3, ui_theme->bg);
    display_fill_rect(x, y + 15, 3, 3, ui_theme->bg);
    display_fill_rect(x + w - 3, y + 15, 3, 3, ui_theme->bg);
    ui_notched(x, y, w, 18, bg, ui_theme->bg, 3);
    font_draw(&font_12_medium, x + 9, y + 1, s, ui_theme->bg, bg, 1);
    display_group_end();
    return w;
}

void ui_divider(int y)
{
    display_group_begin(0, y, UI_W, 4);
    for (int x = 0; x < UI_W; x += 8)
        display_fill_rect(x, y, 4, 4, ui_theme->line);
    for (int x = 4; x < UI_W; x += 8)
        display_fill_rect(x, y, 4, 4, ui_theme->bg);
    display_group_end();
}

void ui_meter(int x, int y, int w, int lit, bool zones, uint16_t mono)
{
    for (int i = 0; i < 20; i++) {
        int x0 = x + (i * (w + 2)) / 20;
        int x1 = x + ((i + 1) * (w + 2)) / 20 - 2;
        uint16_t c = ui_theme->line;
        if (i < lit) {
            if (!zones)
                c = mono;
            else
                c = i < 10 ? ui_theme->ok : i < 15 ? ui_theme->warn : i < 18 ? ui_theme->hot : ui_theme->bad;
        }
        display_fill_rect(x0, y, x1 - x0, 14, c);
    }
}

void ui_columns(int x, int y, int w, int h, const uint32_t *vals, int n, uint32_t max, int bs, int col_w, int gap,
                uint16_t color, bool auto_color, int32_t threshold)
{
    (void)w;
    int max_blocks = h / bs;
    if (max == 0)
        max = 1;
    for (int i = 0; i < n; i++) {
        int cx = x + i * (col_w + gap);
        int cell = cx + col_w + gap <= DISPLAY_W ? col_w + gap : col_w; /* the gap belongs to the column: it is cleaned with it */
        display_group_begin(cx, y, cell, h);
        display_fill_rect(cx, y, cell, h, ui_theme->bg);
        uint64_t v = vals[i] > max ? max : vals[i];
        int blocks = (int)((v * (uint64_t)max_blocks + max / 2) / max);
        if (blocks < 1 && vals[i] > 0)
            blocks = 1;
        for (int k = 0; k < blocks; k++) {
            uint16_t c = color;
            if (auto_color) {
                int pct = k * 100 / max_blocks;
                c = pct < 60 ? ui_theme->acc : pct < 80 ? ui_theme->warn : ui_theme->bad;
            }
            display_fill_rect(cx, y + h - (k + 1) * bs + 1, col_w, bs - 1, c);
        }
        display_group_end();
    }
    if (threshold >= 0) {
        int ty = y + h - (int)((uint64_t)threshold * (uint64_t)h / max);
        if (ty >= y && ty < y + h) {
            display_group_begin(x, ty, n * (col_w + gap), 1);
            for (int dx = 0; dx < n * (col_w + gap); dx += 7)
                display_fill_rect(x + dx, ty, 4, 1, ui_theme->bad);
            display_group_end();
        }
    }
}

/* ---- compound elements ---- */

void ui_battery_icon(int x, int y, int percent, uint16_t color)
{
    display_group_begin(x, y, 22, 10);
    display_fill_rect(x, y, 19, 10, ui_theme->dim);
    display_fill_rect(x + 19, y + 3, 3, 4, ui_theme->dim);
    display_fill_rect(x + 2, y + 2, 15, 6, ui_theme->bg);
    int segs = (percent + 12) / 25;
    if (segs > 4)
        segs = 4;
    for (int i = 0; i < 3 + 1; i++)
        display_fill_rect(x + 3 + i * 4, y + 3, 3, 4, i < segs ? color : ui_theme->bg);
    display_group_end();
}

static void key_box(int x, int y, const char *label, int *wout)
{
    int tw = font_width(&font_12_regular, label, 0);
    int w = tw + 10;
    uint16_t c = ui_theme->dim;
    display_group_begin(x, y, w, 17);
    display_fill_rect(x, y, w, 2, c);
    display_fill_rect(x, y + 15, w, 2, c);
    display_fill_rect(x, y, 2, 17, c);
    display_fill_rect(x + w - 2, y, 2, 17, c);
    display_fill_rect(x + 2, y + 2, w - 4, 13, ui_theme->bg);
    font_draw(&font_12_regular, x + 5, y + 1, label, c, ui_theme->bg, 0);
    display_group_end();
    *wout = w;
}

/* draws text where [XX] becomes a small key box; returns the width used */
static int rich(int x, int y, const char *s, uint16_t fg)
{
    int x0 = x;
    char buf[48];
    while (*s) {
        if (*s == '[') {
            int n = 0;
            s++;
            while (*s && *s != ']' && n < 40)
                buf[n++] = *s++;
            buf[n] = 0;
            if (*s == ']')
                s++;
            int w;
            key_box(x, y - 1, buf, &w);
            x += w + 4;
        } else {
            int n = 0;
            while (*s && *s != '[' && n < 44)
                buf[n++] = *s++;
            buf[n] = 0;
            font_draw(&font_12_regular, x, y, buf, fg, ui_theme->bg, 0);
            x += font_width(&font_12_regular, buf, 0);
        }
    }
    return x - x0;
}

static int rich_width(const char *s)
{
    int w = 0;
    char buf[48];
    while (*s) {
        if (*s == '[') {
            int n = 0;
            s++;
            while (*s && *s != ']' && n < 40)
                buf[n++] = *s++;
            buf[n] = 0;
            if (*s == ']')
                s++;
            w += font_width(&font_12_regular, buf, 0) + 10 + 4;
        } else {
            int n = 0;
            while (*s && *s != '[' && n < 44)
                buf[n++] = *s++;
            buf[n] = 0;
            w += font_width(&font_12_regular, buf, 0);
        }
    }
    return w;
}

void ui_statusbar(const char *left, int battery_percent, bool charging)
{
    display_fill_rect(0, 0, UI_W, UI_STATUS_H, ui_theme->bg);
    font_draw(&font_12_regular, UI_MARGIN, 6, left, ui_theme->dim, ui_theme->bg, 0);

    char pct[8];
    int n = 0;
    int v = battery_percent;
    if (v >= 100) { pct[n++] = '1'; pct[n++] = '0'; pct[n++] = '0'; }
    else { if (v >= 10) pct[n++] = (char)('0' + v / 10); pct[n++] = (char)('0' + v % 10); }
    pct[n++] = '%';
    pct[n] = 0;
    int pw = font_width(&font_12_regular, pct, 0);
    int right = UI_W - UI_MARGIN;
    font_draw(&font_12_regular, right - pw, 6, pct, ui_theme->dim, ui_theme->bg, 0);
    ui_battery_icon(right - pw - 28, 8, battery_percent, charging ? ui_theme->acc : ui_theme->ok);
}

void ui_header(const sprite_t *icon, uint16_t icon_color, const char *title)
{
    display_fill_rect(0, UI_STATUS_H, UI_W, 30, ui_theme->bg);
    int x = UI_MARGIN;
    if (icon) {
        ui_sprite(x, UI_STATUS_H + 4, icon, icon_color, 2);
        x += sprite_w(icon) * 2 + 8;
    }
    font_draw(&font_20_medium, x, UI_STATUS_H - 1, title, ui_theme->ink, ui_theme->bg, 0);
    ui_divider(UI_STATUS_H + 26);
}

void ui_footer(const char *left, const char *right)
{
    int y0 = UI_H - UI_FOOTER_H;
    display_fill_rect(0, y0 + 4, UI_W, UI_FOOTER_H - 4, ui_theme->bg);
    ui_divider(y0);
    if (left && right && rich_width(left) + rich_width(right) + 16 > UI_W - 2 * UI_MARGIN) {
        char both[96]; /* "left | right", so that the report names both texts */
        int n = 0;
        for (const char *p = left; *p && n < 44; p++)
            both[n++] = *p;
        both[n++] = ' ';
        both[n++] = '|';
        both[n++] = ' ';
        for (const char *p = right; *p && n < 94; p++)
            both[n++] = *p;
        both[n] = 0;
        layout_warn("footer", both, rich_width(left) + rich_width(right), UI_W - 2 * UI_MARGIN - 16);
    }
    if (left)
        rich(UI_MARGIN, y0 + 10, left, ui_theme->dim);
    if (right)
        rich(UI_W - UI_MARGIN - rich_width(right), y0 + 10, right, ui_theme->dim);
}

void ui_stat_panel(int x, int y, int w, const char *label, const char *value, const char *unit)
{
    ui_panel(x, y, w, 50, ui_theme->panel);
    font_draw(&font_12_regular, x + 10, y + 5, label, ui_theme->dim, ui_theme->panel, 0);
    int vw = font_width(&font_20_medium, value, 0);
    if (vw + 4 + (unit && *unit ? font_width(&font_12_regular, unit, 0) : 0) + 18 > w)
        layout_warn("panel", value, vw, w - 22);
    if (font_width(&font_12_regular, label, 0) + 20 > w)
        layout_warn("panel label", label, font_width(&font_12_regular, label, 0), w - 20);
    font_draw(&font_20_medium, x + 10, y + 21, value, ui_theme->ink, ui_theme->panel, 0);
    if (unit && *unit)
        font_draw(&font_12_regular, x + 10 + vw + 4, y + 31, unit, ui_theme->dim, ui_theme->panel, 0);
}

void ui_row_icon(int y, const sprite_t *icon, const char *label, bool selected)
{
    uint16_t bg = selected ? ui_theme->acc : ui_theme->bg;
    uint16_t fg = selected ? ui_theme->bg : ui_theme->ink;
    display_fill_rect(0, y, UI_W, 26, bg);
    int iw = 0;
    for (int r = 0; r < icon->h; r++) {
        int c = 0;
        while (icon->rows[r][c])
            c++;
        if (c > iw)
            iw = c;
    }
    if (font_width(&font_14_medium, label, 0) > UI_W - 2 * UI_MARGIN - 28)
        layout_warn("menu row", label, font_width(&font_14_medium, label, 0), UI_W - 2 * UI_MARGIN - 28);
    ui_sprite(UI_MARGIN + (20 - iw) / 2, y + (26 - icon->h) / 2, icon, fg, 1);
    font_draw(&font_14_medium, UI_MARGIN + 28, y + 4, label, fg, bg, 0);
}

void ui_row(int y, const char *label, const char *value, bool selected)
{
    uint16_t bg = selected ? ui_theme->acc : ui_theme->bg;
    uint16_t fg = selected ? ui_theme->bg : ui_theme->ink;
    display_fill_rect(0, y, UI_W, 30, bg);
    if (value && *value && font_width(&font_14_medium, label, 0) + font_width(&font_14_medium, value, 0) + 16 > UI_W - 2 * UI_MARGIN)
        layout_warn("row", label, font_width(&font_14_medium, label, 0) + font_width(&font_14_medium, value, 0), UI_W - 2 * UI_MARGIN - 16);
    font_draw(&font_14_medium, UI_MARGIN, y + 6, label, fg, bg, 0);
    if (value && *value)
        font_draw(&font_14_medium, UI_W - UI_MARGIN - font_width(&font_14_medium, value, 0), y + 6, value,
                  selected ? ui_theme->bg : ui_theme->acc, bg, 0);
}
