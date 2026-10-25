#include "screens.h"
#include "display.h"
#include "fmt.h"
#include "strings.h"
#include "ui.h"

/* ---- helpers ---- */

static uint16_t level_color(int level)
{
    const theme_t *t = ui_theme;
    return level >= 3 ? t->bad : level == 2 ? t->hot : level == 1 ? t->warn : t->ok;
}

static const char *level_name(int level)
{
    return level >= 3 ? S_EVACUATE : level == 2 ? S_DANGER : level == 1 ? S_ATTENTION : S_NORMAL;
}

/* 20 blocks over 0.05 uSv/h .. 100 uSv/h on a logarithmic scale: block i lights at 50 * 2000^((i+1)/20) nSv/h */
static int meter_lit_nsvh(uint32_t nsvh)
{
    static const uint32_t th[20] = {73,   107,  156,  228,  333,   487,   713,   1043,  1525,  2231,
                                    3263, 4773, 6982, 10213, 14938, 21851, 31963, 46754, 68391, 100000};
    int lit = 0;
    while (lit < 20 && nsvh >= th[lit])
        lit++;
    return lit;
}

static int meter_lit_log(uint32_t v, uint32_t base, uint32_t ratio_q16)
{
    uint64_t thr = (uint64_t)base << 16;
    int lit = 0;
    for (int i = 0; i < 20; i++) {
        thr = (thr * ratio_q16) >> 16;
        if (((uint64_t)v << 16) >= thr)
            lit = i + 1;
        else
            break;
    }
    return lit;
}

static int add_str(char *b, int n, const char *s)
{
    while (*s)
        b[n++] = *s++;
    b[n] = 0;
    return n;
}

static void unit_text(dose_unit_t u, char *out)
{
    const char *s = u == UNIT_MSVH ? S_MSVH : S_USVH;
    while (*s)
        *out++ = *s++;
    *out = 0;
}

static uint32_t max_of(const uint32_t *v, int n)
{
    uint32_t m = 1;
    for (int i = 0; i < n; i++)
        if (v[i] > m)
            m = v[i];
    return m;
}

static void clock_text(const ui_data_t *d, char *out) { fmt_clock(out, d->now, d->tz_offset_s); }

/* big number with its unit after it, in a cleared row */
static void big_value(int y, const font_t *nf, const char *num, const char *unit, uint16_t color, const font_t *uf)
{
    const theme_t *t = ui_theme;
    display_fill_rect(UI_MARGIN, y, UI_W - 2 * UI_MARGIN, nf->height, t->bg);
    int nw = font_width(nf, num, 0);
    font_draw(nf, UI_MARGIN, y, num, color, t->bg, 0);
    font_draw(uf, UI_MARGIN + nw + 8, y + nf->height - uf->height - 8, unit, t->dim, t->bg, 0);
}

/* ---- main radiation screen ---- */

static void scr_main(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    char buf[32], unit[12];

    if (full)
        ui_clear();
    clock_text(d, buf);
    ui_statusbar(buf, d->bat_percent, d->bat_state == 1);

    display_fill_rect(UI_MARGIN, 28, UI_W - 2 * UI_MARGIN, 26, t->bg);
    ui_trefoil(UI_MARGIN, 28, level_color(d->level), 2);
    ui_chip(UI_MARGIN + 36, 32, level_name(d->level), level_color(d->level));
    ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 34, S_REC_ON, t->dim, t->bg);

    dose_unit_t u;
    fmt_dose_rate(buf, d->rate_nsvh, &u);
    unit_text(u, unit);
    big_value(64, &font_48_medium, buf, unit, d->level >= 2 ? level_color(d->level) : t->ink, &font_14_medium);

    ui_meter(UI_MARGIN, 134, UI_W - 2 * UI_MARGIN, meter_lit_nsvh(d->rate_nsvh), true, 0);
    if (full) {
        int w = UI_W - 2 * UI_MARGIN;
        ui_text(&font_12_regular, UI_MARGIN, 152, "0.1", t->dim, t->bg);
        ui_text(&font_12_regular, UI_MARGIN + w * 39 / 100 - 4, 152, "1", t->dim, t->bg);
        ui_text(&font_12_regular, UI_MARGIN + w * 70 / 100 - 8, 152, "10", t->dim, t->bg);
        ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 152, "100", t->dim, t->bg);
    }

    char v[16];
    int pw = (UI_W - 2 * UI_MARGIN - 8) / 2;
    fmt_u(v, d->cpm);
    ui_stat_panel(UI_MARGIN, 174, pw, S_CPM, v, "");
    dose_total_unit_t du;
    fmt_dose_total(v, d->dose_today_nsv, &du);
    ui_stat_panel(UI_MARGIN + pw + 8, 174, pw, S_TODAY, v, du == DOSE_MSV ? S_MSV : du == DOSE_USV ? S_USV : S_NSV);

    ui_columns(UI_MARGIN, 236, UI_W - 2 * UI_MARGIN, 38, d->trend, TREND_N, max_of(d->trend, TREND_N), 4, 5, 1, t->acc, false, -1);

    char left[24], bat[24];
    fmt_duration(bat, d->bat_minutes * 60u);
    int n = 0;
    for (const char *p = S_BAT_LEFT; *p;)
        left[n++] = *p++;
    left[n++] = ' ';
    for (const char *p = bat; *p;)
        left[n++] = *p++;
    left[n] = 0;
    if (d->stage && d->stage < 5) {
        char st[24];
        int k = add_str(st, 0, S_STAGE_WORD);
        k += fmt_u(st + k, d->stage);
        add_str(st, k, "/5");
        ui_footer(st, S_LAST_10);
    } else {
        ui_footer(left, S_LAST_10);
    }
}

/* ---- live trend ---- */

static void scr_trend(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    char buf[32], unit[12];

    if (full)
        ui_clear();
    clock_text(d, buf);
    ui_statusbar(buf, d->bat_percent, d->bat_state == 1);

    dose_unit_t u;
    fmt_dose_rate(buf, d->rate_nsvh, &u);
    unit_text(u, unit);
    big_value(30, &font_34_medium, buf, unit, t->ink, &font_12_regular);

    /* change against the start of the window */
    char ch[16];
    int32_t c = d->trend_change_pct;
    int n = add_str(ch, 0, c <= 0 ? "↓" : "↑");
    n += fmt_u(ch + n, (uint32_t)(c < 0 ? -c : c));
    ch[n++] = '%';
    ch[n] = 0;
    ui_label(&font_12_regular, UI_W - UI_MARGIN - 60, 50, 60, ch, c <= 0 ? t->ok : t->warn, t->bg, 1);

    uint32_t mx = max_of(d->trend, TREND_N);
    uint32_t th = d->th_nsvh[0];
    if (th > mx)
        mx = th + th / 4;
    ui_columns(UI_MARGIN, 80, UI_W - 2 * UI_MARGIN, 114, d->trend, TREND_N, mx, 6, 5, 1, t->acc, false, (int32_t)th);

    if (full) {
        ui_text(&font_12_regular, UI_MARGIN, 198, S_AXIS_10MIN, t->dim, t->bg);
        ui_text_center(&font_12_regular, UI_W / 2, 198, "-5", t->dim, t->bg);
        ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 198, S_NOW, t->dim, t->bg);
    }

    int pw = (UI_W - 2 * UI_MARGIN - 12) / 3;
    dose_unit_t u2;
    fmt_dose_rate(buf, d->trend_min, &u2);
    ui_stat_panel(UI_MARGIN, 222, pw, S_MIN, buf, "");
    fmt_dose_rate(buf, d->trend_avg, &u2);
    ui_stat_panel(UI_MARGIN + pw + 6, 222, pw, S_AVG, buf, "");
    fmt_dose_rate(buf, d->trend_max, &u2);
    ui_stat_panel(UI_MARGIN + 2 * (pw + 6), 222, pw, S_MAX, buf, "");

    char th_txt[24];
    int k = 0;
    for (const char *p = S_THRESHOLD; *p;)
        th_txt[k++] = *p++;
    th_txt[k++] = ' ';
    fmt_dose_rate(th_txt + k, d->th_nsvh[0], &u2);
    ui_footer("[<][>] " S_SCALE, th_txt);
}

/* ---- electric and magnetic field ---- */

static void scr_field(const ui_data_t *d, bool magnetic, bool full)
{
    const theme_t *t = ui_theme;
    char buf[32];

    if (full)
        ui_clear();
    clock_text(d, buf);
    ui_statusbar(magnetic ? S_T_HFIELD : S_T_EFIELD, d->bat_percent, d->bat_state == 1);

    uint32_t value = magnetic ? d->h_ut_x100 : d->e_vm_x10;
    int lit = magnetic ? meter_lit_log(value, 1, 103866) : meter_lit_log(value, 10, 95832);
    int level = magnetic ? (value < 100 ? 0 : value < 1000 ? 1 : 2) : (value < 500 ? 0 : value < 5000 ? 1 : 2);

    display_fill_rect(UI_MARGIN, 28, UI_W - 2 * UI_MARGIN, 26, t->bg);
    if (magnetic)
        ui_sprite_multi(UI_MARGIN, 30, &spr_magnet, 2);
    else
        ui_sprite(UI_MARGIN, 28, &spr_bolt, t->warn, 2);
    ui_chip(UI_MARGIN + 30, 32, level == 0 ? S_LOW : level == 1 ? S_MEDIUM : S_HIGH, level == 0 ? t->ok : level == 1 ? t->warn : t->bad);
    ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 34, magnetic ? S_RANGE_H : S_RANGE_E, t->dim, t->bg);

    if (magnetic)
        fmt_fixed2(buf, value);
    else
        fmt_u(buf, (value + 5) / 10);
    big_value(64, &font_48_medium, buf, magnetic ? S_UT : S_VM, t->ink, &font_14_medium);

    ui_meter(UI_MARGIN, 134, UI_W - 2 * UI_MARGIN, lit, true, 0);
    if (full) {
        int w = UI_W - 2 * UI_MARGIN;
        if (magnetic) {
            ui_text(&font_12_regular, UI_MARGIN, 152, "0.01", t->dim, t->bg);
            ui_text(&font_12_regular, UI_MARGIN + w * 25 / 100, 152, "0.1", t->dim, t->bg);
            ui_text(&font_12_regular, UI_MARGIN + w * 50 / 100, 152, "1", t->dim, t->bg);
            ui_text(&font_12_regular, UI_MARGIN + w * 75 / 100, 152, "10", t->dim, t->bg);
            ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 152, "100", t->dim, t->bg);
        } else {
            ui_text(&font_12_regular, UI_MARGIN, 152, "1", t->dim, t->bg);
            ui_text(&font_12_regular, UI_MARGIN + w * 33 / 100, 152, "10", t->dim, t->bg);
            ui_text(&font_12_regular, UI_MARGIN + w * 66 / 100, 152, "100", t->dim, t->bg);
            ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 152, "1k", t->dim, t->bg);
        }
    }

    const uint32_t *trend = magnetic ? d->h_trend : d->e_trend;
    ui_columns(UI_MARGIN, 174, UI_W - 2 * UI_MARGIN, 52, trend, TREND_N, max_of(trend, TREND_N), 4, 5, 1, t->acc, false, -1);
    if (full) {
        ui_text(&font_12_regular, UI_MARGIN, 230, S_AXIS_60S, t->dim, t->bg);
        ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 230, S_NOW, t->dim, t->bg);
    }

    int pw = (UI_W - 2 * UI_MARGIN - 8) / 2;
    char v[16];
    uint32_t avg = magnetic ? d->h_avg_x100 : d->e_avg_x10, peak = magnetic ? d->h_peak_x100 : d->e_peak_x10;
    if (magnetic) fmt_fixed2(v, avg); else fmt_u(v, (avg + 5) / 10);
    ui_stat_panel(UI_MARGIN, 248, pw, S_AVERAGE, v, magnetic ? S_UT : S_VM);
    if (magnetic) fmt_fixed2(v, peak); else fmt_u(v, (peak + 5) / 10);
    ui_stat_panel(UI_MARGIN + pw + 8, 248, pw, S_PEAK, v, magnetic ? S_UT : S_VM);
    /* the panels are 50 high: leave the footer out on this dense screen */
}

static void scr_efield(const ui_data_t *d, const ui_state_t *s, bool full) { (void)s; scr_field(d, false, full); }
static void scr_hfield(const ui_data_t *d, const ui_state_t *s, bool full) { (void)s; scr_field(d, true, full); }

/* ---- battery ---- */

static void scr_battery(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    char buf[32];

    if (full) {
        ui_clear();
        ui_header(&spr_battery, t->ok, S_T_BATTERY);
    }
    clock_text(d, buf);
    ui_statusbar(buf, d->bat_percent, d->bat_state == 1);

    display_fill_rect(UI_MARGIN, 66, UI_W - 2 * UI_MARGIN, 36, t->bg);
    ui_sprite(UI_MARGIN, 70, &spr_battery, t->ink, 5);
    int n = fmt_u(buf, d->bat_percent);
    buf[n++] = '%';
    buf[n] = 0;
    font_draw(&font_34_medium, UI_MARGIN + 116, 64, buf, t->ink, t->bg, 0);

    ui_meter(UI_MARGIN, 112, UI_W - 2 * UI_MARGIN, d->bat_percent / 5, false, t->ok);

    struct { const char *label; char value[20]; } rows[5];
    rows[0].label = S_REMAINING;
    if (d->bat_state == 0)
        fmt_duration(rows[0].value, d->bat_minutes * 60u);
    else {
        const char *src = d->bat_state == 1 ? S_CHARGING : S_CHARGED;
        int k = 0;
        while (*src)
            rows[0].value[k++] = *src++;
        rows[0].value[k] = 0;
    }
    rows[1].label = S_HEALTH;
    n = 0; rows[1].value[n++] = '%'; n += fmt_u(rows[1].value + n, d->bat_health); rows[1].value[n] = 0;
    rows[2].label = S_CYCLES;
    fmt_u(rows[2].value, d->bat_cycles);
    rows[3].label = S_RUNTIME;
    n = fmt_u(rows[3].value, d->on_hours);
    for (const char *hu = L_HOUR_UNIT; *hu;)
        rows[3].value[n++] = *hu++;
    rows[3].value[n] = 0;
    rows[4].label = S_TUBE_LIFE;
    n = 0; rows[4].value[n++] = '%'; n += fmt_u(rows[4].value + n, d->tube_life); rows[4].value[n] = 0;

    for (int i = 0; i < 5; i++) {
        int y = 144 + i * 28;
        display_fill_rect(UI_MARGIN, y, UI_W - 2 * UI_MARGIN, 22, t->bg);
        ui_text(&font_14_medium, UI_MARGIN, y + 2, rows[i].label, t->dim, t->bg);
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 2, rows[i].value, t->ink, t->bg);
    }
    ui_footer(d->usb ? "[OK] " S_DETAILS : "[OK] " S_DETAILS, d->usb ? (d->bat_state == 1 ? S_CHARGING : S_CHARGED) : S_NO_USB);
}

/* ---- history ---- */

static void scr_history(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    char buf[32];

    if (full) {
        ui_clear();
        ui_header(&spr_trefoil, t->acc, S_T_LAST7);
    }
    clock_text(d, buf);
    ui_statusbar(buf, d->bat_percent, d->bat_state == 1);

    uint32_t mx = max_of(d->week_nsv, 7);
    ui_columns(UI_MARGIN, 72, UI_W - 2 * UI_MARGIN, 112, d->week_nsv, 7, mx + mx / 4, 7, 24, 6, t->acc, true, -1);

    static const char *const days[7] = L_WEEKDAYS;
    uint32_t today = (uint32_t)(((int64_t)d->now + d->tz_offset_s) / 86400);
    for (int i = 0; i < 7; i++) {
        uint32_t day = today - (uint32_t)(6 - i);
        int wd = (int)((day + 3) % 7);
        ui_label(&font_12_regular, UI_MARGIN + i * 30, 188, 24, days[wd], t->dim, t->bg, 2);
    }

    int pw = (UI_W - 2 * UI_MARGIN - 8) / 2;
    dose_total_unit_t du;
    char v[16];
    fmt_dose_total(v, d->dose_week_nsv, &du);
    ui_stat_panel(UI_MARGIN, 216, pw, S_WEEK, v, du == DOSE_MSV ? S_MSV : du == DOSE_USV ? S_USV : S_NSV);
    fmt_dose_total(v, d->dose_total_nsv, &du);
    ui_stat_panel(UI_MARGIN + pw + 8, 216, pw, S_TOTAL, v, du == DOSE_MSV ? S_MSV : du == DOSE_USV ? S_USV : S_NSV);

    char rec[24];
    int n = 0;
    n += fmt_u(rec, d->days_stored);
    rec[n++] = ' ';
    for (const char *p = S_DAYS; *p;)
        rec[n++] = *p++;
    rec[n] = 0;
    ui_footer("[OK] " S_DETAILS, rec);
}

/* ---- alarm ---- */

static void scr_alarm(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    char buf[32], unit[12];
    uint16_t c = level_color(d->level);

    if (full)
        ui_clear();

    uint16_t banner = d->blink ? c : t->bg;
    uint16_t txt = d->blink ? t->bg : c;
    display_fill_rect(0, 0, UI_W, 34, banner);
    ui_text(&font_20_medium, UI_MARGIN, 4, level_name(d->level), txt, banner);
    if (d->level >= 3)
        ui_sprite(UI_W - UI_MARGIN - 22, 7, &spr_skull, txt, 2);
    else
        ui_text_right(&font_20_medium, UI_W - UI_MARGIN, 4, "!!", txt, banner);

    if (full)
        ui_hazard(UI_MARGIN, 44, c, 3);
    dose_unit_t u;
    fmt_dose_rate(buf, d->rate_nsvh, &u);
    unit_text(u, unit);
    display_fill_rect(72, 44, UI_W - 72, 52, t->bg);
    font_draw(&font_34_medium, 72, 40, buf, c, t->bg, 0);
    font_draw(&font_12_regular, 72, 82, unit, t->dim, t->bg, 0);

    ui_meter(UI_MARGIN, 104, UI_W - 2 * UI_MARGIN, meter_lit_nsvh(d->rate_nsvh), true, 0);

    uint32_t times = d->bg_nsvh ? d->rate_nsvh / d->bg_nsvh : 0;
    char ln[40];
    int n = 0;
    for (const char *p = S_NORMAL_x; *p;)
        ln[n++] = *p++;
    ln[n++] = ' ';
    n += fmt_u(ln + n, times);
    ln[n++] = ' ';
    for (const char *p = S_TIMES; *p;)
        ln[n++] = *p++;
    ln[n] = 0;
    ui_label(&font_12_regular, UI_MARGIN, 124, 216, ln, t->dim, t->bg, 0);

    ui_columns(UI_MARGIN, 146, UI_W - 2 * UI_MARGIN, 40, d->trend, TREND_N, max_of(d->trend, TREND_N), 4, 5, 1, c, true, -1);

    if (full) {
        const char *steps[3] = {S_STEP1, S_STEP2, S_STEP3};
        for (int i = 0; i < 3; i++) {
            char num[2] = {(char)('1' + i), 0};
            ui_chip(UI_MARGIN, 196 + i * 28, num, c);
            ui_text(&font_14_medium, UI_MARGIN + 34, 196 + i * 28, steps[i], t->ink, t->bg);
        }
        ui_footer("[OK] " S_GUIDE, "[<] " S_MUTE);
    }
}

/* ---- mode selector and menu ---- */

static void scr_mode(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    char buf[32], v[24], unit[12];

    if (full) {
        ui_clear();
        ui_header(&spr_trefoil, t->acc, S_T_MODE);
    }
    clock_text(d, buf);
    ui_statusbar(buf, d->bat_percent, d->bat_state == 1);

    dose_unit_t u;
    fmt_dose_rate(v, d->rate_nsvh, &u);
    unit_text(u, unit);
    int n = 0;
    char line0[32], line1[32], line2[32];
    for (const char *p = v; *p;) line0[n++] = *p++;
    line0[n++] = ' ';
    for (const char *p = unit; *p;) line0[n++] = *p++;
    line0[n] = 0;
    n = fmt_u(line1, (d->e_vm_x10 + 5) / 10);
    line1[n++] = ' ';
    for (const char *p = S_VM; *p;) line1[n++] = *p++;
    line1[n] = 0;
    n = fmt_fixed2(line2, d->h_ut_x100);
    line2[n++] = ' ';
    for (const char *p = S_UT; *p;) line2[n++] = *p++;
    line2[n] = 0;

    const char *names[3] = {S_T_RADIATION, S_T_EFIELD, S_T_HFIELD};
    const char *vals[3] = {line0, line1, line2};
    for (int i = 0; i < 3; i++) {
        int y = 72 + i * 70;
        bool sel = s->sel == i;
        ui_panel(UI_MARGIN, y, UI_W - 2 * UI_MARGIN, 58, t->panel);
        if (sel) {
            display_fill_rect(UI_MARGIN, y + 4, 2, 50, t->acc);
            display_fill_rect(UI_W - UI_MARGIN - 2, y + 4, 2, 50, t->acc);
            display_fill_rect(UI_MARGIN + 4, y, UI_W - 2 * UI_MARGIN - 8, 2, t->acc);
            display_fill_rect(UI_MARGIN + 4, y + 56, UI_W - 2 * UI_MARGIN - 8, 2, t->acc);
        }
        display_fill_rect(UI_MARGIN + 10, y + 10, 40, 38, t->panel);
        if (i == 0)
            ui_trefoil(UI_MARGIN + 14, y + 16, t->ok, 2);
        else if (i == 1)
            ui_sprite(UI_MARGIN + 16, y + 12, &spr_bolt, t->warn, 3);
        else
            ui_sprite_multi(UI_MARGIN + 14, y + 14, &spr_magnet, 3);
        ui_text(&font_14_medium, UI_MARGIN + 58, y + 10, names[i], t->ink, t->panel);
        ui_label(&font_12_regular, UI_MARGIN + 58, y + 32, 140, vals[i], t->dim, t->panel, 0);
    }
    ui_footer("[<][>] " S_SELECT, "[OK] " S_OPEN);
}

#define MENU_N 16
/* same order as open_menu_entry() in app.c */
static const char *const menu_names[MENU_N] = {S_M_GUIDE, S_M_EVENTS, S_M_BUDGET, S_M_SESSIONS, S_M_SAMPLE, S_M_SCAN, S_M_LOG,
                                               S_M_THRESH, S_M_TUBE, S_M_SELFTEST, S_M_USB, S_M_THEMES, S_M_SETTINGS, S_M_CALIB,
                                               S_M_CALWIZ, S_M_ABOUT};

static const sprite_t *const menu_icons[MENU_N] = {
    &spr_book, &spr_list, &spr_trefoil, &spr_signal, &spr_flask, &spr_magnet, &spr_disk, &spr_bolt,
    &spr_tube, &spr_check, &spr_plug, &spr_drop, &spr_gear, &spr_radio, &spr_atom, &spr_flower};

#define MENU_ROWS 6

static void scr_menu(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    char buf[32];
    static const char *const desc[MENU_N] = MENU_DESC;

    if (full) {
        ui_clear();
        ui_header(&spr_list, t->acc, S_T_MENU);
    }
    clock_text(d, buf);
    ui_statusbar(buf, d->bat_percent, d->bat_state == 1);

    for (int i = 0; i < MENU_ROWS; i++) {
        int idx = s->scroll + i;
        int y = 60 + i * 28;
        if (idx >= MENU_N) {
            display_fill_rect(0, y, UI_W, 26, t->bg);
            continue;
        }
        ui_row_icon(y, menu_icons[idx], menu_names[idx], idx == s->sel);
    }

    /* what the selected entry does */
    display_fill_rect(UI_MARGIN, 232, UI_W - 2 * UI_MARGIN, 52, t->panel);
    if (s->sel >= 0 && s->sel < MENU_N)
        ui_text_wrap(&font_12_regular, UI_MARGIN + 8, 238, UI_W - 2 * UI_MARGIN - 16, desc[s->sel], t->ink, t->panel, 3);
    ui_footer("[<] " S_BACK, "[OK] " S_OPEN);
}

/* ---- the rest of the screens live in screens2.c ---- */
void screens2_draw(screen_id_t id, const ui_data_t *d, const ui_state_t *s, bool full);

/* ---- screensaver: a large clock and the radiation level, at the lowest brightness ---- */

static void scr_saver(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    if (full)
        ui_clear();
    char buf[32], unit[12];
    clock_text(d, buf);
    display_fill_rect(0, 50, UI_W, 62, t->bg);
    font_draw_centered(&font_48_medium, 56, buf, t->ink, t->bg, 2);
    fmt_date(buf, d->now, d->tz_offset_s);
    display_fill_rect(0, 124, UI_W, 22, t->bg);
    font_draw_centered(&font_14_medium, 128, buf, t->dim, t->bg, 0);

    dose_unit_t u;
    fmt_dose_rate(buf, d->rate_nsvh, &u);
    unit_text(u, unit);
    uint16_t c = level_color(d->level);
    display_fill_rect(0, 172, UI_W, 110, t->bg);
    font_draw_centered(&font_48_medium, 178, buf, c, t->bg, 0);
    font_draw_centered(&font_14_medium, 236, unit, t->dim, t->bg, 0);
    font_draw_centered(&font_14_medium, 262, level_name(d->level), c, t->bg, 0);
}

void screen_draw(screen_id_t id, const ui_data_t *d, const ui_state_t *s, bool full)
{
    switch (id) {
    case SCR_MAIN:    scr_main(d, s, full); break;
    case SCR_TREND:   scr_trend(d, s, full); break;
    case SCR_EFIELD:  scr_efield(d, s, full); break;
    case SCR_HFIELD:  scr_hfield(d, s, full); break;
    case SCR_BATTERY: scr_battery(d, s, full); break;
    case SCR_HISTORY: scr_history(d, s, full); break;
    case SCR_ALARM:   scr_alarm(d, s, full); break;
    case SCR_MODE:    scr_mode(d, s, full); break;
    case SCR_MENU:    scr_menu(d, s, full); break;
    case SCR_SAVER:   scr_saver(d, s, full); break;
    default:          screens2_draw(id, d, s, full); break;
    }
}
