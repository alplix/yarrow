/* Everything the screens need to know, collected by the application once a second. The screens only read it. */
#ifndef YARROW_UIDATA_H
#define YARROW_UIDATA_H

#include <stdbool.h>
#include <stdint.h>

#define TREND_N 36
#define GUIDE_PAGES 7

typedef struct {
    uint32_t start;
    uint32_t duration_s;
    uint32_t peak_nsvh;
    uint8_t level;
} ui_event_t;

#define SESS_SLOTS 8
#define SESS_KINDS 6

typedef struct {
    uint8_t kind;
    uint32_t start;
    uint32_t seconds;
    uint32_t dose_nsv;
    uint32_t max_nsvh;
} ui_session_t;

typedef struct {
    uint32_t now;           /* unix seconds */
    int32_t tz_offset_s;

    /* radiation */
    uint32_t rate_nsvh;
    uint32_t cpm;
    uint8_t level;          /* 0 normal, 1 attention, 2 danger, 3 evacuate */
    uint32_t bg_nsvh;       /* learned background, for "x times normal" */
    uint64_t dose_today_nsv, dose_week_nsv, dose_total_nsv;
    uint32_t trend[TREND_N];/* nSv/h, oldest first */
    uint32_t trend_min, trend_avg, trend_max;
    int32_t trend_change_pct;
    uint32_t week_nsv[7];   /* dose per day, oldest first, the last one is today */
    uint32_t th_nsvh[3];    /* alarm thresholds */

    /* fields */
    uint32_t e_vm_x10, e_avg_x10, e_peak_x10;
    uint32_t e_trend[TREND_N];
    uint32_t h_ut_x100, h_avg_x100, h_peak_x100;
    uint32_t h_trend[TREND_N];

    /* battery and device */
    uint8_t bat_percent;
    uint8_t bat_state;      /* 0 discharging, 1 charging, 2 full */
    uint32_t bat_minutes;
    uint8_t bat_health;
    uint32_t bat_cycles;
    uint32_t on_hours;
    uint8_t tube_life;
    bool usb;

    /* storage and log */
    uint32_t days_stored;
    uint8_t storage_pct;
    uint32_t oldest_time;
    uint8_t profile;        /* 0 detail .. 4 archive */
    uint32_t alarm_count;
    uint32_t profile_days[5];

    /* events */
    ui_event_t events[8];
    uint8_t event_count;

    /* dose budget */
    bool budget_active;
    uint32_t budget_nsv, budget_used_nsv, budget_start_time;
    uint32_t budget_avg_nsvh;

    /* sample test */
    uint8_t sample_phase;   /* 0 idle, 1 measuring background, 2 measuring sample, 3 done */
    uint32_t sample_bg_nsvh, sample_nsvh;
    uint32_t sample_elapsed_s, sample_goal_s;
    int32_t sample_net_nsvh;

    /* tube */
    uint32_t total_pulses;
    uint32_t bg_cpm;
    bool deadtime_on;
    bool hv_factory;

    /* settings */
    uint8_t theme;
    uint8_t brightness;     /* 1..5 */
    bool sound, vibration;

    /* self test */
    uint8_t selftest_done, selftest_total;
    uint8_t selftest_fail;  /* bit i set: check i failed */
    uint8_t stage;          /* start-up ladder step reached, 1..5 */
    uint8_t scan_field;     /* 0 electric, 1 magnetic */

    /* clock editor: year, month, day, hour, minute */
    uint16_t clk[5];
    /* calibration: sensitivity (CPM per uSv/h x100), dead time (us), electric and magnetic scale (percent) */
    uint32_t cal[4];

    /* calibration wizard */
    uint8_t cw_phase;       /* 0 enter the reference, 1 measuring, 2 result */
    bool cw_saturated;
    uint32_t cw_ref_nsvh, cw_counts, cw_seconds, cw_unc_x10, cw_new_x100;

    /* sessions: a named stretch of time with its own dose */
    bool sess_active;
    uint8_t sess_kind;
    uint8_t sess_saved;     /* how many finished sessions are in the list */
    uint32_t sess_start, sess_seconds, sess_max_nsvh;
    uint64_t sess_dose_nsv;
    ui_session_t sess_list[SESS_SLOTS]; /* newest first */

    /* scan mode */
    uint32_t scan_bars;     /* 0..10 */
    uint32_t scan_value_x10;
    bool scan_rising;

    /* usb */
    bool usb_connected;
    bool usb_synced;
    bool usb_streaming;

    bool blink;             /* toggles every half second, for flashing banners */
    uint8_t anim;           /* counts up every quarter second, for animations */
} ui_data_t;

typedef struct {
    int sel;     /* selected row in a list */
    int scroll;  /* first visible row */
    int page;    /* page in a multi page screen */
    int mode;    /* sub mode of a screen (for example which field is shown) */
} ui_state_t;

#endif
