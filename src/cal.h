/* Sensitivity calibration against a reference: the tube counts next to a source whose dose rate is known, and the
 * counts give the sensitivity. Integer arithmetic only. */
#ifndef YARROW_CAL_H
#define YARROW_CAL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t sum_x100; /* dead-time corrected counts, times 100 */
    uint32_t seconds;
    uint32_t counts;   /* raw pulses */
    bool saturated;    /* the tube was too busy at some moment: the correction is not trustworthy */
} cal_acc_t;

void cal_reset(cal_acc_t *c);

/* Call once per second with the pulses counted in that second. */
void cal_tick(cal_acc_t *c, uint32_t pulses, uint32_t dead_time_us);

/* Statistical uncertainty of the count in percent times 10 (a Poisson count N is known to 100/sqrt(N) percent).
 * 1000 (that is 100 %) when nothing was counted. */
uint32_t cal_uncertainty_x10(const cal_acc_t *c);

/* Sensitivity in counts per minute for 1 uSv/h, times 100, for a reference dose rate in nSv/h.
 * 0 when there is nothing to compute from. */
uint32_t cal_sensitivity_x100(const cal_acc_t *c, uint32_t ref_nsvh);

/* Dead time from the two-source method: four counts of the same length of time, with source A alone, A and B together,
 * B alone, and neither (the background). */
typedef struct {
    uint32_t counts[4];  /* raw pulses: A, A+B, B, background */
    uint32_t seconds[4];
} dt_meas_t;

/* Dead time in microseconds, 0 when the four counts cannot tell it (the sources are too weak, so the difference of the
 * counts is not clearly more than their noise). *unc_us gets the statistical uncertainty. */
uint32_t cal_deadtime_us(const dt_meas_t *m, uint32_t *unc_us);

#endif
