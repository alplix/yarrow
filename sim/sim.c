/* Host simulator: renders every screen in every theme into PPM files.
 * `make sim` builds and runs it, tools/montage.py turns the PPMs into contact sheets. */

#include "display.h"
#include "screens.h"
#include "theme.h"
#include "ui.h"
#include "ui_boot.h"
#include <stdio.h>
#include <string.h>

static uint16_t fb[DISPLAY_W * DISPLAY_H], ref_fb[DISPLAY_W * DISPLAY_H];
static uint16_t *target = fb;
static long sim_writes;
static bool warn_on;
static void sim_layout_warn(const char *kind, const char *text, int width, int room)
{
    if (warn_on)
        printf("LAYOUT %s: \"%s\" needs %d px, room for %d\n", kind, text, width, room);
}
static uint32_t rng = 12345;
static uint32_t rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

typedef struct { screen_id_t id; const ui_data_t *d; const ui_state_t *s; } sim_ctx_t;
static void sim_draw(void *p) { sim_ctx_t *c = p; screen_draw(c->id, c->d, c->s, true); }
static void sim_draw_part(void *p) { sim_ctx_t *c = p; screen_draw(c->id, c->d, c->s, false); }
static int wx, wy, ww, wh, wn;

void display_init(void) {}
void display_sleep(bool sleep) { (void)sleep; }

void display_raw_window(int x, int y, int w, int h)
{
    wx = x; wy = y; ww = w; wh = h; wn = 0;
}

void display_raw_pixel(uint16_t c)
{
    sim_writes++;
    if (wn >= ww * wh)
        return;
    int px = wx + wn % ww, py = wy + wn / ww;
    wn++;
    if (px >= 0 && px < DISPLAY_W && py >= 0 && py < DISPLAY_H)
        target[py * DISPLAY_W + px] = c;
}

void display_raw_end(void) {}

void display_raw_fill_rect(int x, int y, int w, int h, uint16_t c)
{
    if (w <= 0 || h <= 0)
        return;
    if (x < 0 || y < 0 || x + w > DISPLAY_W || y + h > DISPLAY_H)
        fprintf(stderr, "warning: rectangle %d,%d %dx%d leaves the screen\n", x, y, w, h);
    display_raw_window(x, y, w, h);
    for (int i = 0; i < w * h; i++)
        display_raw_pixel(c);
}

static void save(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    fprintf(f, "P6\n%d %d\n255\n", DISPLAY_W, DISPLAY_H);
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++) {
        uint16_t c = fb[i];
        unsigned char rgb[3] = {(unsigned char)(((c >> 11) & 31) * 255 / 31), (unsigned char)(((c >> 5) & 63) * 255 / 63),
                                (unsigned char)((c & 31) * 255 / 31)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

/* ---- made-up but plausible data ---- */

static ui_data_t data(void)
{
    ui_data_t d;
    memset(&d, 0, sizeof d);
    d.now = 1791970860u;
    d.tz_offset_s = 3 * 3600;
    d.rate_nsvh = 120;
    d.cpm = 10;
    d.bg_nsvh = 110;
    d.dose_today_nsv = 2900;
    d.dose_week_nsv = 19600;
    d.dose_total_nsv = 412000;
    static const uint32_t tr[TREND_N] = {100, 112, 97, 130, 108, 90, 122, 105, 125, 99, 94, 115, 128, 104, 118, 96, 110, 122,
                                         107, 101, 114, 131, 105, 94, 117, 124, 108, 100, 121, 112, 103, 97, 115, 108, 99, 120};
    memcpy(d.trend, tr, sizeof tr);
    d.trend_min = 80; d.trend_avg = 110; d.trend_max = 190; d.trend_change_pct = -8;
    static const uint32_t wk[7] = {2800, 3400, 3000, 5200, 4000, 3300, 3800};
    memcpy(d.week_nsv, wk, sizeof wk);
    d.th_nsvh[0] = 500; d.th_nsvh[1] = 10000; d.th_nsvh[2] = 100000;

    d.e_vm_x10 = 120; d.e_avg_x10 = 110; d.e_peak_x10 = 190;
    d.h_ut_x100 = 42; d.h_avg_x100 = 31; d.h_peak_x100 = 124;
    for (int i = 0; i < TREND_N; i++) {
        d.e_trend[i] = 100 + (uint32_t)((i * 37) % 60);
        d.h_trend[i] = (i > 6 && i < 12) ? 40 + (uint32_t)(i * 30) % 90 : 30 + (uint32_t)(i * 7) % 12;
    }

    d.bat_percent = 78; d.bat_minutes = 41 * 60; d.bat_health = 92; d.bat_cycles = 134; d.on_hours = 1284; d.tube_life = 97;
    d.days_stored = 38; d.storage_pct = 65; d.oldest_time = d.now - 38u * 86400u; d.profile = 2; d.alarm_count = 14;
    d.profile_days[0] = 17; d.profile_days[1] = 52; d.profile_days[2] = 188; d.profile_days[3] = 716; d.profile_days[4] = 1600;

    d.event_count = 4;
    static const uint32_t es[4] = {0, 2 * 86400u, 3 * 86400u, 5 * 86400u};
    static const uint32_t ep[4] = {42800, 900, 700, 600};
    static const uint32_t ed[4] = {180, 660, 360, 120};
    for (int i = 0; i < 4; i++) {
        d.events[i].start = d.now - es[i] - 3600u * (uint32_t)i;
        d.events[i].peak_nsvh = ep[i];
        d.events[i].duration_s = ed[i];
        d.events[i].level = i == 0 ? 2 : 1;
    }
    d.budget_active = true; d.budget_nsv = 10000; d.budget_used_nsv = 6400; d.budget_start_time = d.now - 4800; d.budget_avg_nsvh = 4800;
    d.sample_phase = 2; d.sample_bg_nsvh = 110; d.sample_nsvh = 190; d.sample_net_nsvh = 80; d.sample_elapsed_s = 372; d.sample_goal_s = 600;
    d.total_pulses = 2400000; d.bg_cpm = 10; d.deadtime_on = true; d.hv_factory = true;
    d.theme = 0; d.brightness = 4; d.sound = true; d.vibration = true;
    d.selftest_done = 6; d.selftest_total = 7;
    d.scan_bars = 6; d.scan_value_x10 = 380; d.scan_rising = true;
    d.usb = true; d.usb_connected = true; d.usb_synced = true; d.usb_streaming = true;
    d.blink = true;
    d.anim = 2;
    d.bat_state = 1;
    return d;
}

typedef struct {
    const char *name;
    screen_id_t id;
    ui_state_t st;
    int variant; /* 1: alarm data, 2: boot screen */
} shot_t;

/* changes a random part of the data, like a few seconds of live measurement would */
static void mutate(ui_data_t *d, bool keep_level)
{
#define MAYBE (rnd() % 100 < 35)
    if (MAYBE) d->rate_nsvh = 10 + rnd() % (rnd() % 2 ? 2000 : 900000);
    if (MAYBE) d->cpm = rnd() % 5000;
    if (MAYBE && !keep_level) d->level = (uint8_t)(rnd() % 4); /* a new alarm level starts a full redraw in the real app */
    if (MAYBE) d->bat_percent = (uint8_t)(rnd() % 101);
    if (MAYBE) d->bat_state = (uint8_t)(rnd() % 3);
    if (MAYBE) d->bat_minutes = rnd() % 6000;
    if (MAYBE) d->now += rnd() % 4000;
    if (MAYBE) d->e_vm_x10 = rnd() % 100000;
    if (MAYBE) d->h_ut_x100 = rnd() % 100000;
    if (MAYBE) d->e_avg_x10 = rnd() % 100000;
    if (MAYBE) d->h_peak_x100 = rnd() % 100000;
    for (int k = 0; k < TREND_N; k++) {
        if (rnd() % 4 == 0) d->trend[k] = rnd() % 3000;
        if (rnd() % 4 == 0) d->e_trend[k] = rnd() % 3000;
        if (rnd() % 4 == 0) d->h_trend[k] = rnd() % 3000;
    }
    if (MAYBE) { d->trend_min = rnd() % 900; d->trend_avg = rnd() % 9000; d->trend_max = rnd() % 90000; }
    if (MAYBE) d->dose_today_nsv = rnd() % 50000000u;
    if (MAYBE) d->dose_total_nsv = rnd() % 90000000u;
    if (MAYBE) d->dose_week_nsv = rnd() % 90000000u;
    if (MAYBE) d->bg_nsvh = rnd() % 3000;
    if (MAYBE) d->blink = rnd() & 1;
    if (MAYBE) d->anim = (uint8_t)rnd();
    if (MAYBE) d->sample_phase = (uint8_t)(rnd() % 4);
    if (MAYBE) { d->sample_bg_nsvh = rnd() % 9000; d->sample_nsvh = rnd() % 90000; d->sample_net_nsvh = (int32_t)(rnd() % 20000) - 10000; }
    if (MAYBE) d->sample_elapsed_s = rnd() % 700;
    if (MAYBE) d->selftest_done = (uint8_t)(rnd() % 8);
    if (MAYBE) { d->scan_bars = rnd() % 11; d->scan_value_x10 = rnd() % 100000; d->scan_rising = rnd() & 1; }
    if (MAYBE) { d->budget_used_nsv = rnd() % 12000; d->budget_avg_nsvh = rnd() % 90000; }
    if (MAYBE) { d->cw_phase = (uint8_t)(rnd() % 3); d->cw_ref_nsvh = 50 + rnd() % 9000000; d->cw_counts = rnd() % 3000; d->cw_seconds = rnd() % 4000; d->cw_unc_x10 = rnd() % 1000; d->cw_new_x100 = rnd() % 120000; d->cw_saturated = rnd() % 5 == 0; }
    if (MAYBE) { d->sess_active = rnd() & 1; d->sess_kind = (uint8_t)(rnd() % SESS_KINDS); d->sess_seconds = rnd() % 100000; d->sess_dose_nsv = rnd() % 90000000u; d->sess_max_nsvh = rnd() % 900000; }
    if (MAYBE) {
        d->sess_saved = (uint8_t)(rnd() % (SESS_SLOTS + 1));
        for (int k = 0; k < SESS_SLOTS; k++) {
            d->sess_list[k].kind = (uint8_t)(rnd() % SESS_KINDS);
            d->sess_list[k].start = d->now - rnd() % 1000000;
            d->sess_list[k].seconds = rnd() % 100000;
            d->sess_list[k].dose_nsv = rnd() % 90000000u;
            d->sess_list[k].max_nsvh = rnd() % 900000;
        }
    }
    if (MAYBE) d->usb_connected = rnd() & 1;
    if (MAYBE) { d->days_stored = rnd() % 4000; d->storage_pct = (uint8_t)(rnd() % 101); }
#undef MAYBE
}

int main(int argc, char **argv)
{
    layout_warn_hook = sim_layout_warn;
    const char *dir = argc > 1 ? argv[1] : ".";
    static const char *theme_names[THEME_COUNT] = {"dark", "light", "paper", "amber", "sakura", "night", "terminal", "ocean", "forest",
                                                   "contrast", "lavender", "mint", "sunset", "charcoal", "sand", "ice", "cherry", "violet",
                                                   "candy", "mono", "korea"};
    static const shot_t shots[] = {
        {"boot", SCR_MAIN, {0, 0, 0, 0}, 2},
        {"main", SCR_MAIN, {0, 0, 0, 0}, 0},
        {"trend", SCR_TREND, {0, 0, 0, 0}, 0},
        {"efield", SCR_EFIELD, {0, 0, 0, 0}, 0},
        {"hfield", SCR_HFIELD, {0, 0, 0, 0}, 0},
        {"battery", SCR_BATTERY, {0, 0, 0, 0}, 0},
        {"history", SCR_HISTORY, {0, 0, 0, 0}, 0},
        {"alarm", SCR_ALARM, {0, 0, 0, 0}, 1},
        {"mode", SCR_MODE, {1, 0, 0, 0}, 0},
        {"menu", SCR_MENU, {2, 0, 0, 0}, 0},
        {"guide_list", SCR_GUIDE_LIST, {0, 0, 0, 0}, 0},
        {"guide_page0", SCR_GUIDE_PAGE, {0, 0, 0, 0}, 0},
        {"guide_page5", SCR_GUIDE_PAGE, {0, 0, 5, 0}, 0},
        {"settings", SCR_SETTINGS, {0, 0, 0, 0}, 0},
        {"events", SCR_EVENTS, {0, 0, 0, 0}, 0},
        {"budget", SCR_BUDGET, {0, 0, 0, 0}, 0},
        {"sample", SCR_SAMPLE, {0, 0, 0, 0}, 0},
        {"log", SCR_LOG, {2, 0, 0, 0}, 0},
        {"storage", SCR_LOG, {0, 0, 0, 1}, 0},
        {"thresh", SCR_THRESH, {1, 0, 0, 0}, 0},
        {"tube", SCR_TUBE, {0, 0, 0, 0}, 0},
        {"selftest", SCR_SELFTEST, {0, 0, 0, 0}, 0},
        {"usb", SCR_USB, {0, 0, 0, 0}, 0},
        {"scan", SCR_SCAN, {0, 0, 0, 0}, 0},
        {"themes", SCR_THEMES, {7, 0, 0, 0}, 0},
        {"about", SCR_ABOUT, {0, 0, 0, 0}, 0},
        {"about_held", SCR_ABOUT, {0, 0, 0, 0}, 3},
        {"saver", SCR_SAVER, {0, 0, 0, 0}, 0},
        {"health", SCR_HEALTH, {0, 0, 0, 0}, 4},
        {"dt_intro", SCR_DEADTIME, {0, 0, 0, 0}, 6},
        {"dt_ready", SCR_DEADTIME, {0, 0, 0, 0}, 7},
        {"dt_count", SCR_DEADTIME, {0, 0, 0, 0}, 8},
        {"dt_result", SCR_DEADTIME, {0, 0, 0, 0}, 9},
        {"dt_none", SCR_DEADTIME, {0, 0, 0, 0}, 10},
        {"settings_end", SCR_SETTINGS, {13, 8, 0, 0}, 5},
        {"menu_end", SCR_MENU, {11, 6, 0, 0}, 0},
        {"charge", SCR_CHARGE, {0, 0, 0, 0}, 0},
    };
    char path[512];

    for (int t = 0; t < THEME_COUNT; t++) {
        ui_theme = &themes[t];
        for (unsigned i = 0; i < sizeof shots / sizeof *shots; i++) {
            ui_data_t d = data();
            d.theme = (uint8_t)t;
            if (shots[i].variant == 3)
                d.suspect = 0x20;                  /* stage 5 (USB) held back */
            if (shots[i].variant == 4) {                /* a device that had a bad day */
                d.h_boots = 12; d.h_wdt = 1; d.h_fault = 1; d.h_usb_restarts = 2; d.h_fault_pc = 0x08012AB4u;
                d.h_reason = 3; d.uptime_s = 7350; d.stage = 5;
            }
            if (shots[i].variant >= 6) {
                d.dt_phase = shots[i].variant == 6 ? 0 : shots[i].variant == 7 ? 1 : shots[i].variant == 8 ? 2 : 3;
                d.dt_step = shots[i].variant == 7 ? 2 : 3;
                d.dt_counts = 123456; d.dt_seconds = 87;
                d.dt_result_us = shots[i].variant == 10 ? 0 : 92; d.dt_unc_us = 4;
            }
            if (shots[i].variant == 5) {
                d.saver = true; d.station = false;
            }
            memset(fb, 0, sizeof fb);
            if (shots[i].variant == 2) {
                ui_boot_draw(ui_theme);
                ui_boot_progress(ui_theme, 7);
            } else {
                if (shots[i].variant == 1) {
                    d.rate_nsvh = 42800; d.level = 2;
                    static const uint32_t sp[TREND_N] = {100, 110, 100, 105, 120, 110, 100, 108, 115, 110, 100, 120, 140, 200, 340, 520, 700, 860,
                                                         950, 1000, 1000, 980, 1000, 1000, 970, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
                    for (int k = 0; k < TREND_N; k++)
                        d.trend[k] = sp[k] * 40u;
                }
                {
                    sim_ctx_t c = {shots[i].id, &d, &shots[i].st};
                    warn_on = d.theme == 0;
                    if (warn_on)
                        printf("-- %s\n", shots[i].name);
                    display_render(1, true, sim_draw, &c);
                    if (warn_on) { /* the same screen with the widest numbers it can show */
                        ui_data_t w = d;
                        w.days_stored = 3999; w.cpm = 99999; w.rate_nsvh = 888000; w.bat_minutes = 99999; w.dose_today_nsv = 98700000;
                        w.dose_week_nsv = 98700000; w.dose_total_nsv = 98700000; w.on_hours = 99999; w.total_pulses = 99999999;
                        w.e_vm_x10 = 19990; w.h_ut_x100 = 9999; w.sample_elapsed_s = 5999; w.cw_counts = 99999; w.cw_seconds = 99999;
                        w.sess_seconds = 99999 * 3; w.sess_dose_nsv = 98700000; w.scan_value_x10 = 19990;
                        for (int k = 0; k < SESS_SLOTS; k++) w.sess_list[k].dose_nsv = 98700000;
                        sim_ctx_t cw = {shots[i].id, &w, &shots[i].st};
                        display_render(1, true, sim_draw, &cw);
                    }
                    warn_on = false;
                    if (d.theme == 0) { /* what the change-only redraw leaves must equal a full draw of the same data */
                        ui_data_t m = d;
                        for (int it = 0; it < 60; it++) {
                            mutate(&m, shots[i].id == SCR_ALARM);
                            display_render(1, false, sim_draw_part, &(sim_ctx_t){shots[i].id, &m, &shots[i].st});
                            target = ref_fb;
                            memset(ref_fb, 0, sizeof ref_fb);
                            screen_draw(shots[i].id, &m, &shots[i].st, true);
                            target = fb;
                            if (memcmp(fb, ref_fb, sizeof fb) != 0) {
                                long bad = 0;
                                int x0 = 999, y0 = 999, x1 = -1, y1 = -1;
                                for (int q = 0; q < DISPLAY_W * DISPLAY_H; q++)
                                    if (fb[q] != ref_fb[q]) {
                                        if (bad < 6) printf("   pixel %d,%d: panel %04x, full draw %04x\n", q % DISPLAY_W, q / DISPLAY_W, fb[q], ref_fb[q]);
                                        bad++;
                                        int qx = q % DISPLAY_W, qy = q / DISPLAY_W;
                                        if (qx < x0) x0 = qx;
                                        if (qx > x1) x1 = qx;
                                        if (qy < y0) y0 = qy;
                                        if (qy > y1) y1 = qy;
                                    }
                                printf("   differing area x %d..%d, y %d..%d\n", x0, x1, y0, y1);
                                printf("DIFFERS: %s, change %d: %ld pixels are not what a full draw gives\n", shots[i].name, it, bad);
                                break;
                            }
                        }
                        display_render(1, true, sim_draw, &c); /* leave the full frame in place for the saved picture */
                    }
                    if (d.theme == 0) { /* the same frame again must write nothing */
                        display_render(1, false, sim_draw_part, &c);
                        long before = sim_writes;
                        display_render(1, false, sim_draw_part, &c);
                        display_render(1, false, sim_draw_part, &c);
                        if (sim_writes != before)
                            printf("NOT STILL: %s rewrote %ld pixels for an unchanged frame\n", shots[i].name, sim_writes - before);
                    }
                }
            }
            snprintf(path, sizeof path, "%s/%s_%s.ppm", dir, theme_names[t], shots[i].name);
            save(path);
        }
    }
    printf("most drawing operations in one frame: %d\n", display_render_peak());
    return 0;
}
