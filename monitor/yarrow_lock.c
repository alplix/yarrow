#include "yarrow_lock.h"

#ifdef _WIN32
#include <windows.h>

int ylock_acquire(const char *path)
{
    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return 1;
    OVERLAPPED ov;
    ZeroMemory(&ov, sizeof ov);
    if (LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ov))
        return 1;                       /* the handle stays open: the lock lives as long as the process */
    CloseHandle(h);
    return 0;
}
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

int ylock_acquire(const char *path)
{
    int fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0)
        return 1;
    if (flock(fd, LOCK_EX | LOCK_NB) == 0)
        return 1;                       /* the descriptor stays open: the lock lives as long as the process */
    close(fd);
    return 0;
}
#endif
