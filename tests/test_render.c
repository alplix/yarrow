/* The change-only redraw must leave the panel exactly as a full draw would, whatever changed between two frames. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"

static uint16_t fb[DISPLAY_W * DISPLAY_H], ref[DISPLAY_W * DISPLAY_H];
static uint16_t *target = fb;
static int wx, wy, ww, wh, wn;
static long raw_pixels;

void display_init(void) {}
void display_sleep(bool sleep) { (void)sleep; }
void display_raw_window(int x, int y, int w, int h) { wx = x; wy = y; ww = w; wh = h; wn = 0; }
void display_raw_pixel(uint16_t c)
{
    if (wn >= ww * wh)
        return;
    int px = wx + wn % ww, py = wy + wn / ww;
    wn++;
    target[py * DISPLAY_W + px] = c;
    if (target == fb)
        raw_pixels++;
}
void display_raw_end(void) {}
void display_raw_fill_rect(int x, int y, int w, int h, uint16_t c)
{
    display_raw_window(x, y, w, h);
    for (int i = 0; i < w * h; i++)
        display_raw_pixel(c);
}

#define N 60
typedef struct {
    int shape;           /* changes the number of operations */
    uint16_t val[N];
} scene_t;

static uint32_t rs = 0xC0FFEE;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }

static void draw(void *p)
{
    const scene_t *s = p;
    for (int i = 0; i < N; i++) {
        int x = (i % 8) * 28 + (i % 3) * 5, y = 8 + (i / 8) * 36 + (i % 5) * 3, w = 40, h = 30;
        if (i % 4 == 0) { /* a group: a rectangle and a window with a few pixels, like a panel with text */
            display_group_begin(x - 2, y - 2, w + 4, h + 4);
            display_fill_rect(x, y, w, h, (uint16_t)(s->val[i] + 1));
            display_group_begin(x + 3, y + 3, 6, 4); /* a group inside the group */
            display_window(x + 3, y + 3, 6, 4);
            for (int k = 0; k < 24; k++)
                display_pixel((uint16_t)(s->val[i] ^ (k & 3)));
            display_end();
            display_group_end();
            display_group_end();
        } else if (i % 4 == 1) {
            display_window(x, y, 4, 3);
            for (int k = 0; k < 12; k++)
                display_pixel(s->val[i]);
            display_end();
        } else if (i % 8 == 3) { /* a wide background and a text-like group whose width changes with its value */
            display_fill_rect(x, y, w, h, 0x0F0F);
            int gw = 6 + s->val[i] % 30;
            display_group_begin(x, y, gw, 12);
            display_window(x, y, gw, 12);
            for (int k = 0; k < gw * 12; k++)
                display_pixel((uint16_t)(s->val[i] + k % 5));
            display_end();
            display_group_end();
        } else {
            display_fill_rect(x, y, w, h, s->val[i]);
        }
    }
    if (s->shape)
        display_fill_rect(5, 300, 100, 10, 0x1234);
}

int main(void)
{
    scene_t s;
    memset(&s, 0, sizeof s);
    int failures = 0;
    long drawn_total = 0, frames_with_skip = 0;

    for (int frame = 0; frame < 4000; frame++) {
        bool full = frame == 0 || rnd() % 40 == 0;
        int changes = rnd() % 4 == 0 ? 0 : (int)(rnd() % 5);
        for (int c = 0; c < changes; c++)
            s.val[rnd() % N] = (uint16_t)rnd();
        if (rnd() % 50 == 0)
            s.shape = !s.shape;
        uint32_t key = rnd() % 200 == 0 ? 2 : 1;

        /* reference: the plain full draw on its own buffer */
        uint16_t *saved = target;
        target = ref;
        if (full)
            memset(ref, 0, sizeof ref);
        draw(&s);
        target = saved;

        long before = raw_pixels;
        if (full)
            memset(fb, 0, sizeof fb);
        display_render(key, full, draw, &s);
        long drawn = raw_pixels - before;
        drawn_total += drawn;
        if (!full && drawn < 3000)
            frames_with_skip++;

        if (memcmp(fb, ref, sizeof fb) != 0) {
            failures++;
            if (failures < 5)
                printf("FAIL frame %d: panel differs from a full draw\n", frame);
        }
    }
    printf("render: %d failures, %ld pixels written in 4000 frames (%ld frames mostly skipped)\n", failures, drawn_total,
           frames_with_skip);
    if (failures == 0)
        printf("all render checks passed\n");
    return failures != 0;
}
