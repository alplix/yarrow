#include "alarm.h"
#include <string.h>

#define UP_TICKS 2   /* seconds above a threshold before the level rises ... */
#define DOWN_TICKS 10 /* ... and seconds below its hysteresis before it falls */

void alarm_init(alarm_t *a, const alarm_cfg_t *cfg)
{
    memset(a, 0, sizeof *a);
    a->cfg = *cfg;
}

void alarm_budget_start(alarm_t *a, uint64_t budget_nsv, uint64_t dose_now_nsv)
{
    a->budget_active = budget_nsv > 0;
    a->budget_nsv = budget_nsv;
    a->budget_start = dose_now_nsv;
    a->budget_warned = false;
    a->budget_exceeded = false;
}

void alarm_budget_stop(alarm_t *a) { a->budget_active = false; }

uint32_t alarm_budget_percent(const alarm_t *a, uint64_t dose_now_nsv)
{
    if (!a->budget_active || a->budget_nsv == 0)
        return 0;
    uint64_t used = dose_now_nsv - a->budget_start;
    uint64_t p = used * 100u / a->budget_nsv;
    return p > 999 ? 999 : (uint32_t)p;
}

/* The level the rate asks for, given the current level (hysteresis: 20 percent below a threshold to leave it). */
static alarm_level_t wanted(const alarm_t *a, uint32_t rate, bool *instant)
{
    alarm_level_t up = LEVEL_NORMAL;
    *instant = false;
    for (int i = 0; i < 3; i++) {
        if (rate >= a->cfg.th_nsvh[i]) {
            up = (alarm_level_t)(i + 1);
            if (rate / 2 >= a->cfg.th_nsvh[i])
                *instant = true; /* at least twice a threshold: no waiting */
        }
    }
    if (up >= a->level)
        return up;
    /* below the current level: stay while within 20 percent of its threshold */
    alarm_level_t stay = a->level;
    while (stay > LEVEL_NORMAL && (uint64_t)rate * 10u < (uint64_t)a->cfg.th_nsvh[stay - 1] * 8u)
        stay--;
    return stay < up ? up : stay;
}

alarm_result_t alarm_tick(alarm_t *a, uint32_t now, uint32_t rate, uint64_t dose)
{
    alarm_result_t r;
    memset(&r, 0, sizeof r);

    bool instant;
    alarm_level_t w = wanted(a, rate, &instant);

    if (w > a->level) {
        a->down_ticks = 0;
        if (instant || ++a->up_ticks >= UP_TICKS) {
            a->level = w;
            a->up_ticks = 0;
            r.level_changed = true;
        }
    } else if (w < a->level) {
        a->up_ticks = 0;
        if (++a->down_ticks >= DOWN_TICKS) {
            a->level = w;
            a->down_ticks = 0;
            r.level_changed = true;
        }
    } else {
        a->up_ticks = a->down_ticks = 0;
    }

    if (a->level != LEVEL_NORMAL && !a->in_event) {
        a->in_event = true;
        a->ev_start = now;
        a->ev_peak = rate;
        a->ev_dose_start = dose;
        a->ev_max = a->level;
    }
    if (a->in_event) {
        if (rate > a->ev_peak)
            a->ev_peak = rate;
        if (a->level > a->ev_max)
            a->ev_max = a->level;
        if (a->level == LEVEL_NORMAL) {
            r.event_closed = true;
            r.event.start = a->ev_start;
            r.event.duration_s = now - a->ev_start;
            r.event.peak_nsvh = a->ev_peak;
            r.event.dose_nsv = (uint32_t)(dose - a->ev_dose_start);
            r.event.level = (uint8_t)a->ev_max;
            a->in_event = false;
        }
    }

    if (a->budget_active) {
        uint64_t used = dose - a->budget_start;
        if (!a->budget_warned && used * 100u >= a->budget_nsv * 80u) {
            a->budget_warned = true;
            r.budget_warn = true;
        }
        if (!a->budget_exceeded && used >= a->budget_nsv) {
            a->budget_exceeded = true;
            r.budget_exceeded = true;
        }
    }
    return r;
}
