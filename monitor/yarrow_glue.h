/* What the monitor needs from its host: BOINC (yarrow_boinc.cpp) or a plain command line (yarrow_standalone.c). */
#ifndef YARROW_GLUE_H
#define YARROW_GLUE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t interval;     /* 600, 1800 or 3600 */
    char cc[3];
    char city[41];
    char port[64];         /* empty: find the detector */
} yprefs_t;

int glue_init(int argc, char **argv, yprefs_t *prefs);       /* 0 ok */
size_t glue_checkpoint_read(char *buf, size_t cap);
void glue_checkpoint_write(const char *text, size_t n);
int glue_send_trickle(const char *text);                      /* 0 when it was handed over */
int glue_quit_requested(void);
/* The file that keeps a second monitor of this computer out (a lock is taken on it). */
const char *glue_lock_path(void);
void glue_log(const char *line);
void glue_done(int status);
/* The settings are missing or wrong: tell the volunteer and try again later (BOINC restarts the task after an hour). */
void glue_wait_for_prefs(const char *why);
/* No detector has been found for a while: BOINC puts the task aside for six hours so it takes no CPU; the command-line host
 * simply keeps looking. */
void glue_no_detector(const char *why);
/* A second monitor on the same computer: it waits long, one is enough. */
void glue_other_monitor(const char *why);

#ifdef __cplusplus
}
#endif

#endif
