/* Source for the USB counter of the Radioactive@Home project (SBM-20 tube, V-USB firmware, HID device 16c0:05df whose product
 * string contains "radioactiveathome"). The device answers a HID feature report with its own millisecond timer and the
 * cumulative count of raw tube pulses, both 32 bit little endian after the report id:  [0] id, [1..4] timer ms, [5..8] pulses.
 * Facts of the device: the sensitivity of its tube is 171.232876 counts per minute for 1 uSv/h (the factor the project's own
 * program used), the SBM-20 data sheet gives about 190 us of dead time.
 *
 * The counter has no wall clock. A bin edge is therefore taken from the host's clock: a reading counts as the edge reading when it
 * was taken within 50 ms before to 100 ms after the edge (the host's clock is kept right by the network time service, the server
 * checks it as well). The pulses in that short time are about 0.03 of a count at background level and at most a few counts of tens
 * of thousands at a strong source, far below the counting noise of the bin. Every other reading is marked inexact. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "yarrow_src.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The counters of the project (sensors.xml of its program): V-USB boards 16c0:05df (revisions 2.01 to 2.60, the kit) and the PIC
 * board 04d8:f6fe (3.01). The shared V-USB / Microchip ids are only accepted with the project's name in the product string. */
static const struct { uint16_t vid, pid; } RAH_IDS[] = {{0x16c0, 0x05df}, {0x04d8, 0xf6fe}};
#define RAH_NAME "radioactiveathome"
#define RAH_NAME_KIT "Radiation Detector KIT"
#define RAH_SENS_X100 17123          /* 171.232876 counts per minute for 1 uSv/h */
#define RAH_DEAD_US 190
#define RAH_REPORT 129               /* report id + 128 data bytes */
#define EDGE_BEFORE_S 0.05
#define EDGE_AFTER_S 0.10
#define MAX_READ_S 0.04              /* a slower read cannot be placed on the edge */

typedef struct {
    ysrc_t base;
    yhid_t hid;
    yrah_env_t env;
} rah_t;

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int read_raw(rah_t *r, uint32_t *timer_ms, uint32_t *pulses)
{
    uint8_t buf[RAH_REPORT];
    memset(buf, 0, sizeof buf);
    if (r->hid.get_feature(r->hid.ctx, buf, sizeof buf) < 9)
        return -1;
    *timer_ms = le32(buf + 1);
    *pulses = le32(buf + 5);
    return 0;
}

static int rah_poll(ysrc_t *s, uint32_t interval, ysample_t *out)
{
    rah_t *r = (rah_t *)s;
    uint32_t timer, pulses;
    double before = r->env.now();
    if (read_raw(r, &timer, &pulses) != 0)
        return -1;
    double after = r->env.now();
    double w = (before + after) / 2;
    uint32_t sec = (uint32_t)floor(w);
    double edge = floor(w / interval + 0.5) * interval;
    double d = w - edge;
    out->pulses = pulses;
    out->hv_s = timer / 1000;
    out->host_s = w;
    out->inexact = 0;
    if (after - before <= MAX_READ_S && d >= -EDGE_BEFORE_S && d <= EDGE_AFTER_S) {
        out->t = (uint32_t)edge;                 /* this is the edge reading */
    } else {
        out->t = sec;
        if (sec % interval == 0)
            out->inexact = 1;                    /* the edge second, but not at the edge */
    }
    return 0;
}

static int rah_wait(const ysrc_t *s, uint32_t to_edge, uint32_t interval)
{
    (void)s;
    return (to_edge <= 2 || to_edge >= interval - 1) ? 40 : 1000;
}

static void rah_close(ysrc_t *s)
{
    rah_t *r = (rah_t *)s;
    if (r->hid.close)
        r->hid.close(r->hid.ctx);
    free(r);
}

static void make_uid(const char *serial, const char *host, char out[17])
{
    char text[200];
    snprintf(text, sizeof text, "rah|%s|%s", serial, host);
    uint64_t h = 1469598103934665603ULL;           /* FNV-1a, 64 bit */
    for (const char *p = text; *p; p++) {
        h ^= (unsigned char)*p;
        h *= 1099511628211ULL;
    }
    snprintf(out, 17, "%016llx", (unsigned long long)h);
}

ysrc_t *yrah_make(const yhid_t *hid, const yrah_env_t *env, ydev_t *dev)
{
    rah_t *r = calloc(1, sizeof *r);
    if (!r)
        return NULL;
    r->hid = *hid;
    r->env = *env;
    r->base.name = "Radioactive@Home counter";
    r->base.poll = rah_poll;
    r->base.housekeeping = NULL;
    r->base.next_wait_ms = rah_wait;
    r->base.close = rah_close;
    /* it must behave like the counter: the timer runs in milliseconds and the pulse count never goes down */
    uint32_t t1 = 0, p1 = 0, t2 = 0, p2 = 0;
    int ok = read_raw(r, &t1, &p1) == 0;
    if (ok) {
        env->sleep_ms(300);
        ok = read_raw(r, &t2, &p2) == 0 && t2 - t1 >= 200 && t2 - t1 <= 600 && p2 >= p1 && p2 - p1 < 100000;
    }
    if (!ok) {
        static int told;
        if (!told) {                  /* once, not on every scan: what a volunteer needs to tell us when a counter is not accepted */
            told = 1;
            fprintf(stderr, "rah: a device with the Radioactive@Home id answered, but it does not behave like the counter "
                            "(second read %s, timer +%u ms, pulses +%u); not used\n",
                    t2 || p2 ? "ok" : "failed", (unsigned)(t2 - t1), (unsigned)(p2 - p1));
        }
        free(r);
        return NULL;                  /* another device with the same shared USB id: leave the HID handle to the caller */
    }
    memset(dev, 0, sizeof *dev);
    make_uid(hid->serial, env->host, dev->uid);
    if (hid->version)
        snprintf(dev->fw, sizeof dev->fw, "rah-%x.%02x", (hid->version >> 8) & 0xff, hid->version & 0xff);
    else
        snprintf(dev->fw, sizeof dev->fw, "rah");
    dev->sens_x100 = RAH_SENS_X100;
    dev->dead_us = RAH_DEAD_US;
    return &r->base;
}

static void sleep_real(int ms);

ysrc_t *ysrc_rah_find(ydev_t *dev)
{
    yhid_t hid;
    memset(&hid, 0, sizeof hid);
    int found = 0;
    for (size_t i = 0; i < sizeof RAH_IDS / sizeof RAH_IDS[0] && !found; i++)
        found = yhid_open(RAH_IDS[i].vid, RAH_IDS[i].pid, RAH_NAME, &hid) == 0 ||
                yhid_open(RAH_IDS[i].vid, RAH_IDS[i].pid, RAH_NAME_KIT, &hid) == 0;
    if (!found)
        return NULL;
    char host[64] = "";
    yhost_name(host, sizeof host);
    yrah_env_t env = {yhost_wall, sleep_real, host};
    ysrc_t *s = yrah_make(&hid, &env, dev);
    if (!s && hid.close)
        hid.close(hid.ctx);
    return s;
}

#ifdef _WIN32
#include <windows.h>
static void sleep_real(int ms) { Sleep((DWORD)ms); }
#else
#include <time.h>
static void sleep_real(int ms)
{
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}
#endif
