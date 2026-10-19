#if !defined(_WIN32) && !defined(__APPLE__)
#define _DEFAULT_SOURCE 1
#endif
#include "yarrow_serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

struct yser { HANDLE h; };

yser_t *yser_open(const char *path)
{
    char full[64];
    snprintf(full, sizeof full, "\\\\.\\%s", path);
    HANDLE h = CreateFileA(full, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return NULL;
    DCB dcb;
    memset(&dcb, 0, sizeof dcb);
    dcb.DCBlength = sizeof dcb;
    GetCommState(h, &dcb);
    dcb.BaudRate = 115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;      /* the device streams only to a host that has the port open */
    SetCommState(h, &dcb);
    COMMTIMEOUTS to = {20, 0, 0, 0, 0};
    SetCommTimeouts(h, &to);
    yser_t *s = malloc(sizeof *s);
    if (!s) {
        CloseHandle(h);
        return NULL;
    }
    s->h = h;
    return s;
}

void yser_close(yser_t *s)
{
    if (s) {
        CloseHandle(s->h);
        free(s);
    }
}

static int rd(yser_t *s, char *c)
{
    DWORD n = 0;
    return ReadFile(s->h, c, 1, &n, NULL) && n == 1;
}

static int wr(yser_t *s, const char *p, size_t n)
{
    DWORD w = 0;
    return WriteFile(s->h, p, (DWORD)n, &w, NULL) && w == n;
}

double yser_now(void)
{
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
}

void yser_sleep_ms(int ms) { Sleep((DWORD)ms); }

static int port_name(int i, char *out, size_t cap)
{
    snprintf(out, cap, "COM%d", i + 1);
    return i < 64;
}
#else
#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>

struct yser { int fd; };

yser_t *yser_open(const char *path)
{
    int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
        return NULL;
    struct termios t;
    if (tcgetattr(fd, &t) == 0) {
        cfmakeraw(&t);
        cfsetspeed(&t, B115200);
        t.c_cflag |= CLOCAL | CREAD;
        tcsetattr(fd, TCSANOW, &t);
    }
    yser_t *s = malloc(sizeof *s);
    if (!s) {
        close(fd);
        return NULL;
    }
    s->fd = fd;
    return s;
}

void yser_close(yser_t *s)
{
    if (s) {
        close(s->fd);
        free(s);
    }
}

static int rd(yser_t *s, char *c)
{
    struct pollfd p = {s->fd, POLLIN, 0};
    return poll(&p, 1, 20) > 0 && read(s->fd, c, 1) == 1;
}

static int wr(yser_t *s, const char *p, size_t n) { return write(s->fd, p, n) == (ssize_t)n; }

double yser_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
}

void yser_sleep_ms(int ms)
{
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}
#endif

int yser_command(yser_t *s, const char *cmd, char *reply, size_t cap, int timeout_ms)
{
    char line[160];
    size_t n = strlen(cmd);
    if (n + 2 > sizeof line)
        return -1;
    memcpy(line, cmd, n);
    line[n] = '\n';
    if (!wr(s, line, n + 1))
        return -1;
    double end = yser_now() + timeout_ms / 1000.0;
    size_t len = 0;
    while (yser_now() < end) {
        char c;
        if (!rd(s, &c))
            continue;
        if (c == '\r')
            continue;
        if (c != '\n') {
            if (len + 1 < cap && len < sizeof line - 1)
                reply[len++] = c;
            continue;
        }
        reply[len] = 0;
        if (reply[0] == 'L' && reply[1] == ' ') {      /* a live-stream line, not the reply */
            len = 0;
            continue;
        }
        if (len == 0)
            continue;
        return strncmp(reply, "OK", 2) == 0 ? 0 : -1;
    }
    return -1;
}

int yser_lines(yser_t *s, const char *cmd, char *out, size_t cap, int timeout_ms)
{
    char line[160];
    size_t n = strlen(cmd);
    if (n + 2 > sizeof line)
        return -1;
    memcpy(line, cmd, n);
    line[n] = '\n';
    if (!wr(s, line, n + 1))
        return -1;
    size_t len = 0;
    double end = yser_now() + timeout_ms / 1000.0;
    while (yser_now() < end) {
        char c;
        if (!rd(s, &c))
            continue;
        if (c == '\r')
            continue;
        if (len + 2 >= cap)
            return -1;
        out[len++] = c;
        out[len] = 0;
        if (c == '\n' && len >= 4 && strcmp(out + len - 4, "END\n") == 0 && (len == 4 || out[len - 5] == '\n'))
            return strncmp(out, "OK", 2) == 0 ? 0 : -1;
        if (len >= 4 && out[0] == 'E' && out[1] == 'R' && out[2] == 'R' && c == '\n')
            return -1;
    }
    return -1;
}

yser_t *yser_find(char *path_out, size_t cap)
{
    char reply[160];
#ifdef _WIN32
    for (int i = 0; i < 64; i++) {
        char name[16];
        port_name(i, name, sizeof name);
        yser_t *s = yser_open(name);
        if (!s)
            continue;
        yser_command(s, "LIVE 0", reply, sizeof reply, 300);
        if (yser_command(s, "INFO", reply, sizeof reply, 600) == 0 && strstr(reply, "yarrow=")) {
            snprintf(path_out, cap, "%s", name);
            return s;
        }
        yser_close(s);
    }
#else
    static const char *pats[] = {"/dev/ttyACM*", "/dev/cu.usbmodem*", NULL};
    for (int k = 0; pats[k]; k++) {
        glob_t g;
        if (glob(pats[k], 0, NULL, &g) != 0)
            continue;
        for (size_t i = 0; i < g.gl_pathc; i++) {
            yser_t *s = yser_open(g.gl_pathv[i]);
            if (!s)
                continue;
            yser_command(s, "LIVE 0", reply, sizeof reply, 300);
            if (yser_command(s, "INFO", reply, sizeof reply, 600) == 0 && strstr(reply, "yarrow=")) {
                snprintf(path_out, cap, "%s", g.gl_pathv[i]);
                globfree(&g);
                return s;
            }
            yser_close(s);
        }
        globfree(&g);
    }
#endif
    return NULL;
}
