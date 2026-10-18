/* The GC-03 behind hal_t. Written from the pin map and the STM32F1-style register layout of the CH32F203;
 * nothing here has run on the device yet, which is why the application brings features up one stage at a time. */

#include "board.h"
#include "display.h"
#include "hal.h"
#include "image.h"
#include "regs.h"
#include <stdbool.h>

#define TICK_HZ 8000u
#define MASK_ERASED 0x1CC6u /* the CH32F2 reads erased flash as 0xE339: store data XOR this so erased reads 0xFFFF */

/* ---- shared state, touched by the tick and tube interrupts ---- */
static volatile uint32_t pulses;
static volatile uint8_t tick8;
static volatile uint32_t buzz_inc, buzz_phase;
static volatile uint8_t bl_duty = 16, bl_phase; /* full brightness until the app sets a level: the screen must not go dark when the tick starts */
static volatile uint8_t pulse_led_ms;
static volatile bool pulse_led_en = true;
static volatile bool click_en, vibe_en;
static volatile uint8_t click_left, vibe_ms;
static volatile bool alert_on;

static volatile input_event_t evq[8];
static volatile uint8_t evh, evt;

static void ev_push(input_event_t e)
{
    uint8_t n = (uint8_t)((evt + 1) & 7);
    if (n != evh) {
        evq[evt] = e;
        evt = n;
    }
}

static input_event_t h_poll_event(void)
{
    if (evh == evt)
        return EV_NONE;
    input_event_t e = evq[evh];
    evh = (uint8_t)((evh + 1) & 7);
    return e;
}

/* ---- EMF sampling runs inside the tick interrupt: 320 samples, 125 us apart = 40 ms = two mains cycles ---- */
#define EMF_N 320u
static volatile uint8_t emf_state; /* 0 idle, 1 sampling, 2 done */
static volatile uint32_t emf_n, emf_sum;
static volatile uint64_t emf_sumsq;

static void adc_start(void) { ADC1_CR2 |= 1u << 22; }

/* ---- keys and knob, scanned at 1 kHz ---- */
typedef struct {
    uint8_t hist;
    bool down;
    uint32_t held;
} key_t;

static key_t k_pwr, k_back, k_ok;
static bool pwr_armed, pwr_long_sent, combo_sent;
static int8_t knob_prev, knob_sub;
static const int8_t knob_lut[16] = {0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0};
#define KNOB_INVERT 0

static bool debounce(key_t *k, bool raw)
{
    k->hist = (uint8_t)((k->hist << 1) | (raw ? 1 : 0));
    bool was = k->down;
    if ((k->hist & 0x0F) == 0x0F)
        k->down = true;
    else if ((k->hist & 0x0F) == 0)
        k->down = false;
    if (k->down)
        k->held++;
    else
        k->held = 0;
    return k->down && !was; /* a press edge */
}

static void scan_charger(void);

static void scan_inputs(void)
{
    bool p = !(GPIO_IDR(GPIOA) & (1u << 9));
    bool back = !(GPIO_IDR(GPIOB) & (1u << 9));
    bool ok = !(GPIO_IDR(GPIOB) & (1u << 8)) || !(GPIO_IDR(GPIOB) & (1u << 6));

    bool pwr_was_down = k_pwr.down;
    uint32_t pwr_held_before = k_pwr.held;
    debounce(&k_pwr, p);
    if (!k_pwr.down) {
        if (!pwr_armed && k_pwr.hist == 0)
            pwr_armed = true; /* seen released: from now on presses count */
        if (pwr_was_down && pwr_armed && !pwr_long_sent && pwr_held_before >= 30)
            ev_push(EV_POWER);
        pwr_long_sent = false;
    } else if (pwr_armed && !pwr_long_sent && k_pwr.held >= 1500) {
        pwr_long_sent = true;
        ev_push(EV_POWER_LONG);
    }

    if (debounce(&k_back, back))
        ev_push(EV_BACK);
    if (debounce(&k_ok, ok))
        ev_push(EV_OK);

    /* back and ok together for two seconds also switch off, in case the power key reads the wrong way round */
    if (k_back.down && k_ok.down) {
        if (!combo_sent && k_back.held >= 2000 && k_ok.held >= 2000) {
            combo_sent = true;
            ev_push(EV_POWER_LONG);
        }
    } else {
        combo_sent = false;
    }

    int8_t cur = (int8_t)((((GPIO_IDR(GPIOB) >> 5) & 1u) << 1) | ((GPIO_IDR(GPIOB) >> 7) & 1u));
    int8_t d = knob_lut[(knob_prev << 2) | cur];
    knob_prev = cur;
    if (d) {
        knob_sub = (int8_t)(knob_sub + d);
        if (knob_sub >= 2) {
            knob_sub = 0;
            ev_push(KNOB_INVERT ? EV_LEFT : EV_RIGHT);
        } else if (knob_sub <= -2) {
            knob_sub = 0;
            ev_push(KNOB_INVERT ? EV_RIGHT : EV_LEFT);
        }
    }

    scan_charger();

    if (pulse_led_ms && --pulse_led_ms == 0)
        GPIO_BSRR(GPIOB) = 1u << 14; /* pulse LED off (active low) */
    if (vibe_ms && --vibe_ms == 0)
        GPIO_BSRR(GPIOB) = 1u << (1 + 16);
}

/* ---- interrupts ---- */
void systick_handler(void)
{
    tick8++;

    if (click_left) { /* a 4 kHz burst of three milliseconds: the classic counter click */
        click_left--;
        GPIO_BSRR(GPIOC) = (click_left & 1) ? (1u << 11) : (1u << (11 + 16));
    } else if (buzz_inc) {
        buzz_phase += buzz_inc;
        GPIO_BSRR(GPIOC) = (buzz_phase & 0x80000000u) ? (1u << 11) : (1u << (11 + 16));
    }

    bl_phase = (uint8_t)((bl_phase + 1) & 15);
    GPIO_BSRR(GPIOB) = (bl_phase < bl_duty) ? (1u << 13) : (1u << (13 + 16));

    if (emf_state == 1 && (ADC1_SR & 2u)) {
        uint32_t v = ADC1_DR & 0xFFFu;
        emf_sum += v;
        emf_sumsq += (uint64_t)v * v;
        if (++emf_n >= EMF_N)
            emf_state = 2;
        else
            adc_start();
    }

    if ((tick8 & 7) == 0) {
        g_ms++;
        scan_inputs();
    }
}

void exti3_handler(void)
{
    EXTI_PR = 1u << 3;
    pulses++;
    if (click_en && !buzz_inc)
        click_left = 24;
    if (vibe_en) {
        vibe_ms = 25;
        GPIO_BSRR(GPIOB) = 1u << 1;
    }
    if (pulse_led_en) {
        pulse_led_ms = 3;
        GPIO_BSRR(GPIOB) = 1u << (14 + 16); /* pulse LED on */
    }
}

/* ---- sound, light, vibration ---- */
static void h_beep(uint32_t hz)
{
    if (hz == 0) {
        buzz_inc = 0;
        GPIO_BSRR(GPIOC) = 1u << (11 + 16);
    } else {
        buzz_inc = (uint32_t)(((uint64_t)hz << 32) / TICK_HZ);
    }
}

static void h_vibrate(bool on) { GPIO_BSRR(GPIOB) = on ? (1u << 1) : (1u << (1 + 16)); }
static void h_alert_led(bool on) { GPIO_BSRR(GPIOB) = on ? (1u << (15 + 16)) : (1u << 15); }
static void h_status_led(bool on) { GPIO_BSRR(GPIOC) = on ? (1u << (8 + 16)) : (1u << 8); }
static void h_count_click_enable(bool on)
{
    click_en = on;
}

static void h_count_vibe_enable(bool on)
{
    vibe_en = on;
    if (!on)
        GPIO_BSRR(GPIOB) = 1u << (1 + 16);
}

static void h_pulse_led_enable(bool on)
{
    pulse_led_en = on;
    if (!on)
        GPIO_BSRR(GPIOB) = 1u << 14; /* off (active low) */
}

static void h_backlight(uint8_t level)
{
    static const uint8_t duty[6] = {0, 2, 4, 8, 12, 16};
    bl_duty = duty[level > 5 ? 5 : level];
}

static void h_display_enable(bool on)
{
    display_sleep(!on);
    if (!on)
        bl_duty = 0;
}

static void h_power_off(void) { board_power_off(); }
static void h_watchdog_kick(void) { watchdog_kick(); }

/* ---- time ---- */
static bool rtc_ok;

static bool rtc_wait(void)
{
    for (uint32_t g = 0; g < 400000; g++)
        if (RTC_CRL & (1u << 5))
            return true;
    return false;
}

static void rtc_init(void)
{
    RCC_APB1ENR |= (1u << 28) | (1u << 27); /* PWR, BKP */
    PWR_CR |= 1u << 8;                      /* backup domain write access */
    if (!(RCC_BDCR & (1u << 15))) {         /* RTC not running yet */
        RCC_BDCR |= 1u;                     /* LSE on */
        uint32_t g = 0;
        while (!(RCC_BDCR & 2u) && g++ < 2000000)
            ;
        if (!(RCC_BDCR & 2u))
            return; /* no crystal: fall back to the millisecond clock */
        RCC_BDCR = (RCC_BDCR & ~(3u << 8)) | (1u << 8) | (1u << 15);
        if (!rtc_wait())
            return;
        RTC_CRL |= 1u << 4;
        RTC_PRLH = 0;
        RTC_PRLL = 32767;
        RTC_CNTH = 0;
        RTC_CNTL = 0;
        RTC_CRL &= ~(1u << 4);
        rtc_wait();
    }
    rtc_ok = true;
}

static uint32_t h_rtc_now(void)
{
    if (!rtc_ok)
        return g_ms / 1000u;
    uint32_t hi, lo;
    do {
        hi = RTC_CNTH;
        lo = RTC_CNTL;
    } while (hi != RTC_CNTH);
    return (hi << 16) | lo;
}

static void h_rtc_set(uint32_t t)
{
    if (!rtc_ok || !rtc_wait())
        return;
    RTC_CRL |= 1u << 4;
    RTC_CNTH = t >> 16;
    RTC_CNTL = t & 0xFFFFu;
    RTC_CRL &= ~(1u << 4);
    rtc_wait();
}

/* ---- ADC ---- */
static void adc_init(void)
{
    RCC_APB2ENR |= 1u << 9; /* ADC1 */
    gpio_mode(GPIOC, 0, 0x0);
    gpio_mode(GPIOC, 2, 0x0);
    gpio_mode(GPIOC, 3, 0x0);
    ADC1_SMPR1 = 0x00FFFFFFu; /* slowest sampling on channels 10..17 */
    ADC1_SQR1 = 0;
    ADC1_CR2 = 1u;            /* power on */
    for (volatile int i = 0; i < 2000; i++) {}
    ADC1_CR2 |= 1u << 3;      /* reset calibration */
    for (uint32_t g = 0; (ADC1_CR2 & (1u << 3)) && g < 100000; g++) {}
    ADC1_CR2 |= 1u << 2;      /* calibrate */
    for (uint32_t g = 0; (ADC1_CR2 & (1u << 2)) && g < 100000; g++) {}
    ADC1_CR2 = (7u << 17) | (1u << 20) | 1u; /* software trigger */
}

static uint32_t adc_read(uint32_t channel)
{
    ADC1_SQR3 = channel;
    ADC1_SR = 0;
    adc_start();
    for (uint32_t g = 0; !(ADC1_SR & 2u) && g < 100000; g++) {}
    return ADC1_DR & 0xFFFu;
}

static uint32_t h_battery_mv(void)
{
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++)
        sum += adc_read(10);
    /* divided by two, 3.3 V reference assumed */
    return sum / 16u * 3300u * 2u / 4095u;
}

static uint32_t isqrt64(uint64_t v)
{
    uint64_t r = 0, bit = 1ull << 62;
    while (bit > v)
        bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)r;
}

/* AC amplitude (RMS, in ADC counts) of the sensor on a channel. */
static uint32_t emf_rms(uint32_t channel)
{
    ADC1_SQR3 = channel;
    ADC1_SR = 0;
    emf_sum = 0;
    emf_sumsq = 0;
    emf_n = 0;
    emf_state = 1;
    adc_start();
    uint32_t t0 = g_ms;
    while (emf_state != 2 && (uint32_t)(g_ms - t0) < 200)
        __asm__ volatile("wfi");
    if (emf_state != 2) {
        emf_state = 0;
        return 0;
    }
    emf_state = 0;
    uint64_t n = EMF_N;
    uint64_t mean_sq = (uint64_t)emf_sum * emf_sum / n;
    uint64_t var = emf_sumsq > mean_sq ? (emf_sumsq - mean_sq) / n : 0;
    return isqrt64(var);
}

/* UNCALIBRATED: scale factors guessed from the sensor hardware. They must be compared with the stock firmware. */
static uint32_t h_efield_vm_x10(void) { return emf_rms(12) * 18u; }
static uint32_t h_hfield_ut_x100(void)
{
    uint32_t v = emf_rms(13);
    return v * 4u + v * v / 98u;
}

/* The charger chip has two open-drain status pins and no separate cable-sense pin (read from how the stock firmware
 * uses them): PC7 low = charging, PC6 low = charge finished, both high = no cable. Both low is treated like no cable.
 * The state must stay the same for 100 ms before it counts. */
static volatile uint8_t chg_state; /* 0 no cable, 1 charging, 2 full */
static uint8_t chg_raw, chg_count;

static void scan_charger(void)
{
    uint32_t idr = GPIO_IDR(GPIOC);
    bool full = !(idr & (1u << 6)), charging = !(idr & (1u << 7));
    uint8_t raw = (charging && !full) ? 1 : (full && !charging) ? 2 : 0;
    if (raw != chg_raw) {
        chg_raw = raw;
        chg_count = 0;
    } else if (chg_count < 100 && ++chg_count == 100) {
        chg_state = raw;
    }
}

static bool h_usb_present(void) { return chg_state != 0; }
static bool h_charging(void) { return chg_state == 1; }
void usb_gc03_init(void);
bool usb_serial_open(void);
static bool h_usb_connected(void) { return usb_serial_open(); }

/* ---- tube and high voltage ---- */
static bool factory_hv(uint32_t *freq_hz)
{
    if (*(volatile uint32_t *)FACTORY_SIGNATURE != FACTORY_SIGNATURE_VALUE)
        return false;
    uint32_t v = REG16(FACTORY_HV_FREQ);
    if (v < 1 || v > 999)
        return false;
    *freq_hz = v * 100u;
    return true;
}

static bool h_hv_factory_valid(void)
{
    uint32_t f;
    return factory_hv(&f);
}

static uint32_t tube_clock_hz(void) { return 2u * (board_cpu_hz() / 2u); } /* APB1 = HCLK/2, timers run at twice that */

static bool h_hv_start(void)
{
    uint32_t freq;
    if (!factory_hv(&freq))
        return false;

    uint32_t clk = tube_clock_hz();
    uint32_t counts = clk / freq;
    uint32_t psc = 0;
    while (counts / (psc + 1) > 65535u)
        psc++;
    uint32_t arr = counts / (psc + 1) - 1;
    uint32_t on = (uint32_t)((uint64_t)clk * 125u / 10000000u) / (psc + 1); /* 12.5 us pulse */
    if (on == 0 || on * 2 > arr)
        return false; /* refuse anything above 50 percent duty */

    RCC_APB1ENR |= 1u << 3; /* TIM5 */
    gpio_mode(GPIOA, 2, 0xB);
    TIM5_PSC = psc;
    TIM5_ARR = arr;
    TIM5_CCR3 = 0;
    TIM5_CCMR2 = (6u << 4) | (1u << 3); /* PWM mode 1, preload */
    TIM5_CCER = 1u << 8;
    TIM5_CR1 = (1u << 7) | 1u;
    TIM5_EGR = 1;

    for (uint32_t step = 1; step <= 5; step++) { /* soft start */
        TIM5_CCR3 = on * step / 5u;
        delay_ms(80);
        watchdog_kick();
    }

    /* tube pulses: falling edge on PA3 */
    gpio_mode(GPIOA, 3, 0x8);
    gpio_write(GPIOA, 3, true);
    AFIO_EXTICR1 &= ~(0xFu << 12);
    EXTI_RTSR &= ~(1u << 3);
    EXTI_FTSR |= 1u << 3;
    EXTI_PR = 1u << 3;
    EXTI_IMR |= 1u << 3;
    NVIC_IPR(9) = 0x00;
    NVIC_ISER0 = 1u << 9;
    return true;
}

static void h_hv_stop(void)
{
    EXTI_IMR &= ~(1u << 3);
    TIM5_CCR3 = 0;
    TIM5_CR1 = 0;
    gpio_write(GPIOA, 2, false);
    gpio_mode(GPIOA, 2, 0x3);
}

static uint32_t h_tube_pulses(void) { return pulses; }

/* ---- flash windows (never anything outside KV_BASE .. LOG_END) ---- */

static void fl_unlock(void)
{
    if (FLASH_CR & (1u << 7)) {
        FLASH_KEYR = 0x45670123u;
        FLASH_KEYR = 0xCDEF89ABu;
    }
}

static void fl_lock(void) { FLASH_CR |= 1u << 7; }

static bool fl_wait(void)
{
    uint32_t g = 0;
    while ((FLASH_SR & 1u) && g++ < 4000000u) {}
    return !(FLASH_SR & 1u);
}

static bool fl_allowed(uint32_t addr, uint32_t n) { return addr >= KV_BASE && addr + n <= LOG_END; }

static int fl_read_raw(uint32_t addr, void *buf, uint32_t n)
{
    uint8_t *out = buf;
    for (uint32_t i = 0; i < n; i += 2) {
        uint16_t v = REG16(addr + i) ^ MASK_ERASED;
        out[i] = (uint8_t)v;
        out[i + 1] = (uint8_t)(v >> 8);
    }
    return 0;
}

static int fl_prog_raw(uint32_t addr, const void *buf, uint32_t n)
{
    if ((addr & 1u) || (n & 1u) || !fl_allowed(addr, n))
        return -1;
    const uint8_t *in = buf;
    fl_unlock();
    int rc = 0;
    for (uint32_t i = 0; i < n && rc == 0; i += 2) {
        uint16_t v = (uint16_t)((in[i] | in[i + 1] << 8) ^ MASK_ERASED);
        FLASH_SR = (1u << 5) | (1u << 2) | (1u << 4); /* clear EOP and error flags */
        FLASH_CR |= 1u;                               /* PG */
        REG16(addr + i) = v;
        bool done = fl_wait();
        FLASH_CR &= ~1u;
        if (!done || (FLASH_SR & ((1u << 2) | (1u << 4))))
            rc = -1;
    }
    fl_lock();
    return rc;
}

static int fl_erase_sector(uint32_t addr)
{
    if ((addr & (FLASH_SECTOR - 1u)) || !fl_allowed(addr, FLASH_SECTOR))
        return -1;
    fl_unlock();
    FLASH_SR = (1u << 5) | (1u << 2) | (1u << 4);
    FLASH_CR |= 1u << 1; /* PER */
    FLASH_AR = addr;
    FLASH_CR |= 1u << 6; /* STRT */
    bool done = fl_wait();
    FLASH_CR &= ~(1u << 1);
    fl_lock();
    if (!done)
        return -1;
    /* the sector must now read as erased (0xE339 raw); anything else means the flash does not behave as expected */
    for (uint32_t i = 0; i < 64; i += 2)
        if (REG16(addr + i) != (uint16_t)(0xFFFFu ^ MASK_ERASED))
            return -1;
    return 0;
}

/* log window: one logical page per 4 KB sector */
static int log_read(void *c, uint32_t a, void *b, uint32_t n) { (void)c; return fl_read_raw(LOG_BASE + a, b, n); }
static int log_prog(void *c, uint32_t a, const void *b, uint32_t n) { (void)c; return fl_prog_raw(LOG_BASE + a, b, n); }
static int log_erase(void *c, uint32_t p) { (void)c; return fl_erase_sector(LOG_BASE + p * FLASH_SECTOR); }

/* settings window: two logical 1 KB pages, each at the start of its own 4 KB sector */
static uint32_t kv_addr(uint32_t a) { return KV_BASE + (a / KV_PAGE) * FLASH_SECTOR + a % KV_PAGE; }
static int kv_read(void *c, uint32_t a, void *b, uint32_t n)
{
    (void)c;
    uint8_t *o = b;
    while (n) { /* never cross a logical page in one go */
        uint32_t chunk = KV_PAGE - a % KV_PAGE;
        if (chunk > n)
            chunk = n;
        fl_read_raw(kv_addr(a), o, chunk);
        a += chunk; o += chunk; n -= chunk;
    }
    return 0;
}
static int kv_prog(void *c, uint32_t a, const void *b, uint32_t n)
{
    (void)c;
    const uint8_t *i = b;
    while (n) {
        uint32_t chunk = KV_PAGE - a % KV_PAGE;
        if (chunk > n)
            chunk = n;
        if (fl_prog_raw(kv_addr(a), i, chunk) != 0)
            return -1;
        a += chunk; i += chunk; n -= chunk;
    }
    return 0;
}
static int kv_erase(void *c, uint32_t p) { (void)c; return fl_erase_sector(KV_BASE + p * FLASH_SECTOR); }

static const ylog_flash_t log_window = {FLASH_SECTOR, (LOG_END - LOG_BASE) / FLASH_SECTOR, log_read, log_prog, log_erase, 0};
static const ylog_flash_t kv_window = {KV_PAGE, KV_SECTORS, kv_read, kv_prog, kv_erase, 0};

/* ---- bring-up ---- */
/* ---- serial port: USB CDC (usb_gc03.c) ---- */
int usb_serial_read(uint8_t *buf, uint32_t max);
int usb_serial_write(const uint8_t *buf, uint32_t n);
bool usb_serial_open(void);

static int h_serial_read(uint8_t *buf, uint32_t max) { return usb_serial_read(buf, max); }
static int h_serial_write(const uint8_t *buf, uint32_t n) { return usb_serial_write(buf, n); }

static const hal_t gc03 = {
    h_rtc_now, h_rtc_set, h_poll_event, h_tube_pulses, h_battery_mv, h_usb_present, h_charging, h_efield_vm_x10,
    h_hfield_ut_x100, h_hv_factory_valid, h_hv_start, h_hv_stop, h_beep, h_vibrate, h_alert_led, h_backlight,
    h_display_enable, h_power_off, h_watchdog_kick, &log_window, &kv_window, h_usb_connected, h_serial_read, h_serial_write, usb_gc03_init,
    h_status_led, h_pulse_led_enable, h_count_click_enable, h_count_vibe_enable,
};

/* bring-up marker: empty in the firmware, a test image can override it to show how far start-up got */
__attribute__((weak)) void dbg_mark(int step) { (void)step; }

const hal_t *hal_gc03_init(void)
{
    /* outputs idle: buzzer low, vibration off, LEDs off (active low) */
    gpio_write(GPIOC, 11, false); gpio_mode(GPIOC, 11, 0x3);
    gpio_write(GPIOB, 1, false);  gpio_mode(GPIOB, 1, 0x3);
    gpio_write(GPIOB, 14, true);  gpio_mode(GPIOB, 14, 0x3);
    gpio_write(GPIOB, 15, true);  gpio_mode(GPIOB, 15, 0x3);
    gpio_write(GPIOC, 8, true);   gpio_mode(GPIOC, 8, 0x3);

    /* inputs with pull-ups */
    static const struct { uint32_t port; uint8_t pin; } ins[] = {
        {GPIOB, 5}, {GPIOB, 6}, {GPIOB, 7}, {GPIOB, 8}, {GPIOB, 9}, {GPIOC, 6}, {GPIOC, 7},
    };
    for (unsigned i = 0; i < sizeof ins / sizeof *ins; i++) {
        gpio_mode(ins[i].port, ins[i].pin, 0x8);
        gpio_write(ins[i].port, ins[i].pin, true);
    }
    knob_prev = (int8_t)((((GPIO_IDR(GPIOB) >> 5) & 1u) << 1) | ((GPIO_IDR(GPIOB) >> 7) & 1u));

    dbg_mark(1);
    rtc_init();
    dbg_mark(2);
    adc_init();
    dbg_mark(3);
    return &gc03;
}

void hal_gc03_start_tick(void)
{
    SYST_RVR = board_cpu_hz() / TICK_HZ - 1u;
    SYST_CVR = 0;
    SCB_SHPR3_SYSTICK = 0x80; /* below the tube interrupt */
    g_tick_irq = true;
    SYST_CSR = 7u;            /* core clock, interrupt on, enabled */
}
