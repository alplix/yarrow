#include "alarm.h"
#include <stdio.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static const alarm_cfg_t cfg = {{500, 10000, 100000}};

static uint32_t now;
static uint64_t dose;

static alarm_result_t step(alarm_t *a, uint32_t rate)
{
    now++;
    dose += rate / 3600u; /* nSv per second */
    return alarm_tick(a, now, rate, dose);
}

static void test_levels(void)
{
    alarm_t a;
    alarm_init(&a, &cfg);
    now = 0;
    dose = 0;

    for (int i = 0; i < 30; i++)
        step(&a, 120);
    CHECK(a.level == LEVEL_NORMAL, "normal rate raised an alarm");

    step(&a, 700); /* one second is not enough */
    CHECK(a.level == LEVEL_NORMAL, "single spike raised the level");
    alarm_result_t r = step(&a, 700);
    CHECK(a.level == LEVEL_ATTENTION && r.level_changed, "two seconds above 500 should raise attention");

    for (int i = 0; i < 5; i++)
        step(&a, 450); /* within 20 percent below the threshold: stays */
    CHECK(a.level == LEVEL_ATTENTION, "hysteresis: should stay at attention at 450");

    step(&a, 30000); /* more than twice the danger threshold: immediate */
    CHECK(a.level == LEVEL_DANGER || a.level == LEVEL_DANGER + 0, "double threshold should act at once (level %d)", a.level);

    /* drop back */
    int ticks_to_normal = 0;
    while (a.level != LEVEL_NORMAL && ticks_to_normal < 200) {
        r = step(&a, 100);
        ticks_to_normal++;
        if (r.event_closed)
            break;
    }
    CHECK(a.level == LEVEL_NORMAL && r.event_closed, "alarm did not clear (%d ticks)", ticks_to_normal);
    CHECK(r.event.peak_nsvh == 30000 && r.event.level == LEVEL_DANGER, "event record wrong: peak %u level %u",
          (unsigned)r.event.peak_nsvh, r.event.level);
    CHECK(r.event.duration_s > 10 && r.event.duration_s < 60, "event duration %u", (unsigned)r.event.duration_s);
    printf("event: peak %u nSv/h, %u s, %u nSv, level %u\n", (unsigned)r.event.peak_nsvh, (unsigned)r.event.duration_s,
           (unsigned)r.event.dose_nsv, r.event.level);
}

static void test_evacuate_direct(void)
{
    alarm_t a;
    alarm_init(&a, &cfg);
    now = 0;
    dose = 0;
    step(&a, 500000); /* far above everything */
    CHECK(a.level == LEVEL_EVACUATE, "huge rate should jump straight to evacuate (%d)", a.level);
}

static void test_flicker(void)
{
    /* A rate hovering around a threshold must not make the level chatter. */
    alarm_t a;
    alarm_init(&a, &cfg);
    now = 0;
    dose = 0;
    int changes = 0;
    for (int i = 0; i < 600; i++) {
        alarm_result_t r = step(&a, (i % 2) ? 520 : 480);
        changes += r.level_changed;
    }
    CHECK(changes <= 2, "level chattered %d times around a threshold", changes);
}

static void test_budget(void)
{
    alarm_t a;
    alarm_init(&a, &cfg);
    now = 0;
    dose = 5000;
    alarm_budget_start(&a, 1000, dose); /* 1000 nSv budget */
    bool warned = false, exceeded = false;
    for (int i = 0; i < 4000; i++) {
        dose += 1;
        alarm_result_t r = alarm_tick(&a, ++now, 100, dose);
        if (r.budget_warn) {
            CHECK(!warned && alarm_budget_percent(&a, dose) >= 80 && alarm_budget_percent(&a, dose) < 90, "warn at %u%%",
                  (unsigned)alarm_budget_percent(&a, dose));
            warned = true;
        }
        if (r.budget_exceeded) {
            CHECK(!exceeded && warned, "exceeded before warning or twice");
            exceeded = true;
        }
    }
    CHECK(warned && exceeded, "budget alarms missing (%d, %d)", warned, exceeded);
}

int main(void)
{
    test_levels();
    test_evacuate_direct();
    test_flicker();
    test_budget();
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall alarm checks passed\n", failures);
    return failures ? 1 : 0;
}
