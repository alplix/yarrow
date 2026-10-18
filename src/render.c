/* Draws only what changed.
 *
 * The screens redraw themselves in full once a second. Clearing a box and drawing the new text on top of it shows as
 * flicker. Without a frame buffer the way around it is to draw every screen twice:
 *   1. a recording pass: nothing reaches the panel, each drawing operation (a rectangle, a pixel window, or a group such
 *      as one line of text) only gets a fingerprint of its position and pixels;
 *   2. a replay pass that executes only the operations that have to run.
 *
 * Which ones have to run: an operation that differs from the one at the same place in the previous frame, and every
 * operation that touches an area such an operation covers now or covered before (an old wide text leaves pixels that
 * the background drawn earlier has to clean up, a repainted background has to be covered by the text drawn after it).
 * Operations that touch none of those areas keep exactly what the panel shows, so the result is the same as a full
 * draw. A full redraw (or the first frame of a screen) executes everything. */

#include "display.h"

#define OPS_MAX 104
#define DIRTY_MAX 40

typedef struct {
    uint16_t x, y, w, h;
    uint32_t hash;
} op_t;

enum { M_LIVE, M_RECORD, M_REPLAY };

static op_t ops[OPS_MAX];
static op_t prev_ops[OPS_MAX];
static int prev_n;
static uint32_t prev_key;
static bool prev_valid;
static int peak_ops, last_ops;

static uint8_t need[OPS_MAX / 8];
static uint16_t dirty[DIRTY_MAX][4];
static int dirty_n;
static bool dirty_overflow;

static uint8_t mode;
static int op_n, op_i, group_depth;
static bool in_group, skipping, win_draw;
static uint32_t acc;

static void mix(uint32_t v) { acc = (acc ^ v) * 16777619u; }

static bool need_get(int i) { return need[i >> 3] & (1u << (i & 7)); }
static void need_set(int i) { need[i >> 3] |= (uint8_t)(1u << (i & 7)); }

static bool rect_empty(const op_t *o) { return o->w == 0 || o->h == 0; }

static bool hits(const uint16_t *r, const op_t *o)
{
    return !rect_empty(o) && o->x < r[0] + r[2] && r[0] < o->x + o->w && o->y < r[1] + r[3] && r[1] < o->y + o->h;
}

static bool hits_dirty(const op_t *o)
{
    for (int i = 0; i < dirty_n; i++)
        if (hits(dirty[i], o))
            return true;
    return false;
}

static void add_dirty(const op_t *o)
{
    if (rect_empty(o))
        return;
    for (int i = 0; i < dirty_n; i++) { /* already inside a dirty rectangle */
        const uint16_t *r = dirty[i];
        if (o->x >= r[0] && o->y >= r[1] && o->x + o->w <= r[0] + r[2] && o->y + o->h <= r[1] + r[3])
            return;
    }
    if (dirty_n >= DIRTY_MAX) {
        dirty_overflow = true;
        return;
    }
    dirty[dirty_n][0] = o->x; dirty[dirty_n][1] = o->y; dirty[dirty_n][2] = o->w; dirty[dirty_n][3] = o->h;
    dirty_n++;
}

static bool same_op(const op_t *a, const op_t *b)
{
    return a->x == b->x && a->y == b->y && a->w == b->w && a->h == b->h && a->hash == b->hash;
}

/* Decides which operations of the frame just recorded have to run. */
static void choose(int n, uint32_t key, bool full)
{
    bool diff = !full && prev_valid && prev_key == key && n <= OPS_MAX;
    if (!diff) {
        for (unsigned i = 0; i < sizeof need; i++)
            need[i] = 0xFF;
        return;
    }
    for (unsigned i = 0; i < sizeof need; i++)
        need[i] = 0;
    dirty_n = 0;
    dirty_overflow = false;

    int m = n > prev_n ? n : prev_n;
    for (int i = 0; i < m; i++) {
        bool same = i < n && i < prev_n && same_op(&ops[i], &prev_ops[i]);
        if (same)
            continue;
        if (i < n) {
            need_set(i);
            add_dirty(&ops[i]);
        }
        if (i < prev_n)
            add_dirty(&prev_ops[i]);
    }
    bool grew = true;
    while (grew && !dirty_overflow) {
        grew = false;
        for (int i = 0; i < n; i++) {
            if (!need_get(i) && hits_dirty(&ops[i])) {
                need_set(i);
                add_dirty(&ops[i]);
                grew = true;
            }
        }
    }
    if (dirty_overflow)
        for (unsigned i = 0; i < sizeof need; i++)
            need[i] = 0xFF;
}

static void op_begin(int x, int y, int w, int h)
{
    if (mode == M_RECORD) {
        if (op_n < OPS_MAX) {
            op_t *o = &ops[op_n];
            o->x = (uint16_t)x; o->y = (uint16_t)y; o->w = (uint16_t)w; o->h = (uint16_t)h;
        }
        op_n++;
        acc = 2166136261u;
        mix((uint32_t)x | ((uint32_t)y << 16));
        mix((uint32_t)w | ((uint32_t)h << 16));
    } else if (mode == M_REPLAY) {
        skipping = op_i < OPS_MAX && !need_get(op_i);
    }
}

static void op_end(void)
{
    if (mode == M_RECORD) {
        if (op_n <= OPS_MAX)
            ops[op_n - 1].hash = acc;
    } else if (mode == M_REPLAY) {
        op_i++;
        skipping = false;
    }
}

static bool drawing(void) { return mode == M_LIVE || (mode == M_REPLAY && !skipping); }

void display_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > DISPLAY_W || y + h > DISPLAY_H)
        return;
    if (mode == M_LIVE) {
        display_raw_fill_rect(x, y, w, h, color);
        return;
    }
    if (!in_group)
        op_begin(x, y, w, h);
    if (mode == M_RECORD) {
        if (in_group) {
            mix((uint32_t)x | ((uint32_t)y << 16));
            mix((uint32_t)w | ((uint32_t)h << 16));
        }
        mix(0xF0000u | color);
    }
    if (drawing())
        display_raw_fill_rect(x, y, w, h, color);
    if (!in_group)
        op_end();
}

void display_window(int x, int y, int w, int h)
{
    if (mode != M_LIVE) {
        if (!in_group)
            op_begin(x, y, w, h);
        else if (mode == M_RECORD) {
            mix((uint32_t)x | ((uint32_t)y << 16));
            mix((uint32_t)w | ((uint32_t)h << 16));
        }
    }
    win_draw = drawing();
    if (win_draw)
        display_raw_window(x, y, w, h);
}

void display_pixel(uint16_t color)
{
    if (mode == M_RECORD)
        mix(color);
    if (win_draw)
        display_raw_pixel(color);
}

void display_end(void)
{
    if (win_draw)
        display_raw_end();
    win_draw = false;
    if (mode != M_LIVE && !in_group)
        op_end();
}

void display_group_begin(int x, int y, int w, int h)
{
    if (mode == M_LIVE)
        return;
    if (group_depth++ > 0)
        return; /* a group inside a group is part of the outer one */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > DISPLAY_W) w = DISPLAY_W - x;
    if (y + h > DISPLAY_H) h = DISPLAY_H - y;
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    op_begin(x, y, w, h);
    in_group = true;
}

void display_group_end(void)
{
    if (mode == M_LIVE || --group_depth > 0)
        return;
    in_group = false;
    op_end();
}

void display_render(uint32_t key, bool full, void (*draw)(void *), void *arg)
{
    mode = M_RECORD;
    op_n = 0;
    in_group = false;
    group_depth = 0;
    draw(arg);

    last_ops = op_n;
    if (op_n > peak_ops)
        peak_ops = op_n;
    int n = op_n > OPS_MAX ? OPS_MAX : op_n;
    choose(op_n, key, full);

    mode = M_REPLAY;
    op_i = 0;
    skipping = false;
    in_group = false;
    group_depth = 0;
    draw(arg);
    mode = M_LIVE;
    in_group = false;

    if (!full && op_n <= OPS_MAX) {
        for (int i = 0; i < n; i++)
            prev_ops[i] = ops[i];
        prev_n = n;
        prev_key = key;
        prev_valid = true;
    } else {
        prev_valid = false;
    }
}

void display_render_forget(void) { prev_valid = false; }

/* the most operations any frame needed so far (to size the table) */
int display_render_peak(void) { return peak_ops; }
int display_render_last(void) { return last_ops; }

void (*layout_warn_hook)(const char *kind, const char *text, int width, int room);

void layout_warn(const char *kind, const char *text, int width, int room)
{
    if (layout_warn_hook)
        layout_warn_hook(kind, text, width, room);
}
