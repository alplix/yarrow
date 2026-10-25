#ifndef YARROW_SCREENS_H
#define YARROW_SCREENS_H

#include "uidata.h"
#include <stdbool.h>

/* Every draw function repaints its whole screen when `full` is true and only the changing parts otherwise.
 * Static parts are drawn once, when the screen is entered. */
typedef void (*screen_fn)(const ui_data_t *d, const ui_state_t *s, bool full);

typedef enum {
    SCR_MAIN, SCR_TREND, SCR_EFIELD, SCR_HFIELD, SCR_BATTERY, SCR_HISTORY,
    SCR_ALARM, SCR_MODE, SCR_MENU,
    SCR_GUIDE_LIST, SCR_GUIDE_PAGE, SCR_SETTINGS, SCR_EVENTS, SCR_BUDGET, SCR_SAMPLE, SCR_LOG, SCR_THRESH,
    SCR_TUBE, SCR_SELFTEST, SCR_USB, SCR_SCAN, SCR_THEMES, SCR_ABOUT, SCR_CLOCK, SCR_CALIB, SCR_CHARGE, SCR_CALWIZ, SCR_SESSION,
    SCR_SAVER, SCR_HEALTH, SCR_DEADTIME,
    SCR_COUNT
} screen_id_t;

void screen_draw(screen_id_t id, const ui_data_t *d, const ui_state_t *s, bool full);

#endif
