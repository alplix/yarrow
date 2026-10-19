/* Application tests: the whole app against a fake device (time, tube, battery, keys, two RAM flash windows). */
#include "app.h"
#include "display.h"
#include "fmt.h"
#include "proto.h"
#include "theme.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- stub display: counts drawing and catches rectangles outside the screen ---- */
static long draw_calls, bad_rects, raw_px;
void display_init(void) {}
void display_sleep(bool sleep) { (void)sleep; }
void display_raw_window(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void display_raw_pixel(uint16_t c) { (void)c; raw_px++; }
void display_raw_end(void) {}
void display_raw_fill_rect(int x, int y, int w, int h, uint16_t c)
{
    (void)c;
    draw_calls++;
    if (w > 0 && h > 0)
        raw_px += (long)w * h;
    if (w > 0 && h > 0 && (x < 0 || y < 0 || x + w > DISPLAY_W || y + h > DISPLAY_H))
        bad_rects++;
}

/* ---- fake device ---- */
static uint64_t rs = 0x1234ABCDull;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (uint32_t)(rs >> 11); }
static double uni(void) { return ((rnd() & 0xFFFFFF) + 0.5) / 16777216.0; }
static uint32_t poisson(double lam)
{
    if (lam > 500) { double g = sqrt(-2 * log(uni())) * cos(6.283185307 * uni()); double v = lam + sqrt(lam) * g; return v < 0 ? 0 : (uint32_t)(v + 0.5); }
    double L = exp(-lam), p = 1.0; uint32_t k = 0;
    do { k++; p *= uni(); } while (p > L);
    return k - 1;
}

#define LOG_PAGES 16u
#define PSIZE 1024u
#define KVPSIZE KV_MAX_PAGE
static uint8_t log_mem[LOG_PAGES * PSIZE], kv_mem[2 * KVPSIZE];
static long kv_erases;

static int mem_read(const uint8_t *m, size_t size, uint32_t a, void *b, uint32_t n) { if (a + n > size) return -1; memcpy(b, m + a, n); return 0; }
static int mem_prog(uint8_t *m, size_t size, uint32_t a, const void *b, uint32_t n)
{
    if ((a & 1) || (n & 1) || a + n > size) return -1;
    const uint8_t *s = b;
    for (uint32_t i = 0; i < n; i++) { if (m[a + i] != 0xFF && m[a + i] != s[i]) { if ((m[a + i] & s[i]) != s[i]) return -1; } m[a + i] &= s[i]; }
    return 0;
}
static int log_read(void *c, uint32_t a, void *b, uint32_t n) { (void)c; return mem_read(log_mem, sizeof log_mem, a, b, n); }
static int log_prog(void *c, uint32_t a, const void *b, uint32_t n) { (void)c; return mem_prog(log_mem, sizeof log_mem, a, b, n); }
static int log_erase(void *c, uint32_t p) { (void)c; memset(log_mem + p * PSIZE, 0xFF, PSIZE); return 0; }
static int kv_read(void *c, uint32_t a, void *b, uint32_t n) { (void)c; return mem_read(kv_mem, sizeof kv_mem, a, b, n); }
static int kv_prog(void *c, uint32_t a, const void *b, uint32_t n) { (void)c; return mem_prog(kv_mem, sizeof kv_mem, a, b, n); }
static int kv_erase(void *c, uint32_t p) { (void)c; kv_erases++; memset(kv_mem + p * KVPSIZE, 0xFF, KVPSIZE); return 0; }
static const ylog_flash_t log_dev = {PSIZE, LOG_PAGES, log_read, log_prog, log_erase, 0};
static const ylog_flash_t kv_dev = {KVPSIZE, 2, kv_read, kv_prog, kv_erase, 0};

static uint32_t t_now = 1791970860u;          /* unix seconds */
static double t_cps;                          /* tube rate */
static uint32_t t_pulses;
static uint32_t t_mv = 4000;
static bool t_usb, t_charging;
static uint32_t t_e = 120, t_h = 42;
static bool t_hv_valid = true, t_hv_started;
static uint32_t beeps, last_beep_hz, vib_calls, power_offs;
static bool display_enabled = true;
static uint8_t backlight_level;
static input_event_t queue[64];
static int qh, qt;

static void push_ev(input_event_t e) { queue[qt++ % 64] = e; }
static input_event_t pop_ev(void) { return qh == qt ? EV_NONE : queue[qh++ % 64]; }

static uint32_t h_rtc(void) { return t_now; }
static void h_rtc_set(uint32_t v) { t_now = v; }
static uint32_t h_pulses(void) { return t_pulses; }
static uint32_t h_mv(void) { return t_mv; }
static bool h_usb(void) { return t_usb; }
static bool h_chg(void) { return t_charging; }
static uint32_t h_e(void) { return t_e; }
static uint32_t h_h(void) { return t_h; }
static bool h_hvv(void) { return t_hv_valid; }
static bool h_hvs(void) { t_hv_started = true; return true; }
static void h_hvx(void) { t_hv_started = false; }
static void h_beep(uint32_t hz) { if (hz) { beeps++; last_beep_hz = hz; } else last_beep_hz = 0; }
static void h_vib(bool on) { if (on) vib_calls++; }
static void h_led(bool on) { (void)on; }
static void h_bl(uint8_t l) { backlight_level = l; }
static void h_disp(bool on) { display_enabled = on; }
static void h_off(void) { power_offs++; }
static void h_wd(void) {}
static bool h_usbc(void) { return false; }

static char ser_in[512], ser_out[16384];
static int ser_in_n, ser_in_p, ser_out_n;
static int h_sr(uint8_t *b, uint32_t max)
{
    int n = 0;
    while (n < (int)max && ser_in_p < ser_in_n)
        b[n++] = (uint8_t)ser_in[ser_in_p++];
    return n;
}
static int h_sw(const uint8_t *b, uint32_t n)
{
    for (uint32_t i = 0; i < n && ser_out_n < (int)sizeof ser_out - 1; i++)
        ser_out[ser_out_n++] = (char)b[i];
    ser_out[ser_out_n] = 0;
    return (int)n;
}
static void h_usbstart(void) {}
static bool led_status, led_pulse = true;
static void h_statusled(bool on) { led_status = on; }
static void h_pulseled(bool on) { led_pulse = on; }
static bool count_click, count_vibe;
static void h_cclick(bool on) { count_click = on; }
static void h_cvibe(bool on) { count_vibe = on; }
static bool boot_usb, key_down;
static bool h_bootusb(void) { return boot_usb; }
static bool h_keydown(void) { return key_down; }
static uint8_t led_mask_now = 2;
static void h_ledcolor(uint8_t m) { led_mask_now = m; }

static void h_chipid(uint32_t id[3]) { id[0] = 0x11223344u; id[1] = 0x55667788u; id[2] = 0x99AABBCCu; }

static hal_t hal = {h_rtc, h_rtc_set, pop_ev, h_pulses, h_mv, h_usb, h_chg, h_e, h_h, h_hvv, h_hvs, h_hvx, h_beep, h_vib, h_led,
                    h_bl, h_disp, h_off, h_wd, &log_dev, &kv_dev, h_usbc, h_sr, h_sw, h_usbstart, h_statusled, h_pulseled, h_cclick, h_cvibe, h_bootusb, h_keydown, h_ledcolor, h_chipid};

static app_t app;
static uint32_t now_ms;

/* advance simulated time by `ms`, polling every 20 ms; the tube produces pulses at t_cps */
static double carry;
static void run_ms(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 20) {
        now_ms += 20;
        carry += 0.02;
        if (carry >= 1.0) { /* once per simulated second */
            carry -= 1.0;
            t_now++;
            if (t_hv_started)
                t_pulses += poisson(t_cps);
        }
        app_poll(&app, now_ms);
    }
}
static void run_s(uint32_t s) { run_ms(s * 1000); }

static void send(input_event_t e) { push_ev(e); run_ms(40); }

static void reboot(void) { app_init(&app, &hal, now_ms); }

static void fresh(void)
{
    memset(log_mem, 0xFF, sizeof log_mem);
    memset(kv_mem, 0xFF, sizeof kv_mem);
    t_cps = 0.17; t_pulses = 0; t_hv_started = false; t_mv = 4000; t_usb = false; qh = qt = 0;
    beeps = vib_calls = power_offs = 0; carry = 0;
    display_enabled = true;
    now_ms = 100000;
    reboot();
}

static void to_stage5(void) { run_s(60); }

/* ---- tests ---- */

static void test_ladder(void)
{
    fresh();
    CHECK(app.stage == 1, "first boot should start at stage 1, got %d", app.stage);
    run_s(11);
    CHECK(app.stage == 2, "stage after 11 s: %d", app.stage);
    run_s(12);
    CHECK(app.stage == 3, "stage after 23 s: %d", app.stage);
    run_s(40);
    CHECK(app.stage == 5 && app.verified >= 4, "stage after 63 s: %d (verified %d)", app.stage, app.verified);
    CHECK(t_hv_started, "tube high voltage should be running at stage 5");

    reboot();
    CHECK(app.stage == 5 || app.stage == app.verified + 1 || app.stage == app.verified, "boot after success: stage %d", app.stage);
    uint8_t verified_before = app.verified;

    /* a stage that never became stable (power cut or crash during stage v+1) is remembered and not retried */
    fresh();
    run_s(25);                    /* stage 3 started, not yet verified */
    int tried = app.stage;
    uint8_t v = app.verified;
    reboot();                     /* "crash": no shutdown, start again from the same flash */
    CHECK(app.verified == v, "verified stage changed by a crash: %d -> %d", v, app.verified);
    CHECK(app.stage <= v + 0 || app.stage == v, "after a failed stage %d the app must run only verified features, stage %d", tried, app.stage);
    run_s(90);
    CHECK(app.stage <= v, "suspect stage was tried again by itself (stage %d, verified before %d)", app.stage, v);
    CHECK(app.suspect_mask != 0, "suspect mask not recorded");

    /* About -> OK clears the suspicion and the next boot tries again */
    app.sp = 0;
    app.stack[1] = SCR_ABOUT;
    app.sp = 1;
    send(EV_OK);
    reboot();
    run_s(60);
    CHECK(app.stage > v, "stage did not advance after clearing the suspicion (%d)", app.stage);
    (void)verified_before;
    printf("ladder: ok\n");
}

static void test_measurement_and_log(void)
{
    fresh();
    to_stage5();
    t_cps = 10 / 60.0 * 0.96 * 1.0; /* about 0.1 uSv/h */
    run_s(1200);
    uint32_t rate = app.ui.rate_nsvh;
    printf("measurement: rate %u nSv/h, cpm %u\n", (unsigned)rate, (unsigned)app.ui.cpm);
    CHECK(rate > 40 && rate < 250, "background rate out of range: %u", (unsigned)rate);
    CHECK(app.ui.level == 0, "alarm at background");
    CHECK(app.bat.state == BAT_DISCHARGING, "battery state");

    /* log: default profile is 1 minute; after 20 minutes there must be about 19 samples */
    ylog_stats_t st;
    static uint8_t scratch[1024];
    run_s(120);
    ylog_stats(&log_dev, scratch, &st);
    printf("log: %u bytes used, oldest %u newest %u\n", (unsigned)st.bytes_used, (unsigned)st.oldest_time, (unsigned)st.newest_time);
    CHECK(st.pages_used >= 1, "nothing was logged");
    CHECK(st.newest_time > st.oldest_time || st.newest_time == st.oldest_time, "log times");
    CHECK(bad_rects == 0, "%ld rectangles outside the screen", bad_rects);
}

static void test_alarm_and_events(void)
{
    fresh();
    to_stage5();
    t_cps = 10 / 60.0;
    run_s(60);
    beeps = 0;
    t_cps = 57; /* about 42 uSv/h */
    run_s(6);
    printf("alarm: level %d, screen %d, beeps %u\n", app.alarm.level, app_screen(&app), (unsigned)beeps);
    CHECK(app.alarm.level >= 2, "danger level not reached (level %d, rate %u)", app.alarm.level, (unsigned)app.ui.rate_nsvh);
    CHECK(app_screen(&app) == SCR_ALARM, "alarm screen not shown");
    CHECK(beeps > 0, "no alarm sound");
    send(EV_BACK);
    uint32_t b = beeps;
    run_s(3);
    CHECK(beeps == b, "alarm still sounding after mute");
    t_cps = 10 / 60.0;
    int clear_after = 0;
    while (app.alarm.level != 0 && clear_after < 300) {
        run_s(1);
        clear_after++;
    }
    printf("alarm cleared %d s after the source was removed\n", clear_after);
    CHECK(app.alarm.level == 0 && clear_after < 120, "alarm did not clear in time (level %d, rate %u, %d s)", app.alarm.level, (unsigned)app.ui.rate_nsvh, clear_after);
    CHECK(app_screen(&app) != SCR_ALARM, "alarm screen stuck");
    CHECK(app.ui.event_count == 1, "event count %d", app.ui.event_count);
    CHECK(app.ui.events[0].peak_nsvh > 10000, "event peak %u", (unsigned)app.ui.events[0].peak_nsvh);

    reboot();
    CHECK(app.ui.event_count == 1 && app.ui.events[0].peak_nsvh > 10000, "event lost across reboot");
    printf("events: ok\n");
}

/* A page that shows nothing new must not be redrawn: sensors keep wobbling, the screen must stay still. */
static void test_stillness(void)
{
    fresh();
    to_stage5();
    send(EV_OK); /* the menu */
    CHECK(app_screen(&app) == SCR_MENU, "menu open: %d", app_screen(&app));
    run_s(3);
    long before = raw_px;
    for (int i = 0; i < 20; i++) {
        t_e = 100 + (uint32_t)(i * 37) % 90; /* the electric field reading changes every second */
        t_h = 4000 + (uint32_t)(i * 53) % 300;
        run_s(1);
    }
    long still = raw_px - before;
    CHECK(still < 20000, "a static menu wrote %ld pixels in 20 s", still);
    printf("stillness: menu wrote %ld pixels in 20 s\n", still);

    send(EV_BACK);
    CHECK(app_screen(&app) == SCR_MAIN, "back to main: %d", app_screen(&app));
    run_s(5);
    before = raw_px;
    run_s(20);
    printf("stillness: live main screen wrote %ld pixels per second (a full screen is %d)\n", (raw_px - before) / 20,
           DISPLAY_W * DISPLAY_H);

    /* every menu entry and its screen, left alone */
    for (int entry = 0; entry < 16; entry++) {
        fresh();
        to_stage5();
        send(EV_OK);
        CHECK(app_screen(&app) == SCR_MENU, "menu open for entry %d: %d", entry, app_screen(&app));
        for (int i = 0; i < entry; i++)
            send(EV_RIGHT);
        run_s(3);
        before = raw_px;
        long per_s[10];
        for (int k = 0; k < 10; k++) {
            long b0 = raw_px;
            run_s(1);
            per_s[k] = raw_px - b0;
        }
        long menu_px = (raw_px - before) / 10;
        if (menu_px)
            for (int k = 0; k < 10; k++)
                printf("   second %d: %ld px\n", k, per_s[k]);
        send(EV_OK);
        screen_id_t id = app_screen(&app);
        run_s(4);
        before = raw_px;
        run_s(10);
        printf("stillness: menu entry %2d wrote %5ld px/s while selected, its screen %2d wrote %6ld px/s\n", entry, menu_px,
               (int)id, (raw_px - before) / 10);
        CHECK(menu_px < 1500, "the menu keeps redrawing with entry %d selected (%ld px/s)", entry, menu_px);
    }
}

/* The calibration wizard must find the sensitivity the simulated tube really has, and keep it across a restart. */
static void test_calibration_wizard(void)
{
    fresh();
    to_stage5();
    send(EV_OK);
    for (int i = 0; i < 14; i++)
        send(EV_RIGHT);
    send(EV_OK);
    CHECK(app_screen(&app) == SCR_CALWIZ, "wizard screen %d", app_screen(&app));
    CHECK(app.ui.cw_phase == 0, "wizard starts at the reference");
    send(EV_RIGHT);
    CHECK(app.ui.cw_ref_nsvh > 50, "the reference value changes with the knob");
    app.ui.cw_ref_nsvh = 5000; /* a 5 uSv/h source */
    const double true_sens = 70.0;
    t_cps = true_sens * 5.0 / 60.0;
    send(EV_OK);
    CHECK(app.ui.cw_phase == 1, "measuring");
    send(EV_OK); /* too early: nothing counted yet */
    CHECK(app.ui.cw_phase == 1, "finishing with no counts must not work");
    run_s(600);
    CHECK(app.ui.cw_counts > 3000 && app.ui.cw_seconds >= 599, "counts %u in %u s", (unsigned)app.ui.cw_counts, (unsigned)app.ui.cw_seconds);
    send(EV_OK);
    CHECK(app.ui.cw_phase == 2, "result");
    double got = app.ui.cw_new_x100 / 100.0;
    double unc = app.ui.cw_unc_x10 / 10.0;
    printf("wizard: found %.2f (true %.1f), stated uncertainty %.1f %%\n", got, true_sens, unc);
    CHECK(fabs(got - true_sens) < 4.0 * unc / 100.0 * true_sens, "sensitivity %.2f is far from %.1f", got, true_sens);
    uint32_t found = app.ui.cw_new_x100;
    send(EV_OK); /* apply */
    CHECK(app_screen(&app) == SCR_MENU, "back in the menu after applying: %d", app_screen(&app));
    CHECK(app.ui.cal[0] == found && app.meas.cfg.cpm_per_usvh_x100 == found, "applied value %u", (unsigned)app.ui.cal[0]);
    reboot();
    CHECK(app.meas.cfg.cpm_per_usvh_x100 == found, "calibration lost after restart");

    /* cancelling changes nothing */
    to_stage5();
    send(EV_OK);
    for (int i = 0; i < 14; i++)
        send(EV_RIGHT);
    send(EV_OK);
    send(EV_OK);
    run_s(30);
    send(EV_BACK);
    CHECK(app.ui.cw_phase == 0 && app.ui.cal[0] == found, "cancel must keep the old value");
    send(EV_BACK);
    printf("calibration wizard: ok\n");
}

/* Sessions: start, run, finish, list, restart. */
static int st_mode(void) { return app.states[app.sp].mode; }

static void fresh_menu_to_sessions(void)
{
    while (app_screen(&app) != SCR_MAIN)
        send(EV_BACK);
    send(EV_OK);
    for (int i = 0; i < 3; i++)
        send(EV_RIGHT);
    send(EV_OK);
}

static void test_sessions(void)
{
    fresh();
    to_stage5();
    t_cps = 0.4; /* 24 cpm: 0.3 uSv/h, below the first alarm level */
    send(EV_OK);
    for (int i = 0; i < 3; i++)
        send(EV_RIGHT);
    send(EV_OK);
    CHECK(app_screen(&app) == SCR_SESSION && st_mode() == 0, "sessions list");
    send(EV_OK); /* the first row starts a new one */
    CHECK(st_mode() == 1, "kind picker");
    send(EV_RIGHT);
    send(EV_OK);
    CHECK(st_mode() == 2 && app.ui.sess_active && app.ui.sess_kind == 1, "session running, kind %d", app.ui.sess_kind);
    run_s(120);
    CHECK(app.ui.sess_seconds >= 119 && app.ui.sess_seconds <= 122, "session seconds %u", (unsigned)app.ui.sess_seconds);
    double expect = 300.0 * 120 / 3600.0; /* 24 cpm at 80 cpm per uSv/h is 0.3 uSv/h */
    CHECK(fabs((double)app.ui.sess_dose_nsv - expect) < expect * 0.5 + 6, "session dose %llu nSv, expected about %.0f",
          (unsigned long long)app.ui.sess_dose_nsv, expect);
    CHECK(app.ui.sess_max_nsvh > 0, "peak rate");
    send(EV_LEFT); /* the screen went dark: the first key only wakes it */
    send(EV_OK);
    CHECK(app.ui.sess_active, "the first OK only asks");
    send(EV_OK);
    CHECK(!app.ui.sess_active && app.ui.sess_saved == 1 && app.ui.sess_list[0].kind == 1, "finished session listed: active %d saved %d kind %d screen %d mode %d", app.ui.sess_active, app.ui.sess_saved, app.ui.sess_list[0].kind, app_screen(&app), st_mode());
    uint32_t secs = app.ui.sess_list[0].seconds, dose = app.ui.sess_list[0].dose_nsv;

    reboot();
    CHECK(app.ui.sess_saved == 1 && app.ui.sess_list[0].seconds == secs && app.ui.sess_list[0].dose_nsv == dose,
          "saved session lost after restart");

    /* a running session survives a clean power-off and carries on */
    to_stage5();
    send(EV_OK);
    for (int i = 0; i < 3; i++)
        send(EV_RIGHT);
    send(EV_OK);
    send(EV_OK);
    send(EV_OK); /* kind 0 */
    run_s(45);
    CHECK(app.ui.sess_active && app.ui.sess_seconds >= 44, "second session started: active %d, %u s, screen %d mode %d", app.ui.sess_active, (unsigned)app.ui.sess_seconds, app_screen(&app), st_mode());
    app_shutdown(&app);
    reboot();
    CHECK(app.ui.sess_active && app.ui.sess_seconds >= 44 && app.ui.sess_seconds <= 47, "running session after restart: active %d, %u s",
          app.ui.sess_active, (unsigned)app.ui.sess_seconds);
    to_stage5();
    run_s(10);
    CHECK(app.ui.sess_seconds >= 54, "the session carries on after the restart (%u s)", (unsigned)app.ui.sess_seconds);

    /* more than eight sessions: the list keeps the newest eight */
    send(EV_BACK); /* finish the running one first: the picker needs none running */
    app.ui.sess_active = false;
    for (int k = 0; k < 10; k++) {
        fresh_menu_to_sessions();
        send(EV_OK);                /* new session */
        for (int j = 0; j < k % SESS_KINDS; j++)
            send(EV_RIGHT);
        send(EV_OK);                /* start */
        run_s(12);
        send(EV_OK);
        send(EV_OK);                /* finish */
        send(EV_BACK);
        send(EV_BACK);
    }
    CHECK(app.ui.sess_saved == SESS_SLOTS, "list is full: %d", app.ui.sess_saved);
    reboot();
    CHECK(app.ui.sess_saved == SESS_SLOTS, "full list after restart: %d", app.ui.sess_saved);
    printf("sessions: ok\n");
}

/* The settings store must not wear its flash out: count sector erases over a simulated day. */
static void test_flash_wear(void)
{
    fresh();
    to_stage5();
    t_cps = 0.4;
    kv_erases = 0;
    run_s(24 * 3600);
    printf("flash wear: %ld settings sector erases in 24 hours\n", kv_erases);
    CHECK(kv_erases <= 4, "%ld sector erases in a day would wear the flash out", kv_erases);
}

/* The charge LED follows the charger, and both LEDs can be switched off in Settings and stay off after a restart. */
static void test_leds(void)
{
    fresh();
    to_stage5();
    CHECK(led_pulse && !led_status, "start: count LED on, charge LED off");
    t_usb = true; t_charging = true;
    run_s(3);
    CHECK(led_status, "charging must light the charge LED");
    t_charging = false; /* full */
    run_s(3);
    CHECK(!led_status, "a full battery switches the charge LED off");
    t_charging = true;
    run_s(3);
    CHECK(led_status, "charging again");
    t_usb = false; t_charging = false;
    run_s(3);
    CHECK(!led_status, "unplugged");

    /* settings rows 4 and 5 */
    while (app_screen(&app) != SCR_MAIN)
        send(EV_BACK);
    send(EV_OK);
    for (int i = 0; i < 12; i++)
        send(EV_RIGHT);
    send(EV_OK); /* settings */
    for (int i = 0; i < 6; i++)
        send(EV_RIGHT);
    send(EV_OK); /* count LED off */
    CHECK(!app.settings.led_pulse && !led_pulse, "count LED switched off");
    send(EV_RIGHT);
    send(EV_OK); /* charge LED off */
    CHECK(!app.settings.led_charge, "charge LED switched off");
    t_usb = true; t_charging = true;
    run_s(3);
    CHECK(!led_status, "charging with the charge LED switched off");
    reboot();
    CHECK(!app.settings.led_pulse && !app.settings.led_charge && !led_pulse, "LED settings lost after restart");
    /* click and count vibration, off by default */
    CHECK(!count_click && !count_vibe, "click and count vibration start off");
    run_s(2); /* the first application second after a restart may bring up the charge screen: let it come first */
    while (app_screen(&app) != SCR_MAIN)
        send(EV_BACK);
    send(EV_OK);
    for (int i = 0; i < 12; i++)
        send(EV_RIGHT);
    send(EV_OK); /* settings */
    for (int i = 0; i < 4; i++)
        send(EV_RIGHT);
    send(EV_OK); /* count click on */
    send(EV_RIGHT);
    send(EV_OK); /* count vibration on */
    CHECK(app.settings.click && count_click && app.settings.count_vibe && count_vibe, "click and count vibration switched on");
    reboot();
    CHECK(app.settings.click && app.settings.count_vibe && count_click && count_vibe, "click settings lost after restart");
    /* the LED colour: the theme decides unless a colour is chosen */
    fresh();
    to_stage5();
    CHECK(app.settings.led_color == 0, "LED colour starts on the theme");
    int before_mask = led_mask_now;
    app_set_theme(&app, THEME_AMBER);
    CHECK(led_mask_now == 3, "amber theme gives a yellow LED, got %d", led_mask_now);
    app_set_theme(&app, THEME_DARK);
    CHECK(led_mask_now == 6 || led_mask_now == 4, "the dark theme (sky blue) gives a blue or cyan LED, got %d (was %d)", led_mask_now, before_mask);
    while (app_screen(&app) != SCR_MAIN)
        send(EV_BACK);
    send(EV_OK);
    for (int i = 0; i < 12; i++)
        send(EV_RIGHT);
    send(EV_OK); /* settings */
    for (int i = 0; i < 8; i++)
        send(EV_RIGHT);
    send(EV_OK); /* colour: red */
    CHECK(app.settings.led_color == 1 && led_mask_now == 1, "chosen colour red, got mask %d", led_mask_now);
    send(EV_OK); /* green */
    CHECK(led_mask_now == 2, "next colour green");
    app_set_theme(&app, THEME_AMBER);
    CHECK(led_mask_now == 2, "a chosen colour ignores the theme");
    reboot();
    CHECK(app.settings.led_color == 2 && led_mask_now == 2, "LED colour lost after restart");
    printf("leds: ok\n");
    t_usb = false; t_charging = false;
}

/* Start on USB power: the setting decides whether a cable alone switches Yarrow on. */
static void test_usb_start(void)
{
    fresh();
    CHECK(app.settings.usb_start, "starting on USB power is on by default");
    CHECK(app_boot_allowed(&app), "a normal start is allowed");
    boot_usb = true;
    CHECK(app_boot_allowed(&app), "started by the cable with the setting on: allowed");
    while (app_screen(&app) != SCR_MAIN)
        send(EV_BACK);
    to_stage5();
    send(EV_OK);
    for (int i = 0; i < 12; i++)
        send(EV_RIGHT);
    send(EV_OK); /* settings */
    for (int i = 0; i < 9; i++)
        send(EV_RIGHT);
    send(EV_OK); /* start on USB power: off */
    CHECK(!app.settings.usb_start, "setting switched off");
    reboot();
    CHECK(!app.settings.usb_start, "the setting is kept");
    CHECK(!app_boot_allowed(&app), "started by the cable with the setting off: not allowed");
    boot_usb = false;
    CHECK(app_boot_allowed(&app), "started by the power key: always allowed");
    printf("usb start: ok\n");
}

static void test_navigation_and_settings(void)
{
    fresh();
    to_stage5();
    CHECK(app_screen(&app) == SCR_MAIN, "start screen");
    send(EV_RIGHT);
    CHECK(app_screen(&app) == SCR_TREND, "right once: %d", app_screen(&app));
    for (int i = 0; i < 6; i++)
        send(EV_RIGHT);
    CHECK(app_screen(&app) == SCR_TREND, "six steps around the home pages should return to trend");
    send(EV_LEFT);
    CHECK(app_screen(&app) == SCR_MAIN, "left");

    send(EV_OK);
    CHECK(app_screen(&app) == SCR_MENU, "ok opens the menu");
    for (int i = 0; i < 12; i++)
        send(EV_RIGHT);
    send(EV_OK);
    CHECK(app_screen(&app) == SCR_SETTINGS, "menu entry 12 is settings: %d", app_screen(&app));
    send(EV_OK); /* theme picker */
    CHECK(app_screen(&app) == SCR_THEMES, "settings -> themes");
    for (int i = 0; i < 4; i++)
        send(EV_RIGHT);
    CHECK(app.settings.theme == 4, "theme preview %d", app.settings.theme);
    send(EV_BACK);
    CHECK(app.settings.theme == THEME_DARK, "cancelling the picker must restore the theme");
    send(EV_OK);
    for (int i = 0; i < 3; i++)
        send(EV_RIGHT);
    send(EV_OK);
    CHECK(app.settings.theme == 3 && app_screen(&app) == SCR_SETTINGS, "confirmed theme %d", app.settings.theme);

    /* toggle sound, reboot, setting stays */
    send(EV_RIGHT); send(EV_RIGHT); /* sound row */
    send(EV_OK);
    CHECK(!app.settings.sound, "sound toggle");
    reboot();
    CHECK(app.settings.theme == 3 && !app.settings.sound, "settings not persisted");

    /* walk through every menu entry to make sure each screen draws and goes back */
    fresh();
    to_stage5();
    long before = draw_calls;
    for (int entry = 0; entry < 16; entry++) {
        send(EV_OK);
        for (int i = 0; i < entry; i++)
            send(EV_RIGHT);
        send(EV_OK);
        run_s(3);
        send(EV_BACK);
        send(EV_BACK);
        run_ms(100);
    }
    CHECK(draw_calls > before + 100, "screens did not draw");
    CHECK(bad_rects == 0, "%ld rectangles outside the screen", bad_rects);
    CHECK(app_screen(&app) == SCR_MAIN, "did not come back to the main screen (%d)", app_screen(&app));
    printf("navigation: ok\n");
}

static void test_threshold_edit_and_budget(void)
{
    fresh();
    to_stage5();
    send(EV_OK);
    for (int i = 0; i < 7; i++)
        send(EV_RIGHT);
    send(EV_OK);
    CHECK(app_screen(&app) == SCR_THRESH, "thresholds screen");
    send(EV_OK); /* edit attention */
    for (int i = 0; i < 4; i++)
        send(EV_RIGHT);
    send(EV_OK);
    CHECK(app.th[0] > 500 && app.th[0] < app.th[1], "threshold after edit %u", (unsigned)app.th[0]);
    uint32_t t0 = app.th[0];
    reboot();
    CHECK(app.th[0] == t0, "threshold not persisted");

    /* budget */
    to_stage5();
    app.sp = 0;
    app.stack[1] = SCR_BUDGET;
    app.sp = 1;
    send(EV_OK);
    send(EV_OK);
    CHECK(app.alarm.budget_active && app.alarm.budget_nsv == 10000, "budget not started (%u)", (unsigned)app.alarm.budget_nsv);
    printf("thresholds and budget: ok\n");
}

static void test_clock_and_calibration(void)
{
    fresh();
    to_stage5();
    /* settings -> clock editor: set 2027-03-05 07:20 local (UTC+3) */
    send(EV_OK);
    for (int i = 0; i < 12; i++)
        send(EV_RIGHT);
    send(EV_OK); /* settings */
    for (int i = 0; i < 11; i++)
        send(EV_RIGHT);
    send(EV_OK); /* clock row */
    CHECK(app_screen(&app) == SCR_CLOCK, "clock editor screen (%d)", app_screen(&app));
    /* day */
    app.ui.clk[0] = 2026; app.ui.clk[1] = 12; app.ui.clk[2] = 31; app.ui.clk[3] = 23; app.ui.clk[4] = 59;
    send(EV_RIGHT);  /* day 32 -> must wrap to 1 */
    CHECK(app.ui.clk[2] == 1, "day wrapped to %u", app.ui.clk[2]);
    send(EV_OK);     /* month */
    send(EV_RIGHT);  /* 13 -> 1 */
    CHECK(app.ui.clk[1] == 1, "month wrapped to %u", app.ui.clk[1]);
    send(EV_OK);     /* year */
    send(EV_RIGHT);  /* 2027 */
    send(EV_OK);     /* hour */
    for (int i = 0; i < 8; i++)
        send(EV_RIGHT); /* 23 -> 7 */
    send(EV_OK);     /* minute */
    for (int i = 0; i < 21; i++)
        send(EV_RIGHT); /* 59 -> 20 */
    send(EV_OK);     /* save */
    uint32_t want = time_from_civil(2027, 1, 1, 7, 20) - 3 * 3600;
    CHECK(app_screen(&app) == SCR_SETTINGS, "back to settings after saving (%d)", app_screen(&app));
    CHECK(t_now >= want && t_now < want + 5, "clock set to %u, wanted %u", (unsigned)t_now, (unsigned)want);

    /* calibration: raise the sensitivity a few steps and keep it across a restart */
    send(EV_BACK);
    send(EV_BACK);
    send(EV_OK);
    for (int i = 0; i < 13; i++)
        send(EV_RIGHT);
    send(EV_OK);
    CHECK(app_screen(&app) == SCR_CALIB, "calibration screen (%d)", app_screen(&app));
    send(EV_OK); /* edit sensitivity */
    for (int i = 0; i < 5; i++)
        send(EV_RIGHT);
    send(EV_OK);
    CHECK(app.ui.cal[0] > 8000 && app.meas.cfg.cpm_per_usvh_x100 == app.ui.cal[0], "sensitivity %u", (unsigned)app.ui.cal[0]);
    uint32_t cal0 = app.ui.cal[0];
    reboot();
    CHECK(app.meas.cfg.cpm_per_usvh_x100 == cal0, "calibration lost after restart (%u vs %u)", (unsigned)app.meas.cfg.cpm_per_usvh_x100, (unsigned)cal0);

    /* cancelling an edit puts the old value back */
    app.sp = 0; app.stack[1] = SCR_CALIB; app.sp = 1;
    send(EV_OK);
    send(EV_RIGHT);
    send(EV_BACK);
    CHECK(app.ui.cal[0] == cal0, "cancelled edit changed the value");
    printf("clock and calibration: ok\n");
}

static proto_t proto;

static const char *ask(const char *cmd)
{
    ser_out_n = 0;
    ser_out[0] = 0;
    strcpy(ser_in, cmd);
    ser_in_n = (int)strlen(cmd);
    ser_in_p = 0;
    proto_poll(&proto);
    return ser_out;
}

static void test_protocol(void)
{
    fresh();
    to_stage5();
    proto_init(&proto, &app);

    CHECK(strcmp(ask("PING\n"), "OK pong\n") == 0, "ping: %s", ser_out);
    CHECK(strncmp(ask("INFO\n"), "OK yarrow=0.1 stage=5", 21) == 0, "info: %s", ser_out);
    CHECK(strncmp(ask("INFO\r\n"), "OK yarrow", 9) == 0, "carriage return handling");
    CHECK(strcmp(ask("UID\n"), "OK 9367a9ef47f81d05\n") == 0, "uid hash: %s", ser_out);
    {
        const char *r = ask("STATUS\n");
        CHECK(strncmp(r, "OK 10", 5) == 0 && strstr(r, "S radiation rate_nsvh=") && strstr(r, "S battery mv=") &&
                  strstr(r, "S device lang=") && strstr(r, "END\n"),
              "status reply: %s", r);
        for (const char *q = r; *q;) {
            const char *e = strchr(q, 10);
            CHECK(e && e - q <= 95, "status line too long or unterminated");
            if (!e)
                break;
            q = e + 1;
        }
    }
    CHECK(strstr(ask("CAPS\n"), "proto=2") != 0, "caps reply: %s", ser_out);
    CHECK(strncmp(ask("BAT\n"), "OK ", 3) == 0 && strlen(ser_out) > 8, "bat reply: %s", ser_out);
    {
        unsigned long pulses = 0, hvs = 0, tm = 0;
        ask("COUNTS\n");
        CHECK(sscanf(ser_out, "OK %lu %lu %lu", &pulses, &hvs, &tm) == 3 && hvs > 0 && tm > 0,
              "counts reply: %s", ser_out);
    }

    ask("TIME 1900000000\n");
    CHECK(strcmp(ser_out, "OK\n") == 0 && t_now == 1900000000u, "set time: %s (t=%u)", ser_out, (unsigned)t_now);
    CHECK(strncmp(ask("TIME\n"), "OK 19000000", 11) == 0, "get time: %s", ser_out);
    CHECK(strncmp(ask("TIME abc\n"), "ERR", 3) == 0, "bad time");

    CHECK(strncmp(ask("CAL\n"), "OK sens=8000 dead=90 e=100 h=100", 31) == 0, "cal: %s", ser_out);
    CHECK(strcmp(ask("CAL 7800 100 120 90\n"), "OK\n") == 0, "set cal: %s", ser_out);
    CHECK(app.meas.cfg.cpm_per_usvh_x100 == 7800 && app.meas.cfg.dead_time_us == 100 && app.ui.cal[2] == 120, "calibration not applied");
    CHECK(strncmp(ask("CAL 1 1 1 1\n"), "ERR", 3) == 0, "out of range calibration accepted");
    CHECK(strncmp(ask("CAL 8000 90\n"), "ERR", 3) == 0, "short calibration accepted");

    CHECK(strcmp(ask("SET THEME 7\n"), "OK\n") == 0 && app.settings.theme == 7, "set theme");
    CHECK(strncmp(ask("SET THEME 99\n"), "ERR", 3) == 0, "bad theme accepted");
    CHECK(strncmp(ask("HELLO\n"), "ERR unknown", 11) == 0, "unknown command");

    char longline[200];
    memset(longline, 'A', sizeof longline - 2);
    longline[sizeof longline - 2] = '\n';
    longline[sizeof longline - 1] = 0;
    ser_in_n = 0;
    CHECK(strstr(ask(longline), "ERR long") != NULL || ser_out_n > 0, "long line handling");
    CHECK(strcmp(ask("PING\n"), "OK pong\n") == 0, "protocol recovers after a long line");

    /* live stream */
    ask("LIVE 1\n");
    ser_out_n = 0;
    proto_tick(&proto, 1900000100u);
    CHECK(strncmp(ser_out, "L 1900000100 ", 13) == 0, "live line: %s", ser_out);
    ser_out_n = 0;
    proto_tick(&proto, 1900000100u);
    CHECK(ser_out_n == 0, "live line sent twice for one second");
    ask("LIVE 0\n");

    /* log: run long enough for a segment to exist, then read a page back */
    run_s(200);
    ask("LOGINFO\n");
    CHECK(strncmp(ser_out, "OK pages=16 pagesize=1024 used=", 31) == 0, "loginfo: %s", ser_out);
    ask("LOGPAGE 99\n");
    CHECK(strncmp(ser_out, "ERR page", 8) == 0, "bad page accepted");
    ask("LOGPAGE 0\n");
    CHECK(strncmp(ser_out, "OK 0 1024\n", 10) == 0 && strstr(ser_out, "\nEND\n") != NULL, "logpage framing: %.40s", ser_out);
    /* first bytes of a used page are "YL": base64 of 59 4C ... starts with "WUw" */
    CHECK(strncmp(ser_out + 10, "WUw", 3) == 0 || strncmp(ser_out + 10, "/////", 5) == 0, "first page data: %.12s", ser_out + 10);

    ask("EVENTS\n");
    CHECK(strncmp(ser_out, "OK ", 3) == 0 && strstr(ser_out, "END\n") != NULL, "events: %s", ser_out);
    printf("protocol: ok\n");
}

static void test_clock_follows_rtc(void)
{
    /* the millisecond counter runs 1.5 times too fast against the real-time clock: counts must still come out right */
    fresh();
    t_cps = 0.5;
    to_stage5();
    uint32_t hv0 = app.hv_secs;
    uint64_t c0 = app.meas.counts_total;
    uint32_t p0 = t_pulses;
    for (int s = 0; s < 1200; s++) {
        t_now++;
        t_pulses += poisson(t_cps);
        for (int k = 0; k < 33; k++) {         /* 33 x 20 ms = 660 ms of millisecond counter per clock second */
            now_ms += 20;
            app_poll(&app, now_ms);
        }
    }
    uint32_t hv = app.hv_secs - hv0;
    double counted = (double)(app.meas.counts_total - c0), real = (double)(t_pulses - p0);
    CHECK(hv >= 1198 && hv <= 1201, "seconds follow the clock: %u of 1200", (unsigned)hv);
    CHECK(real > 300 && counted > real * 0.97 && counted < real * 1.08, "counts %g of %g real pulses", counted, real);
}

static void test_display_power_and_shutdown(void)
{
    fresh();
    to_stage5();
    send(EV_OK);
    run_s(95);
    CHECK(!display_enabled, "display should be off after 90 s without input");
    send(EV_OK); /* wakes the screen and is consumed */
    CHECK(display_enabled && app_screen(&app) == SCR_MENU, "first key after sleep must only wake (screen %d)", app_screen(&app));

    uint32_t offs = power_offs;
    push_ev(EV_POWER_LONG);
    run_ms(100);
    CHECK(power_offs > offs, "long power press must power off");
    printf("power: ok\n");
}

int main(void)
{
    test_ladder();
    test_measurement_and_log();
    test_alarm_and_events();
    test_stillness();
    test_navigation_and_settings();
    test_leds();
    test_usb_start();
    test_calibration_wizard();
    test_sessions();
    test_flash_wear();
    test_threshold_edit_and_budget();
    test_clock_and_calibration();
    test_protocol();
    test_clock_follows_rtc();
    test_display_power_and_shutdown();
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall app checks passed\n", failures);
    return failures ? 1 : 0;
}
