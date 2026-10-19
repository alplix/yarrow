/* Long-running check of a Yarrow detector: every minute it compares what the device reports as its rate with what the
 * raw pulse counter says, and it keeps the battery and cable state, in one CSV file.
 *   yarrow_watch [--port COM5] [--file watch.csv]
 * Columns: time, battery mV, cable, charging, minutes, raw counts, raw cpm, device cpm, ratio, dose rate nSv/h,
 * dose rate from cpm and sensitivity, verdict. */
#include "yarrow_serial.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile sig_atomic_t quit;
static void on_signal(int sig)
{
    (void)sig;
    quit = 1;
}

/* value of "key=" inside the text, 0 when it is missing */
static int field(const char *text, const char *key, double *v)
{
    char pat[40];
    snprintf(pat, sizeof pat, " %s=", key);
    const char *p = strstr(text, pat);
    if (!p)
        return 0;
    *v = atof(p + strlen(pat));
    return 1;
}

int main(int argc, char **argv)
{
    const char *port = NULL, *file = "watch.csv";
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "--port"))
            port = argv[i + 1];
        else if (!strcmp(argv[i], "--file"))
            file = argv[i + 1];
    }
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    FILE *f = fopen(file, "a");
    if (!f) {
        fprintf(stderr, "cannot write %s\n", file);
        return 1;
    }
    fprintf(f, "time,battery_mV,cable,charging,minutes,raw_counts,raw_cpm,device_cpm,ratio,device_nSvh,expected_nSvh,verdict\n");

    yser_t *s = NULL;
    int have_prev = 0;
    double prev_pulses = 0, prev_hvs = 0, prev_t = 0;
    while (!quit) {
        if (!s) {
            char path[64];
            s = port ? yser_open(port) : yser_find(path, sizeof path);
            have_prev = 0;
            if (!s) {
                fprintf(stderr, "looking for a detector...\n");
                yser_sleep_ms(5000);
                continue;
            }
        }
        static char status[4096];
        char r[160], cnt[160];
        double mv = 0, cable = 0, chg = 0, pulses = 0, hvs = 0, now = 0, cpm = 0, nsvh = 0, sens = 0, dead = 0;
        int ok = yser_lines(s, "STATUS", status, sizeof status, 2000) == 0 && yser_command(s, "COUNTS", cnt, sizeof cnt, 1000) == 0 &&
                 yser_command(s, "BAT", r, sizeof r, 1000) == 0;
        if (ok) {
            unsigned a, b, c;
            unsigned long p, h, t;
            ok = sscanf(r, "OK %u %u %u", &a, &b, &c) == 3 && sscanf(cnt, "OK %lu %lu %lu", &p, &h, &t) == 3 &&
                 field(status, "cpm", &cpm) && field(status, "rate_nsvh", &nsvh) && field(status, "sens_x100", &sens) &&
                 field(status, "dead_us", &dead);
            mv = a;
            cable = b;
            chg = c;
            pulses = (double)p;
            hvs = (double)h;
            now = (double)t;
        }
        if (!ok) {
            yser_close(s);
            s = NULL;
            continue;
        }
        if (have_prev && now - prev_t >= 55 && hvs - prev_hvs >= now - prev_t - 1 && pulses >= prev_pulses) {
            double min = (now - prev_t) / 60.0, counts = pulses - prev_pulses;
            double raw_cpm = counts / min;
            /* the device's rate is dead-time corrected and averaged over its own window, so the two may differ a little:
             * allow 4 sigma of the counting noise plus the correction */
            double corr = 1.0 / (1.0 - (raw_cpm / 60.0) * dead * 1e-6);
            double expect_cpm = raw_cpm * corr;
            double sigma = counts > 0 ? sqrt(counts) / min : 1;
            double ratio = raw_cpm > 0 ? cpm / raw_cpm : 0;
            double expected_nsvh = cpm * 100.0 / sens * 1000.0;
            const char *verdict = "ok";
            if (counts < 100)
                verdict = "few counts";
            else if (fabs(cpm - expect_cpm) > 4 * sigma + 0.05 * expect_cpm)
                verdict = "CHECK cpm";
            else if (nsvh > 0 && fabs(nsvh - expected_nsvh) > 0.03 * expected_nsvh + 5)
                verdict = "CHECK dose rate";
            time_t tt = time(NULL);
            char ts[32];
            strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&tt));
            fprintf(f, "%s,%.0f,%.0f,%.0f,%.2f,%.0f,%.1f,%.1f,%.3f,%.0f,%.0f,%s\n", ts, mv, cable, chg, min, counts, raw_cpm, cpm, ratio,
                    nsvh, expected_nsvh, verdict);
            fflush(f);
            printf("%s  %.0f mV  raw %.1f cpm  device %.1f cpm  %.0f nSv/h (expected %.0f)  %s\n", ts, mv, raw_cpm, cpm, nsvh,
                   expected_nsvh, verdict);
            fflush(stdout);
        }
        if (!have_prev || now - prev_t >= 55) {
            prev_pulses = pulses;
            prev_hvs = hvs;
            prev_t = now;
            have_prev = 1;
        }
        for (int i = 0; i < 300 && !quit; i++)      /* every 30 seconds; a comparison needs about a minute */
            yser_sleep_ms(100);
    }
    fclose(f);
    yser_close(s);
    return 0;
}
