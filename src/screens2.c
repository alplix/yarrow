#include "display.h"
#include "fmt.h"
#include "screens.h"
#include "strings.h"
#include "ui.h"

static const guide_page_t guide_pages[GUIDE_PAGES] = GUIDE_DATA;

static int add(char *b, int n, const char *s)
{
    while (*s)
        b[n++] = *s++;
    b[n] = 0;
    return n;
}

static void status(const ui_data_t *d)
{
    char buf[16];
    fmt_clock(buf, d->now, d->tz_offset_s);
    ui_statusbar(buf, d->bat_percent, d->bat_state == 1);
}

static uint16_t level_color(int level)
{
    const theme_t *t = ui_theme;
    return level >= 3 ? t->bad : level == 2 ? t->hot : level == 1 ? t->warn : t->ok;
}

static const char *level_name(int level)
{
    return level >= 3 ? S_EVACUATE : level == 2 ? S_DANGER : level == 1 ? S_ATTENTION : S_NORMAL;
}

/* ---- emergency guide ---- */

static const sprite_t *const guide_icons[GUIDE_PAGES] = {&spr_house, &spr_drop, &spr_check, &spr_cross,
                                                         &spr_radio, &spr_trefoil, &spr_signal};

static void scr_guide_list(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_house, t->warn, S_T_GUIDE);
    }
    status(d);
    for (int i = 0; i < GUIDE_PAGES; i++) {
        int y = 62 + i * 32;
        bool sel = i == s->sel;
        uint16_t bg = sel ? t->acc : t->bg;
        display_fill_rect(0, y, UI_W, 30, bg);
        ui_sprite(UI_MARGIN, y + 6, guide_icons[i], sel ? t->bg : t->acc, 2);
        ui_text(&font_14_medium, UI_MARGIN + 30, y + 6, guide_pages[i].title, sel ? t->bg : t->ink, bg);
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 6, ">", sel ? t->bg : t->dim, bg);
    }
    ui_footer(S_OFFLINE, "[OK] " S_OPEN);
}

static void scr_guide_page(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    int p = s->page < 0 ? 0 : s->page >= GUIDE_PAGES ? GUIDE_PAGES - 1 : s->page;
    if (full) {
        ui_clear();
        ui_header(guide_icons[p], t->warn, guide_pages[p].title);
    }
    status(d);

    display_fill_rect(0, 62, UI_W, 226, t->bg);
    int y = 68;
    for (int i = 0; i < 7 && guide_pages[p].lines[i]; i++) {
        if (y > 264)
            break;
        display_fill_rect(UI_MARGIN, y + 7, 6, 6, t->warn);
        y = ui_text_wrap(&font_14_medium, UI_MARGIN + 16, y, UI_W - 2 * UI_MARGIN - 16, guide_pages[p].lines[i], t->ink, t->bg, 1) + 6;
    }
    char n[16];
    int k = fmt_u(n, (uint32_t)p + 1);
    n[k++] = ' ';
    n[k++] = '/';
    n[k++] = ' ';
    fmt_u(n + k, GUIDE_PAGES);
    ui_footer(n, "[<][>]");
}

/* ---- settings ---- */

static void toggle(int right, int y, bool on)
{
    const theme_t *t = ui_theme;
    int x = right - 30;
    display_fill_rect(x, y, 30, 14, on ? t->ok : t->line);
    display_fill_rect(on ? x + 19 : x + 3, y + 3, 8, 8, on ? t->bg : t->dim);
}

static void scr_settings(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_gear, t->acc, S_T_SETTINGS);
    }
    status(d);
    static const char *const themes_n[THEME_COUNT] = THEME_NAMES;
    const char *labels[12] = {S_THEME, S_BRIGHTNESS, S_SOUND, S_VIBRATION, S_CLICK, S_COUNT_VIBE, S_LED_PULSE, S_LED_CHARGE, S_LED_COLOR, S_USB_START, S_TIME_ZONE, S_CLOCK_DATE};
    static const char *const desc[12] = SETTINGS_DESC;
    for (int r = 0; r < 6; r++) {
        int i = s->scroll + r;
        int y = 60 + r * 30;
        if (i >= 12) {
            display_fill_rect(0, y, UI_W, 28, t->bg);
            continue;
        }
        bool sel = i == s->sel;
        uint16_t bg = sel ? t->acc : t->bg;
        uint16_t fg = sel ? t->bg : t->ink;
        display_fill_rect(0, y, UI_W, 28, bg);
        ui_text(&font_14_medium, UI_MARGIN, y + 5, labels[i], fg, bg);
        int right = UI_W - UI_MARGIN;
        if (i == 0) {
            char v[24];
            int n = add(v, 0, "< ");
            n = add(v, n, themes_n[d->theme % THEME_COUNT]);
            add(v, n, " >");
            ui_text_right(&font_14_medium, right, y + 5, v, sel ? t->bg : t->acc, bg);
        } else if (i == 1) {
            for (int b = 0; b < 5; b++)
                display_fill_rect(right - 5 * 11 + b * 11, y + 7, 8, 14, b < d->brightness ? (sel ? t->bg : t->acc) : (sel ? t->dim : t->line));
        } else if (i == 2) {
            toggle(right, y + 7, d->sound);
        } else if (i == 3) {
            toggle(right, y + 7, d->vibration);
        } else if (i == 4) {
            toggle(right, y + 7, d->click);
        } else if (i == 5) {
            toggle(right, y + 7, d->count_vibe);
        } else if (i == 6) {
            toggle(right, y + 7, d->led_pulse);
        } else if (i == 7) {
            toggle(right, y + 7, d->led_charge);
        } else if (i == 8) {
            static const char *const color_names[8] = LED_COLOR_NAMES;
            static const uint16_t swatch[8] = {0, 0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F, 0xFFFF};
            static const uint8_t mask_to_swatch[8] = {0, 1, 2, 4, 3, 6, 5, 7};
            uint16_t col = 0xFFFF;
            for (int k = 1; k < 8; k++)
                if (mask_to_swatch[k] == d->led_mask)
                    col = swatch[k];
            display_fill_rect(right - 14, y + 7, 14, 14, col);
            ui_text_right(&font_14_medium, right - 22, y + 5, color_names[d->led_color & 7], sel ? t->bg : t->acc, bg);
        } else if (i == 9) {
            toggle(right, y + 7, d->usb_start);
        } else if (i == 10) {
            int32_t h = d->tz_offset_s / 3600;
            char v[16];
            int n = add(v, 0, "UTC+");
            fmt_u(v + n, (uint32_t)(h < 0 ? -h : h));
            ui_text_right(&font_14_medium, right, y + 5, v, sel ? t->bg : t->acc, bg);
        } else {
            char v[16];
            fmt_clock(v, d->now, d->tz_offset_s);
            ui_text_right(&font_14_medium, right, y + 5, v, sel ? t->bg : t->acc, bg);
        }
    }
    display_fill_rect(UI_MARGIN, 244, UI_W - 2 * UI_MARGIN, 40, t->panel);
    ui_text_wrap(&font_12_regular, UI_MARGIN + 8, 250, UI_W - 2 * UI_MARGIN - 16, desc[s->sel % 12], t->ink, t->panel, 3);
    ui_footer("[<] " S_BACK, "[OK] " S_CHANGE);
}

/* ---- events ---- */

static void scr_events(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(0, 0, S_T_EVENTS);
    }
    status(d);
    for (int i = 0; i < 4; i++) {
        int y = 62 + i * 56;
        int idx = s->scroll + i;
        display_fill_rect(0, y, UI_W, 54, t->bg);
        if (idx >= d->event_count)
            continue;
        const ui_event_t *e = &d->events[idx];
        bool sel = idx == s->sel;
        uint16_t bg = sel ? t->acc : t->bg;
        display_fill_rect(0, y, UI_W, 52, bg);
        char when[24];
        fmt_date_time(when, e->start, d->tz_offset_s);
        ui_text(&font_14_medium, UI_MARGIN, y + 6, when, sel ? t->bg : t->ink, bg);
        if (sel)
            ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 6, level_name(e->level), t->bg, bg);
        else
            ui_chip(UI_W - UI_MARGIN - font_width(&font_12_medium, level_name(e->level), 1) - 18, y + 6, level_name(e->level),
                    level_color(e->level));

        char det[40], v[16];
        dose_unit_t u;
        int n = add(det, 0, S_PEAK_WORD);
        fmt_dose_rate(v, e->peak_nsvh, &u);
        n = add(det, n, v);
        n = add(det, n, u == UNIT_MSVH ? " mSv/h" : " µSv/h");
        n = add(det, n, " · ");
        fmt_duration(v, e->duration_s);
        add(det, n, v);
        ui_text(&font_12_regular, UI_MARGIN, y + 28, det, sel ? t->bg : t->dim, bg);
    }
    char c[16];
    int n = fmt_u(c, d->event_count);
    n = add(c, n, " ");
    add(c, n, S_EVENT_COUNT);
    ui_footer(c, "[OK] " S_DETAILS);
}

/* ---- dose budget ---- */

static void scr_budget(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    char v[24], u[12];
    if (full) {
        ui_clear();
        ui_header(&spr_book, t->acc, S_T_BUDGET);
    }
    status(d);

    ui_label(&font_12_regular, UI_MARGIN, 66, 200, S_MISSION, t->dim, t->bg, 0);
    dose_total_unit_t du;
    fmt_dose_total(v, d->budget_used_nsv, &du);
    int n = add(u, 0, "/ ");
    dose_total_unit_t bu;
    char bv[16];
    fmt_dose_total(bv, d->budget_nsv, &bu);
    n = add(u, n, bv);
    n = add(u, n, " ");
    add(u, n, bu == DOSE_MSV ? S_MSV : bu == DOSE_USV ? S_USV : S_NSV);
    display_fill_rect(UI_MARGIN, 84, 216, 40, t->bg);
    font_draw(&font_34_medium, UI_MARGIN, 82, v, t->ink, t->bg, 0);
    font_draw(&font_14_medium, UI_MARGIN + font_width(&font_34_medium, v, 0) + 8, 100, u, t->dim, t->bg, 0);

    uint32_t pct = d->budget_nsv ? (uint32_t)((uint64_t)d->budget_used_nsv * 100u / d->budget_nsv) : 0;
    int lit = (int)(pct * 20u / 100u);
    ui_meter(UI_MARGIN, 134, 216, lit > 20 ? 20 : lit, true, 0);
    char p[24];
    n = fmt_u(p, pct);
    p[n++] = '%';
    p[n++] = ' ';
    add(p, n, S_USED_PCT);
    ui_label(&font_12_regular, UI_MARGIN, 152, 216, p, t->dim, t->bg, 0);

    const char *labels[4] = {S_START, S_DURATION, S_SESS_AVG, S_LEFT};
    char vals[4][20];
    fmt_clock(vals[0], d->budget_start_time, d->tz_offset_s);
    fmt_duration(vals[1], d->now - d->budget_start_time);
    {
        dose_unit_t ru;
        int k = fmt_dose_rate(vals[2], d->budget_avg_nsvh, &ru);
        add(vals[2], k, ru == UNIT_MSVH ? " mSv/h" : " µSv/h");
    }
    {
        uint32_t left = d->budget_nsv > d->budget_used_nsv ? d->budget_nsv - d->budget_used_nsv : 0;
        dose_total_unit_t lu;
        int k = fmt_dose_total(vals[3], left, &lu);
        add(vals[3], k, lu == DOSE_MSV ? " mSv" : lu == DOSE_USV ? " µSv" : " nSv");
    }
    for (int i = 0; i < 4; i++) {
        int y = 186 + i * 24;
        display_fill_rect(UI_MARGIN, y, 216, 20, t->bg);
        ui_text(&font_14_medium, UI_MARGIN, y + 1, labels[i], t->dim, t->bg);
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 1, vals[i], t->ink, t->bg);
    }
    ui_footer(S_ON_EXCEED, "[OK] " S_NEW_BUDGET);
}

/* ---- sample test ---- */

static void scr_sample(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    char v[24], u[12];
    if (full) {
        ui_clear();
        ui_header(&spr_flask, t->warn, S_T_SAMPLE);
    }
    status(d);

    const char *names[2] = {S_BACKGROUND, S_SAMPLE};
    uint32_t vals[2] = {d->sample_bg_nsvh, d->sample_nsvh};
    for (int i = 0; i < 2; i++) {
        int y = 68 + i * 30;
        display_fill_rect(UI_MARGIN, y, 216, 26, t->bg);
        char num[2] = {(char)('1' + i), 0};
        ui_chip(UI_MARGIN, y + 3, num, i == 0 ? t->ok : t->warn);
        ui_text(&font_14_medium, UI_MARGIN + 34, y + 3, names[i], t->ink, t->bg);
        dose_unit_t du;
        int k = fmt_dose_rate(v, vals[i], &du);
        add(v, k, " µSv/h");
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 3, v, t->ink, t->bg);
    }

    ui_label(&font_12_regular, UI_MARGIN, 134, 216, S_NET_DIFF, t->dim, t->bg, 0);
    dose_unit_t nu;
    int32_t net = d->sample_net_nsvh;
    int k = 0;
    if (net >= 0)
        v[k++] = '+';
    else
        v[k++] = '-';
    k += fmt_dose_rate(v + k, (uint32_t)(net < 0 ? -net : net), &nu);
    (void)u;
    display_fill_rect(UI_MARGIN, 150, 216, 42, t->bg);
    font_draw(&font_34_medium, UI_MARGIN, 148, v, net > 100 ? t->hot : t->ink, t->bg, 0);
    font_draw(&font_14_medium, UI_MARGIN + font_width(&font_34_medium, v, 0) + 8, 168, S_USVH, t->dim, t->bg, 0);

    int lit = d->sample_goal_s ? (int)((uint64_t)d->sample_elapsed_s * 20u / d->sample_goal_s) : 0;
    ui_meter(UI_MARGIN, 204, 216, lit > 20 ? 20 : lit, false, t->warn);
    char tm[40];
    int n = add(tm, 0, S_MEASURE_TIME);
    n = add(tm, n, " ");
    fmt_duration(v, d->sample_elapsed_s);
    n = add(tm, n, v);
    n = add(tm, n, " · ");
    n = add(tm, n, S_GOAL);
    n = add(tm, n, " ");
    fmt_duration(v, d->sample_goal_s);
    add(tm, n, v);
    ui_label(&font_12_regular, UI_MARGIN, 222, 216, tm, t->dim, t->bg, 0);

    ui_panel(UI_MARGIN, 244, 216, 40, t->panel);
    ui_text(&font_12_regular, UI_MARGIN + 10, 249, S_RESULT, t->dim, t->panel);
    ui_text(&font_14_medium, UI_MARGIN + 10, 263, d->sample_net_nsvh > 50 ? S_HIGHER : S_SAME, t->ink, t->panel);
    ui_footer("[OK] " S_FINISH, "[<] " S_RESET);
}

/* ---- log profile and storage ---- */

static void scr_log(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_disk, t->acc, s->mode ? S_T_STORAGE : S_T_LOG);
    }
    status(d);
    char v[24];

    if (s->mode == 0) {
        const char *names[5] = {S_P_DETAIL, S_P_DENSE, S_P_BALANCED, S_P_LONG, S_P_ARCHIVE};
        const char *res[5] = L_PROFILE_STEPS;
        for (int i = 0; i < 5; i++) {
            int y = 62 + i * 32;
            bool sel = i == s->sel;
            uint16_t bg = sel ? t->acc : t->bg;
            uint16_t fg = sel ? t->bg : t->ink;
            display_fill_rect(0, y, UI_W, 30, bg);
            ui_text(&font_14_medium, UI_MARGIN, y + 6, names[i], fg, bg);
            ui_text(&font_12_regular, UI_MARGIN + 100, y + 8, res[i], sel ? t->bg : t->dim, bg);
            uint32_t days = d->profile_days[i];
            int n;
            if (days >= 730) { n = fmt_u(v, days / 365); n = add(v, n, " "); add(v, n, S_YEARS); }
            else if (days >= 60) { n = fmt_u(v, days / 30); n = add(v, n, " "); add(v, n, S_MONTHS); }
            else { n = fmt_u(v, days); n = add(v, n, " "); add(v, n, S_DAYS); }
            ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 6, v, fg, bg);
        }
        display_fill_rect(UI_MARGIN, 228, 216, 40, t->bg);
        ui_panel(UI_MARGIN, 228, 216, 40, t->panel);
        ui_text_wrap(&font_12_regular, UI_MARGIN + 10, 237, 196, S_AUTO_NOTE, t->dim, t->panel, 0);
        ui_footer("[OK] " S_SELECT, S_WHEN_FULL ": " S_OVERWRITE);
    } else {
        int n = fmt_u(v, d->days_stored);
        display_fill_rect(UI_MARGIN, 66, 216, 44, t->bg);
        font_draw(&font_34_medium, UI_MARGIN, 66, v, t->ink, t->bg, 0);
        font_draw(&font_14_medium, UI_MARGIN + font_width(&font_34_medium, v, 0) + 8, 84, S_RECORDED, t->dim, t->bg, 0);
        (void)n;
        ui_meter(UI_MARGIN, 120, 216, d->storage_pct / 5, false, t->acc);
        char l[32];
        n = fmt_u(l, d->storage_pct);
        n = add(l, n, "% ");
        add(l, n, S_FULL_PCT);
        ui_label(&font_12_regular, UI_MARGIN, 138, 216, l, t->dim, t->bg, 0);

        const char *pn[5] = {S_P_DETAIL, S_P_DENSE, S_P_BALANCED, S_P_LONG, S_P_ARCHIVE};
        int pw = 104;
        ui_stat_panel(UI_MARGIN, 164, pw, S_PROFILE, pn[d->profile % 5], "");
        fmt_u(v, d->alarm_count);
        ui_stat_panel(UI_MARGIN + pw + 8, 164, pw, S_ALARM_LOG, v, "");

        char date[16];
        fmt_date(date, d->oldest_time, d->tz_offset_s);
        display_fill_rect(UI_MARGIN, 232, 216, 40, t->bg);
        ui_text(&font_14_medium, UI_MARGIN, 232, S_OLDEST, t->dim, t->bg);
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, 232, date, t->ink, t->bg);
        ui_text(&font_14_medium, UI_MARGIN, 256, S_WHEN_FULL, t->dim, t->bg);
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, 256, S_OVERWRITE, t->ink, t->bg);
        ui_footer("[OK] " S_T_LOG, S_USB_DOWNLOAD);
    }
}

/* ---- alarm thresholds ---- */

static void scr_thresh(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(0, 0, S_T_THRESH);
        ui_hazard(UI_W - UI_MARGIN - 34, UI_STATUS_H + 4, t->warn, 2);
    }
    status(d);
    const char *names[3] = {S_TH_ATTENTION, S_TH_DANGER, S_TH_EVACUATE};
    uint16_t cols[3] = {t->warn, t->hot, t->bad};
    for (int i = 0; i < 3; i++) {
        int y = 62 + i * 52;
        bool sel = i == s->sel;
        uint16_t bg = sel ? t->acc : t->bg;
        uint16_t fg = sel ? t->bg : t->ink;
        display_fill_rect(0, y, UI_W, 48, bg);
        display_fill_rect(UI_MARGIN, y + 8, 10, 10, sel ? t->bg : cols[i]);
        ui_text(&font_14_medium, UI_MARGIN + 18, y + 4, names[i], fg, bg);
        char v[24], num[16];
        dose_unit_t u;
        int n = 0;
        if (sel)
            n = add(v, n, "< ");
        fmt_dose_rate(num, d->th_nsvh[i], &u);
        n = add(v, n, num);
        n = add(v, n, " µSv/h");
        if (sel)
            add(v, n, " >");
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 4, v, fg, bg);
        ui_text(&font_12_regular, UI_MARGIN + 18, y + 28, S_SOUND_LIGHT, sel ? t->bg : t->dim, bg);
    }
    /* the zones on the scale: 20 blocks over the same range as the main meter */
    for (int i = 0; i < 20; i++) {
        uint16_t c = i < 10 ? t->ok : i < 15 ? t->warn : i < 18 ? t->hot : t->bad;
        int x0 = UI_MARGIN + (i * 218) / 20, x1 = UI_MARGIN + ((i + 1) * 218) / 20 - 2;
        display_fill_rect(x0, 232, x1 - x0, 14, c);
    }
    if (full) {
        int w = 216;
        ui_text(&font_12_regular, UI_MARGIN, 250, "0.1", t->dim, t->bg);
        ui_text(&font_12_regular, UI_MARGIN + w * 39 / 100 - 4, 250, "1", t->dim, t->bg);
        ui_text(&font_12_regular, UI_MARGIN + w * 70 / 100 - 8, 250, "10", t->dim, t->bg);
        ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 250, "100", t->dim, t->bg);
    }
    ui_footer("[<][>] " S_CHANGE, "[OK] " S_RESET);
}

/* ---- tube status ---- */

static void scr_tube(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_tube, t->acc, S_T_TUBE);
    }
    status(d);
    char v[24];
    const char *labels[5] = {S_HV, S_TOTAL_PULSES, S_RUNTIME, S_BG_RATE, S_DEADTIME};
    char vals[5][20];
    add(vals[0], 0, d->hv_factory ? S_HV_FACTORY : "—");
    {
        uint32_t p = d->total_pulses;
        int n;
        if (p >= 1000000) { n = fmt_fixed1(vals[1], p / 100000u); n = add(vals[1], n, S_MILLION); }
        else { n = fmt_u(vals[1], p); (void)n; }
    }
    {
        int n = fmt_u(vals[2], d->on_hours);
        add(vals[2], n, L_HOUR_UNIT);
    }
    {
        int n = fmt_u(vals[3], d->bg_cpm);
        add(vals[3], n, " CPM");
    }
    add(vals[4], 0, d->deadtime_on ? S_ON : S_OFF);
    for (int i = 0; i < 5; i++) {
        int y = 66 + i * 30;
        display_fill_rect(UI_MARGIN, y, 216, 24, t->bg);
        ui_text(&font_14_medium, UI_MARGIN, y + 2, labels[i], t->dim, t->bg);
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 2, vals[i], t->ink, t->bg);
    }
    ui_label(&font_12_regular, UI_MARGIN, 224, 216, S_TUBE_LIFE, t->dim, t->bg, 0);
    ui_meter(UI_MARGIN, 242, 216, d->tube_life / 5, false, t->ok);
    int n = fmt_u(v, d->tube_life);
    v[n++] = '%';
    v[n] = 0;
    ui_label(&font_12_regular, UI_MARGIN, 260, 216, v, t->dim, t->bg, 0);
    ui_footer("[OK] " S_T_SELFTEST, S_HV_PROTECTED);
}

/* ---- self test ---- */

static void scr_selftest(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_check, t->ok, S_T_SELFTEST);
    }
    status(d);
    const char *names[7] = {S_CHK_HV, S_CHK_TUBE, S_CHK_BAT, S_CHK_E, S_CHK_H, S_CHK_CLOCK, S_CHK_MEM};
    for (int i = 0; i < 7; i++) {
        int y = 66 + i * 28;
        display_fill_rect(UI_MARGIN, y, 216, 22, t->bg);
        if (i < d->selftest_done)
            ui_sprite(UI_MARGIN, y + 3, &spr_check, t->ok, 2);
        else if (i == d->selftest_done) /* always the same drawing operation, only its colour blinks */
            display_fill_rect(UI_MARGIN, y + 5, 14, 12, d->blink ? t->acc : t->bg);
        ui_text(&font_14_medium, UI_MARGIN + 26, y + 2, names[i], t->ink, t->bg);
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 2, i < d->selftest_done ? S_DONE : "…", t->dim, t->bg);
    }
    ui_meter(UI_MARGIN, 270 - 4, 216, d->selftest_total ? d->selftest_done * 20 / d->selftest_total : 0, false, t->ok);
    char n[16];
    int k = fmt_u(n, d->selftest_done);
    k = add(n, k, " / ");
    fmt_u(n + k, d->selftest_total);
    ui_footer(n, "[<] " S_CANCEL);
}

/* ---- usb ---- */

static void scr_usb(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_plug, t->acc, S_T_USB);
    }
    status(d);
    ui_text(&font_14_medium, UI_MARGIN, 70, "Yarrow Hub", t->ink, t->bg);
    display_fill_rect(UI_W - UI_MARGIN - 80, 66, 80, 22, t->bg);
    if (d->usb_connected)
        ui_chip(UI_W - UI_MARGIN - font_width(&font_12_medium, S_USB_STATE, 1) - 18, 68, S_USB_STATE, t->ok);

    const char *labels[4] = {S_STREAM, S_CHARGE_A, S_CLOCK_SYNC, S_RECORDS};
    char vals[4][20];
    add(vals[0], 0, d->usb_streaming ? S_ON : S_OFF);
    add(vals[1], 0, d->usb ? "1 A" : "—");
    add(vals[2], 0, d->usb_synced ? S_SYNCED : "—");
    {
        int n = fmt_u(vals[3], d->days_stored);
        n = add(vals[3], n, " ");
        add(vals[3], n, S_DAYS);
    }
    for (int i = 0; i < 4; i++) {
        int y = 100 + i * 30;
        display_fill_rect(UI_MARGIN, y, 216, 24, t->bg);
        ui_text(&font_14_medium, UI_MARGIN, y + 2, labels[i], t->dim, t->bg);
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 2, vals[i], t->ink, t->bg);
    }
    ui_panel(UI_MARGIN, 232, 216, 48, t->panel);
    ui_text(&font_12_regular, UI_MARGIN + 10, 238, S_TRANSFER, t->dim, t->panel);
    ui_meter(UI_MARGIN + 10, 256, 196, d->usb_synced ? 20 : 0, false, t->acc);
    ui_footer("[OK] " S_DOWNLOAD_NOW, S_AUTO_ON);
}

/* ---- scan (find a source) ---- */

static void scr_scan(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_signal, t->warn, S_T_SCAN);
    }
    status(d);
    display_fill_rect(UI_MARGIN, 66, 216, 132, t->bg);
    for (int i = 0; i < 10; i++) {
        int h = 14 + i * 11;
        uint16_t c = (uint32_t)i < d->scan_bars ? (i < 4 ? t->ok : i < 7 ? t->warn : t->bad) : t->line;
        display_fill_rect(UI_MARGIN + i * 22, 194 - h, 18, h, c);
    }
    char v[16];
    int n = fmt_u(v, (d->scan_value_x10 + 5) / 10);
    display_fill_rect(UI_MARGIN, 206, 216, 44, t->bg);
    font_draw(&font_34_medium, UI_MARGIN, 204, v, t->ink, t->bg, 0);
    font_draw(&font_14_medium, UI_MARGIN + font_width(&font_34_medium, v, 0) + 8, 224, S_VM, t->dim, t->bg, 0);
    (void)n;
    if (d->scan_rising)
        ui_text_right(&font_12_regular, UI_W - UI_MARGIN, 226, S_APPROACH, t->hot, t->bg);
    ui_text_wrap(&font_12_regular, UI_MARGIN, 258, 216, S_FIND_SRC, t->dim, t->bg, 0);
    ui_footer("[OK] " S_SOUND_ON, "[<] " S_EXIT);
}

/* ---- theme picker: every theme as a small live preview ---- */

static void scr_themes(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    static const char *const names[THEME_COUNT] = THEME_NAMES;
    if (full) {
        ui_clear();
        ui_header(&spr_flower, t->petal, S_T_THEMES);
    }
    status(d);

    const int cols = 4, tw = 51, th = 34, gx = 4, gy = 4;
    for (int i = 0; i < THEME_COUNT; i++) {
        int x = UI_MARGIN + (i % cols) * (tw + gx);
        int y = 60 + (i / cols) * (th + gy);
        const theme_t *p = &themes[i];
        bool sel = i == s->sel;
        display_group_begin(x, y, tw, th);
        display_fill_rect(x, y, tw, th, sel ? t->acc : t->bg);
        display_fill_rect(x + 2, y + 2, tw - 4, th - 4, p->bg);
        display_fill_rect(x + 5, y + 5, 24, 3, p->ink);
        display_fill_rect(x + 5, y + 10, 16, 2, p->dim);
        display_fill_rect(x + tw - 14, y + 5, 8, 8, p->acc);
        display_fill_rect(x + 5, y + 16, tw - 10, 8, p->panel);
        display_fill_rect(x + 8, y + 18, 5, 4, p->ok);
        display_fill_rect(x + 15, y + 18, 5, 4, p->warn);
        display_fill_rect(x + 22, y + 18, 5, 4, p->bad);
        display_fill_rect(x + 5, y + 27, tw - 10, 2, p->line);
        display_group_end();
    }
    ui_footer("[<][>] " S_SELECT, "[OK] " S_DONE);
    ui_label(&font_14_medium, UI_MARGIN, 254, 216, names[s->sel % THEME_COUNT], t->ink, t->bg, 2);
}

/* ---- about: the flower, name and links ---- */

static void scr_about(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_flower, t->petal, S_T_ABOUT);
        ui_sprite_multi(82, 52, &spr_flower, 5);
        for (int x = 0; x < UI_W; x += 8)
            display_fill_rect(x, 152, 4, 4, t->dim);
        font_draw_centered(&font_28_medium, 168, "YARROW", t->ink, t->bg, 6);
        font_draw_centered(&font_12_regular, 206, "GC-03 · v0.1", t->dim, t->bg, 0);
        font_draw_centered(&font_14_medium, 228, "yarrow.athena.org.tr", t->acc, t->bg, 0);
        font_draw_centered(&font_12_regular, 248, "github.com/alplix/yarrow", t->dim, t->bg, 0);
        font_draw_centered(&font_12_regular, 266, S_OPEN_SOURCE, t->dim, t->bg, 0);
    }
    status(d);
    ui_footer("[<] " S_BACK, 0);
}

/* ---- clock and date editor ---- */

static void scr_clock(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_gear, t->acc, S_T_CLOCK);
    }
    status(d);
    const char *labels[5] = {S_DAY, S_MONTH, S_YEAR, S_HOUR, S_MINUTE};
    /* d->clk is year, month, day, hour, minute; the editor shows the day first */
    const int order[5] = {2, 1, 0, 3, 4};
    for (int i = 0; i < 5; i++) {
        int y = 66 + i * 36;
        bool sel = i == s->sel;
        uint16_t bg = sel ? t->acc : t->bg;
        uint16_t fg = sel ? t->bg : t->ink;
        display_fill_rect(0, y, UI_W, 32, bg);
        ui_text(&font_14_medium, UI_MARGIN, y + 7, labels[i], fg, bg);
        char v[12];
        fmt_u(v, d->clk[order[i]]);
        if (order[i] >= 3 && d->clk[order[i]] < 10) {
            v[1] = v[0];
            v[0] = '0';
            v[2] = 0;
        }
        char shown[20];
        int k = 0;
        if (sel)
            k = add(shown, k, "< ");
        k = add(shown, k, v);
        if (sel)
            add(shown, k, " >");
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 7, shown, fg, bg);
    }
    ui_footer("[<][>] " S_CHANGE, s->sel == 4 ? "[OK] " S_SAVE : "[OK] " S_NEXT);
}

/* ---- calibration ---- */

static void scr_calib(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    if (full) {
        ui_clear();
        ui_header(&spr_tube, t->warn, S_T_CALIB);
    }
    status(d);
    const char *labels[4] = {S_CAL_SENS, S_CAL_DEAD, S_CAL_E, S_CAL_H};
    for (int i = 0; i < 4; i++) {
        int y = 66 + i * 38;
        bool sel = i == s->sel;
        uint16_t bg = sel ? t->acc : t->bg;
        uint16_t fg = sel ? t->bg : t->ink;
        display_fill_rect(0, y, UI_W, 34, bg);
        ui_text(&font_14_medium, UI_MARGIN, y + 8, labels[i], fg, bg);
        char v[24];
        int n = 0;
        if (sel && s->mode)
            n = add(v, n, "< ");
        if (i == 0) {
            n += fmt_fixed2(v + n, d->cal[0]);
            n = add(v, n, " CPM");
        } else if (i == 1) {
            n += fmt_u(v + n, d->cal[1]);
            n = add(v, n, " µs");
        } else {
            n += fmt_u(v + n, d->cal[i]);
            n = add(v, n, " %");
        }
        if (sel && s->mode)
            add(v, n, " >");
        ui_text_right(&font_14_medium, UI_W - UI_MARGIN, y + 8, v, fg, bg);
    }
    if (full)
        ui_text_wrap(&font_12_regular, UI_MARGIN, 228, 216, S_CAL_NOTE, t->dim, t->bg, 2);
    ui_footer(s->mode ? "[<][>] " S_CHANGE : "[<] " S_BACK, s->mode ? "[OK] " S_SAVE : "[OK] " S_CHANGE);
}

/* ---- charging: a big pixel battery that fills up ---- */

static void scr_charge(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    if (full)
        ui_clear();

    const int bx = 36, by = 96, bw = 168, bh = 84; /* body */
    const int seg_n = 8;
    bool charging = d->bat_state == 1;
    uint16_t col = d->bat_percent < 15 ? t->bad : d->bat_percent < 35 ? t->warn : t->ok;

    /* outline, cap and the eight segments */
    display_fill_rect(bx, by, bw, bh, t->dim);
    display_fill_rect(bx + bw, by + 24, 10, 36, t->dim);
    display_fill_rect(bx + 6, by + 6, bw - 12, bh - 12, t->bg);
    int lit = (d->bat_percent * seg_n + 50) / 100;
    int sw = (bw - 12 - 6 - (seg_n - 1) * 4) / seg_n;
    for (int i = 0; i < seg_n; i++) {
        bool on = i < lit;
        /* while charging the first empty segment blinks, like a heartbeat */
        if (charging && i == lit && (d->anim & 2))
            on = true;
        display_fill_rect(bx + 9 + i * (sw + 4), by + 9, sw, bh - 18, on ? col : t->line);
    }

    /* the lightning bolt hops up and down while charging */
    int hop = charging ? ((d->anim & 1) ? -3 : 0) : 0;
    display_fill_rect(bx + bw / 2 - 17, by + 14 + hop - 1, 36, 60, charging ? t->bg : t->bg);
    ui_sprite(bx + bw / 2 - 15, by + 14 + hop, &spr_bolt, charging ? t->warn : t->dim, 5);

    char buf[16];
    int n = fmt_u(buf, d->bat_percent);
    buf[n++] = '%';
    buf[n] = 0;
    ui_label(&font_34_medium, 0, 200, UI_W, buf, t->ink, t->bg, 2);
    ui_label(&font_14_medium, 0, 248, UI_W, charging ? S_CHARGING : S_FULL_BAT, charging ? t->warn : t->ok, t->bg, 2);
    if (full)
        ui_text_center(&font_12_regular, UI_W / 2, 290, S_PRESS_ANY, t->dim, t->bg);
}

/* ---- calibration wizard and sessions ---- */

static const char *rate_unit(dose_unit_t u) { return u == UNIT_NSVH ? S_NSVH : u == UNIT_MSVH ? S_MSVH : S_USVH; }
static const char *dose_unit_name(dose_total_unit_t u) { return u == DOSE_NSV ? S_NSV : u == DOSE_MSV ? S_MSV : S_USV; }

static void rate_text(char *out, uint32_t nsvh)
{
    dose_unit_t u;
    int n = fmt_dose_rate(out, nsvh, &u);
    out[n++] = ' ';
    add(out, n, rate_unit(u));
}

static void dose_text(char *out, uint64_t nsv)
{
    dose_total_unit_t u;
    int n = fmt_dose_total(out, nsv, &u);
    out[n++] = ' ';
    add(out, n, dose_unit_name(u));
}

/* a big number with a small unit next to it, centred */
static void big_number(int y, const char *num, const char *unit, uint16_t color)
{
    const theme_t *t = ui_theme;
    int w1 = font_width(&font_34_medium, num, 0), w2 = font_width(&font_14_medium, unit, 0);
    int x = (UI_W - (w1 + 8 + w2)) / 2;
    if (x < 0)
        x = 0;
    display_fill_rect(0, y, UI_W, 44, t->bg);
    font_draw(&font_34_medium, x, y, num, color, t->bg, 0);
    font_draw(&font_14_medium, x + w1 + 8, y + 20, unit, t->dim, t->bg, 0);
}

static void natural_text(char *out, uint64_t nsv)
{
    uint32_t x10 = (uint32_t)(nsv * 10u / 6600u); /* natural background is about 6.6 uSv a day */
    if (x10 == 0) {
        add(out, 0, S_NATURAL_LESS);
        return;
    }
    int n = add(out, 0, S_APPROX);
    n += fmt_fixed1(out + n, x10);
    out[n++] = ' ';
    add(out, n, S_NATURAL_DAYS);
}

static void scr_calwiz(const ui_data_t *d, const ui_state_t *s, bool full)
{
    (void)s;
    const theme_t *t = ui_theme;
    char v[48];
    if (full) {
        ui_clear();
        ui_header(&spr_atom, t->warn, S_T_CALIB);
    }
    status(d);
    int ph = d->cw_phase > 2 ? 2 : d->cw_phase;
    static const char *const titles[3] = {S_CW_T0, S_CW_T1, S_CW_T2};
    char step[4] = {(char)('1' + ph), '/', '3', 0};
    display_fill_rect(UI_MARGIN, 60, 216, 24, t->bg);
    int cw = ui_chip(UI_MARGIN, 62, step, t->acc);
    ui_text(&font_14_medium, UI_MARGIN + cw + 8, 63, titles[ph], t->ink, t->bg);

    if (ph == 0) {
        char num[16];
        dose_unit_t u;
        fmt_dose_rate(num, d->cw_ref_nsvh, &u);
        big_number(98, num, rate_unit(u), t->ink);
        ui_text_wrap(&font_12_regular, UI_MARGIN, 160, 216, S_CW_HELP0, t->dim, t->bg, 3);
        ui_footer("[<][>] " S_CHANGE, "[OK] " S_BEGIN);
    } else if (ph == 1) {
        fmt_u(v, d->cw_counts);
        big_number(98, v, S_CW_COUNTS, t->ink);
        int n = fmt_duration(v, d->cw_seconds);
        n = add(v, n, " · ±");
        n += fmt_fixed1(v + n, d->cw_unc_x10);
        add(v, n, " %");
        ui_label(&font_14_medium, UI_MARGIN, 148, 216, v, t->ink, t->bg, 2);
        int lit = (int)(d->cw_counts > 1600 ? 20 : d->cw_counts * 20 / 1600);
        ui_meter(UI_MARGIN, 176, 216, lit, false, t->warn);
        display_fill_rect(UI_MARGIN, 198, 216, 52, t->bg);
        const char *msg = d->cw_saturated ? S_CW_SAT : d->cw_counts < 100 ? S_CW_NEEDMORE : S_CW_HINT1;
        ui_text_wrap(&font_12_regular, UI_MARGIN, 202, 216, msg, d->cw_saturated ? t->bad : t->dim, t->bg, 3);
        ui_footer("[<] " S_CANCEL, "[OK] " S_FINISH);
    } else {
        int n = fmt_fixed2(v, d->cal[0]);
        add(v, n, " CPM");
        ui_row(92, S_CW_OLD, v, false);
        n = fmt_fixed2(v, d->cw_new_x100);
        add(v, n, " CPM");
        ui_row(124, S_CW_NEW, v, false);
        int64_t old = d->cal[0] ? d->cal[0] : 1;
        int64_t diff_x10 = ((int64_t)d->cw_new_x100 - old) * 1000 / old;
        n = 0;
        v[n++] = diff_x10 < 0 ? '-' : '+';
        n += fmt_fixed1(v + n, (uint32_t)(diff_x10 < 0 ? -diff_x10 : diff_x10));
        add(v, n, " %");
        ui_row(156, S_CW_DIFF, v, false);
        n = add(v, 0, "±");
        n += fmt_fixed1(v + n, d->cw_unc_x10);
        add(v, n, " %");
        ui_row(188, S_CW_UNC, v, false);
        display_fill_rect(UI_MARGIN, 224, 216, 24, t->bg);
        if (d->cw_new_x100 < 1000 || d->cw_new_x100 > 100000)
            ui_text_wrap(&font_12_regular, UI_MARGIN, 228, 216, S_CW_RANGE, t->bad, t->bg, 3);
        ui_footer("[<] " S_DISCARD, "[OK] " S_APPLY);
    }
}

static void scr_session(const ui_data_t *d, const ui_state_t *s, bool full)
{
    const theme_t *t = ui_theme;
    static const char *const kinds[SESS_KINDS] = SESS_KINDS_NAMES;
    char v[48];
    if (full) {
        ui_clear();
        ui_header(&spr_signal, t->acc, S_T_SESSIONS);
    }
    status(d);

    if (s->mode == 0) {
        int rows = 1 + d->sess_saved;
        for (int i = 0; i < 5; i++) {
            int idx = s->scroll + i;
            int y = 60 + i * 32;
            if (idx >= rows) {
                display_fill_rect(0, y, UI_W, 30, t->bg);
                continue;
            }
            if (idx == 0) {
                ui_row(y, d->sess_active ? S_SESS_RUNNING : S_SESS_NEW, "", idx == s->sel);
            } else {
                const ui_session_t *e = &d->sess_list[idx - 1];
                dose_text(v, e->dose_nsv);
                ui_row(y, kinds[e->kind % SESS_KINDS], v, idx == s->sel);
            }
        }
        display_fill_rect(UI_MARGIN, 226, 216, 58, t->bg);
        if (d->sess_saved == 0)
            ui_text_wrap(&font_12_regular, UI_MARGIN, 232, 216, S_SESS_HINT, t->dim, t->bg, 3);
        ui_footer("[<] " S_BACK, "[OK] " S_OPEN);
    } else if (s->mode == 1) {
        ui_label(&font_14_medium, UI_MARGIN, 58, 216, S_SESS_PICK, t->dim, t->bg, 0);
        for (int i = 0; i < SESS_KINDS; i++)
            ui_row(80 + i * 32, kinds[i], "", i == s->sel);
        ui_footer("[<] " S_BACK, "[OK] " S_BEGIN);
    } else if (s->mode == 2) {
        ui_label(&font_14_medium, UI_MARGIN, 58, 216, kinds[d->sess_kind % SESS_KINDS], t->acc, t->bg, 0);
        dose_total_unit_t du;
        char num[16];
        fmt_dose_total(num, d->sess_dose_nsv, &du);
        big_number(82, num, dose_unit_name(du), t->ink);
        fmt_duration(v, d->sess_seconds);
        ui_row(132, S_SESS_TIME, v, false);
        uint32_t avg = d->sess_seconds ? (uint32_t)(d->sess_dose_nsv * 3600u / d->sess_seconds) : 0;
        rate_text(v, avg);
        ui_row(162, S_SESS_AVG, v, false);
        rate_text(v, d->sess_max_nsvh);
        ui_row(192, S_SESS_MAX, v, false);
        rate_text(v, d->rate_nsvh);
        ui_row(222, S_SESS_NOW, v, false);
        display_fill_rect(UI_MARGIN, 258, 216, 20, t->bg);
        if (s->page) {
            ui_text(&font_12_regular, UI_MARGIN, 262, S_SESS_ASK, t->warn, t->bg);
        } else {
            natural_text(v, d->sess_dose_nsv);
            ui_text(&font_12_regular, UI_MARGIN, 262, v, t->dim, t->bg);
        }
        ui_footer("[<] " S_BACK, "[OK] " S_FINISH);
    } else if (d->sess_saved) {
        const ui_session_t *e = &d->sess_list[s->page % d->sess_saved];
        ui_label(&font_14_medium, UI_MARGIN, 58, 216, kinds[e->kind % SESS_KINDS], t->acc, t->bg, 0);
        fmt_date_time(v, e->start, d->tz_offset_s);
        ui_label(&font_12_regular, UI_MARGIN, 78, 216, v, t->dim, t->bg, 0);
        dose_total_unit_t du;
        char num[16];
        fmt_dose_total(num, e->dose_nsv, &du);
        big_number(98, num, dose_unit_name(du), t->ink);
        fmt_duration(v, e->seconds);
        ui_row(148, S_SESS_TIME, v, false);
        uint32_t avg = e->seconds ? (uint32_t)((uint64_t)e->dose_nsv * 3600u / e->seconds) : 0;
        rate_text(v, avg);
        ui_row(178, S_SESS_AVG, v, false);
        rate_text(v, e->max_nsvh);
        ui_row(208, S_SESS_MAX, v, false);
        natural_text(v, e->dose_nsv);
        ui_label(&font_12_regular, UI_MARGIN, 248, 216, v, t->dim, t->bg, 0);
        ui_footer("[<][>] " S_BROWSE, "[OK] " S_BACK);
    }
}

void screens2_draw(screen_id_t id, const ui_data_t *d, const ui_state_t *s, bool full)
{
    switch (id) {
    case SCR_GUIDE_LIST: scr_guide_list(d, s, full); break;
    case SCR_GUIDE_PAGE: scr_guide_page(d, s, full); break;
    case SCR_SETTINGS:   scr_settings(d, s, full); break;
    case SCR_EVENTS:     scr_events(d, s, full); break;
    case SCR_BUDGET:     scr_budget(d, s, full); break;
    case SCR_SAMPLE:     scr_sample(d, s, full); break;
    case SCR_LOG:        scr_log(d, s, full); break;
    case SCR_THRESH:     scr_thresh(d, s, full); break;
    case SCR_TUBE:       scr_tube(d, s, full); break;
    case SCR_SELFTEST:   scr_selftest(d, s, full); break;
    case SCR_USB:        scr_usb(d, s, full); break;
    case SCR_SCAN:       scr_scan(d, s, full); break;
    case SCR_THEMES:     scr_themes(d, s, full); break;
    case SCR_ABOUT:      scr_about(d, s, full); break;
    case SCR_CLOCK:      scr_clock(d, s, full); break;
    case SCR_CALIB:      scr_calib(d, s, full); break;
    case SCR_CHARGE:     scr_charge(d, s, full); break;
    case SCR_CALWIZ:     scr_calwiz(d, s, full); break;
    case SCR_SESSION:    scr_session(d, s, full); break;
    default: break;
    }
}
