/* Canary E: the same as canary A (known to start), but it shows how long the device really stays alive.
 * A bar grows 6 px every second for 35 s, then the device switches off. If it ends earlier, something outside
 * our code (for example a watchdog left running by the bootloader) resets the chip, and the bar length shows when. */
#include "board.h"
#include "display.h"

/* same size as canary A, which is known to start */
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
    for (int s = 1; s <= 35; s++) {
        display_fill_rect(10, 150, 6 * s, 20, RGB565(255, 255, 255));
        delay_ms(1000);
    }
    board_power_off();
    return 0;
}
