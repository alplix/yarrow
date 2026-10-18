/* The application: turns hardware events and sensor readings into measurements, alarms, logs and screens.
 * It talks to the device only through hal_t, so the same code runs on the GC-03, in the simulator and in tests. */
#ifndef YARROW_APP_H
#define YARROW_APP_H

#include "alarm.h"
#include "cal.h"
#include "bat.h"
#include "hal.h"
#include "kv.h"
#include "meas.h"
#include "screens.h"
#include "uidata.h"
#include "ylog.h"

#define APP_STACK 6
#define APP_EVENTS_KEPT 8
#define APP_STAGES 5

/* settings kept in the key/value store */
typedef struct {
    uint8_t theme;
    uint8_t brightness; /* 1..5 */
    bool sound, vibration;
    int8_t tz_hours;
    uint8_t profile;    /* log profile 0..4 */
} app_settings_t;

typedef struct {
    const hal_t *hal;

    /* start-up ladder: features are switched on one step at a time, each step must run stably first */
    uint8_t stage;            /* features available now, 1..APP_STAGES */
    uint8_t verified;         /* highest stage that ran stably */
    uint8_t suspect_mask;     /* bit n: stage n failed to run stably last time */
    uint32_t stage_since_ms;
    bool kv_ok;

    meas_t meas;
    alarm_t alarm;
    bat_t bat;
    ylog_t log;
    kv_t kv;
    bool log_ok;
    bool hv_on;
    bool usb_started;

    /* time */
    uint32_t next_second_ms;
    uint32_t last_input_ms;
    uint32_t blink_ms;
    bool display_on;
    bool dimmed;
    uint32_t last_pulses;
    uint32_t seconds_run;

    /* screens */
    screen_id_t stack[APP_STACK];
    ui_state_t states[APP_STACK];
    int sp;
    int home_idx;
    bool need_full;
    bool need_draw;
    bool alarm_muted;
    bool alarm_dismissed;
    uint32_t drawn_sig;      /* fingerprint of what the screen showed at the last redraw */
    bool usb_was_present;
    uint32_t anim_ms;
    app_settings_t settings;
    int theme_before;         /* theme to return to when the picker is cancelled */
    int edit;                 /* 0 none, 1 editing a value on the current screen */
    uint32_t edit_value;

    /* data */
    ui_data_t ui;
    uint64_t dose_base_nsv;
    uint32_t pulses_base;
    uint32_t day_nsv[7];
    uint32_t day_index;
    uint64_t dose_prev_total;
    uint32_t trend_timer, field_timer;
    uint32_t bg_x16;          /* learned background, nSv/h times 16 */
    uint32_t th[3];

    /* log */
    uint32_t log_acc;
    uint32_t log_start;
    uint32_t log_elapsed;
    uint32_t log_interval;

    /* sample test */
    uint32_t sample_counts, sample_seconds;
    /* calibration wizard and sessions */
    cal_acc_t cw;
    uint64_t sess_prev_total;
    /* scan */
    uint32_t scan_base_x10;
    uint32_t scan_prev_x10;
    bool scan_sound;
    /* self test */
    uint32_t selftest_tick;
    /* sound */
    uint32_t sound_ms;
    uint8_t sound_phase;
    /* power */
    bool shutting_down;
    uint32_t last_save_s, last_slow_save_s;
} app_t;

void app_init(app_t *a, const hal_t *hal, uint32_t now_ms);

/* Call every 10-20 ms with a monotonic millisecond clock. Handles input, the one-second tick and drawing. */
void app_poll(app_t *a, uint32_t now_ms);

/* Saves everything and stops logging cleanly (call before the power goes). */
void app_shutdown(app_t *a);

screen_id_t app_screen(const app_t *a);

/* page-sized scratch buffer shared by the log scans */
uint8_t *app_scratch(void);

/* settings changed from outside (serial protocol); return false for values out of range */
bool app_set_calibration(app_t *a, const uint32_t cal[4]);
bool app_set_theme(app_t *a, uint32_t theme);
void app_set_time(app_t *a, uint32_t unix_s);

#endif
