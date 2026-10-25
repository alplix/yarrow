#include "cal.h"
#include "meas.h"

void cal_reset(cal_acc_t *c)
{
    c->sum_x100 = 0;
    c->seconds = 0;
    c->counts = 0;
    c->saturated = false;
}

void cal_tick(cal_acc_t *c, uint32_t pulses, uint32_t dead_time_us)
{
    bool sat = false;
    c->sum_x100 += meas_correct_cps_x100(pulses, dead_time_us, &sat);
    c->counts += pulses;
    c->seconds++;
    if (sat)
        c->saturated = true;
}

static uint32_t isqrt(uint32_t v)
{
    uint32_t r = 0, bit = 1u << 30;
    while (bit > v)
        bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

uint32_t cal_deadtime_us(const dt_meas_t *m, uint32_t *unc_us)
{
    *unc_us = 0;
    int64_t r[4]; /* counts per second times 100 */
    uint64_t var = 0;
    for (int i = 0; i < 4; i++) {
        if (m->seconds[i] == 0)
            return 0;
        r[i] = (int64_t)(((uint64_t)m->counts[i] * 100u + m->seconds[i] / 2) / m->seconds[i]);
        var += (uint64_t)m->counts[i] * 10000u / ((uint64_t)m->seconds[i] * m->seconds[i]);
    }
    int64_t r1 = r[0], r12 = r[1], r2 = r[2], rb = r[3];
    int64_t x = r1 * r2 - rb * r12;
    int64_t s = r1 + r2 - r12 - rb;
    if (x <= 0 || s <= 0)
        return 0;
    uint32_t sigma = isqrt(var > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)var);
    if (s < 3 * (int64_t)sigma)
        return 0;
    /* first order: tau = S / 2X; the next terms (Z = Y S / X^2, with Y the second moment) matter once the tube is a
     * tenth dead */
    int64_t y = r1 * r2 * (r12 + rb) - rb * r12 * (r1 + r2);
    int64_t z = (y / x) * s * 1000 / x; /* Z times 1000 */
    if (z < 0 || z > 600)
        return 0;
    int64_t f = 1000 + z / 4 + z * z / 8000;
    int64_t tau = (s * 50000000 * f + x * 500) / (x * 1000);
    if (tau <= 0 || tau > 10000)
        return 0;
    *unc_us = (uint32_t)(tau * sigma / s) + 1;
    return (uint32_t)tau;
}

uint32_t cal_uncertainty_x10(const cal_acc_t *c)
{
    if (c->counts == 0)
        return 1000;
    uint32_t u = 1000u / isqrt(c->counts); /* 100 / sqrt(N) percent, times 10 */
    return u > 1000 ? 1000 : u;
}

uint32_t cal_sensitivity_x100(const cal_acc_t *c, uint32_t ref_nsvh)
{
    if (c->seconds == 0 || ref_nsvh == 0 || c->sum_x100 == 0)
        return 0;
    /* corrected counts per minute = sum / 100 / seconds * 60; sensitivity x100 = cpm * 100 / (ref_nsvh / 1000) */
    uint64_t num = c->sum_x100 * 60u * 1000u;
    uint64_t den = (uint64_t)c->seconds * ref_nsvh;
    uint64_t v = (num + den / 2) / den;
    return v > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)v;
}
