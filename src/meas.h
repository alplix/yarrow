/* Measurement engine: turns one pulse count per second into counts per minute, dose rate and dose.
 * Integer arithmetic only. The averaging window adapts: long and quiet when the rate is low,
 * short and quick the moment the rate really changes. */
#ifndef YARROW_MEAS_H
#define YARROW_MEAS_H

#include <stdbool.h>
#include <stdint.h>

#define MEAS_HISTORY 300 /* seconds of history kept */

typedef struct {
    uint32_t cpm_per_usvh_x100; /* tube sensitivity, counts per minute for 1 uSv/h, times 100 (J321: 8000) */
    uint32_t dead_time_us;      /* tube dead time, 0 turns the correction off */
} meas_cfg_t;

typedef struct {
    meas_cfg_t cfg;
    uint16_t hist[MEAS_HISTORY]; /* measured pulses per second (saturating), ring */
    uint32_t pos;                /* next slot */
    uint32_t seconds;            /* seconds measured so far (saturates) */
    uint64_t counts_total;       /* dead-time corrected pulses since start */
    uint32_t window_s;           /* window the current rate was taken over */
    uint32_t window_sum;         /* raw pulses inside that window */
    uint32_t regime_s;           /* seconds since the rate last jumped: older seconds are not averaged in */
    uint32_t rate_cps_x100;      /* current corrected rate, counts per second times 100 (rounded) */
    uint32_t rate_cps_x10k;      /* the same times 10000: the resolution the dose rate is computed from */
    bool saturated;              /* the tube is too busy for the correction to be trustworthy */
    bool changing;               /* the rate moved faster than the long window can follow */
} meas_t;

void meas_init(meas_t *m, const meas_cfg_t *cfg);

/* Call once per second with the pulses counted in that second. */
void meas_tick(meas_t *m, uint32_t pulses);

uint32_t meas_cpm(const meas_t *m);          /* counts per minute, corrected, over the current window */
uint32_t meas_rate_nsvh(const meas_t *m);    /* dose rate in nSv/h */
uint64_t meas_dose_nsv(const meas_t *m);     /* dose since start in nSv */

/* True when the current rate rests on enough pulses to act on (an alarm must not fire from a handful of counts). */
bool meas_reliable(const meas_t *m);

/* Dose rate in nSv/h for a rate in counts per second times 100 (dead time corrected). */
uint32_t meas_nsvh_from_cps_x100(const meas_cfg_t *cfg, uint32_t cps_x100);

/* Corrected rate for a raw per-second count (exposed for tests and the scan mode). */
uint32_t meas_correct_cps_x100(uint32_t pulses_per_s, uint32_t dead_time_us, bool *saturated);

#endif
