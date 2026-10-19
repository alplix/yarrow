/* Yarrow counter test for Windows: one small window. Plug the Radioactive@Home counter in and start the program: it tests the
 * counter by itself for 25 minutes (the same test as yarrow_rah_probe), shows what happens and has a button that copies the
 * whole report so it can be pasted into a message. The report is also saved as yarrow_rah_report.txt next to the program. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yarrow_rah_probe.h"

#define ID_COPY 1
#define ID_STOP 2
#define ID_TEXT 3
#define ID_STATUS 4
#define ID_HELP 5
#define WM_LINE (WM_APP + 1)
#define WM_FINISHED (WM_APP + 2)
#define MINUTES 25

static HWND g_main, g_text, g_status, g_copy, g_stop, g_help;
static HFONT g_mono, g_ui;
static volatile int g_stop_flag;
static DWORD g_start_tick;
static int g_running;
static char *g_report;
static size_t g_report_len;
static FILE *g_file;

/* the window is English only: the second text of T() is dropped by the preprocessor */
#define T(en, tr) en

static void out(const char *line)               /* called from the test thread */
{
    char *copy = _strdup(line);
    if (copy)
        PostMessageA(g_main, WM_LINE, 0, (LPARAM)copy);
}

static DWORD WINAPI worker(LPVOID arg)
{
    (void)arg;
    rah_probe_run(MINUTES, out, &g_stop_flag);
    PostMessageA(g_main, WM_FINISHED, 0, 0);
    return 0;
}

static void append_line(const char *line)
{
    size_t n = strlen(line);
    char *r = realloc(g_report, g_report_len + n + 1);
    if (r) {
        g_report = r;
        memcpy(g_report + g_report_len, line, n + 1);
        g_report_len += n;
    }
    if (g_file) {
        fputs(line, g_file);
        fflush(g_file);
    }
    /* the edit control wants CR LF */
    size_t extra = 0;
    for (const char *p = line; *p; p++)
        if (*p == '\n')
            extra++;
    char *buf = malloc(n + extra + 1);
    if (!buf)
        return;
    char *q = buf;
    for (const char *p = line; *p; p++) {
        if (*p == '\n')
            *q++ = '\r';
        *q++ = *p;
    }
    *q = 0;
    int len = GetWindowTextLengthA(g_text);
    SendMessageA(g_text, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(g_text, EM_REPLACESEL, FALSE, (LPARAM)buf);
    free(buf);
}

static void layout(int w, int h)
{
    int pad = 10, top = 10, help_h = 64, bar_h = 30;
    MoveWindow(g_help, pad, top, w - 2 * pad, help_h, TRUE);
    MoveWindow(g_status, pad, top + help_h + 4, w - 2 * pad, 20, TRUE);
    MoveWindow(g_text, pad, top + help_h + 28, w - 2 * pad, h - (top + help_h + 28) - bar_h - pad - 6, TRUE);
    MoveWindow(g_copy, pad, h - bar_h - pad, 200, bar_h, TRUE);
    MoveWindow(g_stop, pad + 210, h - bar_h - pad, 160, bar_h, TRUE);
}

static void copy_report(void)
{
    if (!g_report || !OpenClipboard(g_main))
        return;
    size_t extra = 0;
    for (const char *p = g_report; *p; p++)
        if (*p == '\n')
            extra++;
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, g_report_len + extra + 1);
    if (h) {
        char *d = GlobalLock(h);
        for (const char *p = g_report; *p; p++) {
            if (*p == '\n')
                *d++ = '\r';
            *d++ = *p;
        }
        *d = 0;
        GlobalUnlock(h);
        EmptyClipboard();
        SetClipboardData(CF_TEXT, h);
    }
    CloseClipboard();
    SetWindowTextA(g_status, T("The report is copied: paste it into a message to us.", "Rapor kopyalandı: bize bir mesaja yapıştırın."));
}

static LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        g_main = hwnd;
        g_ui = CreateFontA(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
        g_mono = CreateFontA(-14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
        g_help = CreateWindowA("STATIC", T("Plug the Radioactive@Home counter into the computer and close the Radioactive@Home / BOINC program. "
                                          "The test runs by itself for 25 minutes. When it says Done, press \"Copy report\" and paste the text into a message to us.",
                                          "Radioactive@Home sayacını bilgisayara takın ve Radioactive@Home / BOINC programını kapatın. "
                                          "Test 25 dakika boyunca kendiliğinden çalışır. \"Tamamlandı\" yazınca \"Raporu kopyala\"ya basıp metni bize bir mesaja yapıştırın."),
                               WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, hwnd, (HMENU)ID_HELP, NULL, NULL);
        g_status = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, hwnd, (HMENU)ID_STATUS, NULL, NULL);
        g_text = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 0, 0, 10, 10, hwnd,
                               (HMENU)ID_TEXT, NULL, NULL);
        g_copy = CreateWindowA("BUTTON", T("Copy report", "Raporu kopyala"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED, 0, 0, 10, 10, hwnd, (HMENU)ID_COPY, NULL, NULL);
        g_stop = CreateWindowA("BUTTON", T("Stop the test", "Testi durdur"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, (HMENU)ID_STOP, NULL, NULL);
        SendMessageA(g_help, WM_SETFONT, (WPARAM)g_ui, TRUE);
        SendMessageA(g_status, WM_SETFONT, (WPARAM)g_ui, TRUE);
        SendMessageA(g_text, WM_SETFONT, (WPARAM)g_mono, TRUE);
        SendMessageA(g_copy, WM_SETFONT, (WPARAM)g_ui, TRUE);
        SendMessageA(g_stop, WM_SETFONT, (WPARAM)g_ui, TRUE);
        SendMessageA(g_text, EM_SETLIMITTEXT, 1000000, 0);
        RECT r;
        GetClientRect(hwnd, &r);
        layout(r.right, r.bottom);
        char path[MAX_PATH];
        DWORD n = GetModuleFileNameA(NULL, path, sizeof path);
        while (n > 0 && path[n - 1] != '\\')
            n--;
        path[n] = 0;
        strcat(path, "yarrow_rah_report.txt");
        g_file = fopen(path, "w");
        if (!g_file) {
            GetTempPathA(sizeof path, path);
            strcat(path, "yarrow_rah_report.txt");
            g_file = fopen(path, "w");
        }
        g_start_tick = GetTickCount();
        g_running = 1;
        SetTimer(hwnd, 1, 1000, NULL);
        CloseHandle(CreateThread(NULL, 0, worker, NULL, 0, NULL));
        return 0;
    }
    case WM_SIZE:
        layout(LOWORD(lp), HIWORD(lp));
        return 0;
    case WM_TIMER:
        if (g_running) {
            unsigned s = (GetTickCount() - g_start_tick) / 1000;
            char buf[128];
            snprintf(buf, sizeof buf, "%s  %u:%02u / %d:00", T("Running...", "Çalışıyor..."), s / 60, s % 60, MINUTES);
            SetWindowTextA(g_status, buf);
        }
        return 0;
    case WM_LINE:
        append_line((const char *)lp);
        free((void *)lp);
        return 0;
    case WM_FINISHED:
        g_running = 0;
        KillTimer(hwnd, 1);
        EnableWindow(g_copy, TRUE);
        EnableWindow(g_stop, FALSE);
        SetWindowTextA(g_status, T("Done. Press \"Copy report\" and paste it into a message to us (it is also saved as yarrow_rah_report.txt).",
                                   "Tamamlandı. \"Raporu kopyala\"ya basıp bize bir mesaja yapıştırın (yarrow_rah_report.txt olarak da kaydedildi)."));
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == ID_COPY)
            copy_report();
        else if (LOWORD(wp) == ID_STOP) {
            g_stop_flag = 1;
            EnableWindow(g_stop, FALSE);
        }
        return 0;
    case WM_CLOSE:
        if (g_running && MessageBoxA(hwnd, T("The test is still running. Close it anyway?", "Test hâlâ çalışıyor. Yine de kapatılsın mı?"), "Yarrow", MB_YESNO | MB_ICONQUESTION) != IDYES)
            return 0;
        g_stop_flag = 1;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    (void)prev; (void)cmd;
    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "YarrowRahTest";
    wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    RegisterClassA(&wc);
    HWND w = CreateWindowA("YarrowRahTest", T("Yarrow - Radioactive@Home counter test", "Yarrow - Radioactive@Home sayaç testi"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, 820, 600, NULL, NULL, inst, NULL);
    if (!w)
        return 1;
    ShowWindow(w, show);
    MSG m;
    while (GetMessageA(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
    return 0;
}
