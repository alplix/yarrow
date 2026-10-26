/* Battery model for a single Li-ion cell measured only through a voltage divider.
 * Charge level comes from the discharge curve, remaining time from the drain observed over the last hours,
 * and wear from how long a full discharge takes compared with the best the cell has shown. */
#ifndef YARROW_BAT_H
#define YARROW_BAT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { BAT_DISCHARGING, BAT_CHARGING, BAT_FULL } bat_state_t;

#define BAT_HIST 36 /* level samples, one per 10 minutes of discharging */

/* what is kept in flash between runs */
typedef struct {
    uint32_t discharged_x10;     /* total discharge in tenths of a percent, for the cycle count */
    uint32_t on_seconds;         /* total running time */
    uint32_t best_runtime_h_x10; /* longest full-discharge runtime seen, in tenths of hours */
} bat_stats_t;

typedef struct {
    bat_stats_t stats;
    uint32_t mv_filtered_x64;   /* voltage, slow filter (about 64 s) */
    uint32_t filter_n;          /* readings in the filter so far, up to 64 (the first minute is a plain mean) */
    uint32_t up_ticks;          /* seconds the filtered level has been above the shown one */
    bool primed;
    uint32_t level_x10;         /* shown level, 0..1000, never rises while discharging (except a slow recovery) */
    bat_state_t state;
    uint32_t drain_x10_per_h;   /* observed drain, tenths of a percent per hour */
    uint16_t hist[BAT_HIST];    /* filtered level every 10 minutes, oldest first */
    uint32_t hist_n;
    uint32_t sample_timer;
    uint32_t on_battery_s;      /* seconds since the last start or cable, while the level still follows the cell freely */
    uint32_t cv_ticks;          /* seconds spent at the charger's constant-voltage stage */
} bat_t;

void bat_init(bat_t *b, const bat_stats_t *stats);

/* Call once per second with the cell voltage in millivolts. */
void bat_tick(bat_t *b, uint32_t mv, bool usb_present, bool charging);

uint32_t bat_percent(const bat_t *b);            /* 0..100 */
uint32_t bat_remaining_minutes(const bat_t *b);  /* while discharging, from the observed drain */
uint32_t bat_cycles(const bat_t *b);             /* equivalent full discharge cycles */
uint32_t bat_health_percent(const bat_t *b);     /* 0..100 */
uint32_t bat_full_runtime_h_x10(const bat_t *b); /* hours a full charge lasts at the current drain, tenths */

/* Voltage (mV) to charge level in tenths of a percent, exposed for tests. */
uint32_t bat_level_from_mv(uint32_t mv);

#endif
