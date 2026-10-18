#ifndef YARROW_BOARD_H
#define YARROW_BOARD_H

#include <stdbool.h>
#include <stdint.h>

/* Brings up power hold, clocks and GPIO. Power is latched first thing. */
void board_init(void);

void delay_ms(uint32_t ms);

extern volatile uint32_t g_ms;    /* milliseconds since the tick interrupt started */
extern volatile bool g_tick_irq;  /* true once the tick interrupt is running */
uint32_t board_cpu_hz(void);

/* Power key (PA9, input with pull-up): true while the line is low. */
bool board_power_key_low(void);

/* Releases the power latch; the device switches itself off. */
void board_power_off(void);

void board_backlight(bool on);

/* Independent watchdog, about 4 s. If the main loop stops calling watchdog_kick() the chip resets,
 * and start-up then switches the device off instead of running again. */
void watchdog_start(void);
void watchdog_kick(void);

void gpio_mode(uint32_t port, unsigned pin, uint32_t mode);
void gpio_write(uint32_t port, unsigned pin, bool high);
bool gpio_read(uint32_t port, unsigned pin);

#endif
