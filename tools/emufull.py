#!/usr/bin/env python3
"""Runs the whole firmware image in an ARM emulator with models of the flash controller, ADC, RTC and clocks.

Checks that start-up reaches the main loop, the stages advance, the tube high voltage is programmed from the factory
page, and above all that nothing ever writes or erases outside the settings and log windows."""
import re
import struct
import subprocess
import sys

from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_ARCH_ARM, UC_MODE_MCLASS, UC_MODE_THUMB, Uc, UcError
from unicorn.arm_const import UC_ARM_REG_PC, UC_ARM_REG_SP

BASE = 0x08006000
import os

if os.environ.get("YBIG"):  # the Chinese, Japanese and Korean layout
    KV_BASE, LOG_BASE, LOG_END = 0x08026000, 0x08028000, 0x0803F000
else:
    KV_BASE, LOG_BASE, LOG_END = 0x0801E000, 0x08020000, 0x0803F000
FLASH_END = 0x08040000
ERASED = b"\x39\xe3"  # raw erased pattern of the CH32F2 flash

RCC_CR, RCC_CFGR, RCC_BDCR = 0x40021000, 0x40021004, 0x40021020
FLASH_SR, FLASH_CR, FLASH_AR = 0x4002200C, 0x40022010, 0x40022014
ADC1_SR, ADC1_DR, ADC1_SQR3 = 0x40012400, 0x4001244C, 0x40012434
RTC_CRL, RTC_CNTH, RTC_CNTL = 0x40002804, 0x40002818, 0x4000281C
GPIOA_IDR, GPIOB_IDR, GPIOC_IDR = 0x40010808, 0x40010C08, 0x40011008
GPIOA_BSRR, GPIOB_BSRR = 0x40010810, 0x40010C10
TIM5_CR1, TIM5_ARR, TIM5_CCR3 = 0x40000C00, 0x40000C2C, 0x40000C3C
EXTI_IMR = 0x40010400
SYST_CSR = 0xE000E010


def symbols(elf):
    out = subprocess.run(["arm-none-eabi-nm", elf], capture_output=True, text=True, check=True).stdout
    return {m.group(3): int(m.group(1), 16) for m in re.finditer(r"^([0-9a-f]+) (\w) (\S+)$", out, re.M)}


def main():
    binfile = sys.argv[1]
    elf = binfile.replace(".bin", ".elf")
    seconds = int(sys.argv[2]) if len(sys.argv) > 2 else 70
    sym = symbols(elf)
    image = open(binfile, "rb").read()

    mu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    mu.mem_map(0x08000000, 0x40000)
    mu.mem_write(0x08000000, ERASED * (0x40000 // 2))
    mu.mem_write(BASE, image)
    mu.mem_write(0x0803FC00, struct.pack("<I", 0x4B6E4564))   # factory tube calibration
    mu.mem_write(0x0803FC1A, struct.pack("<H", 40))           # 4000 Hz
    mu.mem_map(0x20000000, 0x10000)
    mu.mem_map(0x40000000, 0x30000)
    mu.mem_map(0xE0000000, 0x100000)

    st = {"regs": {}, "violations": [], "writes": 0, "erases": 0, "off": False, "pg": False, "bl_on": 0,
          "spi": 0, "flash_prog_addrs": set(), "adc_ch": 10, "ms": 0}

    def ram32(addr):
        return struct.unpack("<I", mu.mem_read(addr, 4))[0]

    def rd(uc, access, addr, size, value, data):
        r = st["regs"]
        if addr == RCC_CR:
            v = 0x03030000 | (r.get(addr, 0) & 0xFFFF)
        elif addr == RCC_CFGR:
            x = r.get(addr, 0)
            v = (x & ~0xC) | ((x & 3) << 2)
        elif addr == RCC_BDCR:
            v = r.get(addr, 0) | 2
        elif addr == FLASH_SR:
            v = 0
        elif addr == 0x40013008:   # SPI1 status: transmit buffer empty, not busy
            v = 2
        elif addr == ADC1_SR:
            v = 2
        elif addr == ADC1_DR:
            ch = r.get(ADC1_SQR3, 10)
            v = 2482 if ch == 10 else 2048 + ((st["ms"] * 7) % 200)
        elif addr == RTC_CRL:
            v = r.get(addr, 0) | 0x20
        elif addr in (RTC_CNTH, RTC_CNTL):
            t = 1791970860 + st["ms"] // 1000
            v = (t >> 16) if addr == RTC_CNTH else (t & 0xFFFF)
        elif addr in (GPIOA_IDR, GPIOB_IDR, GPIOC_IDR):
            v = 0xFFFF
        elif addr == SYST_CSR:
            v = 0x10005
        else:
            return
        uc.mem_write(addr, struct.pack("<I", v))

    def wr(uc, access, addr, size, value, data):
        r = st["regs"]
        r[addr] = value
        if addr == FLASH_CR:
            st["pg"] = bool(value & 1)
            if value & (1 << 6) and value & (1 << 1):  # erase a sector
                a = r.get(FLASH_AR, 0)
                if a & 0xFFF or not (KV_BASE <= a < LOG_END):
                    st["violations"].append(f"erase at 0x{a:08x}")
                else:
                    uc.mem_write(a, ERASED * 0x800)
                    st["erases"] += 1
        if 0x08000000 <= addr < FLASH_END and not (addr == FLASH_AR):
            if not st["pg"] or not (KV_BASE <= addr < LOG_END):
                st["violations"].append(f"flash write at 0x{addr:08x} (pg={st['pg']})")
            else:
                st["writes"] += 1
        if addr == GPIOB_BSRR and value & (1 << 13):
            st["bl_on"] += 1
        if addr == GPIOA_BSRR and value & (1 << 26):
            st["off"] = True
            from unicorn.arm_const import UC_ARM_REG_LR
            st["off_at"] = (uc.reg_read(UC_ARM_REG_PC), uc.reg_read(UC_ARM_REG_LR))
            uc.emu_stop()
        if addr == 0x40013000 + 0x0C:
            st["spi"] += 1

    mu.hook_add(UC_HOOK_MEM_READ, rd, begin=0x40000000, end=0x40030000)
    mu.hook_add(UC_HOOK_MEM_READ, rd, begin=0xE000E000, end=0xE000F000)
    mu.hook_add(UC_HOOK_MEM_WRITE, wr, begin=0x40000000, end=0x40030000)
    mu.hook_add(UC_HOOK_MEM_WRITE, wr, begin=0x08000000, end=FLASH_END)

    sp_device, reset = struct.unpack("<2L", image[:8])
    # Measure how deep the stack really goes: start it high in a bigger RAM than the device has, fill the free area with
    # a pattern and see afterwards how far down the pattern was destroyed. The device has sp_device - _ebss bytes.
    sp = 0x2000FF00
    ebss = sym["_ebss"]
    PATTERN = bytes([0xA5]) * 4
    mu.mem_write(ebss, PATTERN * ((sp - 64 - ebss) // 4))
    mu.reg_write(UC_ARM_REG_SP, sp)
    pc = reset & ~1
    g_ms = sym.get("g_ms")
    g_tick = sym.get("g_tick_irq")
    chunk = 3000
    total_ms = seconds * 1000
    try:
        # first run until the application has started its tick (g_tick_irq set), then advance time chunk by chunk
        pc_cur = reset
        steps = 0
        while st["ms"] < total_ms and not st["off"]:
            mu.emu_start(pc_cur, 0, count=chunk)
            pc_cur = mu.reg_read(UC_ARM_REG_PC) | 1
            steps += 1
            if g_tick and mu.mem_read(g_tick, 1)[0]:
                st["ms"] += 1
                mu.mem_write(g_ms, struct.pack("<I", ram32(g_ms) + 1))
            elif steps > 60000:
                break
    except UcError as e:
        print("emulation error:", e, "at pc=0x%08x" % mu.reg_read(UC_ARM_REG_PC))
        sys.exit(1)

    r = st["regs"]
    if st.get("off_at"):
        byaddr = sorted((a, n) for n, a in sym.items())
        def near(a):
            best = None
            for ad, n in byaddr:
                if ad <= (a & ~1):
                    best = (n, (a & ~1) - ad)
            return best
        print("powered off at", near(st["off_at"][0]), "called from", near(st["off_at"][1]))
    print(f"simulated {st['ms'] / 1000:.0f} s: display bytes {st['spi']}, backlight writes {st['bl_on']}, "
          f"flash programs {st['writes']}, sector erases {st['erases']}")
    print(f"TIM5 ARR {r.get(TIM5_ARR)}, CCR3 {r.get(TIM5_CCR3)}, CR1 {r.get(TIM5_CR1)}, EXTI IMR {r.get(EXTI_IMR)}")

    # lowest address the stack has reached
    # the 32 retained bytes at the top of the device's RAM are written at start-up and are not stack: count them as untouched
    area = bytearray(mu.mem_read(ebss, sp - 64 - ebss))
    off = sym["_retained"] - ebss
    if 0 <= off <= len(area) - 32:
        area[off:off + 32] = PATTERN * 8
    low = 0
    while low + 4 <= len(area) and area[low:low + 4] == PATTERN:
        low += 4
    depth = (sp - 64) - (ebss + low) + 64
    room = sp_device - ebss
    print(f"stack: deepest use {depth} bytes, room on the device {room} bytes")

    fails = 0

    def check(ok, msg):
        nonlocal fails
        print(("PASS " if ok else "FAIL ") + msg)
        fails += not ok

    check(not st["violations"], "no write or erase outside the settings and log windows" +
          (": " + "; ".join(st["violations"][:4]) if st["violations"] else ""))
    check(not st["off"], "the firmware keeps running (no fault, no power off)")
    check(st["ms"] >= total_ms, "the simulated clock reached the end")
    check(st["spi"] > 100000, "the display was driven")
    check(st["writes"] > 0 and st["erases"] > 0, "settings store was created in flash")
    check(r.get(TIM5_ARR) == 23999, "tube high voltage period from the factory page (4 kHz -> ARR 23999)")
    check(r.get(TIM5_CCR3, 0) > 0 and r.get(TIM5_CCR3, 0) * 2 < 23999, "tube high voltage duty is set and below 50 percent")
    check(bool(r.get(EXTI_IMR, 0) & 8), "tube pulse interrupt is enabled")
    check(depth + 1024 <= room, f"the stack keeps 1 KB of room on the device ({depth} of {room} bytes used)")
    sys.exit(1 if fails else 0)


main()
