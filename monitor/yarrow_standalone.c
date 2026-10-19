/* Command-line host for the monitor: prints the trickles instead of sending them, for trying a detector without BOINC.
 *   yarrow_monitor --country TR --city Denizli [--interval 600] [--port COM5] [--state yarrow_state.txt]            */
#include "yarrow_glue.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile sig_atomic_t quit;
static const char *state_path = "yarrow_state.txt";

static void on_signal(int sig)
{
    (void)sig;
    quit = 1;
}

int glue_init(int argc, char **argv, yprefs_t *p)
{
    p->interval = 600;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "--interval"))
            p->interval = (uint32_t)atoi(argv[i + 1]);
        else if (!strcmp(argv[i], "--country"))
            snprintf(p->cc, sizeof p->cc, "%s", argv[i + 1]);
        else if (!strcmp(argv[i], "--city"))
            snprintf(p->city, sizeof p->city, "%s", argv[i + 1]);
        else if (!strcmp(argv[i], "--port"))
            snprintf(p->port, sizeof p->port, "%s", argv[i + 1]);
        else if (!strcmp(argv[i], "--state"))
            state_path = argv[i + 1];
        else {
            fprintf(stderr, "unknown option %s\n", argv[i]);
            return -1;
        }
    }
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    return 0;
}

size_t glue_checkpoint_read(char *buf, size_t cap)
{
    FILE *f = fopen(state_path, "rb");
    if (!f)
        return 0;
    size_t n = fread(buf, 1, cap, f);
    fclose(f);
    return n;
}

void glue_checkpoint_write(const char *text, size_t n)
{
    FILE *f = fopen(state_path, "wb");
    if (!f)
        return;
    fwrite(text, 1, n, f);
    fclose(f);
}

int glue_send_trickle(const char *text)
{
    printf("TRICKLE %s\n", text);
    fflush(stdout);
    return 0;
}

const char *glue_lock_path(void)
{
    static char p[300];
    snprintf(p, sizeof p, "%s.lock", state_path);
    return p;
}

int glue_quit_requested(void) { return quit; }

void glue_log(const char *line)
{
    time_t t = time(NULL);
    char ts[32];
    strftime(ts, sizeof ts, "%H:%M:%S", localtime(&t));
    fprintf(stderr, "%s %s\n", ts, line);
}

void glue_done(int status) { (void)status; }

void glue_wait_for_prefs(const char *why)
{
    glue_log(why);
    exit(1);
}

void glue_no_detector(const char *why) { (void)why; }
