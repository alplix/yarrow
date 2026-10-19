/* The detector's USB serial port, on Windows and POSIX. Line based: a command goes out, one reply line comes back. */
#ifndef YARROW_SERIAL_H
#define YARROW_SERIAL_H

#include <stddef.h>

typedef struct yser yser_t;

yser_t *yser_open(const char *path);          /* NULL when the port cannot be opened */
void yser_close(yser_t *s);
int yser_command(yser_t *s, const char *cmd, char *reply, size_t cap, int timeout_ms);   /* 0 on an "OK ..." reply */
/* Sends a command and collects every line of the reply up to and including END (for STATUS, CAPS, EVENTS). 0 on success. */
int yser_lines(yser_t *s, const char *cmd, char *out, size_t cap, int timeout_ms);
double yser_now(void);                        /* monotonic seconds */
void yser_sleep_ms(int ms);

/* Tries the serial ports of this machine one by one and returns the first that answers like a Yarrow detector. */
yser_t *yser_find(char *path_out, size_t cap);

#endif
