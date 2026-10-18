/* Canary G: finds the step where start-up stops. No text is needed to read the result: 18 squares in three rows turn
 * green one by one, one second apart, each only after the step before it went through. If the device switches off or
 * freezes, the number of green squares at that moment names the step that failed:
 *  1 flash data read   2 glyph table read   3 glyph search   4 division   5 multiply
 *  6 pixel window      7 one letter drawn   8 text drawn
 *  9 buzzer pin   10 vibration pin   11-13 LED pins   14 key pins   15 pins again   16 clock set   17 ADC set   18 tick running
 * After the last step a bar grows for 25 s, then the device switches off. */
#include "board.h"
#include "display.h"
#include "font.h"
#include "hal.h"
#include "regs.h"

const hal_t *hal_gc03_init(void);
void hal_gc03_start_tick(void);

__attribute__((used)) const unsigned char ballast[60000] = {1, 2, 3};

#define BG RGB565(10, 14, 20)
#define GREEN RGB565(74, 222, 128)

static void sq(int i, uint16_t c) { display_fill_rect(10 + (i % 6) * 38, 12 + (i / 6) * 40, 32, 32, c); }

static void done(int step) /* step 1..18 */
{
    sq(step - 1, GREEN);
    delay_ms(1000);
}

void dbg_mark(int step) { done(14 + step); } /* hal_gc03_init reports 1, 2, 3 -> squares 15, 16, 17 */

int main(void)
{
    const unsigned char *volatile bp = ballast;
    volatile unsigned char keep = bp[100] + bp[59999];
    (void)keep;
    board_init();
    display_init();
    display_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, BG);
    board_backlight(true);
    for (int i = 0; i < 18; i++)
        sq(i, RGB565(60, 20, 20));
    delay_ms(1000);

    volatile uint8_t b = font_20_medium.bits[3];
    (void)b;
    done(1);
    volatile uint16_t cp = font_20_medium.glyphs[5].cp;
    (void)cp;
    done(2);
    volatile int w = font_width(&font_20_medium, "YARROW", 0);
    (void)w;
    done(3);
    volatile int num = 1000, den = 255;
    volatile int q = num / den;
    (void)q;
    done(4);
    volatile short sa = 123, sb = 77;
    volatile int prod = sa * sb;
    (void)prod;
    done(5);
    display_window(150, 200, 5, 5);
    for (int i = 0; i < 25; i++)
        display_pixel(GREEN);
    display_end();
    done(6);
    font_draw(&font_20_medium, 10, 150, "Y", RGB565(255, 255, 255), BG, 0);
    done(7);
    font_draw(&font_20_medium, 10, 190, "YARROW 123", RGB565(255, 255, 255), BG, 0);
    done(8);

    /* the pins one by one, in the same order as the hardware layer sets them */
    gpio_write(GPIOC, 11, false); gpio_mode(GPIOC, 11, 0x3); /* buzzer */
    done(9);
    gpio_write(GPIOB, 1, false);  gpio_mode(GPIOB, 1, 0x3);  /* vibration */
    done(10);
    gpio_write(GPIOB, 14, true);  gpio_mode(GPIOB, 14, 0x3); /* LED */
    done(11);
    gpio_write(GPIOB, 15, true);  gpio_mode(GPIOB, 15, 0x3); /* LED */
    done(12);
    gpio_write(GPIOC, 8, true);   gpio_mode(GPIOC, 8, 0x3);  /* LED */
    done(13);
    for (int pin = 5; pin <= 9; pin++) {                       /* keys and knob */
        gpio_mode(GPIOB, pin, 0x8);
        gpio_write(GPIOB, pin, true);
    }
    gpio_mode(GPIOC, 6, 0x8); gpio_write(GPIOC, 6, true);
    gpio_mode(GPIOC, 7, 0x8); gpio_write(GPIOC, 7, true);
    done(14);

    const hal_t *h = hal_gc03_init(); /* marks 15, 16, 17 */
    hal_gc03_start_tick();
    done(18);

    uint32_t next = g_ms;
    int sec = 0;
    for (;;) {
        if ((int32_t)(g_ms - next) >= 0) {
            next += 1000;
            sec++;
            display_fill_rect(10, 260, 8 * sec, 14, GREEN);
            if (sec >= 25)
                h->power_off();
        }
        __asm__ volatile("wfi");
    }
}
