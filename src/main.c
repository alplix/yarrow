/* Yarrow for the FNIRSI GC-03: start-up and the main loop. */

#include "app.h"
#include "board.h"
#include "display.h"
#include "proto.h"
#include "theme.h"
#include "ui_boot.h"

const hal_t *hal_gc03_init(void);
void hal_gc03_start_tick(void);

static app_t app;
static proto_t proto;

int main(void)
{
    const theme_t *t = &themes[THEME_DARK];

    board_init();
    watchdog_start();
    display_init();
    ui_boot_draw(t);
    board_backlight(true);

    const hal_t *hal = hal_gc03_init();
    ui_boot_progress(t, 3);
    hal_gc03_start_tick();
    ui_boot_progress(t, 6);

    app_init(&app, hal, g_ms);
    proto_init(&proto, &app);
    ui_boot_progress(t, 10);
    for (int i = 0; i < 4; i++) { /* keep the boot screen readable for about two seconds */
        watchdog_kick();
        delay_ms(500);
    }

    for (;;) {
        uint32_t now = g_ms;
        app_poll(&app, now);
        proto_poll(&proto);
        proto_tick(&proto, app.ui.now);
        while (g_ms == now)
            __asm__ volatile("wfi");
    }
}
