/* Writes the battery voltage and charge state of a Yarrow detector to a CSV file every 30 seconds.
 *   yarrow_batlog [--port COM5] [--every 30] [--file batlog.csv]
 * Used to see how the charger behaves while the cable stays plugged in. */
#include "yarrow_serial.h"

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

int main(int argc, char **argv)
{
    const char *port = NULL, *file = "batlog.csv";
    int every = 30;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "--port"))
            port = argv[i + 1];
        else if (!strcmp(argv[i], "--every"))
            every = atoi(argv[i + 1]);
        else if (!strcmp(argv[i], "--file"))
            file = argv[i + 1];
    }
    if (every < 5)
        every = 5;
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    FILE *f = fopen(file, "a");
    if (!f) {
        fprintf(stderr, "cannot write %s\n", file);
        return 1;
    }
    fprintf(f, "time,battery_mV,cable,charging\n");
    yser_t *s = NULL;
    while (!quit) {
        if (!s) {
            char path[64];
            s = port ? yser_open(port) : yser_find(path, sizeof path);
            if (!s) {
                fprintf(stderr, "looking for a detector...\n");
                yser_sleep_ms(5000);
                continue;
            }
        }
        char r[160];
        if (yser_command(s, "BAT", r, sizeof r, 1000) == 0) {
            unsigned mv, cable, chg;
            if (sscanf(r, "OK %u %u %u", &mv, &cable, &chg) == 3) {
                time_t t = time(NULL);
                char ts[32];
                strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&t));
                fprintf(f, "%s,%u,%u,%u\n", ts, mv, cable, chg);
                fflush(f);
                printf("%s  %u mV  cable %u  charging %u\n", ts, mv, cable, chg);
                fflush(stdout);
            }
        } else {
            yser_close(s);
            s = NULL;
        }
        for (int i = 0; i < every * 10 && !quit; i++)
            yser_sleep_ms(100);
    }
    fclose(f);
    yser_close(s);
    return 0;
}
