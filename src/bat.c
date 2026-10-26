#include "bat.h"
#include <string.h>

#define DEFAULT_RUNTIME_H_X10 360u /* until measured: a full charge lasts about 36 h */
#define SAMPLE_S 600u              /* one level sample per 10 minutes of discharging */
#define PRIOR_SAMPLES 12u          /* the default drain counts as this many samples (2 h) of evidence */
#define BEST_AFTER 24u             /* wear is judged once 4 h of data back the estimate */
/* While charging, the voltage at the terminal is the cell's own plus the charge current times the resistance of cell,
 * protection circuit and wiring (a 1 A charger: roughly 0.1-0.15 V), so the curve must not be read from it directly: the
 * level would jump by tens of percent the moment the cable goes in. The constant-current stage is read with that drop
 * taken off; at the charger's 4.2 V stage the current tapers over about an hour and the level climbs from 85 % to 99 %
 * in that time. The charger's own "done" signal is what shows 100 %. */
#define SETTLE_S 60u               /* after the start or after the cable is pulled the level follows the cell freely for this long */
#define UP_TICKS 20u
#define UP_BAND 20u                /* the shown level rises only when the cell reads this much (2 %) above it: noise never moves it up */
#define CHARGE_DROP_MV 120u
#define CV_START_MV 4170u
#define CV_LEVEL_X10 850u
#define CV_SPAN_S 3600u

/* typical single-cell Li-ion curve under light load, millivolts against tenths of a percent */
static const struct { uint16_t mv; uint16_t x10; } curve[] = {
    {3300, 0},    {3400, 20},   {3610, 50},   {3690, 100},  {3710, 150},  {3730, 200},  {3750, 250},
    {3770, 300},  {3790, 350},  {3800, 400},  {3820, 450},  {3840, 500},  {3850, 550},  {3870, 600},
    {3910, 650},  {3950, 700},  {3980, 750},  {4020, 800},  {4080, 850},  {4110, 900},  {4150, 950},
    {4200, 1000},
};

uint32_t bat_level_from_mv(uint32_t mv)
{
    const unsigned n = sizeof curve / sizeof *curve;
    if (mv <= curve[0].mv)
        return 0;
    if (mv >= curve[n - 1].mv)
        return 1000;
    for (unsigned i = 1; i < n; i++) {
        if (mv <= curve[i].mv) {
            uint32_t span = curve[i].mv - curve[i - 1].mv;
            uint32_t y = curve[i].x10 - curve[i - 1].x10;
            return curve[i - 1].x10 + (mv - curve[i - 1].mv) * y / span;
        }
    }
    return 1000;
}

static uint32_t default_drain(void) { return 10000u / DEFAULT_RUNTIME_H_X10; }

void bat_init(bat_t *b, const bat_stats_t *stats)
{
    memset(b, 0, sizeof *b);
    if (stats)
        b->stats = *stats;
    b->drain_x10_per_h = default_drain();
    b->level_x10 = 1000;
}

/* Drain from the stored levels: the mean of the oldest third against the mean of the newest third. */
static void update_drain(bat_t *b)
{
    uint32_t n = b->hist_n;
    if (n < 6)
        return;
    uint32_t k = n / 3;
    uint32_t old_sum = 0, new_sum = 0;
    for (uint32_t i = 0; i < k; i++) {
        old_sum += b->hist[i];
        new_sum += b->hist[n - 1 - i];
    }
    if (old_sum <= new_sum)
        return;
    /* the two means are (n - k) samples apart, each sample 10 minutes */
    uint32_t drop_x10 = (old_sum - new_sum) / k;
    uint32_t measured = drop_x10 * 6u / (n - k); /* tenths of a percent per hour */
    uint32_t w = n < PRIOR_SAMPLES ? n : PRIOR_SAMPLES;
    uint32_t d = (default_drain() * (PRIOR_SAMPLES - w) + measured * w) / PRIOR_SAMPLES;
    b->drain_x10_per_h = d ? d : 1;

    if (n >= BEST_AFTER) {
        uint32_t runtime = 10000u / b->drain_x10_per_h;
        if (runtime > b->stats.best_runtime_h_x10)
            b->stats.best_runtime_h_x10 = runtime;
    }
}

static void push_sample(bat_t *b, uint32_t level)
{
    if (b->hist_n == BAT_HIST) {
        memmove(b->hist, b->hist + 1, (BAT_HIST - 1) * sizeof b->hist[0]);
        b->hist_n--;
    }
    b->hist[b->hist_n++] = (uint16_t)level;
    update_drain(b);
}

void bat_tick(bat_t *b, uint32_t mv, bool usb_present, bool charging)
{
    b->stats.on_seconds++;

    /* smooth the voltage slowly: single readings are jumpy and the load makes brief dips (backlight, buzzer) */
    if (!b->primed) {
        b->mv_filtered_x64 = mv * 64u;
        b->filter_n = 1;
        b->primed = true;
    } else if (b->filter_n < 64u) {
        /* the first minute: a plain running mean, so a reading or two taken under the start-up load do not weigh for minutes */
        b->mv_filtered_x64 = (b->mv_filtered_x64 * b->filter_n + mv * 64u) / (b->filter_n + 1u);
        b->filter_n++;
    } else {
        b->mv_filtered_x64 = b->mv_filtered_x64 - b->mv_filtered_x64 / 64u + mv;
    }
    uint32_t level = bat_level_from_mv(b->mv_filtered_x64 / 64u);

    if (usb_present) {
        b->state = charging ? BAT_CHARGING : BAT_FULL;
        if (charging) {
            uint32_t v = b->mv_filtered_x64 / 64u;
            if (v >= CV_START_MV) {
                if (b->cv_ticks < CV_SPAN_S)
                    b->cv_ticks++;
                level = CV_LEVEL_X10 + (1000u - 10u - CV_LEVEL_X10) * b->cv_ticks / CV_SPAN_S;
            } else {
                b->cv_ticks = 0;
                level = bat_level_from_mv(v > CHARGE_DROP_MV ? v - CHARGE_DROP_MV : 0u);
                if (level > CV_LEVEL_X10)
                    level = CV_LEVEL_X10;
            }
            if (level < b->level_x10 && b->level_x10 - level < 150u)
                level = b->level_x10;                /* the shown level does not wobble downwards while charging */
        } else {
            level = 1000u;                           /* the charger says it is done */
            b->cv_ticks = 0;
        }
        b->level_x10 = level;
        b->hist_n = 0;
        b->sample_timer = 0;
        b->up_ticks = 0;
        b->on_battery_s = 0;
        return;
    }

    b->state = BAT_DISCHARGING;
    uint32_t before = b->level_x10;
    if (b->on_battery_s < SETTLE_S) {
        /* the first minute on the battery: the level is whatever the cell reads, up or down. The shown level used to start at
         * 100 % and could only fall, so a first reading taken under the start-up load stayed too low for hours. */
        b->on_battery_s++;
        b->level_x10 = level;
        b->up_ticks = 0;
        return;
    }
    if (level < b->level_x10) {
        b->level_x10 = level; /* down: follow at once */
        b->up_ticks = 0;
    } else if (level >= b->level_x10 + UP_BAND && ++b->up_ticks >= UP_TICKS) {
        b->level_x10++;       /* up: a slow recovery after a dip, one tenth of a percent per 20 seconds */
        b->up_ticks = 0;
    }
    if (before > b->level_x10)
        b->stats.discharged_x10 += before - b->level_x10;

    if (++b->sample_timer >= SAMPLE_S) {
        b->sample_timer = 0;
        push_sample(b, level);
    }
}

uint32_t bat_percent(const bat_t *b) { return (b->level_x10 + 5u) / 10u; }

uint32_t bat_remaining_minutes(const bat_t *b)
{
    if (b->state != BAT_DISCHARGING || b->drain_x10_per_h == 0)
        return 0;
    return b->level_x10 * 60u / b->drain_x10_per_h;
}

uint32_t bat_cycles(const bat_t *b) { return b->stats.discharged_x10 / 1000u; }

uint32_t bat_full_runtime_h_x10(const bat_t *b) { return 10000u / (b->drain_x10_per_h ? b->drain_x10_per_h : 1u); }

uint32_t bat_health_percent(const bat_t *b)
{
    uint32_t best = b->stats.best_runtime_h_x10;
    if (best == 0)
        return 100;
    uint32_t now = bat_full_runtime_h_x10(b);
    uint32_t h = now * 100u / best;
    return h > 100 ? 100 : h;
}
