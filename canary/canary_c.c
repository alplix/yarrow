/* Canary C: the text drawing and the watchdog, with a bar that shows progress even if the text looks wrong.
 * Top: two lines of text. Below: a bar grows each second while the watchdog is kicked (10 s), then the kicks stop.
 * A working watchdog resets the chip about 4 s later and start-up then switches the device off.
 * If the bar keeps growing past 20 s the watchdog does not work; the device switches itself off at 40 s anyway. */
#include "board.h"
#include "display.h"
#include "font.h"
#include "fmt.h"

void systick_handler(void) {}
void exti3_handler(void) {}
void usb_isr(void) {}

#define BG RGB565(10, 14, 20)

int main(void)
{
    board_init();
    display_init();
    display_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, BG);
    board_backlight(true);

    /* plain blocks first: if only these show, the text path is the problem */
    display_fill_rect(10, 10, 220, 6, RGB565(74, 222, 128));
    font_draw(&font_20_medium, 10, 30, "YARROW 123", RGB565(238, 243, 248), BG, 0);
    font_draw(&font_12_regular, 10, 70, "bekci testi", RGB565(238, 243, 248), BG, 0);
    display_fill_rect(10, 100, 220, 6, RGB565(74, 222, 128));

    watchdog_start();

    for (int s = 1; s <= 40; s++) {
        display_fill_rect(10, 130, 5 * s, 14, s <= 10 ? RGB565(74, 222, 128) : RGB565(248, 113, 113));
        for (int i = 0; i < 10; i++) {
            if (s <= 10)
                watchdog_kick();
            delay_ms(100);
        }
    }
    board_power_off();
    return 0;
}
