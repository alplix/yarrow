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
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
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
    int r, tries = 0;
    do {
        r = ioctl(p->fd, HIDIOCGFEATURE(len), buf);
    } while (r < 0 && errno == EINTR && ++tries < 5);          /* a signal can interrupt the request: ask again */
    return r;
}

static void posix_close(void *ctx)
{
    posix_hid_t *p = ctx;
    close(p->fd);
    free(p);
}

/* the USB ids of /dev/hidrawN from sysfs: readable by everyone, also when the node itself may not be opened */
static int sysfs_ids(const char *node, unsigned *vid, unsigned *pid)
{
    static const char *up[] = {"../", "../../", "../../../", NULL};
    for (int i = 0; up[i]; i++) {
        char pv[300], pp[300];
        snprintf(pv, sizeof pv, "/sys/class/hidraw/%s/device/%sidVendor", node, up[i]);
        snprintf(pp, sizeof pp, "/sys/class/hidraw/%s/device/%sidProduct", node, up[i]);
        FILE *fv = fopen(pv, "r"), *fp = fopen(pp, "r");
        int ok = fv && fp && fscanf(fv, "%x", vid) == 1 && fscanf(fp, "%x", pid) == 1;
        if (fv)
            fclose(fv);
        if (fp)
            fclose(fp);
        if (ok)
            return 1;
    }
    return 0;
}

/* each hidraw device that was opened but did not match is reported once, with its identity: this is what
 * a volunteer needs when a detector is present but not accepted (wrong VID:PID, different name) */
static void yhid_see(const char *path, unsigned vid, unsigned pid, const char *name)
{
    static struct { char path[64]; } seen[64];
    static int n;
    for (int i = 0; i < n; i++)
        if (strcmp(seen[i].path, path) == 0)
            return;
    if (n < 64) {
        snprintf(seen[n].path, sizeof seen[n].path, "%.63s", path);
        n++;
    }
    if (name[0])
        fprintf(stderr, "yhid: %s: %04x:%04x \"%s\"\n", path, vid, pid, name);
    else
        fprintf(stderr, "yhid: %s: %04x:%04x\n", path, vid, pid);
}

/* Each hidraw problem is reported once (not again on every scan, the monitor looks for a detector every
 * 15 seconds): a volunteer debugging "no detector found" then sees what the app actually saw. */
static void yhid_note(const char *path, int err)
{
    static struct { char path[64]; int err; } seen[64];
    static int n;
    for (int i = 0; i < n; i++)
        if (seen[i].err == err && strcmp(seen[i].path, path) == 0)
            return;
    if (n < 64) {
        snprintf(seen[n].path, sizeof seen[n].path, "%.63s", path);
        seen[n].err = err;
        n++;
    }
    if (err == EACCES)
        fprintf(stderr, "yhid: %s: this is the counter the monitor looks for, but the BOINC client's user may not open it. "
                        "Install the udev rule from %s, then unplug the counter and plug it in again.\n", path, "https://yarrow.athena.org.tr/linux");
    else if (err == ENOENT || err == ENODEV)
        fprintf(stderr, "yhid: %s: the device went away\n", path);
    else
        fprintf(stderr, "yhid: %s: %s (errno %d)\n", path, strerror(err), err);
}

int yhid_open(uint16_t vid, uint16_t pid, const char *name_part, yhid_t *out)
{
    unsigned found = 0;                    /* hidraw nodes present this scan */
    DIR *d = opendir("/dev");
    if (!d)
        return -1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, "hidraw", 6) != 0)
            continue;
        found++;
        char path[300];
        snprintf(path, sizeof path, "/dev/%s", e->d_name);
        int fd = open(path, O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            int err = errno;
            unsigned sv = 0, sp = 0;
            /* a keyboard or mouse the BOINC user may not open is normal and not worth a line; our counter is */
            if (err != EACCES || (sysfs_ids(e->d_name, &sv, &sp) && sv == vid && sp == pid))
                yhid_note(path, err);
            continue;
        }
        struct yhidraw_devinfo info;
        char name[96] = "";
        int got = ioctl(fd, HIDIOCGRAWINFO, &info) == 0;
        if (got && (uint16_t)info.vendor == vid && (uint16_t)info.product == pid) {
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
                close(fd);
                closedir(d);
                return -1;
            }
            yhid_see(path, (unsigned)(uint16_t)info.vendor, (unsigned)(uint16_t)info.product, name);
        } else {
            yhid_see(path, got ? (unsigned)(uint16_t)info.vendor : 0, got ? (unsigned)(uint16_t)info.product : 0, "");
        }
        close(fd);
    }
    closedir(d);
    /* a one-line summary whenever the number of hidraw devices differs from the last scan of the same
     * VID:PID, so a plugged-in device is visible in the log without repeating lines every 15 seconds */
    static struct { uint16_t vid, pid; unsigned found; } rep[8];
    int have = 0;
    for (int i = 0; i < 8; i++)
        if (rep[i].vid == vid && rep[i].pid == pid)
            have = rep[i].found == found;
    if (!have) {
        fprintf(stderr, "yhid: %u hidraw device(s) present, none matched %04x:%04x this scan\n", found, vid, pid);
        int free_slot = -1;
        for (int i = 0; i < 8; i++)
            if (rep[i].vid == (uint16_t)vid && rep[i].pid == (uint16_t)pid) {
                free_slot = i;
                break;
            }
        if (free_slot < 0)
            for (int i = 0; i < 8; i++)
                if (rep[i].vid == 0 && rep[i].pid == 0) {
                    free_slot = i;
                    break;
                }
        if (free_slot >= 0) {
            rep[free_slot].vid = (uint16_t)vid;
            rep[free_slot].pid = (uint16_t)pid;
            rep[free_slot].found = found;
        }
    }
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
