/* Canary B: the hardware layer in read-only mode. No flash writes, no tube high voltage, no USB.
 * Shows live what the device reports so every assumption in hal_gc03.c can be checked by eye:
 *   uptime (SysTick interrupt), keys and knob events, battery mV,
 *   field sensor readings, clock, USB/charge pins, factory calibration page and the raw erased flash pattern.
 * Switches off after 3 minutes or on a long press of the power key. */
#include "board.h"
#include "display.h"
#include "font.h"
#include "fmt.h"
#include "hal.h"
#include "image.h"
#include "regs.h"

const hal_t *hal_gc03_init(void);
void hal_gc03_start_tick(void);

static const char *const names[] = {"yok", "sol", "sag", "tamam", "geri", "guc", "guc uzun"};

static int put(char *b, int n, const char *s)
{
    while (*s)
        b[n++] = *s++;
    b[n] = 0;
    return n;
}

static int hex(char *b, int n, uint32_t v, int digits)
{
    for (int i = digits - 1; i >= 0; i--)
        b[n++] = "0123456789ABCDEF"[(v >> (4 * i)) & 15];
    b[n] = 0;
    return n;
}

static void line(int row, const char *s, uint16_t fg)
{
    const uint16_t bg = RGB565(10, 14, 20);
    display_fill_rect(0, 6 + row * 19, DISPLAY_W, 17, bg);
    font_draw(&font_12_regular, 8, 8 + row * 19, s, fg, bg, 0);
}

static void square(int x, int y, bool on)
{
    display_fill_rect(x, y, 14, 14, on ? RGB565(74, 222, 128) : RGB565(38, 52, 74));
}

int main(void)
{
    const uint16_t white = RGB565(238, 243, 248), dim = RGB565(140, 157, 178), warn = RGB565(250, 204, 21);
    board_init();
    display_init();
    display_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, RGB565(10, 14, 20));
    board_backlight(true);

    const hal_t *h = hal_gc03_init();
    hal_gc03_start_tick();

    uint32_t counts[7] = {0};
    int last = 0;
    uint32_t next = g_ms;
    char b[64];

    line(0, "Yarrow test 2: sadece okuma, bekci yok", warn);

    for (;;) {
        h->watchdog_kick();
        input_event_t ev;
        while ((ev = h->poll_event()) != EV_NONE) {
            counts[ev]++;
            last = (int)ev;
            if (ev == EV_POWER_LONG)
                h->power_off();
        }

        if ((int32_t)(g_ms - next) >= 0) {
            next += 1000;
            int n;

            n = put(b, 0, "calisma sn ");
            fmt_u(b + n, g_ms / 1000);
            line(1, b, white);

            /* live key levels, read straight from the pins (low = pressed) */
            bool pw = !(GPIO_IDR(GPIOA) & (1u << 9)), bk = !(GPIO_IDR(GPIOB) & (1u << 9));
            bool ok = !(GPIO_IDR(GPIOB) & (1u << 8)), kb = !(GPIO_IDR(GPIOB) & (1u << 6));
            line(2, "guc  geri  tamam  dugme", dim);
            square(8, 6 + 3 * 19 + 1, pw);
            square(50, 6 + 3 * 19 + 1, bk);
            square(100, 6 + 3 * 19 + 1, ok);
            square(160, 6 + 3 * 19 + 1, kb);
            display_fill_rect(0, 6 + 3 * 19 + 17, DISPLAY_W, 2, RGB565(10, 14, 20));

            n = put(b, 0, "son olay: ");
            n = put(b, n, names[last]);
            line(4, b, white);
            n = put(b, 0, "sol ");
            n += fmt_u(b + n, counts[EV_LEFT]);
            n = put(b, n, "  sag ");
            n += fmt_u(b + n, counts[EV_RIGHT]);
            n = put(b, n, "  tamam ");
            n += fmt_u(b + n, counts[EV_OK]);
            n = put(b, n, "  geri ");
            fmt_u(b + n, counts[EV_BACK]);
            line(5, b, white);

            n = put(b, 0, "pil mV ");
            n += fmt_u(b + n, h->battery_mv());
            n = put(b, n, h->usb_present() ? "  usb var" : "  usb yok");
            n = put(b, n, h->charging() ? "  sarj" : "");
            line(6, b, white);

            n = put(b, 0, "E ");
            n += fmt_u(b + n, h->efield_vm_x10());
            n = put(b, n, "  H ");
            fmt_u(b + n, h->hfield_ut_x100());
            line(7, b, white);

            n = put(b, 0, "saat ");
            fmt_u(b + n, h->rtc_now());
            line(8, b, white);

            uint32_t sig = *(volatile uint32_t *)FACTORY_SIGNATURE;
            n = put(b, 0, "fabrika imza ");
            n = hex(b, n, sig, 8);
            n = put(b, n, h->hv_factory_valid() ? " gecerli" : " GECERSIZ");
            line(10, b, h->hv_factory_valid() ? white : warn);
            n = put(b, 0, "hv frekans (x100 Hz) ");
            fmt_u(b + n, REG16(FACTORY_HV_FREQ));
            line(11, b, white);

            /* raw reads of flash that is erased or unused: the CH32F2 is said to read erased cells as E339 */
            n = put(b, 0, "ham 0x0803E000 ");
            n = hex(b, n, REG16(0x0803E000u), 4);
            n = put(b, n, "  0x0803E7F0 ");
            hex(b, n, REG16(0x0803E7F0u), 4);
            line(12, b, white);
            n = put(b, 0, "ham KV 0x0801A000 ");
            hex(b, n, REG16(KV_BASE), 4);
            line(13, b, white);

            if (g_ms > 180000)
                h->power_off();
        }
        __asm__ volatile("wfi");
    }
}
