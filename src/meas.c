#include "meas.h"
#include <string.h>

#define TARGET_COUNTS 100u /* about 10 percent statistical noise */
#define FAST_WINDOW   5u
#define CHANGE_Z2     16u  /* z = 4, squared */

static const uint16_t windows[] = {5, 10, 20, 40, 80, 160, 300};

void meas_init(meas_t *m, const meas_cfg_t *cfg)
{
    memset(m, 0, sizeof *m);
    m->cfg = *cfg;
    m->window_s = FAST_WINDOW;
}

/* Non-paralyzable dead time: true = measured / (1 - measured * tau). */
uint32_t meas_correct_cps_x100(uint32_t n, uint32_t tau_us, bool *saturated)
{
    uint64_t busy = (uint64_t)n * tau_us; /* microseconds the tube was dead per second */
    if (busy >= 900000u) {                /* more than 90 percent dead: no meaningful correction */
        if (saturated)
            *saturated = true;
        busy = 900000u;
    }
    return (uint32_t)((uint64_t)n * 100u * 1000000u / (1000000u - busy));
}

static uint32_t sum_last(const meas_t *m, uint32_t seconds)
{
    uint64_t s = 0;
    for (uint32_t i = 1; i <= seconds; i++)
        s += m->hist[(m->pos + MEAS_HISTORY - i) % MEAS_HISTORY];
    return s > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)s;
}

/* pulses in the seconds from `from` to `to` ago (1 is the most recent second) */
static uint32_t sum_range(const meas_t *m, uint32_t from, uint32_t to)
{
    uint64_t s = 0;
    for (uint32_t i = from; i <= to; i++)
        s += m->hist[(m->pos + MEAS_HISTORY - i) % MEAS_HISTORY];
    return s > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)s;
}

void meas_tick(meas_t *m, uint32_t pulses)
{
    bool sat = false;
    m->hist[m->pos] = pulses > 0xFFFFu ? 0xFFFFu : (uint16_t)pulses;
    m->pos = (m->pos + 1) % MEAS_HISTORY;
    if (m->seconds < 0xFFFFFFFFu)
        m->seconds++;
    m->counts_total += meas_correct_cps_x100(pulses, m->cfg.dead_time_us, &sat) / 100u;
    m->saturated = sat;

    if (m->regime_s < MEAS_HISTORY)
        m->regime_s++;
    uint32_t have = m->seconds < MEAS_HISTORY ? m->seconds : MEAS_HISTORY;
    if (have > m->regime_s)
        have = m->regime_s; /* seconds from before the last jump of the rate do not belong to this rate */

    /* Which window? The shortest one that is expected to hold enough pulses, judged from the seconds BEFORE it.
     * Judging from the pulses inside the window itself would pick short windows when they happen to hold many pulses
     * and make the estimate too high; the older seconds are independent of the window, so the choice cannot lean. */
    uint32_t w = have;
    for (unsigned i = 0; i < sizeof windows / sizeof *windows; i++) {
        uint32_t cand = windows[i];
        if (cand >= have) {
            w = have;
            break;
        }
        uint32_t older_s = have - cand;
        uint64_t older = sum_range(m, cand + 1, have);
        if (older * cand / older_s >= TARGET_COUNTS) {
            w = cand;
            break;
        }
    }
    uint32_t sum = sum_last(m, w);

    /* Is the last few seconds still consistent with that window? If not, the rate has moved: follow it. */
    m->changing = false;
    if (w > FAST_WINDOW && have >= FAST_WINDOW) {
        uint32_t recent = sum_last(m, FAST_WINDOW);
        uint64_t expected = (uint64_t)sum * FAST_WINDOW / w; /* what the long window predicts for 5 s */
        int64_t diff = (int64_t)recent - (int64_t)expected;
        /* diff^2 > z^2 * expected (Poisson variance), with a floor so tiny counts do not trigger */
        if ((uint64_t)(diff * diff) > (uint64_t)CHANGE_Z2 * (expected + 4)) {
            w = FAST_WINDOW;
            sum = recent;
            m->changing = true;
            m->regime_s = FAST_WINDOW; /* a new regime starts five seconds back */
        }
    }

    if (w == 0)
        w = 1;
    m->window_s = w;
    m->window_sum = sum;

    /* pulses over w seconds -> per-second rate (times 10000, rounded), then the dead-time correction */
    uint64_t per_s = ((uint64_t)sum * 10000u + w / 2u) / w;
    uint64_t busy = per_s * m->cfg.dead_time_us / 10000u;
    if (busy >= 900000u) {
        busy = 900000u;
        m->saturated = true;
    }
    uint64_t corrected = per_s * 1000000u / (1000000u - busy);
    m->rate_cps_x10k = corrected > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)corrected;
    m->rate_cps_x100 = (m->rate_cps_x10k + 50u) / 100u;
}

/* A rate may start an alarm when it rests on as many pulses as the normal measurement aims at, or when the last few
 * seconds departed from the long window by four standard deviations (a real jump) and show a clear count. */
bool meas_reliable(const meas_t *m)
{
    return m->window_sum >= TARGET_COUNTS || (m->changing && m->window_sum >= 40u);
}

uint32_t meas_cpm(const meas_t *m) { return (uint32_t)(((uint64_t)m->rate_cps_x10k * 60u + 5000u) / 10000u); }

uint32_t meas_rate_nsvh(const meas_t *m)
{
    /* nSv/h = cpm * 1000 / (cpm per uSv/h); with the rate times 10000 and the sensitivity times 100 that is x * 600 / s */
    uint64_t num = (uint64_t)m->rate_cps_x10k * 600u;
    return (uint32_t)((num + m->cfg.cpm_per_usvh_x100 / 2u) / m->cfg.cpm_per_usvh_x100);
}

uint64_t meas_dose_nsv(const meas_t *m)
{
    /* one count = 1000 nSv / (cpm_per_usvh * 60) */
    return m->counts_total * 1000u * 100u / ((uint64_t)m->cfg.cpm_per_usvh_x100 * 60u);
}

uint32_t meas_nsvh_from_cps_x100(const meas_cfg_t *cfg, uint32_t cps_x100)
{
    uint64_t busy = (uint64_t)cps_x100 * cfg->dead_time_us / 100u;
    if (busy >= 900000u)
        busy = 900000u;
    uint64_t corrected = (uint64_t)cps_x100 * 1000000u / (1000000u - busy);
    return (uint32_t)(corrected * 60u * 1000u / cfg->cpm_per_usvh_x100);
}
