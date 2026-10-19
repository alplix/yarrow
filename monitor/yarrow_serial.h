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


/* Raw access for counters that do not speak the line protocol (binary replies). */
yser_t *yser_open_baud(const char *path, int baud);
int yser_write(yser_t *s, const void *p, size_t n);                 /* 0 on success */
int yser_read(yser_t *s, void *buf, size_t n, int timeout_ms);      /* bytes read, fewer than n on timeout */
void yser_drain(yser_t *s);                                         /* throw away what is waiting */
int yser_ports(char out[][64], int max);                            /* serial ports this computer may have; returns how many */

#endif
