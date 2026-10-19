/* Source for the GQ GMC family (GMC-280, GMC-300, GMC-320, GMC-500, ...) on a USB serial port. Protocol: GQ-RFC1201 plus the
 * layout of the configuration page. Commands are text between '<' and '>>', replies are binary:
 *   <GETVER>>     14 ASCII bytes: 7 model, 7 firmware ("GMC-300Re 4.20")
 *   <GETSERIAL>>  7 bytes
 *   <GETCFG>>     the configuration page (256 bytes, 512 on the GMC-500): three calibration points, each a 16 bit counts-per-minute
 *                 value (big endian) at 0x08, 0x0e, 0x14 and the matching dose rate in uSv/h as a big endian float at 0x0a, 0x10, 0x16
 *   <HEARTBEAT1>> the device then sends, every second, 2 bytes: the pulses counted in that second (big endian, low 14 bits)
 *   <HEARTBEAT0>> stops it
 * The sensitivity (counts per minute for 1 uSv/h) is taken from the device's own calibration, so every model and tube is handled
 * without a table. Each heartbeat closes one second of the device's own clock; its arrival on the host is the time of that second's
 * end. The heartbeat that arrives within half a second of a bin edge is the edge reading: the counted window is then exactly the
 * bin length in the device's seconds, shifted by under a second from the wall clock, which does not change what was counted.
 * A lost heartbeat (a second missing) drops the bin in progress, because the seconds of the stream are counted as the tube time. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "yarrow_gmc.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GMC_DEAD_US 90               /* the M4011 tube of the GMC-280/300/320 */
#define MAX_CFG 512

typedef struct {
    ysrc_t base;
    ygmc_io_t io;
    ygmc_env_t env;
    uint64_t pulses;                 /* cumulative, from the heartbeats */
    uint32_t seconds;                /* heartbeats received since the stream was last whole */
    double last_a;                   /* arrival of the previous heartbeat */
} gmc_t;

static int cmd(const ygmc_io_t *io, const char *text)
{
    return io->write(io->ctx, text, strlen(text));
}

static int gmc_poll(ysrc_t *s, uint32_t interval, ysample_t *out)
{
    gmc_t *g = (gmc_t *)s;
    uint8_t b[2];
    if (g->io.read(g->io.ctx, b, 2, 2500) != 2)
        return -1;                                   /* the stream stopped */
    double a = g->env.now();
    unsigned v = (unsigned)b[0] << 8 | b[1];
    if (v & 0xc000u)
        return -1;                                   /* the reserved bits are never set: the framing is lost */
    g->pulses += v & 0x3fffu;
    /* the arrivals must be a second apart: a longer gap is a lost second, a shorter one is a burst after the host stalled and
     * the arrival times no longer tell when the seconds ended. Either way the stream count restarts, which drops the bin. */
    if (g->last_a > 0 && (a - g->last_a > 1.5 || a - g->last_a < 0.5))
        g->seconds = 0;
    g->last_a = a;
    g->seconds++;
    out->pulses = (uint32_t)g->pulses;
    out->hv_s = g->seconds;
    out->host_s = a;
    out->inexact = 0;
    double edge = floor(a / interval + 0.5) * interval;
    if (a - edge >= -0.5 && a - edge < 0.5) {
        out->t = (uint32_t)edge;
    } else {
        out->t = (uint32_t)floor(a);
        if (out->t % interval == 0)
            out->inexact = 1;                        /* the edge second, but not the edge reading */
    }
    return 0;
}

static int gmc_wait(const ysrc_t *s, uint32_t to_edge, uint32_t interval)
{
    (void)s; (void)to_edge; (void)interval;
    return 0;                                        /* poll() waits for the next heartbeat itself */
}

static void gmc_close(ysrc_t *s)
{
    gmc_t *g = (gmc_t *)s;
    cmd(&g->io, "<HEARTBEAT0>>");
    if (g->io.close)
        g->io.close(g->io.ctx);
    free(g);
}

static void make_uid(const uint8_t *serial, const char *model, const char *host, char out[17])
{
    int blank = 1;
    for (int i = 0; i < 7; i++)
        if (serial[i] != 0 && serial[i] != 0xff)
            blank = 0;
    char text[128];
    size_t n = (size_t)snprintf(text, sizeof text, "gmc|%s|", model);
    if (blank)
        snprintf(text + n, sizeof text - n, "host:%s", host);      /* a counter without a serial number: one per computer */
    else
        for (int i = 0; i < 7; i++)
            n += (size_t)snprintf(text + n, sizeof text - n, "%02x", serial[i]);
    uint64_t h = 1469598103934665603ULL;
    for (const char *p = text; *p; p++) {
        h ^= (unsigned char)*p;
        h *= 1099511628211ULL;
    }
    snprintf(out, 17, "%016llx", (unsigned long long)h);
}

static double be_float(const uint8_t *p)
{
    uint32_t u = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
    float f;
    memcpy(&f, &u, 4);
    return f;
}

/* The device's own calibration: counts per minute for 1 uSv/h, times 100; 0 when the page holds nothing usable. */
uint32_t ygmc_sensitivity(const uint8_t *cfg, size_t n)
{
    static const int cpm_at[3] = {0x08, 0x0e, 0x14};
    for (int i = 0; i < 3; i++) {
        int o = cpm_at[i];
        if ((size_t)o + 6 > n)
            break;
        double cpm = (double)((unsigned)cfg[o] << 8 | cfg[o + 1]);
        double sv = be_float(cfg + o + 2);
        if (cpm > 0 && sv > 0 && sv < 1e6 && cpm / sv >= 10 && cpm / sv <= 2000)
            return (uint32_t)floor(cpm / sv * 100 + 0.5);       /* 10 .. 2000 counts per minute per uSv/h */
    }
    return 0;
}

ysrc_t *ygmc_make(const ygmc_io_t *io, const ygmc_env_t *env, ydev_t *dev)
{
    uint8_t ver[16], ser[8], cfg[MAX_CFG];
    memset(ver, 0, sizeof ver);
    memset(ser, 0, sizeof ser);
    if (env->drain)
        env->drain(io->ctx);
    if (cmd(io, "<GETVER>>") != 0 || io->read(io->ctx, ver, 14, 800) != 14 || memcmp(ver, "GMC-", 4) != 0)
        return NULL;
    for (int i = 0; i < 14; i++)
        if (ver[i] < 0x20 || ver[i] > 0x7e)
            return NULL;
    if (env->drain)
        env->drain(io->ctx);
    if (cmd(io, "<GETSERIAL>>") != 0 || io->read(io->ctx, ser, 7, 800) != 7)
        memset(ser, 0, 7);
    if (env->drain)
        env->drain(io->ctx);
    size_t want = memcmp(ver, "GMC-500", 7) == 0 ? 512 : 256;
    if (cmd(io, "<GETCFG>>") != 0 || io->read(io->ctx, cfg, want, 1500) != (int)want)
        return NULL;
    uint32_t sens = ygmc_sensitivity(cfg, want);
    if (!sens)
        return NULL;                                  /* no usable calibration: the dose rate cannot be told */
    if (env->drain)
        env->drain(io->ctx);
    gmc_t *g = calloc(1, sizeof *g);
    if (!g)
        return NULL;
    g->io = *io;
    g->env = *env;
    g->base.name = "GQ GMC counter";
    g->base.poll = gmc_poll;
    g->base.housekeeping = NULL;
    g->base.next_wait_ms = gmc_wait;
    g->base.close = gmc_close;
    if (cmd(io, "<HEARTBEAT1>>") != 0) {
        free(g);
        return NULL;
    }
    memset(dev, 0, sizeof *dev);
    char model[8];
    memcpy(model, ver, 7);
    model[7] = 0;
    make_uid(ser, model, env->host, dev->uid);
    size_t k = 0;
    for (int i = 0; i < 14 && k + 1 < sizeof dev->fw; i++)       /* "GMC-300Re 4.20" -> "GMC-300Re-4.20" */
        dev->fw[k++] = isalnum(ver[i]) || ver[i] == '.' || ver[i] == '-' ? (char)ver[i] : '-';
    dev->fw[k] = 0;
    dev->sens_x100 = sens;
    dev->dead_us = GMC_DEAD_US;
    return &g->base;
}
