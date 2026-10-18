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
    int n = put_s(b, 0, "OK yarrow=0.1 stage=");
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
