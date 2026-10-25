#include "proto.h"
#include "fmt.h"
#include <string.h>

static void out(const proto_t *p, const char *s)
{
    uint32_t n = 0;
    while (s[n])
        n++;
    if (p->app->hal->serial_write)
        p->app->hal->serial_write((const uint8_t *)s, n);
}

static int put_u(char *b, int n, uint32_t v) { return n + fmt_u(b + n, v); }
static int put_s(char *b, int n, const char *s) { while (*s) b[n++] = *s++; b[n] = 0; return n; }

/* parses an unsigned number at *s, advances past it and following spaces; false if there is none */
static bool take_u(const char **s, uint32_t *v)
{
    const char *p = *s;
    if (*p < '0' || *p > '9')
        return false;
    uint64_t x = 0;
    while (*p >= '0' && *p <= '9') {
        x = x * 10 + (uint64_t)(*p - '0');
        if (x > 0xFFFFFFFFu)
            return false;
        p++;
    }
    while (*p == ' ')
        p++;
    *s = p;
    *v = (uint32_t)x;
    return true;
}

static void send_base64(const proto_t *p, const uint8_t *d, uint32_t n)
{
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char line[80];
    int k = 0;
    for (uint32_t i = 0; i < n; i += 3) {
        uint32_t v = (uint32_t)d[i] << 16 | (i + 1 < n ? (uint32_t)d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
        line[k++] = tbl[(v >> 18) & 63];
        line[k++] = tbl[(v >> 12) & 63];
        line[k++] = i + 1 < n ? tbl[(v >> 6) & 63] : '=';
        line[k++] = i + 2 < n ? tbl[v & 63] : '=';
        if (k >= 76) {
            line[k++] = '\n';
            line[k] = 0;
            out(p, line);
            k = 0;
        }
    }
    if (k) {
        line[k++] = '\n';
        line[k] = 0;
        out(p, line);
    }
}

static void cmd_info(proto_t *p)
{
    app_t *a = p->app;
    char b[160];
    int n = put_s(b, 0, "OK yarrow=0.1.2 stage=");
    n = put_u(b, n, a->stage);
    n = put_s(b, n, " verified=");
    n = put_u(b, n, a->verified);
    n = put_s(b, n, " time=");
    n = put_u(b, n, a->hal->rtc_now());
    n = put_s(b, n, " theme=");
    n = put_u(b, n, a->settings.theme);
    n = put_s(b, n, " profile=");
    n = put_u(b, n, a->settings.profile);
    n = put_s(b, n, " pages=");
    n = put_u(b, n, a->hal->log_flash ? a->hal->log_flash->page_count : 0);
    n = put_s(b, n, " pagesize=");
    n = put_u(b, n, a->hal->log_flash ? a->hal->log_flash->page_size : 0);
    put_s(b, n, "\n");
    out(p, b);
}

/* FNV-1a 64 over the 96-bit chip id: the same id always gives the same 16 hex digits, the id itself cannot be read back */
static void cmd_uid(proto_t *p)
{
    uint32_t id[3] = {0, 0, 0};
    if (!p->app->hal->chip_id) {
        out(p, "ERR unsupported\n");
        return;
    }
    p->app->hal->chip_id(id);
    uint64_t h = 0xcbf29ce484222325ull;
    for (int i = 0; i < 12; i++) {
        h ^= (uint8_t)(id[i / 4] >> (8 * (i % 4)));
        h *= 0x100000001b3ull;
    }
    char b[32] = "OK ";
    for (int i = 0; i < 16; i++)
        b[3 + i] = "0123456789abcdef"[(h >> (60 - 4 * i)) & 15];
    b[19] = '\n';
    b[20] = 0;
    out(p, b);
}

#ifndef YARROW_LANG_CODE
#define YARROW_LANG_CODE "xx"
#endif
#define PROTO_VERSION 2

/* "S <group> key=value key=value ...": a group continues on the next line (same group name) when a line is full */
typedef struct {
    const proto_t *p;
    const char *group;
    char b[96];
    int n;
} sl_t;

static void sl_begin(sl_t *l, const proto_t *p, const char *group)
{
    l->p = p;
    l->group = group;
    l->n = put_s(l->b, 0, "S ");
    l->n = put_s(l->b, l->n, group);
}

static void sl_end(sl_t *l)
{
    if (l->n > 0) {
        put_s(l->b, l->n, "\n");
        out(l->p, l->b);
    }
    l->n = 0;
}

static void sl_key(sl_t *l, const char *k)
{
    int klen = 0;
    while (k[klen])
        klen++;
    if (l->n + 1 + klen + 1 + 11 > 94) {            /* the longest value is a signed 32-bit number */
        sl_end(l);
        l->n = put_s(l->b, 0, "S ");
        l->n = put_s(l->b, l->n, l->group);
    }
    l->n = put_s(l->b, l->n, " ");
    l->n = put_s(l->b, l->n, k);
    l->n = put_s(l->b, l->n, "=");
}

static void sl_u(sl_t *l, const char *k, uint32_t v)
{
    sl_key(l, k);
    l->n = put_u(l->b, l->n, v);
}

static void sl_i(sl_t *l, const char *k, int32_t v)
{
    sl_key(l, k);
    if (v < 0) {
        l->n = put_s(l->b, l->n, "-");
        v = -v;
    }
    l->n = put_u(l->b, l->n, (uint32_t)v);
}

static void sl_s(sl_t *l, const char *k, const char *v)
{
    sl_key(l, k);
    l->n = put_s(l->b, l->n, v);
}

static uint32_t sat32(uint64_t v) { return v > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)v; }

/* Everything the device knows, in groups. The Hub and other hosts read this instead of scraping screens. */
static void cmd_status(proto_t *p)
{
    app_t *a = p->app;
    const ui_data_t *d = &a->ui;
    sl_t l;
    out(p, "OK 10\n");
    sl_begin(&l, p, "radiation");
    sl_u(&l, "rate_nsvh", d->rate_nsvh);
    sl_u(&l, "cpm", d->cpm);
    sl_u(&l, "level", d->level);
    sl_u(&l, "bg_nsvh", d->bg_nsvh);
    sl_u(&l, "dose_today_nsv", sat32(d->dose_today_nsv));
    sl_u(&l, "dose_week_nsv", sat32(d->dose_week_nsv));
    sl_u(&l, "dose_total_nsv", sat32(d->dose_total_nsv));
    sl_end(&l);
    sl_begin(&l, p, "tube");
    sl_u(&l, "pulses", a->hal->tube_pulses());
    sl_u(&l, "hv", a->hv_on);
    sl_u(&l, "hv_s", a->hv_secs);
    sl_u(&l, "total_pulses", d->total_pulses);
    sl_u(&l, "bg_cpm", d->bg_cpm);
    sl_u(&l, "deadtime", d->deadtime_on);
    sl_u(&l, "factory", d->hv_factory);
    sl_u(&l, "life", d->tube_life);
    sl_end(&l);
    sl_begin(&l, p, "fields");
    sl_u(&l, "e_vm_x10", d->e_vm_x10);
    sl_u(&l, "e_avg_x10", d->e_avg_x10);
    sl_u(&l, "e_peak_x10", d->e_peak_x10);
    sl_u(&l, "h_ut_x100", d->h_ut_x100);
    sl_u(&l, "h_avg_x100", d->h_avg_x100);
    sl_u(&l, "h_peak_x100", d->h_peak_x100);
    sl_end(&l);
    sl_begin(&l, p, "battery");
    sl_u(&l, "mv", a->hal->battery_mv());
    sl_u(&l, "pct", d->bat_percent);
    sl_u(&l, "state", d->bat_state);
    sl_u(&l, "minutes", d->bat_minutes);
    sl_u(&l, "health", d->bat_health);
    sl_u(&l, "cycles", d->bat_cycles);
    sl_u(&l, "cable", d->usb);
    sl_end(&l);
    sl_begin(&l, p, "clock");
    sl_u(&l, "now", a->hal->rtc_now());
    sl_i(&l, "tz_s", d->tz_offset_s);
    sl_u(&l, "uptime_s", a->seconds_run);
    sl_u(&l, "on_hours", d->on_hours);
    sl_end(&l);
    sl_begin(&l, p, "log");
    sl_u(&l, "days", d->days_stored);
    sl_u(&l, "storage_pct", d->storage_pct);
    sl_u(&l, "oldest", d->oldest_time);
    sl_u(&l, "profile", d->profile);
    sl_u(&l, "alarms", d->alarm_count);
    sl_u(&l, "events", d->event_count);
    sl_end(&l);
    sl_begin(&l, p, "settings");
    sl_u(&l, "theme", d->theme);
    sl_u(&l, "brightness", d->brightness);
    sl_u(&l, "sound", d->sound);
    sl_u(&l, "vibration", d->vibration);
    sl_u(&l, "pulse_led", d->led_pulse);
    sl_u(&l, "charge_led", d->led_charge);
    sl_u(&l, "click", d->click);
    sl_u(&l, "count_vibe", d->count_vibe);
    sl_u(&l, "led_color", d->led_color);
    sl_u(&l, "usb_start", d->usb_start);
    sl_end(&l);
    sl_begin(&l, p, "thresholds");
    sl_u(&l, "attention_nsvh", d->th_nsvh[0]);
    sl_u(&l, "danger_nsvh", d->th_nsvh[1]);
    sl_u(&l, "evacuate_nsvh", d->th_nsvh[2]);
    sl_u(&l, "budget_active", d->budget_active);
    sl_u(&l, "budget_nsv", d->budget_nsv);
    sl_u(&l, "budget_used_nsv", d->budget_used_nsv);
    sl_end(&l);
    sl_begin(&l, p, "session");
    sl_u(&l, "active", d->sess_active);
    sl_u(&l, "kind", d->sess_kind);
    sl_u(&l, "start", d->sess_start);
    sl_u(&l, "seconds", d->sess_seconds);
    sl_u(&l, "dose_nsv", sat32(d->sess_dose_nsv));
    sl_end(&l);
    sl_begin(&l, p, "device");
    sl_s(&l, "lang", YARROW_LANG_CODE);
    sl_u(&l, "ms_per_s", a->ms_per_s);                 /* 1000 when the core clock is as assumed; 0 until measured */
    sl_u(&l, "stage", a->stage);
    sl_u(&l, "verified", a->verified);
    sl_u(&l, "suspect", a->suspect_mask);
    sl_u(&l, "selftest_fail", d->selftest_fail);
    sl_u(&l, "sens_x100", d->cal[0]);
    sl_u(&l, "dead_us", d->cal[1]);
    sl_u(&l, "e_scale", d->cal[2]);
    sl_u(&l, "h_scale", d->cal[3]);
    sl_end(&l);
    out(p, "END\n");
}

/* What this firmware understands, so a host can adapt to old and new devices */
static void cmd_caps(proto_t *p)
{
    char b[48];
    int n = put_s(b, 0, "OK proto=");
    n = put_u(b, n, PROTO_VERSION);
    put_s(b, n, "\n");
    out(p, b);
    out(p, "C PING INFO TIME LIVE LOGINFO LOGPAGE EVENTS CAL SET\n");
    out(p, "C COUNTS UID BAT STATUS CAPS\n");
    out(p, "END\n");
}

static void cmd_loginfo(proto_t *p)
{
    app_t *a = p->app;
    if (!a->hal->log_flash) {
        out(p, "ERR no log\n");
        return;
    }
    ylog_stats_t st;
    ylog_stats(a->hal->log_flash, app_scratch(), &st);
    char b[160];
    int n = put_s(b, 0, "OK pages=");
    n = put_u(b, n, a->hal->log_flash->page_count);
    n = put_s(b, n, " pagesize=");
    n = put_u(b, n, a->hal->log_flash->page_size);
    n = put_s(b, n, " used=");
    n = put_u(b, n, st.pages_used);
    n = put_s(b, n, " oldest=");
    n = put_u(b, n, st.oldest_time);
    n = put_s(b, n, " newest=");
    n = put_u(b, n, st.newest_time);
    put_s(b, n, "\n");
    out(p, b);
}

static void cmd_logpage(proto_t *p, uint32_t page)
{
    const ylog_flash_t *fl = p->app->hal->log_flash;
    if (!fl || page >= fl->page_count) {
        out(p, "ERR page\n");
        return;
    }
    char b[48];
    int n = put_s(b, 0, "OK ");
    n = put_u(b, n, page);
    n = put_s(b, n, " ");
    n = put_u(b, n, fl->page_size);
    put_s(b, n, "\n");
    out(p, b);
    uint8_t chunk[192];
    for (uint32_t off = 0; off < fl->page_size; off += sizeof chunk) {
        uint32_t len = fl->page_size - off < sizeof chunk ? fl->page_size - off : (uint32_t)sizeof chunk;
        if (fl->read(fl->ctx, page * fl->page_size + off, chunk, len) != 0) {
            out(p, "ERR read\n");
            return;
        }
        send_base64(p, chunk, len);
        p->app->hal->watchdog_kick();
    }
    out(p, "END\n");
}

static void cmd_events(proto_t *p)
{
    app_t *a = p->app;
    char b[96];
    int n = put_s(b, 0, "OK ");
    n = put_u(b, n, a->ui.event_count);
    put_s(b, n, "\n");
    out(p, b);
    for (int i = 0; i < a->ui.event_count; i++) {
        const ui_event_t *e = &a->ui.events[i];
        n = put_s(b, 0, "E ");
        n = put_u(b, n, e->start);
        n = put_s(b, n, " ");
        n = put_u(b, n, e->duration_s);
        n = put_s(b, n, " ");
        n = put_u(b, n, e->peak_nsvh);
        n = put_s(b, n, " ");
        n = put_u(b, n, e->level);
        put_s(b, n, "\n");
        out(p, b);
    }
    out(p, "END\n");
}

static void cmd_cal(proto_t *p, const char *args)
{
    app_t *a = p->app;
    if (*args) {
        uint32_t v[4];
        const char *s = args;
        for (int i = 0; i < 4; i++)
            if (!take_u(&s, &v[i])) {
                out(p, "ERR args\n");
                return;
            }
        if (!app_set_calibration(a, v)) {
            out(p, "ERR range\n");
            return;
        }
        out(p, "OK\n");
        return;
    }
    char b[120];
    int n = put_s(b, 0, "OK sens=");
    n = put_u(b, n, a->ui.cal[0]);
    n = put_s(b, n, " dead=");
    n = put_u(b, n, a->ui.cal[1]);
    n = put_s(b, n, " e=");
    n = put_u(b, n, a->ui.cal[2]);
    n = put_s(b, n, " h=");
    n = put_u(b, n, a->ui.cal[3]);
    put_s(b, n, "\n");
    out(p, b);
}

static bool starts(const char *s, const char *word)
{
    while (*word) {
        if (*s++ != *word++)
            return false;
    }
    return *s == 0 || *s == ' ';
}

static void run_line(proto_t *p, const char *line)
{
    app_t *a = p->app;
    while (*line == ' ')
        line++;
    if (!*line)
        return;

    const char *args = line;
    while (*args && *args != ' ')
        args++;
    while (*args == ' ')
        args++;

    if (starts(line, "PING")) {
        out(p, "OK pong\n");
    } else if (starts(line, "INFO")) {
        cmd_info(p);
    } else if (starts(line, "TIME")) {
        uint32_t t;
        const char *s = args;
        if (*args && take_u(&s, &t)) {
            app_set_time(a, t);
            out(p, "OK\n");
        } else if (*args) {
            out(p, "ERR args\n");
        } else {
            char b[32];
            int n = put_s(b, 0, "OK ");
            n = put_u(b, n, a->hal->rtc_now());
            put_s(b, n, "\n");
            out(p, b);
        }
    } else if (starts(line, "LIVE")) {
        p->live = *args == '1';
        p->live_sent_s = 0;
        out(p, "OK\n");
    } else if (starts(line, "COUNTS")) {
        char b[64];
        int n = put_s(b, 0, "OK ");
        n = put_u(b, n, a->hal->tube_pulses());
        n = put_s(b, n, " ");
        n = put_u(b, n, a->hv_secs);
        n = put_s(b, n, " ");
        n = put_u(b, n, a->hal->rtc_now());
        put_s(b, n, "\n");
        out(p, b);
    } else if (starts(line, "BAT")) {
        char b[64];
        int n = put_s(b, 0, "OK ");
        n = put_u(b, n, a->hal->battery_mv());
        n = put_s(b, n, a->hal->usb_present() ? " 1" : " 0");
        n = put_s(b, n, a->hal->charging() ? " 1" : " 0");
        put_s(b, n, "\n");
        out(p, b);
    } else if (starts(line, "STATUS")) {
        cmd_status(p);
    } else if (starts(line, "CAPS")) {
        cmd_caps(p);
    } else if (starts(line, "UID")) {
        cmd_uid(p);
    } else if (starts(line, "LOGINFO")) {
        cmd_loginfo(p);
    } else if (starts(line, "LOGPAGE")) {
        uint32_t n;
        const char *s = args;
        if (take_u(&s, &n))
            cmd_logpage(p, n);
        else
            out(p, "ERR args\n");
    } else if (starts(line, "EVENTS")) {
        cmd_events(p);
    } else if (starts(line, "CAL")) {
        cmd_cal(p, args);
    } else if (starts(line, "SET")) {
        uint32_t v;
        const char *s = args;
        if (starts(args, "THEME")) {
            while (*s && *s != ' ')
                s++;
            while (*s == ' ')
                s++;
            if (take_u(&s, &v) && app_set_theme(a, v))
                out(p, "OK\n");
            else
                out(p, "ERR args\n");
        } else {
            out(p, "ERR unknown\n");
        }
    } else {
        out(p, "ERR unknown\n");
    }
}

void proto_init(proto_t *p, app_t *app)
{
    memset(p, 0, sizeof *p);
    p->app = app;
}

void proto_feed(proto_t *p, const uint8_t *data, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        char c = (char)data[i];
        if (c == '\r')
            continue;
        if (c == '\n') {
            if (p->overflow)
                out(p, "ERR long\n");
            else {
                p->line[p->len] = 0;
                run_line(p, p->line);
            }
            p->len = 0;
            p->overflow = false;
        } else if (p->len < PROTO_LINE - 1) {
            p->line[p->len++] = c;
        } else {
            p->overflow = true;
        }
    }
}

void proto_poll(proto_t *p)
{
    const hal_t *h = p->app->hal;
    if (!h->serial_read)
        return;
    uint8_t buf[64];
    int n;
    while ((n = h->serial_read(buf, sizeof buf)) > 0)
        proto_feed(p, buf, (uint32_t)n);
}

void proto_tick(proto_t *p, uint32_t now_s)
{
    if (!p->live || now_s == p->live_sent_s)
        return;
    p->live_sent_s = now_s;
    const ui_data_t *u = &p->app->ui;
    char b[120];
    int n = put_s(b, 0, "L ");
    n = put_u(b, n, now_s);
    n = put_s(b, n, " ");
    n = put_u(b, n, u->rate_nsvh);
    n = put_s(b, n, " ");
    n = put_u(b, n, u->cpm);
    n = put_s(b, n, " ");
    n = put_u(b, n, u->e_vm_x10);
    n = put_s(b, n, " ");
    n = put_u(b, n, u->h_ut_x100);
    n = put_s(b, n, " ");
    n = put_u(b, n, u->bat_percent);
    put_s(b, n, "\n");
    out(p, b);
}
