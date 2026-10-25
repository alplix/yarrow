/* The test of the Radioactive@Home USB counter, shared by the console program and the Windows window: it finds the counter, prints
 * what the device really sends, then reads it for a while with the same code the Yarrow monitor uses and shows the dose rate and
 * the 10-minute bins. Every line goes to the callback of the caller. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "yarrow_rah_probe.h"
#include "yarrow_src.h"

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static void nap(int ms) { Sleep((DWORD)ms); }
#else
static void nap(int ms)
{
    /* a signal ends nanosleep early (the BOINC library fires a timer signal many times a second, so a 300 ms wait took 50 ms):
     * go on with what is left of the time */
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L}, rem;
    while (nanosleep(&ts, &rem) == -1 && errno == EINTR)
        ts = rem;
}
#endif

#define SENS 171.232876      /* counts per minute for 1 uSv/h, the factor of the project's own program */

static void (*sink)(const char *);

static void say(const char *fmt, ...)
{
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    if (sink)
        sink(line);
}

static void stamp(char *out, size_t cap, double t)
{
    time_t s = (time_t)t;
    struct tm *tm = gmtime(&s);
    strftime(out, cap, "%Y-%m-%d %H:%M:%S UTC", tm);
}

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* YARROW_PROBE_FAKE=1 runs the program against a made-up counter (for checking the program itself, no hardware needed) */
static double fake_t0;
static int fake_get(void *ctx, uint8_t *buf, size_t len)
{
    (void)ctx; (void)len;
    double el = yhost_wall() - fake_t0;
    uint32_t timer = (uint32_t)(el * 1000.0) + 5000, pulses = (uint32_t)(el * 0.31);
    memset(buf, 0, 129);
    for (int i = 0; i < 4; i++) {
        buf[1 + i] = (uint8_t)(timer >> (8 * i));
        buf[5 + i] = (uint8_t)(pulses >> (8 * i));
    }
    return 129;
}
static void fake_close(void *ctx) { (void)ctx; }

static int bins_seen;
static void on_bin(void *ctx, uint32_t start, uint32_t count)
{
    (void)ctx;
    char ts[48];
    stamp(ts, sizeof ts, start);
    bins_seen++;
    say("  BIN  %s  600 s  %u counts  =  %.1f cpm  =  %.3f uSv/h\n", ts, (unsigned)count, count / 10.0, count / 10.0 / SENS);
}

int rah_probe_run(int minutes, void (*out)(const char *), volatile int *stop)
{
    sink = out;
    bins_seen = 0;
    char ts[48];
    stamp(ts, sizeof ts, yhost_wall());
    say("Yarrow test for the Radioactive@Home counter\nstarted %s, will run %d minutes\n\n", ts, minutes);

    static const struct { uint16_t vid, pid; } ids[] = {{0x16c0, 0x05df}, {0x04d8, 0xf6fe}};
    yhid_t hid;
    memset(&hid, 0, sizeof hid);
    int found = 0;
    if (getenv("YARROW_PROBE_FAKE")) {
        fake_t0 = yhost_wall();
        hid.get_feature = fake_get;
        hid.close = fake_close;
        snprintf(hid.name, sizeof hid.name, "FAKE radioactiveathome.org GRS");
        hid.version = 0x0251;
        found = 1;
        say("(made-up counter, no hardware)\n\n");
    }
    for (size_t i = 0; i < 2 && !found; i++)
        if (yhid_open(ids[i].vid, ids[i].pid, NULL, &hid) == 0) {
            found = 1;
            say("found a USB HID device %04x:%04x\n  product: \"%s\"\n  serial:  \"%s\"\n  release: %x.%02x (USB bcdDevice)\n\n", ids[i].vid, ids[i].pid, hid.name,
                hid.serial, (hid.version >> 8) & 0xff, hid.version & 0xff);
        }
    if (!found) {
        say("NO counter found (looked for USB ids 16c0:05df and 04d8:f6fe).\n"
            "- Is the counter plugged in with a data cable, and does it show up in the system?\n"
            "- On Linux/Raspberry Pi the program may need administrator rights: run it as  sudo ./yarrow_rah_probe\n"
            "- Close the Radioactive@Home / BOINC program first: only one program can read the counter.\n");
        return 1;
    }

    say("What the counter sends (feature report, first 16 bytes; byte 0 is the report id):\n");
    for (int k = 0; k < 3; k++) {
        uint8_t buf[129];
        memset(buf, 0, sizeof buf);
        int n = hid.get_feature(hid.ctx, buf, sizeof buf);
        if (n < 9) {
            say("  read %d failed (%d)\n", k + 1, n);
        } else {
            say("  read %d:", k + 1);
            for (int i = 0; i < 16; i++)
                say(" %02x", buf[i]);
            say("   timer %u ms   counter %u\n", (unsigned)le32(buf + 1), (unsigned)le32(buf + 5));
        }
        nap(1000);
    }
    say("(the timer should grow by about 1000 every second, the counter slowly)\n\n");

    yrah_env_t env = {yhost_wall, nap, "probe"};
    ydev_t dev;
    ysrc_t *src = yrah_make(&hid, &env, &dev);
    if (!src) {
        say("The device did not behave like a Radioactive@Home counter (its timer does not run in milliseconds,\n"
            "or its counter goes backwards). Please send this file as it is.\n");
        return 2;
    }
    say("accepted as a counter: identity %s, firmware text %s, sensitivity %.2f cpm per uSv/h\n\n", dev.uid, dev.fw, SENS);

    ybinner_t binner;
    ybin_init(&binner, 600, on_bin, NULL);
    double t0 = yhost_wall(), end = t0 + minutes * 60.0, next_line = t0 + 10;
    ysample_t s, first;
    memset(&s, 0, sizeof s);
    int have_first = 0, polls = 0, fails = 0;
    static struct { double t; uint32_t c; } ring[400];
    int rn = 0;
    while (yhost_wall() < end && !*stop) {
        if (src->poll(src, 600, &s) != 0) {
            if (++fails > 20) {
                say("the counter stopped answering\n");
                break;
            }
            nap(500);
            continue;
        }
        polls++;
        if (!have_first) {
            first = s;
            have_first = 1;
        }
        ybin_feed(&binner, &s);
        double now = yhost_wall();
        if (now >= next_line) {
            next_line += 10;
            ring[rn % 400].t = now;
            ring[rn % 400].c = s.pulses;
            rn++;
            double cpm = 0;
            if (rn > 6) {                                 /* the last minute */
                int a = (rn - 7) % 400, b = (rn - 1) % 400;
                cpm = (ring[b].c - ring[a].c) / ((ring[b].t - ring[a].t) / 60.0);
            }
            char t2[48];
            stamp(t2, sizeof t2, now);
            say("%s  device timer %u s  counter %u  last minute %.1f cpm = %.3f uSv/h\n", t2, (unsigned)s.hv_s, (unsigned)s.pulses, cpm, cpm / SENS);
        }
        nap(src->next_wait_ms(src, ybin_to_edge(&binner, s.t), 600));
    }
    if (have_first) {
        double host = s.host_s - first.host_s, devs = (double)s.hv_s - first.hv_s, counts = (double)s.pulses - first.pulses;
        say("\nSummary\n  reads: %d  failed reads: %d\n  host time: %.0f s   device timer: %.0f s   (ratio %.4f)\n", polls, fails, host, devs, host > 0 ? devs / host : 0);
        say("  counts: %.0f   mean: %.1f cpm = %.3f uSv/h\n  10-minute bins completed: %d\n", counts, host > 0 ? counts * 60.0 / host : 0, host > 0 ? counts * 60.0 / host / SENS : 0, bins_seen);
    }
    say("\nDone.\n");
    src->close(src);
    return 0;
}
