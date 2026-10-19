/* USB full-speed device on the CH32F203 (STM32F1-compatible USB core): a CDC serial port.
 * Interrupt driven. Nothing here has run on the device yet; it is the last start-up stage so a fault in it
 * leaves the rest of the firmware usable. */

#include "board.h"
#include "regs.h"
#include <stdbool.h>
#include <stdint.h>

#define USB_BASE   0x40005C00u
#define USB_EPR(n) REG(USB_BASE + 4u * (n))
#define USB_CNTR   REG(USB_BASE + 0x40)
#define USB_ISTR   REG(USB_BASE + 0x44)
#define USB_DADDR  REG(USB_BASE + 0x4C)
#define USB_BTABLE REG(USB_BASE + 0x50)
#define PMA_BASE   0x40006000u
#define EXTEN_CTR  REG(0x40023800u)

#define PMA_HW(off) (*(volatile uint16_t *)(PMA_BASE + (uint32_t)(off) * 2u))

/* PMA layout */
#define BT_OFF 0u
#define EP0_TX 64u
#define EP0_RX 128u
#define EP1_RX 192u
#define EP2_TX 256u
#define EP3_TX 320u

/* EPR bits */
#define EP_CTR_RX  0x8000u
#define EP_DTOG_RX 0x4000u
#define EP_STAT_RX 0x3000u
#define EP_SETUP   0x0800u
#define EP_TYPE    0x0600u
#define EP_KIND    0x0100u
#define EP_CTR_TX  0x0080u
#define EP_DTOG_TX 0x0040u
#define EP_STAT_TX 0x0030u
#define EP_TOGGLE  (EP_DTOG_RX | EP_STAT_RX | EP_DTOG_TX | EP_STAT_TX)
#define EP_BULK 0x0000u
#define EP_CONTROL 0x0200u
#define EP_INTERRUPT 0x0600u
#define STAT_DISABLED 0u
#define STAT_STALL 1u
#define STAT_NAK 2u
#define STAT_VALID 3u

static void pma_write(uint32_t off, const uint8_t *buf, uint32_t n)
{
    for (uint32_t i = 0; i < n; i += 2) {
        uint16_t w = buf[i];
        if (i + 1 < n)
            w |= (uint16_t)(buf[i + 1] << 8);
        PMA_HW(off + i) = w;
    }
}

static void pma_read(uint32_t off, uint8_t *buf, uint32_t n)
{
    for (uint32_t i = 0; i < n; i += 2) {
        uint16_t w = PMA_HW(off + i);
        buf[i] = (uint8_t)w;
        if (i + 1 < n)
            buf[i + 1] = (uint8_t)(w >> 8);
    }
}

static void ep_set_stat_rx(uint32_t n, uint32_t stat)
{
    uint32_t epr = USB_EPR(n);
    uint32_t cur = (epr & EP_STAT_RX) >> 12;
    USB_EPR(n) = (epr & ~EP_TOGGLE & ~0u & 0xFFFFu) | EP_CTR_RX | EP_CTR_TX | ((cur ^ stat) << 12);
}

static void ep_set_stat_tx(uint32_t n, uint32_t stat)
{
    uint32_t epr = USB_EPR(n);
    uint32_t cur = (epr & EP_STAT_TX) >> 4;
    USB_EPR(n) = (epr & ~EP_TOGGLE & 0xFFFFu) | EP_CTR_RX | EP_CTR_TX | ((cur ^ stat) << 4);
}

static void ep_clear_ctr_rx(uint32_t n) { USB_EPR(n) = (USB_EPR(n) & ~EP_TOGGLE & ~EP_CTR_RX & 0xFFFFu) | EP_CTR_TX; }
static void ep_clear_ctr_tx(uint32_t n) { USB_EPR(n) = (USB_EPR(n) & ~EP_TOGGLE & ~EP_CTR_TX & 0xFFFFu) | EP_CTR_RX; }

static void ep_config(uint32_t n, uint32_t type, uint32_t addr)
{
    USB_EPR(n) = type | addr | EP_CTR_RX | EP_CTR_TX; /* all toggle bits written as 0: unchanged */
    /* fresh toggle state: force DTOG bits and status to known values */
    uint32_t epr = USB_EPR(n);
    if (epr & EP_DTOG_RX)
        USB_EPR(n) = (epr & ~EP_TOGGLE & 0xFFFFu) | EP_CTR_RX | EP_CTR_TX | EP_DTOG_RX;
    epr = USB_EPR(n);
    if (epr & EP_DTOG_TX)
        USB_EPR(n) = (epr & ~EP_TOGGLE & 0xFFFFu) | EP_CTR_RX | EP_CTR_TX | EP_DTOG_TX;
}

static void btable_set(uint32_t n, uint32_t tx_addr, uint32_t rx_addr, uint32_t rx_count_field)
{
    PMA_HW(BT_OFF + n * 8 + 0) = (uint16_t)tx_addr;
    PMA_HW(BT_OFF + n * 8 + 2) = 0;
    PMA_HW(BT_OFF + n * 8 + 4) = (uint16_t)rx_addr;
    PMA_HW(BT_OFF + n * 8 + 6) = (uint16_t)rx_count_field;
}

/* ---- descriptors ---- */
static const uint8_t dev_desc[18] = {
    18, 1, 0x00, 0x02, 0x02, 0x00, 0x00, 64, 0x09, 0x12, 0x01, 0x00, 0x00, 0x01, 1, 2, 3, 1,
};

static const uint8_t cfg_desc[67] = {
    9, 2, 67, 0, 2, 1, 0, 0xC0, 50,
    9, 4, 0, 0, 1, 2, 2, 1, 0,           /* communication interface, ACM */
    5, 0x24, 0, 0x10, 0x01,              /* header */
    5, 0x24, 1, 0x00, 1,                 /* call management */
    4, 0x24, 2, 0x02,                    /* ACM */
    5, 0x24, 6, 0, 1,                    /* union */
    7, 5, 0x83, 3, 8, 0, 0xFF,           /* interrupt IN */
    9, 4, 1, 0, 2, 0x0A, 0, 0, 0,        /* data interface */
    7, 5, 0x01, 2, 64, 0, 0,             /* bulk OUT */
    7, 5, 0x82, 2, 64, 0, 0,             /* bulk IN */
};

static const uint8_t str0[4] = {4, 3, 0x09, 0x04};
static const uint8_t str1[14] = {14, 3, 'Y', 0, 'a', 0, 'r', 0, 'r', 0, 'o', 0, 'w', 0};
static const uint8_t str2[26] = {26, 3, 'Y', 0, 'a', 0, 'r', 0, 'r', 0, 'o', 0, 'w', 0, ' ', 0, 'G', 0, 'C', 0, '-', 0, '0', 0, '3', 0};
static const uint8_t str3[10] = {10, 3, '0', 0, '0', 0, '0', 0, '1', 0};

/* ---- state ---- */
typedef struct {
    uint8_t bmRequestType, bRequest;
    uint16_t wValue, wIndex, wLength;
} setup_t;

static volatile uint8_t configured;
static volatile uint8_t port_open;
static uint8_t addr_pending;
static const uint8_t *ctl_data;
static uint32_t ctl_len, ctl_sent;
static bool ctl_zlp;
static uint8_t line_coding[7] = {0x00, 0xC2, 0x01, 0x00, 0, 0, 8}; /* 115200 8N1 */
static bool expect_line_coding;

#define RXN 512u
#define TXN 1024u
static uint8_t rx_ring[RXN], tx_ring[TXN];
static volatile uint32_t rx_h, rx_t, tx_h, tx_t;
static volatile bool tx_busy;

static void ep0_arm_rx(void) { ep_set_stat_rx(0, STAT_VALID); }

static void ep0_send_chunk(void)
{
    uint32_t left = ctl_len - ctl_sent;
    uint32_t n = left > 64 ? 64 : left;
    pma_write(EP0_TX, ctl_data + ctl_sent, n);
    PMA_HW(BT_OFF + 0 * 8 + 2) = (uint16_t)n;
    ctl_sent += n;
    ep_set_stat_tx(0, STAT_VALID);
}

static void ep0_send(const uint8_t *data, uint32_t len, uint32_t max)
{
    if (len > max)
        len = max;
    ep0_arm_rx();
    ctl_data = data;
    ctl_len = len;
    ctl_sent = 0;
    ctl_zlp = (len < max) && (len % 64 == 0); /* a short answer that fills whole packets needs a zero length packet */
    if (len == 0) {
        PMA_HW(BT_OFF + 0 * 8 + 2) = 0;
        ep_set_stat_tx(0, STAT_VALID);
        return;
    }
    ep0_send_chunk();
}

static void ep0_status(void)
{
    ep0_arm_rx();
    ctl_len = 0;
    ctl_sent = 0;
    ctl_zlp = false;
    PMA_HW(BT_OFF + 0 * 8 + 2) = 0;
    ep_set_stat_tx(0, STAT_VALID);
}

static void ep0_stall(void)
{
    ep_set_stat_tx(0, STAT_STALL);
    ep_set_stat_rx(0, STAT_STALL);
}

static void handle_setup(const setup_t *s)
{
    uint8_t type = (s->bmRequestType >> 5) & 3;
    expect_line_coding = false;

    if (type == 0) { /* standard */
        switch (s->bRequest) {
        case 6: { /* GET_DESCRIPTOR */
            uint8_t kind = (uint8_t)(s->wValue >> 8), idx = (uint8_t)s->wValue;
            if (kind == 1) { ep0_send(dev_desc, sizeof dev_desc, s->wLength); return; }
            if (kind == 2) { ep0_send(cfg_desc, sizeof cfg_desc, s->wLength); return; }
            if (kind == 3) {
                const uint8_t *d = idx == 0 ? str0 : idx == 1 ? str1 : idx == 2 ? str2 : idx == 3 ? str3 : 0;
                if (d) { ep0_send(d, d[0], s->wLength); return; }
            }
            break;
        }
        case 5: /* SET_ADDRESS */
            addr_pending = (uint8_t)(s->wValue & 0x7F);
            ep0_status();
            return;
        case 9: /* SET_CONFIGURATION */
            configured = (uint8_t)s->wValue;
            ep_set_stat_rx(1, configured ? STAT_VALID : STAT_NAK);
            ep0_status();
            return;
        case 8: { /* GET_CONFIGURATION */
            static uint8_t c;
            c = configured;
            ep0_send(&c, 1, s->wLength);
            return;
        }
        case 0: { /* GET_STATUS */
            static const uint8_t st[2] = {1, 0};
            ep0_send(st, 2, s->wLength);
            return;
        }
        case 11: /* SET_INTERFACE */
        case 1:  /* CLEAR_FEATURE */
        case 3:  /* SET_FEATURE */
            ep0_status();
            return;
        default:
            break;
        }
    } else if (type == 1) { /* class: CDC */
        switch (s->bRequest) {
        case 0x20: /* SET_LINE_CODING: 7 bytes follow */
            expect_line_coding = true;
            ep0_arm_rx();
            return;
        case 0x21: /* GET_LINE_CODING */
            ep0_send(line_coding, 7, s->wLength);
            return;
        case 0x22: /* SET_CONTROL_LINE_STATE: DTR in bit 0 means a program has the port open */
            port_open = (s->wValue & 1u) ? 1 : 0;
            ep0_status();
            return;
        default:
            ep0_status();
            return;
        }
    }
    ep0_stall();
}

static void bulk_in_start(void)
{
    uint32_t n = 0;
    uint8_t chunk[64];
    while (n < 64 && tx_h != tx_t) {
        chunk[n++] = tx_ring[tx_h];
        tx_h = (tx_h + 1) % TXN;
    }
    if (n == 0) {
        tx_busy = false;
        return;
    }
    pma_write(EP2_TX, chunk, n);
    PMA_HW(BT_OFF + 2 * 8 + 2) = (uint16_t)n;
    tx_busy = true;
    ep_set_stat_tx(2, STAT_VALID);
}

static void usb_reset(void)
{
    addr_pending = 0;
    configured = 0;
    port_open = 0;
    tx_busy = false;
    tx_h = tx_t = 0;

    btable_set(0, EP0_TX, EP0_RX, 0x8400);
    btable_set(1, 0, EP1_RX, 0x8400);
    btable_set(2, EP2_TX, 0, 0);
    btable_set(3, EP3_TX, 0, 0);

    ep_config(0, EP_CONTROL, 0);
    ep_set_stat_tx(0, STAT_NAK);
    ep_set_stat_rx(0, STAT_VALID);
    ep_config(1, EP_BULK, 1);
    ep_set_stat_rx(1, STAT_NAK);
    ep_config(2, EP_BULK, 2);
    ep_set_stat_tx(2, STAT_NAK);
    ep_config(3, EP_INTERRUPT, 3);
    ep_set_stat_tx(3, STAT_NAK);
    USB_DADDR = 0x80; /* enable the function, address 0 */
}

void usb_isr(void)
{
    uint32_t istr;
    while ((istr = USB_ISTR) & 0xFC00u) {
        if (istr & (1u << 10)) { /* reset */
            USB_ISTR = ~(1u << 10);
            usb_reset();
            continue;
        }
        if (istr & (1u << 15)) { /* correct transfer */
            uint32_t n = istr & 0xF;
            bool out_dir = (istr & 0x10) != 0;
            uint32_t epr = USB_EPR(n);
            if (n == 0) {
                if (out_dir) {
                    uint32_t count = PMA_HW(BT_OFF + 0 * 8 + 6) & 0x3FF;
                    if (epr & EP_SETUP) {
                        uint8_t raw[8];
                        pma_read(EP0_RX, raw, 8);
                        ep_clear_ctr_rx(0);
                        setup_t s = {raw[0], raw[1], (uint16_t)(raw[2] | raw[3] << 8), (uint16_t)(raw[4] | raw[5] << 8),
                                     (uint16_t)(raw[6] | raw[7] << 8)};
                        handle_setup(&s);
                    } else {
                        if (expect_line_coding && count >= 7) {
                            pma_read(EP0_RX, line_coding, 7);
                            expect_line_coding = false;
                            ep_clear_ctr_rx(0);
                            ep0_status();
                        } else {
                            ep_clear_ctr_rx(0); /* status stage of an IN transfer */
                        }
                        ep0_arm_rx();
                    }
                } else {
                    ep_clear_ctr_tx(0);
                    if (addr_pending) {
                        USB_DADDR = 0x80u | addr_pending;
                        addr_pending = 0;
                    }
                    if (ctl_sent < ctl_len)
                        ep0_send_chunk();
                    else if (ctl_zlp) {
                        ctl_zlp = false;
                        PMA_HW(BT_OFF + 0 * 8 + 2) = 0;
                        ep_set_stat_tx(0, STAT_VALID);
                    }
                }
            } else if (n == 1 && out_dir) { /* bulk OUT: serial data from the host */
                uint32_t count = PMA_HW(BT_OFF + 1 * 8 + 6) & 0x3FF;
                uint8_t chunk[64];
                if (count > 64)
                    count = 64;
                pma_read(EP1_RX, chunk, count);
                ep_clear_ctr_rx(1);
                for (uint32_t i = 0; i < count; i++) {
                    uint32_t nt = (rx_t + 1) % RXN;
                    if (nt != rx_h) {
                        rx_ring[rx_t] = chunk[i];
                        rx_t = nt;
                    }
                }
                ep_set_stat_rx(1, STAT_VALID);
            } else if (n == 2 && !out_dir) { /* bulk IN finished */
                ep_clear_ctr_tx(2);
                bulk_in_start();
            } else {
                if (out_dir)
                    ep_clear_ctr_rx(n);
                else
                    ep_clear_ctr_tx(n);
            }
        }
        USB_ISTR = ~(istr & 0x3F00u & ~(1u << 15)); /* clear the other event flags we do not use */
    }
}

/* ---- the serial port as seen by the application ---- */
bool usb_serial_open(void) { return configured && port_open; }

int usb_serial_read(uint8_t *buf, uint32_t max)
{
    uint32_t n = 0;
    while (n < max && rx_h != rx_t) {
        buf[n++] = rx_ring[rx_h];
        rx_h = (rx_h + 1) % RXN;
    }
    return (int)n;
}

int usb_serial_write(const uint8_t *buf, uint32_t n)
{
    if (!usb_serial_open())
        return 0;
    uint32_t sent = 0;
    uint32_t t0 = g_ms;
    while (sent < n) {
        uint32_t nt = (tx_t + 1) % TXN;
        if (nt == tx_h) { /* ring full: wait for the interrupt to drain it */
            if ((uint32_t)(g_ms - t0) > 200 || !usb_serial_open())
                break; /* the host stopped reading */
            watchdog_kick();
            __asm__ volatile("wfi");
            continue;
        }
        tx_ring[tx_t] = buf[sent++];
        tx_t = nt;
    }
    if (!tx_busy) {
        NVIC_ICER0 = 1u << 20; /* keep the interrupt away while the first packet is queued */
        if (!tx_busy)
            bulk_in_start();
        NVIC_ISER0 = 1u << 20;
    }
    return (int)sent;
}

void usb_gc03_init(void)
{
    /* USB clock = 96 MHz / 2 = 48 MHz. USBPRE[1:0] (CFGR bits 23:22) keeps the old single-bit meaning for 0 and 1
     * (00 = PLL/1.5, 01 = PLL/1) and adds 10 = PLL/2, 11 = PLL/3. */
    RCC_CFGR = (RCC_CFGR & ~(3u << 22)) | (2u << 22);
    RCC_APB1ENR |= 1u << 23;
    EXTEN_CTR &= ~(1u << 1); /* pull-up off: the host sees a disconnect */
    delay_ms(50);

    USB_CNTR = 1u; /* force reset */
    for (volatile int i = 0; i < 1000; i++) {}
    USB_CNTR = 0;
    USB_ISTR = 0;
    USB_BTABLE = BT_OFF;
    USB_CNTR = (1u << 15) | (1u << 10); /* correct transfer and reset interrupts */
    NVIC_IPR(20) = 0x40;
    NVIC_ISER0 = 1u << 20;
    EXTEN_CTR |= 1u << 1; /* pull-up on: connect */
}
