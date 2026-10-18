/* Canary A: same image size as the full firmware, but the only thing it does is paint the screen green,
 * wait 20 seconds and switch the device off. If this runs, size and the start-up integrity check are fine. */
#include "board.h"
#include "display.h"

/* ballast so the integrity check has to read as much flash as the real image */
__attribute__((used)) const unsigned char ballast[60000] = {1, 2, 3};

void systick_handler(void) {}
void exti3_handler(void) {}
void usb_isr(void) {}

int main(void)
{
    const unsigned char *volatile p = ballast; /* an opaque pointer keeps the linker from dropping the ballast */
    volatile unsigned char keep = p[100] + p[59999];
    (void)keep;
    board_init();
    display_init();
    display_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, RGB565(0, 160, 60));
    display_fill_rect(60, 120, 120, 80, RGB565(255, 255, 255));
    board_backlight(true);
    delay_ms(20000);
    board_power_off();
    return 0;
}
