/* Everything the application needs from the hardware. The real device (hal/gc03), the simulator (sim) and the
 * tests each provide one of these. The application never touches a register. */
#ifndef YARROW_HAL_H
#define YARROW_HAL_H

#include "ylog.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    EV_NONE = 0,
    EV_LEFT,       /* knob counter-clockwise */
    EV_RIGHT,      /* knob clockwise */
    EV_OK,         /* knob press or the enter key */
    EV_BACK,       /* the back key */
    EV_POWER,      /* short press of the power key */
    EV_POWER_LONG, /* long press of the power key */
} input_event_t;

typedef struct {
    /* time: unix seconds, 0 while the clock has never been set */
    uint32_t (*rtc_now)(void);
    void (*rtc_set)(uint32_t unix_s);

    input_event_t (*poll_event)(void);

    /* sensors */
    uint32_t (*tube_pulses)(void);       /* cumulative pulse count since power on */
    uint32_t (*battery_mv)(void);
    bool (*usb_present)(void);
    bool (*charging)(void);
    uint32_t (*efield_vm_x10)(void);     /* electric field, tenths of V/m */
    uint32_t (*hfield_ut_x100)(void);    /* magnetic field, hundredths of uT */

    /* tube high voltage: started from the factory calibration; refused (false) if that is missing or damaged */
    bool (*hv_factory_valid)(void);
    bool (*hv_start)(void);
    void (*hv_stop)(void);

    /* outputs */
    void (*beep)(uint32_t hz);           /* 0 stops the tone */
    void (*vibrate)(bool on);
    void (*alert_led)(bool on);
    void (*backlight)(uint8_t level);    /* 0 off, 1..5 */
    void (*display_enable)(bool on);
    void (*power_off)(void);
    void (*watchdog_kick)(void);

    /* flash windows for the log (many pages) and the settings store (two pages) */
    const ylog_flash_t *log_flash;
    const ylog_flash_t *kv_flash;

    bool (*usb_connected)(void);  /* a host has the serial port open */

    /* serial port (USB or UART): non-blocking read returns the bytes available, write sends all of them */
    int (*serial_read)(uint8_t *buf, uint32_t max);
    int (*serial_write)(const uint8_t *buf, uint32_t n);
    void (*usb_start)(void);      /* brings the USB port up (last start-up stage) */
} hal_t;

#endif
