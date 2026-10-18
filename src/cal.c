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
