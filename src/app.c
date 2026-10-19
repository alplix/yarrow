#include "app.h"
#include "display.h"
#include "fmt.h"
#include "theme.h"
#include "ui.h"
#include <string.h>

/* ---- persistent keys ---- */
enum {
    K_SETTINGS = 1, K_THRESH, K_BATSTATS, K_DOSE, K_DAYS, K_BUDGET, K_BG, K_PULSES, K_BOOT, K_STAGE, K_EVHEAD, K_CALIB,
    K_EVT0 = 0x40, /* 16 slots */
    K_SESS0 = 0x60, /* 8 slots */
    K_SESSHEAD = 0x70,
    K_SESSACT = 0x71
};
#define EVT_SLOTS 16
#define SETTINGS_ROWS 12

/* Flash sectors wear out with every erase. The settings store fills up and is compacted into the other sector every
 * few kilobytes written, so the counters are saved rarely: a clean power-off saves everything anyway, only a pulled
 * battery loses the last quarter of an hour. */
#define SAVE_COUNTERS_S 900u
#define SAVE_SLOW_S 3600u

static const uint32_t profile_interval[5] = {1, 10, 60, 300, 3600};
/* days a full log lasts per profile with the default region: filled in from the measured rate when known */
static const uint32_t budget_presets[6] = {1000, 10000, 50000, 100000, 500000, 1000000};

#define STABLE_MS 10000u      /* a stage must run this long before the next one is tried */
#define DIM_MS 20000u
#define OFF_MS 90000u
#define TREND_STEP_S 17u      /* 36 points over about ten minutes */
#define FIELD_STEP_S 2u

/* ---- small helpers ---- */

static const hal_t *H(const app_t *a) { return a->hal; }

uint8_t *app_scratch(void)
{
    static uint8_t scratch[YLOG_MAX_PAGE];
    return scratch;
}

static void le32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t rl32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

static void kv_put32(app_t *a, uint8_t key, uint32_t v)
{
    if (a->kv_ok)
        kv_set_u32(&a->kv, key, v);
}

static screen_id_t cur(const app_t *a) { return a->stack[a->sp]; }
static ui_state_t *st(app_t *a) { return &a->states[a->sp]; }

typedef struct {
    app_t *a;
    bool full;
} draw_ctx_t;

static void draw_cb(void *p)
{
    draw_ctx_t *c = p;
    screen_draw(cur(c->a), &c->a->ui, st(c->a), c->full);
}

/* A fingerprint of everything the current screen shows. The screen is redrawn only when it changes, so a page
 * that shows nothing new stays perfectly still instead of flashing once a second. */
static uint32_t screen_sig(app_t *a)
{
    ui_data_t u = a->ui;
    u.now /= 60; /* the clock shows minutes */
    u.anim = 0;
    u.blink = false;
    uint32_t h = 2166136261u ^ (uint32_t)cur(a);
    const uint8_t *p = (const uint8_t *)&u;
    for (unsigned i = 0; i < sizeof u; i++)
        h = (h ^ p[i]) * 16777619u;
    p = (const uint8_t *)st(a);
    for (unsigned i = 0; i < sizeof(ui_state_t); i++)
        h = (h ^ p[i]) * 16777619u;
    return h;
}

static void invalidate(app_t *a) { a->need_full = true; a->need_draw = true; }

static void push(app_t *a, screen_id_t id)
{
    if (a->sp + 1 >= APP_STACK)
        return;
    a->sp++;
    a->stack[a->sp] = id;
    memset(&a->states[a->sp], 0, sizeof(ui_state_t));
    a->edit = 0;
    invalidate(a);
}

static void pop(app_t *a)
{
    if (a->sp > 0)
        a->sp--;
    a->edit = 0;
    invalidate(a);
}

static const screen_id_t home_pages[6] = {SCR_MAIN, SCR_TREND, SCR_EFIELD, SCR_HFIELD, SCR_BATTERY, SCR_HISTORY};

static void set_home(app_t *a, int idx)
{
    a->home_idx = (idx % 6 + 6) % 6;
    a->stack[0] = home_pages[a->home_idx];
    a->sp = 0;
    invalidate(a);
}

/* ---- settings and persistence ---- */

/* LED colour masks: 1 red, 2 green, 4 blue. The numbers 1..7 of the setting are red, green, blue, yellow, cyan, magenta, white. */
static const uint8_t led_masks[8] = {0, 1, 2, 4, 3, 6, 5, 7};

/* the colour of a theme's accent: the channels that are strong enough, or white when it is grey */
static uint8_t led_mask_of_rgb565(uint16_t c)
{
    int r = ((c >> 11) & 31) * 255 / 31, g = ((c >> 5) & 63) * 255 / 63, b = (c & 31) * 255 / 31;
    int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    int mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    if (mx - mn < 40)
        return 7;
    uint8_t m = 0;
    if (r * 10 >= mx * 6) m |= 1;
    if (g * 10 >= mx * 6) m |= 2;
    if (b * 10 >= mx * 6) m |= 4;
    return m;
}

static void apply_led_color(app_t *a)
{
    uint8_t m = a->settings.led_color ? led_masks[a->settings.led_color & 7] : led_mask_of_rgb565(themes[a->settings.theme % THEME_COUNT].acc);
    a->led_mask = m;
    a->ui.led_mask = m;
    H(a)->led_color(m);
}

static void load_settings(app_t *a)
{
    app_settings_t *s = &a->settings;
    s->theme = THEME_DARK;
    s->brightness = 4;
    s->sound = true;
    s->vibration = true;
    s->led_pulse = true;
    s->led_charge = true;
    s->click = false;
    s->count_vibe = false;
    s->led_color = 0;
    s->usb_start = true;
    s->tz_hours = 3;
    s->profile = 2;
    a->th[0] = 500; a->th[1] = 10000; a->th[2] = 100000;
    a->ui.cal[0] = 18000; a->ui.cal[1] = 90; a->ui.cal[2] = 100; a->ui.cal[3] = 100;
    if (!a->kv_ok)
        return;

    uint8_t b[KV_MAX_VALUE];
    if (kv_get(&a->kv, K_SETTINGS, b, sizeof b) >= 6) {
        s->theme = b[0] < THEME_COUNT ? b[0] : THEME_DARK;
        s->brightness = b[1] >= 1 && b[1] <= 5 ? b[1] : 4;
        s->sound = (b[2] & 1) != 0;
        s->vibration = (b[2] & 2) != 0;
        s->led_pulse = (b[2] & 4) == 0; /* stored as 'off' so older records keep the LEDs on */
        s->led_charge = (b[2] & 8) == 0;
        s->click = (b[2] & 16) != 0;
        s->count_vibe = (b[2] & 32) != 0;
        s->led_color = b[5] <= 7 ? b[5] : 0;
        s->usb_start = b[6] == 0; /* stored as 'off' so older records start on USB power */
        s->tz_hours = (int8_t)b[3];
        s->profile = b[4] < 5 ? b[4] : 2;
    }
    if (kv_get(&a->kv, K_THRESH, b, sizeof b) == 12) {
        uint32_t t0 = rl32(b), t1 = rl32(b + 4), t2 = rl32(b + 8);
        if (t0 > 0 && t0 < t1 && t1 < t2) { a->th[0] = t0; a->th[1] = t1; a->th[2] = t2; }
    }
    if (kv_get(&a->kv, K_CALIB, b, sizeof b) == 16) {
        uint32_t c0 = rl32(b), c1 = rl32(b + 4), c2 = rl32(b + 8), c3 = rl32(b + 12);
        if (c0 >= 1000 && c0 <= 100000 && c1 <= 2000 && c2 >= 10 && c2 <= 1000 && c3 >= 10 && c3 <= 1000) {
            a->ui.cal[0] = c0; a->ui.cal[1] = c1; a->ui.cal[2] = c2; a->ui.cal[3] = c3;
        }
    }
    uint32_t v;
    if (kv_get_u32(&a->kv, K_PULSES, &v))
        a->pulses_base = v;
    if (kv_get_u32(&a->kv, K_BG, &v))
        a->bg_x16 = v;

    uint8_t d8[8];
    if (kv_get(&a->kv, K_DOSE, d8, 8) == 8)
        a->dose_base_nsv = rl32(d8) | (uint64_t)rl32(d8 + 4) << 32;
    if (kv_get(&a->kv, K_DAYS, b, sizeof b) == 32) {
        a->day_index = rl32(b + 28);
        for (int i = 0; i < 7; i++)
            a->day_nsv[i] = rl32(b + 4 * i) ;
    }
    if (kv_get(&a->kv, K_BATSTATS, b, sizeof b) == 12) {
        bat_stats_t bs = {rl32(b), rl32(b + 4), rl32(b + 8)};
        bat_init(&a->bat, &bs);
    }
    if (kv_get(&a->kv, K_BUDGET, b, sizeof b) == 16) {
        uint32_t nsv = rl32(b);
        uint64_t start = rl32(b + 4) | (uint64_t)rl32(b + 8) << 32;
        a->ui.budget_start_time = rl32(b + 12);
        if (nsv)
            alarm_budget_start(&a->alarm, nsv, start);
    }
    uint32_t head;
    if (kv_get_u32(&a->kv, K_EVHEAD, &head)) {
        uint32_t n = head < APP_EVENTS_KEPT ? head : APP_EVENTS_KEPT;
        for (uint32_t i = 0; i < n; i++) {
            uint8_t e[16];
            uint32_t slot = (head - 1 - i) % EVT_SLOTS;
            if (kv_get(&a->kv, (uint8_t)(K_EVT0 + slot), e, sizeof e) == 16) {
                a->ui.events[i].start = rl32(e);
                a->ui.events[i].duration_s = rl32(e + 4);
                a->ui.events[i].peak_nsvh = rl32(e + 8);
                a->ui.events[i].level = e[12];
                a->ui.event_count = (uint8_t)(i + 1);
            }
        }
    }
}

static void save_settings(app_t *a)
{
    if (!a->kv_ok)
        return;
    uint8_t b[8] = {a->settings.theme, a->settings.brightness,
                    (uint8_t)((a->settings.sound ? 1 : 0) | (a->settings.vibration ? 2 : 0) | (a->settings.led_pulse ? 0 : 4) |
                              (a->settings.led_charge ? 0 : 8) | (a->settings.click ? 16 : 0) | (a->settings.count_vibe ? 32 : 0)),
                    (uint8_t)a->settings.tz_hours, a->settings.profile, a->settings.led_color, (uint8_t)(a->settings.usb_start ? 0 : 1), 0};
    kv_set(&a->kv, K_SETTINGS, b, 8);
}

static void save_thresholds(app_t *a)
{
    if (!a->kv_ok)
        return;
    uint8_t b[12];
    le32(b, a->th[0]); le32(b + 4, a->th[1]); le32(b + 8, a->th[2]);
    kv_set(&a->kv, K_THRESH, b, 12);
    a->alarm.cfg.th_nsvh[0] = a->th[0];
    a->alarm.cfg.th_nsvh[1] = a->th[1];
    a->alarm.cfg.th_nsvh[2] = a->th[2];
}

static void restore_calibration(app_t *a)
{
    a->ui.cal[0] = 18000; a->ui.cal[1] = 90; a->ui.cal[2] = 100; a->ui.cal[3] = 100;
    uint8_t b[KV_MAX_VALUE];
    if (a->kv_ok && kv_get(&a->kv, K_CALIB, b, sizeof b) == 16) {
        uint32_t c0 = rl32(b), c1 = rl32(b + 4), c2 = rl32(b + 8), c3 = rl32(b + 12);
        if (c0 >= 1000 && c0 <= 100000 && c1 <= 2000 && c2 >= 10 && c2 <= 1000 && c3 >= 10 && c3 <= 1000) {
            a->ui.cal[0] = c0; a->ui.cal[1] = c1; a->ui.cal[2] = c2; a->ui.cal[3] = c3;
        }
    }
}

static void apply_calibration(app_t *a)
{
    a->meas.cfg.cpm_per_usvh_x100 = a->ui.cal[0];
    a->meas.cfg.dead_time_us = a->ui.cal[1];
}

static void save_calibration(app_t *a)
{
    apply_calibration(a);
    if (!a->kv_ok)
        return;
    uint8_t b[16];
    le32(b, a->ui.cal[0]); le32(b + 4, a->ui.cal[1]); le32(b + 8, a->ui.cal[2]); le32(b + 12, a->ui.cal[3]);
    kv_set(&a->kv, K_CALIB, b, 16);
}

static void save_budget(app_t *a, uint64_t start_dose)
{
    if (!a->kv_ok)
        return;
    uint8_t b[16];
    le32(b, (uint32_t)a->alarm.budget_nsv);
    le32(b + 4, (uint32_t)start_dose);
    le32(b + 8, (uint32_t)(start_dose >> 32));
    le32(b + 12, a->ui.budget_start_time);
    kv_set(&a->kv, K_BUDGET, b, 16);
}

static uint64_t total_dose(const app_t *a) { return a->dose_base_nsv + meas_dose_nsv(&a->meas); }
static uint32_t total_pulses(const app_t *a) { return a->pulses_base + a->last_pulses; }

static void save_counters(app_t *a)
{
    if (!a->kv_ok)
        return;
    uint8_t d8[8];
    uint64_t d = total_dose(a);
    le32(d8, (uint32_t)d);
    le32(d8 + 4, (uint32_t)(d >> 32));
    kv_set(&a->kv, K_DOSE, d8, 8);
    kv_set_u32(&a->kv, K_PULSES, total_pulses(a));
}

static void save_slow(app_t *a)
{
    if (!a->kv_ok)
        return;
    uint8_t b[32];
    for (int i = 0; i < 7; i++)
        le32(b + 4 * i, a->day_nsv[i]);
    le32(b + 28, a->day_index);
    kv_set(&a->kv, K_DAYS, b, 32);
    uint8_t bs[12];
    le32(bs, a->bat.stats.discharged_x10);
    le32(bs + 4, a->bat.stats.on_seconds);
    le32(bs + 8, a->bat.stats.best_runtime_h_x10);
    kv_set(&a->kv, K_BATSTATS, bs, 12);
    kv_put32(a, K_BG, a->bg_x16);
}

static void store_event(app_t *a, const alarm_event_t *e)
{
    memmove(&a->ui.events[1], &a->ui.events[0], (APP_EVENTS_KEPT - 1) * sizeof a->ui.events[0]);
    a->ui.events[0].start = e->start;
    a->ui.events[0].duration_s = e->duration_s;
    a->ui.events[0].peak_nsvh = e->peak_nsvh;
    a->ui.events[0].level = e->level;
    if (a->ui.event_count < APP_EVENTS_KEPT)
        a->ui.event_count++;
    if (!a->kv_ok)
        return;
    uint32_t head = 0;
    kv_get_u32(&a->kv, K_EVHEAD, &head);
    uint8_t b[16] = {0};
    le32(b, e->start); le32(b + 4, e->duration_s); le32(b + 8, e->peak_nsvh);
    b[12] = e->level;
    kv_set(&a->kv, (uint8_t)(K_EVT0 + head % EVT_SLOTS), b, 16);
    kv_set_u32(&a->kv, K_EVHEAD, head + 1);
}

/* ---- sessions: a named stretch of time with its own dose, average and peak ---- */

static void sess_save_active(app_t *a)
{
    if (!a->kv_ok)
        return;
    const ui_data_t *u = &a->ui;
    uint8_t b[20] = {0};
    b[0] = u->sess_kind;
    b[1] = u->sess_active ? 1 : 0;
    le32(b + 4, u->sess_start);
    le32(b + 8, u->sess_seconds);
    le32(b + 12, u->sess_dose_nsv > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)u->sess_dose_nsv);
    le32(b + 16, u->sess_max_nsvh);
    kv_set(&a->kv, K_SESSACT, b, 20);
}

static void sess_load(app_t *a)
{
    ui_data_t *u = &a->ui;
    u->sess_saved = 0;
    u->sess_active = false;
    if (!a->kv_ok)
        return;
    uint8_t b[KV_MAX_VALUE];
    if (kv_get(&a->kv, K_SESSACT, b, sizeof b) == 20 && b[1] == 1) {
        u->sess_active = true;
        u->sess_kind = b[0] % SESS_KINDS;
        u->sess_start = rl32(b + 4);
        u->sess_seconds = rl32(b + 8);
        u->sess_dose_nsv = rl32(b + 12);
        u->sess_max_nsvh = rl32(b + 16);
    }
    uint32_t head = 0;
    kv_get_u32(&a->kv, K_SESSHEAD, &head);
    for (uint32_t i = 0; i < SESS_SLOTS && i < head; i++) {
        uint32_t idx = head - 1 - i;
        if (kv_get(&a->kv, (uint8_t)(K_SESS0 + idx % SESS_SLOTS), b, sizeof b) != 20)
            break;
        ui_session_t *e = &u->sess_list[u->sess_saved++];
        e->kind = b[0] % SESS_KINDS;
        e->start = rl32(b + 4);
        e->seconds = rl32(b + 8);
        e->dose_nsv = rl32(b + 12);
        e->max_nsvh = rl32(b + 16);
    }
}

static void sess_start(app_t *a, uint8_t kind)
{
    ui_data_t *u = &a->ui;
    u->sess_active = true;
    u->sess_kind = kind % SESS_KINDS;
    u->sess_start = H(a)->rtc_now();
    u->sess_seconds = 0;
    u->sess_dose_nsv = 0;
    u->sess_max_nsvh = 0;
    a->sess_prev_total = total_dose(a);
    sess_save_active(a);
}

static void sess_stop(app_t *a)
{
    ui_data_t *u = &a->ui;
    if (!u->sess_active)
        return;
    u->sess_active = false;
    if (u->sess_seconds >= 10) { /* a start by accident is not worth a place in the list */
        memmove(&u->sess_list[1], &u->sess_list[0], (SESS_SLOTS - 1) * sizeof u->sess_list[0]);
        ui_session_t *e = &u->sess_list[0];
        e->kind = u->sess_kind;
        e->start = u->sess_start;
        e->seconds = u->sess_seconds;
        e->dose_nsv = u->sess_dose_nsv > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)u->sess_dose_nsv;
        e->max_nsvh = u->sess_max_nsvh;
        if (u->sess_saved < SESS_SLOTS)
            u->sess_saved++;
        if (a->kv_ok) {
            uint32_t head = 0;
            kv_get_u32(&a->kv, K_SESSHEAD, &head);
            uint8_t b[20] = {0};
            b[0] = e->kind;
            le32(b + 4, e->start);
            le32(b + 8, e->seconds);
            le32(b + 12, e->dose_nsv);
            le32(b + 16, e->max_nsvh);
            kv_set(&a->kv, (uint8_t)(K_SESS0 + head % SESS_SLOTS), b, 20);
            kv_set_u32(&a->kv, K_SESSHEAD, head + 1);
        }
    }
    sess_save_active(a);
}

static void sess_tick(app_t *a, uint64_t dose, uint32_t rate)
{
    ui_data_t *u = &a->ui;
    if (!u->sess_active)
        return;
    if (dose > a->sess_prev_total)
        u->sess_dose_nsv += dose - a->sess_prev_total;
    a->sess_prev_total = dose;
    u->sess_seconds++;
    if (rate > u->sess_max_nsvh)
        u->sess_max_nsvh = rate;
    if (u->sess_seconds % SAVE_COUNTERS_S == 0)
        sess_save_active(a);
}

/* ---- start-up ladder ---- */

/* Marks the stage being tried as unfinished before it starts; cleared once it has run stably. */
static void ladder_begin(app_t *a, uint8_t stage, uint32_t now_ms)
{
    a->stage = stage;
    a->stage_since_ms = now_ms;
    uint8_t b[2] = {1, stage};
    if (a->kv_ok)
        kv_set(&a->kv, K_BOOT, b, 2);
}

static void ladder_init(app_t *a, uint32_t now_ms)
{
    a->verified = 0;
    a->suspect_mask = 0;
    uint8_t b[KV_MAX_VALUE];
    uint32_t v;
    if (a->kv_ok && kv_get_u32(&a->kv, K_STAGE, &v)) {
        a->verified = (uint8_t)(v & 0xFF);
        a->suspect_mask = (uint8_t)((v >> 8) & 0xFF);
    }
    if (a->kv_ok && kv_get(&a->kv, K_BOOT, b, sizeof b) == 2 && b[0] != 0) {
        /* last time a stage was started and never declared stable: it is suspect */
        if (b[1] >= 1 && b[1] <= APP_STAGES && b[1] > a->verified) {
            a->suspect_mask |= (uint8_t)(1u << b[1]);
            kv_put32(a, K_STAGE, a->verified | (uint32_t)a->suspect_mask << 8);
        }
    }
    uint8_t next = a->verified >= APP_STAGES ? APP_STAGES : (uint8_t)(a->verified + 1);
    uint8_t start = a->verified ? a->verified : 1; /* run what already worked */
    if (a->suspect_mask & (1u << next))
        next = start; /* do not try the same failing step again by itself */
    ladder_begin(a, next, now_ms);
}

static void ladder_poll(app_t *a, uint32_t now_ms)
{
    if (now_ms - a->stage_since_ms < STABLE_MS)
        return;
    /* the current stage has run long enough: remember it, and try the next step */
    if (a->stage > a->verified) {
        a->verified = a->stage;
        if (a->kv_ok) {
            a->suspect_mask &= (uint8_t)~(1u << a->stage);
            kv_put32(a, K_STAGE, a->verified | (uint32_t)a->suspect_mask << 8);
            uint8_t b[2] = {0, a->stage};
            kv_set(&a->kv, K_BOOT, b, 2);
        }
    }
    if (a->stage < APP_STAGES && !(a->suspect_mask & (1u << (a->stage + 1)))) {
        ladder_begin(a, (uint8_t)(a->stage + 1), now_ms);
        invalidate(a);
    } else {
        a->stage_since_ms = now_ms; /* nothing more to try */
    }
}

/* ---- hardware features by stage ---- */

static bool feat_sensors(const app_t *a) { return a->stage >= 2; }
static bool feat_log(const app_t *a) { return a->stage >= 3; }
static bool feat_tube(const app_t *a) { return a->stage >= 4; }
static bool feat_usb(const app_t *a) { return a->stage >= 5; }

static void start_log(app_t *a)
{
    if (a->log_ok || !feat_log(a) || !H(a)->log_flash)
        return;
    if (ylog_open(&a->log, H(a)->log_flash) == 0) {
        a->log_ok = true;
        a->log_interval = profile_interval[a->settings.profile];
        a->log_acc = 0;
        a->log_elapsed = 0;
        a->log_start = 0;
    }
}

static void start_tube(app_t *a)
{
    if (a->hv_on || !feat_tube(a))
        return;
    if (H(a)->hv_factory_valid && H(a)->hv_factory_valid() && H(a)->hv_start && H(a)->hv_start())
        a->hv_on = true;
}

/* ---- sound ---- */

static void beep_now(app_t *a, uint32_t hz)
{
    if (a->settings.sound && feat_sensors(a))
        H(a)->beep(hz);
}

static void sound_poll(app_t *a, uint32_t now_ms)
{
    if (!feat_sensors(a))
        return;
    int level = a->alarm.level;
    bool active = level > 0 && !a->alarm_muted;
    if (!active) {
        if (a->sound_phase) {
            H(a)->beep(0);
            H(a)->vibrate(false);
            H(a)->alert_led(false);
            a->sound_phase = 0;
        }
        return;
    }
    uint32_t period = level == 1 ? 3000 : level == 2 ? 600 : 300;
    uint32_t on_ms = level == 1 ? 120 : level == 2 ? 300 : 150;
    uint32_t t = now_ms - a->sound_ms;
    if (t >= period) {
        a->sound_ms = now_ms;
        t = 0;
    }
    bool on = t < on_ms;
    uint32_t hz = level == 3 ? ((now_ms / 150) & 1 ? 3100u : 2300u) : level == 2 ? 2700u : 2000u;
    if (on != (a->sound_phase == 1)) {
        a->sound_phase = on ? 1 : 2;
        if (a->settings.sound)
            H(a)->beep(on ? hz : 0);
        if (a->settings.vibration && level >= 2)
            H(a)->vibrate(on);
        H(a)->alert_led(on);
    }
}

static void chirp(app_t *a, uint32_t hz)
{
    beep_now(a, hz); /* the next sound_poll or the one-second tick silences it */
    a->sound_ms = 0;
}

/* ---- the one-second tick ---- */

static void push_trend(uint32_t *arr, uint32_t v)
{
    memmove(arr, arr + 1, (TREND_N - 1) * sizeof *arr);
    arr[TREND_N - 1] = v;
}

static void stats_of(const uint32_t *arr, uint32_t *mn, uint32_t *avg, uint32_t *mx)
{
    uint64_t sum = 0;
    uint32_t lo = 0xFFFFFFFFu, hi = 0;
    for (int i = 0; i < TREND_N; i++) {
        sum += arr[i];
        if (arr[i] < lo) lo = arr[i];
        if (arr[i] > hi) hi = arr[i];
    }
    *mn = lo;
    *avg = (uint32_t)(sum / TREND_N);
    *mx = hi;
}

static void log_tick(app_t *a, uint32_t now_s, uint32_t pulses)
{
    if (!a->log_ok || !feat_log(a))
        return;
    uint32_t iv = a->log_interval;
    if (a->log_start == 0) { /* wait for the next interval boundary so samples line up */
        if (now_s % iv != 0)
            return;
        a->log_start = now_s;
        a->log_acc = 0;
        a->log_elapsed = 0;
    }
    a->log_acc += pulses;
    a->log_elapsed++;
    if (a->log_elapsed >= iv) {
        bool valid = now_s > 1000000000u;
        ylog_add(&a->log, a->log_start, iv, valid, a->log_acc);
        a->log_start += iv;
        a->log_acc = 0;
        a->log_elapsed = 0;
    }
}

static void selftest_step(app_t *a)
{
    ui_data_t *u = &a->ui;
    if (u->selftest_done >= u->selftest_total)
        return;
    bool ok = true;
    switch (u->selftest_done) {
    case 0: ok = a->hv_on; break;                                     /* high voltage running */
    case 1: ok = a->hv_on && a->meas.seconds > 5; break;              /* tube alive: pulses arrived at some point */
    case 2: { uint32_t mv = H(a)->battery_mv(); ok = mv > 3000 && mv < 4500; break; }
    case 3: { uint32_t v = H(a)->efield_vm_x10(); ok = v < 20000; break; }
    case 4: { uint32_t v = H(a)->hfield_ut_x100(); ok = v < 10000; break; }
    case 5: ok = H(a)->rtc_now() > 1700000000u; break;                /* clock set to a plausible date */
    case 6: ok = a->kv_ok && (a->log_ok || !feat_log(a)); break;
    default: break;
    }
    if (!ok)
        u->selftest_fail |= (uint8_t)(1u << u->selftest_done);
    u->selftest_done++;
}

static void scan_tick(app_t *a)
{
    ui_data_t *u = &a->ui;
    uint32_t v = u->scan_field ? u->h_ut_x100 * 10u : u->e_vm_x10;
    if (a->scan_base_x10 == 0)
        a->scan_base_x10 = v + 1;
    /* slow baseline so that moving towards a source raises the bars */
    a->scan_base_x10 += (int32_t)(v - a->scan_base_x10) / 32;
    uint32_t ratio = v * 4 / (a->scan_base_x10 + 1);
    uint32_t bars = 1;
    while (ratio > 1 && bars < 10) { ratio >>= 1; bars++; }
    u->scan_rising = v > a->scan_prev_x10 + a->scan_prev_x10 / 20;
    a->scan_prev_x10 = v;
    u->scan_bars = bars;
    u->scan_value_x10 = v;
    if (a->scan_sound)
        beep_now(a, 700 + bars * 250);
}

static void run_second(app_t *a, uint32_t now_ms)
{
    ui_data_t *u = &a->ui;
    uint32_t now_s = H(a)->rtc_now();
    a->seconds_run++;

    /* tube */
    uint32_t pulses = 0;
    if (feat_tube(a)) {
        start_tube(a);
        uint32_t t = H(a)->tube_pulses();
        pulses = t - a->last_pulses;
        a->last_pulses = t;
    }
    if (feat_usb(a) && !a->usb_started && H(a)->usb_start) {
        a->usb_started = true;
        H(a)->usb_start();
    }
    meas_tick(&a->meas, a->hv_on ? pulses : 0);
    if (a->hv_on)
        a->hv_secs++;
    uint32_t rate = meas_rate_nsvh(&a->meas);
    uint64_t dose = total_dose(a);

    /* day boundaries and per-day dose */
    uint32_t day = (uint32_t)(((int64_t)now_s + a->settings.tz_hours * 3600) / 86400);
    if (now_s > 1000000000u) {
        if (a->day_index == 0)
            a->day_index = day;
        while (a->day_index < day) {
            memmove(a->day_nsv, a->day_nsv + 1, 6 * sizeof a->day_nsv[0]);
            a->day_nsv[6] = 0;
            a->day_index++;
            if (day - a->day_index > 7)
                a->day_index = day - 7;
        }
    }
    if (dose > a->dose_prev_total)
        a->day_nsv[6] += (uint32_t)(dose - a->dose_prev_total);
    a->dose_prev_total = dose;

    /* alarms and events */
    alarm_result_t r = alarm_tick(&a->alarm, now_s, meas_reliable(&a->meas) ? rate : 0, dose);
    if (r.level_changed) {
        a->alarm_muted = false;
        a->alarm_dismissed = false;
        if (a->alarm.level > 0 && cur(a) != SCR_ALARM && cur(a) != SCR_CALWIZ) {
            push(a, SCR_ALARM);
            a->last_input_ms = now_ms; /* wake the screen */
        }
        if (a->alarm.level == 0 && cur(a) == SCR_ALARM)
            pop(a);
    }
    if (cur(a) == SCR_CALWIZ) { /* calibrating next to a source: the source is meant to be there, stay quiet */
        a->alarm_muted = true;
        a->alarm_dismissed = true;
    }
    if (r.event_closed)
        store_event(a, &r.event);
    if (r.budget_warn || r.budget_exceeded) {
        chirp(a, r.budget_exceeded ? 3000 : 2200);
        if (a->settings.vibration && feat_sensors(a))
            H(a)->vibrate(true);
    }

    /* battery */
    uint32_t mv = feat_sensors(a) ? H(a)->battery_mv() : 4000;
    bool usb = feat_sensors(a) && H(a)->usb_present();
    bat_tick(&a->bat, mv, usb, usb && H(a)->charging());
    if (usb && !a->usb_was_present && cur(a) != SCR_ALARM && cur(a) != SCR_CHARGE) {
        push(a, SCR_CHARGE); /* cable plugged in: show the charging screen */
        a->last_input_ms = now_ms;
    }
    if (!usb && a->usb_was_present && cur(a) == SCR_CHARGE)
        pop(a);
    a->usb_was_present = usb;
    {
        bool lit = feat_sensors(a) && ((a->settings.led_charge && usb && H(a)->charging()) || a->led_preview_s > 0);
        if (a->led_preview_s)
            a->led_preview_s--;
        if (lit != a->status_led_on) {
            a->status_led_on = lit;
            H(a)->status_led(lit);
        }
    }

    /* trends */
    if (++a->trend_timer >= TREND_STEP_S) {
        a->trend_timer = 0;
        push_trend(u->trend, rate);
        stats_of(u->trend, &u->trend_min, &u->trend_avg, &u->trend_max);
        uint32_t first = u->trend[0] ? u->trend[0] : 1;
        u->trend_change_pct = (int32_t)(((int64_t)u->trend[TREND_N - 1] - u->trend[0]) * 100 / first);
    }
    if (feat_sensors(a) && ++a->field_timer >= FIELD_STEP_S) {
        a->field_timer = 0;
        u->e_vm_x10 = (uint32_t)((uint64_t)H(a)->efield_vm_x10() * u->cal[2] / 100u);
        u->h_ut_x100 = (uint32_t)((uint64_t)H(a)->hfield_ut_x100() * u->cal[3] / 100u);
        push_trend(u->e_trend, u->e_vm_x10);
        push_trend(u->h_trend, u->h_ut_x100);
        uint32_t mn, avg, mx;
        stats_of(u->e_trend, &mn, &avg, &mx);
        u->e_avg_x10 = avg; u->e_peak_x10 = mx;
        stats_of(u->h_trend, &mn, &avg, &mx);
        u->h_avg_x100 = avg; u->h_peak_x100 = mx;
    }

    /* learned background: slow average of the normal rate (24 h time constant, updated once a minute) */
    if (a->seconds_run % 60 == 0 && a->alarm.level == 0 && rate > 0) {
        if (a->bg_x16 == 0)
            a->bg_x16 = rate * 16u;
        else
            a->bg_x16 = a->bg_x16 + (int32_t)(rate * 16u - a->bg_x16) / 1440;
    }

    /* log */
    log_tick(a, now_s, a->hv_on ? pulses : 0);

    /* sample test */
    if (u->sample_phase == 1 || u->sample_phase == 2) {
        a->sample_counts += pulses;
        a->sample_seconds++;
        u->sample_elapsed_s = a->sample_seconds;
        uint32_t cps_x100 = a->sample_seconds ? (uint32_t)((uint64_t)a->sample_counts * 100u / a->sample_seconds) : 0;
        uint32_t nsvh = meas_nsvh_from_cps_x100(&a->meas.cfg, cps_x100);
        if (u->sample_phase == 1)
            u->sample_bg_nsvh = nsvh;
        else
            u->sample_nsvh = nsvh;
        u->sample_net_nsvh = (int32_t)u->sample_nsvh - (int32_t)u->sample_bg_nsvh;
        if (a->sample_seconds >= u->sample_goal_s)
            chirp(a, 2400);
    }

    /* calibration wizard: counts next to the reference */
    if (cur(a) == SCR_CALWIZ && u->cw_phase == 1) {
        uint32_t before = a->cw.counts;
        cal_tick(&a->cw, a->hv_on ? pulses : 0, u->cal[1]);
        u->cw_counts = a->cw.counts;
        u->cw_seconds = a->cw.seconds;
        u->cw_unc_x10 = cal_uncertainty_x10(&a->cw);
        u->cw_saturated = a->cw.saturated;
        u->cw_new_x100 = cal_sensitivity_x100(&a->cw, u->cw_ref_nsvh);
        if (before < 1000 && a->cw.counts >= 1000)
            chirp(a, 2400);
    }

    sess_tick(a, dose, rate);

    /* self test and scan */
    if (cur(a) == SCR_SELFTEST && ++a->selftest_tick % 1 == 0)
        selftest_step(a);
    if (cur(a) == SCR_SCAN)
        scan_tick(a);

    /* periodic saves */
    if (a->seconds_run - a->last_save_s >= SAVE_COUNTERS_S) {
        a->last_save_s = a->seconds_run;
        save_counters(a);
    }
    if (a->seconds_run - a->last_slow_save_s >= SAVE_SLOW_S) {
        a->last_slow_save_s = a->seconds_run;
        save_slow(a);
    }
    if (a->sound_phase == 0 && a->settings.vibration)
        H(a)->vibrate(false);

    /* what the screens read */
    u->now = now_s;
    u->tz_offset_s = a->settings.tz_hours * 3600;
    u->rate_nsvh = rate;
    u->cpm = meas_cpm(&a->meas);
    u->level = (uint8_t)a->alarm.level;
    u->bg_nsvh = a->bg_x16 / 16u;
    u->dose_total_nsv = dose;
    u->dose_today_nsv = a->day_nsv[6];
    uint64_t week = 0;
    for (int i = 0; i < 7; i++) {
        u->week_nsv[i] = a->day_nsv[i];
        week += a->day_nsv[i];
    }
    u->dose_week_nsv = week;
    u->th_nsvh[0] = a->th[0]; u->th_nsvh[1] = a->th[1]; u->th_nsvh[2] = a->th[2];
    u->bat_percent = (uint8_t)bat_percent(&a->bat);
    u->bat_state = (uint8_t)a->bat.state;
    u->bat_minutes = bat_remaining_minutes(&a->bat);
    u->bat_health = (uint8_t)bat_health_percent(&a->bat);
    u->bat_cycles = bat_cycles(&a->bat);
    u->on_hours = a->bat.stats.on_seconds / 3600u;
    u->usb = usb;
    u->usb_connected = feat_usb(a) && H(a)->usb_connected && H(a)->usb_connected();
    u->usb_streaming = u->usb_connected;
    u->tube_life = (uint8_t)(100u - (total_pulses(a) / 100000000u > 100 ? 100 : total_pulses(a) / 100000000u));
    u->total_pulses = total_pulses(a);
    u->bg_cpm = u->cpm;
    u->deadtime_on = a->meas.cfg.dead_time_us > 0;
    u->hv_factory = H(a)->hv_factory_valid ? H(a)->hv_factory_valid() : false;
    u->theme = a->settings.theme;
    u->brightness = a->settings.brightness;
    u->sound = a->settings.sound;
    u->vibration = a->settings.vibration;
    u->led_pulse = a->settings.led_pulse;
    u->led_charge = a->settings.led_charge;
    u->click = a->settings.click;
    u->count_vibe = a->settings.count_vibe;
    u->led_color = a->settings.led_color;
    u->usb_start = a->settings.usb_start;
    u->profile = a->settings.profile;
    u->stage = a->stage;
    u->budget_active = a->alarm.budget_active;
    u->budget_nsv = (uint32_t)a->alarm.budget_nsv;
    u->budget_used_nsv = (uint32_t)(dose - a->alarm.budget_start);
    uint32_t bdur = now_s - u->budget_start_time;
    u->budget_avg_nsvh = bdur ? (uint32_t)((uint64_t)u->budget_used_nsv * 3600u / bdur) : 0;

    if (a->log_ok) {
        ylog_stats_t ls;
        if (a->seconds_run % 60 == 1) { /* scanning the whole log is slow: once a minute is plenty */
            ylog_stats(H(a)->log_flash, app_scratch(), &ls);
            u->storage_pct = (uint8_t)(ls.bytes_total ? (uint64_t)ls.bytes_used * 100u / ls.bytes_total : 0);
            u->oldest_time = ls.oldest_time;
            u->days_stored = ls.newest_time > ls.oldest_time ? (ls.newest_time - ls.oldest_time) / 86400u : 0;
        }
    }
    u->alarm_count = u->event_count;
    /* days a full log lasts per profile: about 126 KB of log space at the measured coding cost */
    static const uint32_t bytes_per_day_x10[5] = {75000, 24000, 6800, 1760, 72};
    for (int i = 0; i < 5; i++)
        u->profile_days[i] = 1260000u / bytes_per_day_x10[i];
}

/* ---- input ---- */

static int wrap(int v, int n) { return (v % n + n) % n; }

static void apply_led_color(app_t *a);

/* the theme also decides the LED colour, unless a colour was chosen */
static void apply_theme(app_t *a)
{
    ui_theme = &themes[a->settings.theme % THEME_COUNT];
    apply_led_color(a);
}

static void backlight_apply(app_t *a)
{
    if (a->display_on && feat_sensors(a))
        H(a)->backlight(a->dimmed ? 1 : a->settings.brightness);
}

#define MENU_ENTRIES 16

static void open_menu_entry(app_t *a, int idx)
{
    ui_data_t *u = &a->ui;
    switch (idx) {
    case 0: push(a, SCR_GUIDE_LIST); break;
    case 1: push(a, SCR_EVENTS); break;
    case 2: push(a, SCR_BUDGET); break;
    case 3:
        push(a, SCR_SESSION);
        st(a)->mode = u->sess_active ? 2 : 0;
        break;
    case 4:
        push(a, SCR_SAMPLE);
        a->sample_counts = 0;
        a->sample_seconds = 0;
        u->sample_phase = 1;
        u->sample_goal_s = 600;
        u->sample_bg_nsvh = u->sample_nsvh = 0;
        u->sample_net_nsvh = 0;
        u->sample_elapsed_s = 0;
        break;
    case 5:
        push(a, SCR_SCAN);
        a->scan_base_x10 = 0;
        a->scan_prev_x10 = 0;
        a->scan_sound = false;
        u->scan_field = 0;
        break;
    case 6:
        push(a, SCR_LOG);
        st(a)->mode = 1;
        break;
    case 7: push(a, SCR_THRESH); break;
    case 8: push(a, SCR_TUBE); break;
    case 9:
        push(a, SCR_SELFTEST);
        u->selftest_done = 0;
        u->selftest_fail = 0;
        u->selftest_total = 7;
        a->selftest_tick = 0;
        break;
    case 10: push(a, SCR_USB); break;
    case 11:
        push(a, SCR_THEMES);
        a->theme_before = a->settings.theme;
        st(a)->sel = a->settings.theme;
        break;
    case 12: push(a, SCR_SETTINGS); break;
    case 13: push(a, SCR_CALIB); break;
    case 14:
        push(a, SCR_CALWIZ);
        u->cw_phase = 0;
        u->cw_ref_nsvh = u->rate_nsvh < 50 ? 50 : u->rate_nsvh;
        break;
    case 15: push(a, SCR_ABOUT); break;
    default: break;
    }
}

static void change_threshold(app_t *a, int idx, int dir)
{
    uint32_t v = a->edit_value;
    v = dir > 0 ? v * 115u / 100u + 1u : v * 100u / 115u;
    if (v < 10)
        v = 10;
    if (v > 5000000u)
        v = 5000000u;
    /* keep the three thresholds in order */
    if (idx > 0 && v <= a->th[idx - 1])
        v = a->th[idx - 1] + 1;
    if (idx < 2 && v >= a->th[idx + 1])
        v = a->th[idx + 1] - 1;
    a->edit_value = v;
    a->ui.th_nsvh[idx] = v;
}

static void silence(app_t *a)
{
    H(a)->beep(0);
    H(a)->vibrate(false);
    H(a)->alert_led(false);
}

static void open_clock_editor(app_t *a)
{
    uint32_t now = H(a)->rtc_now();
    uint32_t local = now > 1700000000u ? now + (uint32_t)(a->settings.tz_hours * 3600) : time_from_civil(2026, 1, 1, 0, 0);
    uint32_t y, m, d, hh, mm;
    time_to_civil(local, &y, &m, &d, &hh, &mm);
    a->ui.clk[0] = (uint16_t)y; a->ui.clk[1] = (uint16_t)m; a->ui.clk[2] = (uint16_t)d;
    a->ui.clk[3] = (uint16_t)hh; a->ui.clk[4] = (uint16_t)mm;
    push(a, SCR_CLOCK);
}

static void clock_change(app_t *a, int field, int dir)
{
    uint16_t *c = a->ui.clk; /* year, month, day, hour, minute */
    int v;
    switch (field) {
    case 0: v = (int)c[2] + dir; c[2] = (uint16_t)(v < 1 ? (int)days_in_month(c[0], c[1]) : v); break; /* day */
    case 1: v = (int)c[1] + dir; c[1] = (uint16_t)(v < 1 ? 12 : v > 12 ? 1 : v); break;              /* month */
    case 2: v = (int)c[0] + dir; c[0] = (uint16_t)(v < 2024 ? 2024 : v > 2099 ? 2099 : v); break;     /* year */
    case 3: v = (int)c[3] + dir; c[3] = (uint16_t)(v < 0 ? 23 : v > 23 ? 0 : v); break;
    default: v = (int)c[4] + dir; c[4] = (uint16_t)(v < 0 ? 59 : v > 59 ? 0 : v); break;
    }
    uint32_t dim = days_in_month(c[0], c[1]);
    if (c[2] > dim)
        c[2] = (uint16_t)(field == 0 ? 1 : dim);
}

static void calib_change(app_t *a, int idx, int dir)
{
    uint32_t *v = &a->ui.cal[idx];
    switch (idx) {
    case 0: { uint32_t step = *v / 100u; if (step < 10) step = 10; *v = dir > 0 ? *v + step : (*v > 1000 + step ? *v - step : 1000); if (*v > 100000) *v = 100000; break; }
    case 1: *v = dir > 0 ? (*v + 5 > 2000 ? 2000 : *v + 5) : (*v < 5 ? 0 : *v - 5); break;
    default: *v = dir > 0 ? (*v + 1 > 1000 ? 1000 : *v + 1) : (*v <= 10 ? 10 : *v - 1); break;
    }
}

static void on_event(app_t *a, input_event_t ev)
{
    ui_state_t *s = st(a);
    screen_id_t id = cur(a);
    bool left = ev == EV_LEFT, right = ev == EV_RIGHT;
    int dir = right ? 1 : -1;

    if (ev == EV_POWER) {
        a->display_on = !a->display_on;
        H(a)->display_enable(a->display_on);
        a->dimmed = false;
        backlight_apply(a);
        invalidate(a);
        return;
    }
    if (ev == EV_POWER_LONG) {
        a->shutting_down = true;
        return;
    }

    bool home = a->sp == 0 && id == home_pages[a->home_idx];
    if (home) {
        if (left || right) {
            set_home(a, a->home_idx + dir);
        } else if (ev == EV_OK) {
            push(a, SCR_MENU);
        } else if (ev == EV_BACK) {
            push(a, SCR_MODE);
            a->states[a->sp].sel = a->home_idx == 2 ? 1 : a->home_idx == 3 ? 2 : 0;
        }
        return;
    }

    if (id == SCR_CHARGE) {
        pop(a);
        return;
    }

    switch (id) {
    case SCR_ALARM:
        if (ev == EV_BACK) {
            if (!a->alarm_muted) {
                a->alarm_muted = true;
                silence(a);
            } else {
                a->alarm_dismissed = true;
                pop(a);
            }
        } else if (ev == EV_OK) {
            a->alarm_muted = true;
            silence(a);
            push(a, SCR_GUIDE_LIST);
        }
        break;

    case SCR_MODE:
        if (left || right) { s->sel = wrap(s->sel + dir, 3); a->need_draw = true; }
        else if (ev == EV_OK) set_home(a, s->sel == 0 ? 0 : s->sel == 1 ? 2 : 3);
        else if (ev == EV_BACK) pop(a);
        break;

    case SCR_MENU:
        if (left || right) {
            s->sel = wrap(s->sel + dir, MENU_ENTRIES);
            if (s->sel < s->scroll) s->scroll = s->sel;
            if (s->sel >= s->scroll + 6) s->scroll = s->sel - 5;
            a->need_draw = true;
        } else if (ev == EV_OK) {
            open_menu_entry(a, s->sel);
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    case SCR_GUIDE_LIST:
        if (left || right) { s->sel = wrap(s->sel + dir, GUIDE_PAGES); a->need_draw = true; }
        else if (ev == EV_OK) { int p = s->sel; push(a, SCR_GUIDE_PAGE); st(a)->page = p; }
        else if (ev == EV_BACK) pop(a);
        break;

    case SCR_GUIDE_PAGE:
        if (left || right) {
            int p = s->page + dir;
            if (p >= 0 && p < GUIDE_PAGES) { s->page = p; invalidate(a); }
        } else if (ev == EV_BACK || ev == EV_OK) {
            a->states[a->sp - 1].sel = s->page;
            pop(a);
        }
        break;

    case SCR_SETTINGS:
        if (left || right) {
            s->sel = wrap(s->sel + dir, SETTINGS_ROWS);
            if (s->sel < s->scroll)
                s->scroll = s->sel;
            if (s->sel >= s->scroll + 6)
                s->scroll = s->sel - 5;
            a->need_draw = true;
        } else if (ev == EV_OK) {
            switch (s->sel) {
            case 0:
                push(a, SCR_THEMES);
                a->theme_before = a->settings.theme;
                st(a)->sel = a->settings.theme;
                break;
            case 1: a->settings.brightness = (uint8_t)(a->settings.brightness % 5 + 1); a->dimmed = false; backlight_apply(a); break;
            case 2: a->settings.sound = !a->settings.sound; break;
            case 3: a->settings.vibration = !a->settings.vibration; break;
            case 4:
                a->settings.click = !a->settings.click;
                H(a)->count_click_enable(a->settings.click);
                break;
            case 5:
                a->settings.count_vibe = !a->settings.count_vibe;
                H(a)->count_vibe_enable(a->settings.count_vibe);
                break;
            case 6:
                a->settings.led_pulse = !a->settings.led_pulse;
                H(a)->pulse_led_enable(a->settings.led_pulse);
                break;
            case 7: a->settings.led_charge = !a->settings.led_charge; break;
            case 8:
                a->settings.led_color = (uint8_t)((a->settings.led_color + 1) % 8);
                apply_led_color(a);
                a->led_preview_s = 3; /* show it */
                break;
            case 9: a->settings.usb_start = !a->settings.usb_start; break;
            case 10: a->settings.tz_hours = (int8_t)(a->settings.tz_hours >= 14 ? -12 : a->settings.tz_hours + 1); break;
            case 11: open_clock_editor(a); break;
            default: break;
            }
            save_settings(a);
            a->need_draw = true;
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    case SCR_THEMES:
        if (left || right) {
            s->sel = wrap(s->sel + dir, THEME_COUNT);
            a->settings.theme = (uint8_t)s->sel;
            apply_theme(a);
            invalidate(a);
        } else if (ev == EV_OK) {
            save_settings(a);
            pop(a);
        } else if (ev == EV_BACK) {
            a->settings.theme = (uint8_t)a->theme_before;
            apply_theme(a);
            pop(a);
        }
        break;

    case SCR_EVENTS:
        if ((left || right) && a->ui.event_count) {
            s->sel = wrap(s->sel + dir, a->ui.event_count);
            if (s->sel < s->scroll) s->scroll = s->sel;
            if (s->sel >= s->scroll + 4) s->scroll = s->sel - 3;
            a->need_draw = true;
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    case SCR_BUDGET:
        if (a->edit) {
            if (left || right) {
                a->edit_value = (uint32_t)wrap((int)a->edit_value + dir, 6);
                a->need_draw = true;
            } else if (ev == EV_OK) {
                uint64_t now_dose = total_dose(a);
                alarm_budget_start(&a->alarm, budget_presets[a->edit_value], now_dose);
                a->ui.budget_start_time = H(a)->rtc_now();
                save_budget(a, now_dose);
                a->edit = 0;
                a->need_draw = true;
            } else if (ev == EV_BACK) {
                a->edit = 0;
                a->need_draw = true;
            }
        } else if (ev == EV_OK) {
            a->edit = 1;
            a->edit_value = 1;
            a->need_draw = true;
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    case SCR_SAMPLE: {
        ui_data_t *u = &a->ui;
        if (ev == EV_OK) {
            if (u->sample_phase == 1) {
                u->sample_phase = 2; a->sample_counts = 0; a->sample_seconds = 0; u->sample_elapsed_s = 0;
            } else if (u->sample_phase == 2) {
                u->sample_phase = 3;
            } else {
                u->sample_phase = 1; a->sample_counts = 0; a->sample_seconds = 0; u->sample_elapsed_s = 0; u->sample_nsvh = 0;
            }
            a->need_draw = true;
        } else if (ev == EV_BACK) {
            u->sample_phase = 0;
            pop(a);
        }
        break;
    }

    case SCR_CALWIZ: {
        ui_data_t *u = &a->ui;
        if (u->cw_phase == 0) {
            if (left || right) {
                uint32_t r = u->cw_ref_nsvh;
                uint32_t step = r < 1000 ? 10 : r < 10000 ? 100 : r < 100000 ? 1000 : r < 1000000 ? 10000 : 100000;
                if (dir > 0)
                    r += step;
                else
                    r = r > step + 50 ? r - step : 50;
                if (r > 9000000)
                    r = 9000000;
                u->cw_ref_nsvh = r;
                a->need_draw = true;
            } else if (ev == EV_OK) {
                cal_reset(&a->cw);
                u->cw_phase = 1;
                u->cw_counts = 0;
                u->cw_seconds = 0;
                u->cw_unc_x10 = 1000;
                u->cw_new_x100 = 0;
                u->cw_saturated = false;
                invalidate(a);
            } else if (ev == EV_BACK) {
                pop(a);
            }
        } else if (u->cw_phase == 1) {
            if (ev == EV_OK) {
                if (a->cw.counts >= 100 && !a->cw.saturated)
                    u->cw_phase = 2;
                else
                    chirp(a, 500);
                invalidate(a);
            } else if (ev == EV_BACK) {
                u->cw_phase = 0;
                invalidate(a);
            }
        } else {
            if (ev == EV_OK) {
                if (u->cw_new_x100 >= 1000 && u->cw_new_x100 <= 100000) {
                    u->cal[0] = u->cw_new_x100;
                    save_calibration(a);
                    u->cw_phase = 0;
                    pop(a);
                } else {
                    chirp(a, 500);
                }
            } else if (ev == EV_BACK) {
                u->cw_phase = 0;
                invalidate(a);
            }
        }
        break;
    }

    case SCR_SESSION: {
        ui_data_t *u = &a->ui;
        int rows = 1 + (int)u->sess_saved;
        if (s->mode == 0) {
            if (left || right) {
                s->sel = wrap(s->sel + dir, rows);
                if (s->sel < s->scroll)
                    s->scroll = s->sel;
                if (s->sel >= s->scroll + 5)
                    s->scroll = s->sel - 4;
                a->need_draw = true;
            } else if (ev == EV_OK) {
                if (s->sel == 0) {
                    s->mode = u->sess_active ? 2 : 1;
                    s->sel = 0;
                    s->page = 0;
                } else {
                    s->mode = 3;
                    s->page = s->sel - 1;
                }
                invalidate(a);
            } else if (ev == EV_BACK) {
                pop(a);
            }
        } else if (s->mode == 1) {
            if (left || right) {
                s->sel = wrap(s->sel + dir, SESS_KINDS);
                a->need_draw = true;
            } else if (ev == EV_OK) {
                sess_start(a, (uint8_t)s->sel);
                s->mode = 2;
                s->page = 0;
                invalidate(a);
            } else if (ev == EV_BACK) {
                s->mode = 0;
                s->sel = 0;
                invalidate(a);
            }
        } else if (s->mode == 2) {
            if (ev == EV_OK) {
                if (s->page == 0) {
                    s->page = 1;
                    a->need_draw = true;
                } else {
                    sess_stop(a);
                    s->mode = 0;
                    s->sel = 0;
                    s->scroll = 0;
                    s->page = 0;
                    invalidate(a);
                }
            } else if (ev == EV_BACK) {
                if (s->page) {
                    s->page = 0;
                    a->need_draw = true;
                } else {
                    s->mode = 0;
                    s->sel = 0;
                    invalidate(a);
                }
            }
        } else {
            if ((left || right) && u->sess_saved) {
                s->page = wrap(s->page + dir, (int)u->sess_saved);
                a->need_draw = true;
            } else if (ev == EV_BACK || ev == EV_OK) {
                s->mode = 0;
                s->sel = 0;
                invalidate(a);
            }
        }
        break;
    }

    case SCR_LOG:
        if (s->mode == 1) {
            if (ev == EV_OK) { s->mode = 0; s->sel = a->settings.profile; invalidate(a); }
            else if (ev == EV_BACK) pop(a);
        } else if (left || right) {
            s->sel = wrap(s->sel + dir, 5);
            a->need_draw = true;
        } else if (ev == EV_OK) {
            a->settings.profile = (uint8_t)s->sel;
            save_settings(a);
            if (a->log_ok) {
                ylog_close(&a->log);
                a->log_interval = profile_interval[s->sel];
                a->log_start = 0;
            }
            a->need_draw = true;
        } else if (ev == EV_BACK) {
            s->mode = 1;
            invalidate(a);
        }
        break;

    case SCR_THRESH:
        if (a->edit) {
            if (left || right) { change_threshold(a, s->sel, dir); a->need_draw = true; }
            else if (ev == EV_OK) { a->th[s->sel] = a->edit_value; save_thresholds(a); a->edit = 0; a->need_draw = true; }
            else if (ev == EV_BACK) { a->ui.th_nsvh[s->sel] = a->th[s->sel]; a->edit = 0; a->need_draw = true; }
        } else if (left || right) {
            s->sel = wrap(s->sel + dir, 3);
            a->need_draw = true;
        } else if (ev == EV_OK) {
            a->edit = 1;
            a->edit_value = a->th[s->sel];
            a->need_draw = true;
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    case SCR_TUBE:
        if (ev == EV_OK) {
            push(a, SCR_SELFTEST);
            a->ui.selftest_done = 0;
            a->ui.selftest_fail = 0;
            a->ui.selftest_total = 7;
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    case SCR_SCAN:
        if (left || right) {
            a->ui.scan_field ^= 1;
            a->scan_base_x10 = 0;
            a->scan_prev_x10 = 0;
        } else if (ev == EV_OK) {
            a->scan_sound = !a->scan_sound;
            if (!a->scan_sound)
                H(a)->beep(0);
            a->need_draw = true;
        } else if (ev == EV_BACK) {
            H(a)->beep(0);
            pop(a);
        }
        break;

    case SCR_CLOCK:
        if (left || right) {
            clock_change(a, s->sel, dir);
            a->need_draw = true;
        } else if (ev == EV_OK) {
            if (s->sel < 4) {
                s->sel++;
                a->need_draw = true;
            } else {
                uint16_t *c = a->ui.clk;
                int64_t unix_s = (int64_t)time_from_civil(c[0], c[1], c[2], c[3], c[4]) - (int64_t)a->settings.tz_hours * 3600;
                if (unix_s > 0)
                    H(a)->rtc_set((uint32_t)unix_s);
                pop(a);
            }
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    case SCR_CALIB:
        if (s->mode) {
            if (left || right) { calib_change(a, s->sel, dir); a->need_draw = true; }
            else if (ev == EV_OK) { save_calibration(a); s->mode = 0; a->need_draw = true; }
            else if (ev == EV_BACK) { restore_calibration(a); s->mode = 0; a->need_draw = true; }
        } else if (left || right) {
            s->sel = wrap(s->sel + dir, 4);
            a->need_draw = true;
        } else if (ev == EV_OK) {
            s->mode = 1;
            a->need_draw = true;
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    case SCR_ABOUT:
        if (ev == EV_OK && a->kv_ok) { /* try the suspect start-up steps again */
            a->suspect_mask = 0;
            kv_put32(a, K_STAGE, a->verified);
        } else if (ev == EV_BACK) {
            pop(a);
        }
        break;

    default: /* self test, usb and the rest only go back */
        if (ev == EV_BACK)
            pop(a);
        break;
    }
}

/* ---- init and poll ---- */

void app_prepare(app_t *a, const hal_t *hal)
{
    memset(a, 0, sizeof *a);
    a->hal = hal;
    a->kv_ok = hal->kv_flash && kv_open(&a->kv, hal->kv_flash) == 0;
    load_settings(a);
    a->prepared = true;
}

bool app_boot_allowed(const app_t *a)
{
    return a->settings.usb_start || !a->hal->started_by_usb();
}

void app_init(app_t *a, const hal_t *hal, uint32_t now_ms)
{
    if (!a->prepared)
        app_prepare(a, hal);
    a->prepared = false;
    a->hal = hal;
    a->display_on = true;
    a->last_input_ms = now_ms;
    a->next_second_ms = now_ms + 1000;
    a->last_rtc = hal->rtc_now();
    a->last_tick_ms = now_ms;

    meas_cfg_t mc = {18000, 90};
    meas_init(&a->meas, &mc);
    alarm_cfg_t ac = {{500, 10000, 100000}};
    alarm_init(&a->alarm, &ac);
    bat_init(&a->bat, 0);

    a->alarm.cfg.th_nsvh[0] = a->th[0];
    a->alarm.cfg.th_nsvh[1] = a->th[1];
    a->alarm.cfg.th_nsvh[2] = a->th[2];
    apply_theme(a);
    apply_calibration(a);
    hal->pulse_led_enable(a->settings.led_pulse);
    hal->count_click_enable(a->settings.click);
    hal->count_vibe_enable(a->settings.count_vibe);
    apply_led_color(a);

    sess_load(a);
    a->sess_prev_total = total_dose(a);

    ui_data_t *u = &a->ui;
    u->selftest_total = 7;
    u->sample_goal_s = 600;
    u->now = hal->rtc_now();
    u->tz_offset_s = a->settings.tz_hours * 3600;
    u->profile = a->settings.profile;

    ladder_init(a, now_ms);

    set_home(a, 0);
    hal->display_enable(true);
    if (feat_sensors(a))
        hal->backlight(a->settings.brightness);
}

screen_id_t app_screen(const app_t *a) { return a->stack[a->sp]; }

void app_shutdown(app_t *a)
{
    const hal_t *h = a->hal;
    if (a->log_ok)
        ylog_close(&a->log);
    save_counters(a);
    save_slow(a);
    save_settings(a);
    sess_save_active(a);
    if (a->hv_on)
        h->hv_stop();
    a->hv_on = false;
    silence(a);
    h->display_enable(false);
    h->power_off();
}

void app_poll(app_t *a, uint32_t now_ms)
{
    const hal_t *h = a->hal;
    h->watchdog_kick();

    if (a->shutting_down) {
        app_shutdown(a);
        return;
    }

    /* input: the first event after the screen went dark only wakes it */
    for (;;) {
        input_event_t ev = h->poll_event();
        if (ev == EV_NONE)
            break;
        a->last_input_ms = now_ms;
        if (!a->display_on && ev != EV_POWER_LONG && ev != EV_POWER) {
            a->display_on = true;
            a->dimmed = false;
            h->display_enable(true);
            backlight_apply(a);
            invalidate(a);
            continue;
        }
        if (a->dimmed) {
            a->dimmed = false;
            backlight_apply(a);
        }
        on_event(a, ev);
    }

    ladder_poll(a, now_ms);
    start_log(a);

    /* The application second follows the real-time clock (a crystal), not the millisecond counter, whose rate rests on
     * the assumed core clock: an error there once made every count look 1.5 times too slow. If the clock stands still
     * for 2.5 s the millisecond counter takes over, so measuring never stops. */
    uint32_t rt = a->hal->rtc_now();
    bool second = false;
    if (rt != a->last_rtc) {
        if (rt - a->last_rtc == 1u) {                 /* compare the two clocks over a minute of undisturbed seconds */
            if (a->clk_rtc0 == 0) {
                a->clk_rtc0 = rt;
                a->clk_ms0 = now_ms;
            } else if (rt - a->clk_rtc0 >= 60u) {
                a->ms_per_s = (now_ms - a->clk_ms0) / (rt - a->clk_rtc0);
                a->clk_rtc0 = 0;
            }
        } else {
            a->clk_rtc0 = 0;                          /* the clock was set: start over */
        }
        a->last_rtc = rt;
        a->last_tick_ms = now_ms;
        second = true;
    } else if ((int32_t)(now_ms - a->last_tick_ms) >= 2500) {
        a->last_tick_ms = now_ms - 1500;
        second = true;
    }
    if (second) {
        a->next_second_ms = now_ms + 1000;
        run_second(a, now_ms);
        if (screen_sig(a) != a->drawn_sig)
            a->need_draw = true;
    }

    if (now_ms - a->anim_ms >= 250) {
        a->anim_ms = now_ms;
        a->ui.anim++;
        if (cur(a) == SCR_CHARGE)
            a->need_draw = true;
    }

    if (now_ms - a->blink_ms >= 500) {
        a->blink_ms = now_ms;
        a->ui.blink = !a->ui.blink;
        if (cur(a) == SCR_ALARM || cur(a) == SCR_SELFTEST)
            a->need_draw = true;
    }

    /* the screen: an alarm keeps it awake, so does a measurement somebody is watching, otherwise it dims and then
     * switches off */
    bool attended = (cur(a) == SCR_CALWIZ && a->ui.cw_phase == 1) ||
                    (cur(a) == SCR_SAMPLE && (a->ui.sample_phase == 1 || a->ui.sample_phase == 2));
    if (attended)
        a->last_input_ms = now_ms;
    if (a->alarm.level > 0 && !a->alarm_dismissed) {
        a->last_input_ms = now_ms;
        if (!a->display_on) {
            a->display_on = true;
            h->display_enable(true);
            a->dimmed = false;
            backlight_apply(a);
            invalidate(a);
        }
    } else if (a->display_on && feat_sensors(a)) {
        if (now_ms - a->last_input_ms > OFF_MS) {
            a->display_on = false;
            h->display_enable(false);
        } else if (!a->dimmed && now_ms - a->last_input_ms > DIM_MS && a->settings.brightness > 1) {
            a->dimmed = true;
            backlight_apply(a);
        }
    }

    sound_poll(a, now_ms);

    if (a->display_on && (a->need_full || a->need_draw)) {
        bool full = a->need_full;
        a->need_full = false;
        a->need_draw = false;
        a->drawn_sig = screen_sig(a);
        draw_ctx_t ctx = {a, full};
        display_render((uint32_t)cur(a) | ((uint32_t)a->ui.theme << 8), full, draw_cb, &ctx);
    }
}

bool app_set_calibration(app_t *a, const uint32_t cal[4])
{
    if (cal[0] < 1000 || cal[0] > 100000 || cal[1] > 2000 || cal[2] < 10 || cal[2] > 1000 || cal[3] < 10 || cal[3] > 1000)
        return false;
    for (int i = 0; i < 4; i++)
        a->ui.cal[i] = cal[i];
    save_calibration(a);
    a->need_draw = true;
    return true;
}

bool app_set_theme(app_t *a, uint32_t theme)
{
    if (theme >= THEME_COUNT)
        return false;
    a->settings.theme = (uint8_t)theme;
    apply_theme(a);
    save_settings(a);
    invalidate(a);
    return true;
}

void app_set_time(app_t *a, uint32_t unix_s)
{
    H(a)->rtc_set(unix_s);
    a->ui.now = unix_s;
    a->need_draw = true;
}
