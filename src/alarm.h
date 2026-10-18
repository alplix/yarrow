/* Alarm levels with hysteresis, the alarm event record and the dose budget.
 * Thresholds are fixed values in nSv/h chosen by the user; there is no statistical guessing. */
#ifndef YARROW_ALARM_H
#define YARROW_ALARM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { LEVEL_NORMAL = 0, LEVEL_ATTENTION, LEVEL_DANGER, LEVEL_EVACUATE } alarm_level_t;

typedef struct {
    uint32_t th_nsvh[3]; /* attention, danger, evacuate; must be increasing */
} alarm_cfg_t;

/* one finished alarm, from leaving normal to returning to it */
typedef struct {
    uint32_t start;      /* time the level left normal */
    uint32_t duration_s;
    uint32_t peak_nsvh;
    uint32_t dose_nsv;   /* dose collected during the event */
    uint8_t level;       /* highest level reached */
} alarm_event_t;

typedef struct {
    alarm_cfg_t cfg;
    alarm_level_t level;
    uint8_t up_ticks;
    uint8_t down_ticks;

    bool in_event;
    uint32_t ev_start;
    uint32_t ev_peak;
    uint64_t ev_dose_start;
    alarm_level_t ev_max;

    bool budget_active;
    uint64_t budget_nsv;
    uint64_t budget_start;
    bool budget_warned;
    bool budget_exceeded;
} alarm_t;

typedef struct {
    bool level_changed;
    bool event_closed;
    alarm_event_t event;     /* valid when event_closed */
    bool budget_warn;        /* crossed 80 percent this tick */
    bool budget_exceeded;    /* crossed 100 percent this tick */
} alarm_result_t;

void alarm_init(alarm_t *a, const alarm_cfg_t *cfg);

/* Starts a dose budget counted from the current total dose. */
void alarm_budget_start(alarm_t *a, uint64_t budget_nsv, uint64_t dose_now_nsv);
void alarm_budget_stop(alarm_t *a);

/* Percent of the budget used (0 when none is active). */
uint32_t alarm_budget_percent(const alarm_t *a, uint64_t dose_now_nsv);

/* Call once per second. */
alarm_result_t alarm_tick(alarm_t *a, uint32_t now, uint32_t rate_nsvh, uint64_t dose_nsv);

#endif
