/* Yarrow monitor: reads a Yarrow detector live, cuts the counts into bins and reports them as trickles. */
#include "yarrow_bin.h"
#include "yarrow_glue.h"
#include "yarrow_lock.h"
#include "yarrow_serial.h"
#include "yarrow_src.h"
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
    if (!ylock_acquire(glue_lock_path())) {
        glue_log("another Yarrow monitor is already running on this computer");
        glue_no_detector("Another Yarrow monitor is already running on this computer; one is enough.");
        glue_done(1);
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

    ysrc_t *src = NULL;
    glue_log("looking for a detector (plugged in, switched on, on the main screen)");
    double next_find = 0, next_cal = 0, searching_since = yser_now();
    ysample_t s;
    memset(&s, 0, sizeof s);
    while (!glue_quit_requested()) {
        if (!src) {
            if (yser_now() < next_find) {
                yser_sleep_ms(500);
                continue;
            }
            next_find = yser_now() + 15;
            ydev_t nd;
            memset(&nd, 0, sizeof nd);
            src = ysrc_find(pr.port, &nd);
            if (!src) {
                if (yser_now() - searching_since > 300) {      /* nothing plugged in: do not hold a CPU for it */
                    flush(1);
                    save_state();
                    glue_no_detector("No detector found. Plug it in, switch it on (a Yarrow detector on its main screen).");
                    searching_since = yser_now();
                }
                continue;
            }
            snprintf(nd.cc, sizeof nd.cc, "%s", pr.cc);
            snprintf(nd.city, sizeof nd.city, "%s", pr.city);
            nd.interval = pr.interval;
            if (dev.uid[0] && (strcmp(dev.uid, nd.uid) != 0 || dev.sens_x100 != nd.sens_x100 || dev.dead_us != nd.dead_us ||
                               dev.interval != nd.interval)) {
                flush(1);                                /* what is pending was counted by another set-up */
                ypending_drop(&pending, pending.n);
            }
            dev = nd;
            ybin_break(&binner);
            next_cal = yser_now() + RECAL_EVERY_S;
            last_send = yser_now();
            char line[96];
            snprintf(line, sizeof line, "found: %s", src->name);
            glue_log(line);
            searching_since = yser_now();
        }
        if (src->poll(src, pr.interval, &s) != 0) {
            src->close(src);
            src = NULL;
            ybin_break(&binner);
            glue_log("detector lost");
            searching_since = yser_now();
            continue;
        }
        ybin_feed(&binner, &s);
        if (yser_now() >= next_cal) {
            next_cal = yser_now() + RECAL_EVERY_S;
            if (src->housekeeping) {
                yhk_t hk;
                src->housekeeping(src, &dev, &s, &hk);
                if (hk.clock_set)
                    ybin_break(&binner);                   /* the bin that spans the change is not exact */
                if (hk.cal_changed) {
                    flush(1);                              /* bins sent so far keep the calibration they were counted with */
                    dev.sens_x100 = hk.sens_x100;
                    dev.dead_us = hk.dead_us;
                    ybin_break(&binner);
                }
            }
        }
        flush(0);
        yser_sleep_ms(src->next_wait_ms(src, ybin_to_edge(&binner, s.t), pr.interval));
    }
    flush(1);
    save_state();
    if (src)
        src->close(src);
    glue_done(0);
    return 0;
}
