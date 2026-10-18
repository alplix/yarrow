/* Canary F: canary E plus the text drawing, nothing else. No watchdog, no hardware layer.
 * Big text at the top, the progress bar below. Off after 30 s. */
#include "board.h"
#include "display.h"
#include "font.h"

__attribute__((used)) const unsigned char ballast[60000] = {1, 2, 3};

void systick_handler(void) {}
void exti3_handler(void) {}
void usb_isr(void) {}

int main(void)
{
    const unsigned char *volatile p = ballast;
    volatile unsigned char keep = p[100] + p[59999];
    (void)keep;
    board_init();
    display_init();
    display_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, RGB565(0, 90, 40));
    board_backlight(true);
    display_fill_rect(10, 100, 220, 6, RGB565(255, 255, 255));
    font_draw(&font_20_medium, 10, 20, "YARROW 123", RGB565(255, 255, 255), RGB565(0, 90, 40), 0);
    font_draw(&font_12_regular, 10, 60, "yazi testi", RGB565(255, 255, 255), RGB565(0, 90, 40), 0);
    for (int s = 1; s <= 30; s++) {
        display_fill_rect(10, 150, 7 * s, 20, RGB565(255, 255, 255));
        delay_ms(1000);
    }
    board_power_off();
    return 0;
}
