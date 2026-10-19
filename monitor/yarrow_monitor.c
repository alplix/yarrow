/* Yarrow monitor: reads a Yarrow detector live, cuts the counts into bins and reports them as trickles. */
#include "yarrow_bin.h"
#include "yarrow_glue.h"
#include "yarrow_serial.h"
#include "yarrow_trickle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SEND_EVERY_S 600.0
#define RECAL_EVERY_S 60.0

static ypending_t pending;
static ydev_t dev;
static ybinner_t binner;

static void save_state(void);

static void on_bin(void *ctx, uint32_t start, uint32_t count)
{
    (void)ctx;
    ypending_add(&pending, start, count);
    save_state();                  /* a finished bin survives a crash or a closed window */
}

static void save_state(void)
{
    static char buf[YT_MAX_PENDING * 24 + 128];
    size_t n = 0;
    if (dev.uid[0])
        n = (size_t)snprintf(buf, 128, "D %s %s %u %u %u\n", dev.uid, dev.fw, dev.sens_x100, dev.dead_us, dev.interval);
    glue_checkpoint_write(buf, n + ypending_save(&pending, buf + n, sizeof buf - n));
}

static double last_send;

static void flush(int force)
{
    if (!pending.n || dev.uid[0] == 0)
        return;
    if (!force && yser_now() - last_send < SEND_EVERY_S)
        return;
    static char xml[4096];
    uint32_t k = ypending_xml(&pending, &dev, xml, sizeof xml);
    if (!k)
        return;
    if (glue_send_trickle(xml) == 0) {
        ypending_drop(&pending, k);
        last_send = yser_now();
        save_state();
    }
}

static int num(const char *s, unsigned *out)
{
    char *end;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s)
        return 0;
    *out = (unsigned)v;
    return 1;
}

/* Reads identity and calibration. 0 on success. */
static int identify(yser_t *s, const yprefs_t *pr, ydev_t *nd)
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
    if (yser_command(s, "CAL", r, sizeof r, 800) != 0)
        return -1;
    unsigned sens = 0, dead = 0;
    const char *p = strstr(r, "sens="), *q = strstr(r, "dead=");
    if (!p || !q || !num(p + 5, &sens) || !num(q + 5, &dead) || !sens || !dead)
        return -1;
    nd->sens_x100 = sens;
    nd->dead_us = dead;
    snprintf(nd->cc, sizeof nd->cc, "%s", pr->cc);
    snprintf(nd->city, sizeof nd->city, "%s", pr->city);
    nd->interval = pr->interval;
    return 0;
}

static int poll_counts(yser_t *s, ysample_t *out)
{
    char r[160];
    if (yser_command(s, "COUNTS", r, sizeof r, 700) != 0)
        return -1;
    unsigned p, h, t;
    if (sscanf(r, "OK %u %u %u", &p, &h, &t) != 3)
        return -1;
    out->pulses = p;
    out->hv_s = h;
    out->t = t;
    out->host_s = yser_now();
    return 0;
}

int main(int argc, char **argv)
{
    yprefs_t pr;
    memset(&pr, 0, sizeof pr);
    if (glue_init(argc, argv, &pr) != 0) {
        glue_done(1);
        return 1;
    }
    if (pr.interval != 600 && pr.interval != 1800 && pr.interval != 3600)
        pr.interval = 600;
    if (!ydev_valid_place(pr.cc, pr.city)) {
        glue_wait_for_prefs("Set your country (two capital letters) and your city in the project preferences on the web site");
        return 1;
    }
    static char saved[YT_MAX_PENDING * 24];
    saved[glue_checkpoint_read(saved, sizeof saved - 1)] = 0;
    ypending_load(&pending, saved);
    if (saved[0] == 'D') {
        char uid[17] = "", fw[17] = "";
        unsigned sens = 0, dead = 0, iv = 0;
        if (sscanf(saved, "D %16s %16s %u %u %u", uid, fw, &sens, &dead, &iv) == 5 && strlen(uid) == 16) {
            snprintf(dev.uid, sizeof dev.uid, "%s", uid);
            snprintf(dev.fw, sizeof dev.fw, "%s", fw);
            snprintf(dev.cc, sizeof dev.cc, "%s", pr.cc);
            snprintf(dev.city, sizeof dev.city, "%s", pr.city);
            dev.sens_x100 = sens;
            dev.dead_us = dead;
            dev.interval = iv;
            flush(1);                                    /* bins left over from the last run */
        }
    }
    if (pending.n && !dev.uid[0])
        ypending_drop(&pending, pending.n);
    ybin_init(&binner, pr.interval, on_bin, 0);

    yser_t *ser = NULL;
    glue_log("looking for a detector (plugged in, switched on, on the main screen)");
    double next_find = 0, next_cal = 0, searching_since = yser_now();
    ysample_t s;
    while (!glue_quit_requested()) {
        if (!ser) {
            if (yser_now() < next_find) {
                yser_sleep_ms(500);
                continue;
            }
            next_find = yser_now() + 15;
            char path[64] = "";
            if (pr.port[0]) {
                ser = yser_open(pr.port);
            } else {
                ser = yser_find(path, sizeof path);
            }
            if (!ser) {
                if (yser_now() - searching_since > 300) {      /* nothing plugged in: do not hold a CPU for it */
                    flush(1);
                    save_state();
                    glue_no_detector("No Yarrow detector found. Plug it in, switch it on and leave it on the main screen.");
                    searching_since = yser_now();
                }
                continue;
            }
            ydev_t nd;
            memset(&nd, 0, sizeof nd);
            if (identify(ser, &pr, &nd) != 0) {
                yser_close(ser);
                ser = NULL;
                continue;
            }
            if (dev.uid[0] && (strcmp(dev.uid, nd.uid) != 0 || dev.sens_x100 != nd.sens_x100 || dev.dead_us != nd.dead_us ||
                               dev.interval != nd.interval)) {
                flush(1);                                /* what is pending was counted by another set-up */
                ypending_drop(&pending, pending.n);
            }
            dev = nd;
            ybin_break(&binner);
            next_cal = yser_now() + RECAL_EVERY_S;
            last_send = yser_now();
            glue_log("detector found");
            searching_since = yser_now();
        }
        if (poll_counts(ser, &s) != 0) {
            yser_close(ser);
            ser = NULL;
            ybin_break(&binner);
            glue_log("detector lost");
            searching_since = yser_now();
            continue;
        }
        ybin_feed(&binner, &s);
        if (yser_now() >= next_cal) {
            next_cal = yser_now() + RECAL_EVERY_S;
            char r[160];
            unsigned sens = 0, dead = 0;
            const char *p, *q;
            if (yser_command(ser, "CAL", r, sizeof r, 800) == 0 && (p = strstr(r, "sens=")) && (q = strstr(r, "dead=")) &&
                num(p + 5, &sens) && num(q + 5, &dead) && sens && dead && (sens != dev.sens_x100 || dead != dev.dead_us)) {
                flush(1);                              /* bins sent so far keep the calibration they were counted with */
                dev.sens_x100 = sens;
                dev.dead_us = dead;
                ybin_break(&binner);
            }
        }
        flush(0);
        /* poll fast around the bin edges, slowly otherwise */
        uint32_t to_edge = ybin_to_edge(&binner, s.t);
        yser_sleep_ms(to_edge <= 3 || to_edge >= pr.interval - 2 ? 100 : 1000);
    }
    flush(1);
    save_state();
    yser_close(ser);
    glue_done(0);
    return 0;
}
