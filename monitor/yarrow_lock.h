/* One monitor per computer: two of them would fight for the same detector. */
#ifndef YARROW_LOCK_H
#define YARROW_LOCK_H

/* Takes an exclusive lock on the file (created when missing) and keeps it until the process ends. 1 when this process holds it,
 * 0 when another process already does. A file that cannot be opened at all counts as "no other monitor" (1). */
int ylock_acquire(const char *path);

#endif
