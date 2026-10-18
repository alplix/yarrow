#ifndef YARROW_UI_BOOT_H
#define YARROW_UI_BOOT_H

#include "theme.h"

#define BOOT_STEPS 10

/* Draws the whole start-up screen. */
void ui_boot_draw(const theme_t *t);

/* Fills the loading bar up to `steps` blocks (0..BOOT_STEPS). */
void ui_boot_progress(const theme_t *t, int steps);

#endif
