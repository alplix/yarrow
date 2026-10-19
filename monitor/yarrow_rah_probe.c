/* Console test program for the Radioactive@Home USB counter (see yarrow_rah_probe_core.c). It needs no BOINC and no account;
 * everything printed is also written to yarrow_rah_probe.txt so it can be sent back.
 *   yarrow_rah_probe [minutes]        (default 25: long enough to complete at least one 10-minute bin) */
#include "yarrow_rah_probe.h"

#include <stdio.h>
#include <stdlib.h>

static FILE *log_file;
static volatile int never;

static void out(const char *line)
{
    fputs(line, stdout);
    fflush(stdout);
    if (log_file) {
        fputs(line, log_file);
        fflush(log_file);
    }
}

int main(int argc, char **argv)
{
    int minutes = argc > 1 ? atoi(argv[1]) : 25;
    if (minutes < 1)
        minutes = 25;
    log_file = fopen("yarrow_rah_probe.txt", "w");
    int rc = rah_probe_run(minutes, out, &never);
    out("\nPlease send the file yarrow_rah_probe.txt (it is in the folder you started the program from).\n");
    return rc;
}
