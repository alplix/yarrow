#ifndef YARROW_RAH_PROBE_H
#define YARROW_RAH_PROBE_H

/* Runs the counter test for `minutes` (or until *stop becomes non-zero); every line of output goes to out(). 0 when it ran. */
int rah_probe_run(int minutes, void (*out)(const char *), volatile int *stop);

#endif
