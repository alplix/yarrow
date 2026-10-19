/* Source for a detector running the Yarrow firmware on a USB serial port (protocol: docs/serial-protocol.md). The detector has a
 * wall clock of its own, latches its counters once per second and reports its tube voltage time, so its bins are exact. */
#include "yarrow_glue.h"
#include "yarrow_serial.h"
#include "yarrow_src.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CLOCK_SLACK_S 90

typedef struct {
    ysrc_t base;
    yser_t *ser;
} yy_t;

static int num(const char *s, unsigned *out)
{
    char *end;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s)
        return 0;
    *out = (unsigned)v;
    return 1;
}

/* The server refuses bins from a device whose clock is far from real time, so a device whose clock is wrong (never set, or
 * drifted) is set to the host's UTC time before anything is counted. Returns 1 when the clock was set. */
static int fix_clock(yser_t *s, uint32_t dev_t)
{
    uint32_t now = (uint32_t)time(NULL);
    uint32_t diff = dev_t > now ? dev_t - now : now - dev_t;
    if (diff <= CLOCK_SLACK_S)
        return 0;
    char cmd[40], r[160];
    snprintf(cmd, sizeof cmd, "TIME %u", (unsigned)now);
    if (yser_command(s, cmd, r, sizeof r, 800) != 0)
        return 0;
    glue_log("the detector's clock was off, set to the computer's time (UTC)");
    return 1;
}

static int read_cal(yser_t *s, unsigned *sens, unsigned *dead)
{
    char r[160];
    const char *p, *q;
    *sens = *dead = 0;
    if (yser_command(s, "CAL", r, sizeof r, 800) != 0 || !(p = strstr(r, "sens=")) || !(q = strstr(r, "dead=")))
        return -1;
    if (!num(p + 5, sens) || !num(q + 5, dead) || !*sens || !*dead)
        return -1;
    return 0;
}

static int yy_poll(ysrc_t *s, uint32_t interval, ysample_t *out)
{
    (void)interval;
    yy_t *y = (yy_t *)s;
    char r[160];
    if (yser_command(y->ser, "COUNTS", r, sizeof r, 700) != 0)
        return -1;
    unsigned p, h, t;
    if (sscanf(r, "OK %u %u %u", &p, &h, &t) != 3)
        return -1;
    out->pulses = p;
    out->hv_s = h;
    out->t = t;
    out->host_s = yser_now();
    out->inexact = 0;
    return 0;
}

static void yy_housekeeping(ysrc_t *s, const ydev_t *dev, const ysample_t *last, yhk_t *out)
{
    yy_t *y = (yy_t *)s;
    memset(out, 0, sizeof *out);
    if (last->t >= 1700000000u && fix_clock(y->ser, last->t))
        out->clock_set = 1;
    unsigned sens, dead;
    if (read_cal(y->ser, &sens, &dead) == 0 && (sens != dev->sens_x100 || dead != dev->dead_us)) {
        out->cal_changed = 1;
        out->sens_x100 = sens;
        out->dead_us = dead;
    }
}

static int yy_wait(const ysrc_t *s, uint32_t to_edge, uint32_t interval)
{
    (void)s;
    return (to_edge <= 3 || to_edge >= interval - 2) ? 100 : 1000;   /* fast around the bin edges, slowly otherwise */
}

static void yy_close(ysrc_t *s)
{
    yy_t *y = (yy_t *)s;
    yser_close(y->ser);
    free(y);
}

/* Reads identity and calibration. 0 on success. */
static int identify(yser_t *s, ydev_t *nd)
{
    char r[160];
    yser_command(s, "LIVE 0", r, sizeof r, 300);
    if (yser_command(s, "INFO", r, sizeof r, 800) != 0 || !strstr(r, "yarrow="))
        return -1;
    const char *v = strstr(r, "yarrow=");
    size_t i = 0;
    for (v += 7; *v && *v != ' ' && i < sizeof nd->fw - 1; v++)
        nd->fw[i++] = *v;
    nd->fw[i] = 0;
    if (yser_command(s, "UID", r, sizeof r, 800) != 0 || strlen(r) != 19) {
        glue_log("this firmware cannot report its identity (UID); update the detector");
        return -1;
    }
    memcpy(nd->uid, r + 3, 16);
    nd->uid[16] = 0;
    char probe[160];
    if (yser_command(s, "COUNTS", probe, sizeof probe, 800) != 0) {
        glue_log("this firmware cannot report raw counts (COUNTS); update the detector");
        return -1;
    }
    unsigned pp, hh, tt;
    if (sscanf(probe, "OK %u %u %u", &pp, &hh, &tt) == 3)
        fix_clock(s, tt);
    unsigned sens, dead;
    if (read_cal(s, &sens, &dead) != 0)
        return -1;
    nd->sens_x100 = sens;
    nd->dead_us = dead;
    return 0;
}

ysrc_t *ysrc_yarrow_find(const char *port, ydev_t *dev)
{
    char path[64] = "";
    yser_t *ser = (port && port[0]) ? yser_open(port) : yser_find(path, sizeof path);
    if (!ser)
        return NULL;
    memset(dev, 0, sizeof *dev);
    if (identify(ser, dev) != 0) {
        yser_close(ser);
        return NULL;
    }
    yy_t *y = calloc(1, sizeof *y);
    if (!y) {
        yser_close(ser);
        return NULL;
    }
    y->ser = ser;
    y->base.name = "Yarrow detector";
    y->base.poll = yy_poll;
    y->base.housekeeping = yy_housekeeping;
    y->base.next_wait_ms = yy_wait;
    y->base.close = yy_close;
    return &y->base;
}
