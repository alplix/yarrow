#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "yarrow_hid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>

typedef struct { HANDLE h; } win_hid_t;

static int win_get_feature(void *ctx, uint8_t *buf, size_t len)
{
    win_hid_t *w = ctx;
    if (!HidD_GetFeature(w->h, buf, (ULONG)len))
        return -1;
    return (int)len;
}

static void win_close(void *ctx)
{
    win_hid_t *w = ctx;
    CloseHandle(w->h);
    free(w);
}

static void narrow(const wchar_t *w, char *out, size_t cap)
{
    size_t i = 0;
    for (; w[i] && i + 1 < cap; i++)
        out[i] = w[i] < 128 ? (char)w[i] : '?';
    out[i] = 0;
}

int yhid_open(uint16_t vid, uint16_t pid, const char *name_part, yhid_t *out)
{
    GUID g;
    HidD_GetHidGuid(&g);
    HDEVINFO set = SetupDiGetClassDevs(&g, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE)
        return -1;
    for (DWORD i = 0;; i++) {
        SP_DEVICE_INTERFACE_DATA di;
        di.cbSize = sizeof di;
        if (!SetupDiEnumDeviceInterfaces(set, NULL, &g, i, &di))
            break;
        DWORD need = 0;
        SetupDiGetDeviceInterfaceDetailA(set, &di, NULL, 0, &need, NULL);
        if (!need)
            continue;
        SP_DEVICE_INTERFACE_DETAIL_DATA_A *d = malloc(need);
        if (!d)
            continue;
        d->cbSize = sizeof *d;
        if (SetupDiGetDeviceInterfaceDetailA(set, &di, d, need, NULL, NULL)) {
            HANDLE h = CreateFileA(d->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                   OPEN_EXISTING, 0, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                HIDD_ATTRIBUTES a;
                a.Size = sizeof a;
                wchar_t wn[96] = L"", ws[64] = L"";
                if (HidD_GetAttributes(h, &a) && a.VendorID == vid && a.ProductID == pid) {
                    HidD_GetProductString(h, wn, sizeof wn);
                    HidD_GetSerialNumberString(h, ws, sizeof ws);
                    char name[96];
                    narrow(wn, name, sizeof name);
                    if (!name_part || strstr(name, name_part)) {
                        win_hid_t *w = malloc(sizeof *w);
                        if (w) {
                            w->h = h;
                            out->ctx = w;
                            out->get_feature = win_get_feature;
                            out->close = win_close;
                            snprintf(out->name, sizeof out->name, "%s", name);
                            narrow(ws, out->serial, sizeof out->serial);
                            out->version = a.VersionNumber;
                            free(d);
                            SetupDiDestroyDeviceInfoList(set);
                            return 0;
                        }
                    }
                }
                CloseHandle(h);
            }
        }
        free(d);
    }
    SetupDiDestroyDeviceInfoList(set);
    return -1;
}

double yhost_wall(void)
{
    FILETIME ft;
    typedef VOID(WINAPI * precise_fn)(LPFILETIME);
    static precise_fn precise;
    static int looked;
    if (!looked) {
        HMODULE k = GetModuleHandleA("kernel32.dll");
        precise = k ? (precise_fn)(void *)GetProcAddress(k, "GetSystemTimePreciseAsFileTime") : NULL;
        looked = 1;
    }
    if (precise)
        precise(&ft);
    else
        GetSystemTimeAsFileTime(&ft);
    unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    return (double)(t - 116444736000000000ULL) / 1e7;
}

void yhost_name(char *out, size_t cap)
{
    DWORD n = (DWORD)cap;
    if (!GetComputerNameA(out, &n))
        out[0] = 0;
}

#else /* POSIX */

#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

/* the hidraw ioctls, spelled out so that old kernel headers are not needed */
#ifndef HIDIOCGRAWINFO
struct yhidraw_devinfo {
    uint32_t bustype;
    int16_t vendor;
    int16_t product;
};
#define HIDIOCGRAWINFO _IOR('H', 0x03, struct yhidraw_devinfo)
#define HIDIOCGRAWNAME(len) _IOC(_IOC_READ, 'H', 0x04, len)
#define HIDIOCGRAWUNIQ(len) _IOC(_IOC_READ, 'H', 0x08, len)
#define HIDIOCGFEATURE(len) _IOC(_IOC_READ | _IOC_WRITE, 'H', 0x07, len)
#endif

#ifdef __linux__
typedef struct { int fd; } posix_hid_t;

/* the USB device release number of /dev/hidrawN, from sysfs (the USB device is one or two levels above the HID device) */
static uint16_t sysfs_bcd(const char *node)
{
    static const char *up[] = {"../", "../../", NULL};
    for (int i = 0; up[i]; i++) {
        char path[300];
        snprintf(path, sizeof path, "/sys/class/hidraw/%s/device/%sbcdDevice", node, up[i]);
        FILE *f = fopen(path, "r");
        if (!f)
            continue;
        unsigned v = 0;
        int ok = fscanf(f, "%x", &v) == 1;
        fclose(f);
        if (ok)
            return (uint16_t)v;
    }
    return 0;
}

static int posix_get_feature(void *ctx, uint8_t *buf, size_t len)
{
    posix_hid_t *p = ctx;
    return ioctl(p->fd, HIDIOCGFEATURE(len), buf);
}

static void posix_close(void *ctx)
{
    posix_hid_t *p = ctx;
    close(p->fd);
    free(p);
}

int yhid_open(uint16_t vid, uint16_t pid, const char *name_part, yhid_t *out)
{
    DIR *d = opendir("/dev");
    if (!d)
        return -1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, "hidraw", 6) != 0)
            continue;
        char path[300];
        snprintf(path, sizeof path, "/dev/%s", e->d_name);
        int fd = open(path, O_RDWR | O_CLOEXEC);
        if (fd < 0)
            continue;
        struct yhidraw_devinfo info;
        char name[96] = "";
        if (ioctl(fd, HIDIOCGRAWINFO, &info) == 0 && (uint16_t)info.vendor == vid && (uint16_t)info.product == pid) {
            ioctl(fd, HIDIOCGRAWNAME(sizeof name - 1), name);
            if (!name_part || strstr(name, name_part)) {
                posix_hid_t *p = malloc(sizeof *p);
                if (p) {
                    p->fd = fd;
                    out->ctx = p;
                    out->get_feature = posix_get_feature;
                    out->close = posix_close;
                    snprintf(out->name, sizeof out->name, "%s", name);
                    out->serial[0] = 0;
                    ioctl(fd, HIDIOCGRAWUNIQ(sizeof out->serial - 1), out->serial);
                    out->version = sysfs_bcd(e->d_name);
                    closedir(d);
                    return 0;
                }
            }
        }
        close(fd);
    }
    closedir(d);
    return -1;
}
#else
int yhid_open(uint16_t vid, uint16_t pid, const char *name_part, yhid_t *out)
{
    (void)vid; (void)pid; (void)name_part; (void)out;
    return -1;                       /* no HID access on this system yet */
}
#endif

double yhost_wall(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
}

void yhost_name(char *out, size_t cap)
{
    if (gethostname(out, cap) != 0)
        out[0] = 0;
    else
        out[cap - 1] = 0;
}
#endif
