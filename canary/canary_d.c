/* Canary D: where does hardware start-up stop? No text, only coloured squares and bars.
 * Row of squares: 0 display ready, 1 pins set, 2 clock (RTC) set up, 3 ADC set up, 4 tick running.
 * Then a bar grows once per second (tick works) and live bars show keys and battery. Off after 60 s. */
#include "board.h"
#include "display.h"
#include "hal.h"
#include "regs.h"

const hal_t *hal_gc03_init(void);
void hal_gc03_start_tick(void);

/* same ballast as canary A (that size is known to start): rules out the image size as the cause */
__attribute__((used)) const unsigned char ballast[60000] = {1, 2, 3};

#define BG RGB565(10, 14, 20)

static void box(int x, int y, int w, int h, uint16_t c) { display_fill_rect(x, y, w, h, c); }

void dbg_mark(int step) { box(10 + step * 40, 40, 30, 30, RGB565(74, 222, 128)); }

int main(void)
{
    const unsigned char *volatile bp = ballast;
    volatile unsigned char keep = bp[100] + bp[59999];
    (void)keep;
    board_init();
    display_init();
    box(0, 0, DISPLAY_W, DISPLAY_H, BG);
    board_backlight(true);
    for (int i = 0; i < 5; i++)
        box(10 + i * 40, 40, 30, 30, RGB565(60, 20, 20)); /* red = not reached yet */
    dbg_mark(0);

    const hal_t *h = hal_gc03_init();
    hal_gc03_start_tick();
    dbg_mark(4);

    uint32_t next = g_ms;
    int sec = 0;
    for (;;) {
        if ((int32_t)(g_ms - next) >= 0) {
            next += 1000;
            sec++;
            box(10, 90, 4 * sec, 12, RGB565(250, 204, 21));       /* grows 4 px per second */
            uint32_t mv = h->battery_mv();
            int w = mv > 3000 ? (int)((mv - 3000) / 6) : 0;         /* 3.0 V .. 4.4 V over 230 px */
            box(10, 120, 220, 12, RGB565(38, 52, 74));
            box(10, 120, w > 220 ? 220 : w, 12, RGB565(74, 222, 128));
            bool k[4] = {!(GPIO_IDR(GPIOA) & (1u << 9)), !(GPIO_IDR(GPIOB) & (1u << 9)),
                         !(GPIO_IDR(GPIOB) & (1u << 8)), !(GPIO_IDR(GPIOB) & (1u << 6))};
            for (int i = 0; i < 4; i++)
                box(10 + i * 40, 150, 30, 30, k[i] ? RGB565(74, 222, 128) : RGB565(38, 52, 74));
            if (sec >= 60)
                h->power_off();
        }
        __asm__ volatile("wfi");
    }
}
