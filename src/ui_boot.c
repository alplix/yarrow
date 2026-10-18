#include "ui_boot.h"
#include "display.h"
#include "font.h"
#include "ui.h"

#define FLOWER_W 19
#define FLOWER_H 22
#define FLOWER_SCALE 6

#define BAR_BLOCK_W 12
#define BAR_BLOCK_H 10
#define BAR_GAP 3
#define BAR_W (BOOT_STEPS * BAR_BLOCK_W + (BOOT_STEPS - 1) * BAR_GAP)
#define BAR_Y 236

static void flower(const theme_t *t, int ox, int oy)
{
    char g[FLOWER_H][FLOWER_W];
    for (int y = 0; y < FLOWER_H; y++)
        for (int x = 0; x < FLOWER_W; x++)
            g[y][x] = '.';

#define PUT(px, py, c) do { int xx = (px), yy = (py); \
    if (xx >= 0 && xx < FLOWER_W && yy >= 0 && yy < FLOWER_H) g[yy][xx] = (c); } while (0)

    for (int y = 11; y < FLOWER_H; y++)
        PUT(9, y, 'g');
    for (int y = 13; y <= 19; y += 3) {
        PUT(8, y, 'g'); PUT(7, y - 1, 'g'); PUT(6, y - 1, 'g');
        PUT(10, y, 'g'); PUT(11, y - 1, 'g'); PUT(12, y - 1, 'g');
    }
    PUT(8, 10, 'g'); PUT(10, 10, 'g'); PUT(7, 9, 'g'); PUT(11, 9, 'g');

    static const signed char fl[10][2] = {{5, 3}, {9, 2}, {13, 3}, {3, 6}, {7, 6},
                                          {11, 6}, {15, 6}, {5, 9}, {9, 9}, {13, 9}};
    for (int i = 0; i < 10; i++) {
        int cx = fl[i][0], cy = fl[i][1];
        PUT(cx - 1, cy, 'w'); PUT(cx + 1, cy, 'w');
        PUT(cx, cy - 1, 'w'); PUT(cx, cy + 1, 'w');
        PUT(cx, cy, 'y');
    }

    for (int y = 0; y < FLOWER_H; y++)
        for (int x = 0; x < FLOWER_W; x++) {
            char c = g[y][x];
            if (c == '.')
                continue;
            uint16_t col = c == 'w' ? t->petal : c == 'y' ? t->warn : t->leaf;
            display_fill_rect(ox + x * FLOWER_SCALE, oy + y * FLOWER_SCALE, FLOWER_SCALE, FLOWER_SCALE, col);
        }
}

void ui_boot_progress(const theme_t *t, int steps)
{
    int x0 = (DISPLAY_W - BAR_W) / 2;
    for (int i = 0; i < BOOT_STEPS; i++)
        display_fill_rect(x0 + i * (BAR_BLOCK_W + BAR_GAP), BAR_Y, BAR_BLOCK_W, BAR_BLOCK_H,
                          i < steps ? t->ok : t->line);
}

void ui_boot_draw(const theme_t *t)
{
    display_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, t->bg);

    flower(t, (DISPLAY_W - FLOWER_W * FLOWER_SCALE) / 2, 24);

    font_draw_centered(&font_28_medium, 170, "YARROW", t->ink, t->bg, 6);
    font_draw_centered(&font_12_regular, 210, "GC-03 \xC2\xB7 v0.1", t->dim, t->bg, 0);

    ui_boot_progress(t, 0);

    for (int x = 0; x < DISPLAY_W; x += 8)
        display_fill_rect(x, 270, 4, 4, t->line);
    font_draw_centered(&font_14_medium, 280, "yarrow.athena.org.tr", t->acc, t->bg, 0);
    font_draw_centered(&font_12_regular, 299, "github.com/alplix/yarrow", t->dim, t->bg, 0);
}
