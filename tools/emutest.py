#!/usr/bin/env python3
"""Runs a built Yarrow image in an ARM Cortex-M emulator (pip install unicorn).

The peripherals are stubs, so this checks control flow, not hardware timing:
  - the intact image draws, turns the backlight on and eventually switches the device off,
  - every damaged or incomplete image switches the device off at once instead of hanging.
"""
import struct
import sys

from unicorn import UC_ARCH_ARM, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_MCLASS, UC_MODE_THUMB, Uc, UcError
from unicorn.arm_const import UC_ARM_REG_PC, UC_ARM_REG_SP

BASE = 0x08006000
RCC_CR, RCC_CFGR, RCC_CSR = 0x40021000, 0x40021004, 0x40021024
SPI1_SR, SPI1_DR = 0x40013008, 0x4001300C
SYST_CSR = 0xE000E010
GPIOA_IDR, GPIOA_BSRR, GPIOB_BSRR = 0x40010808, 0x40010810, 0x40010C10


def run(image, budget, csr=0):
    mu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    mu.mem_map(0x08000000, 0x40000)
    mu.mem_write(BASE, bytes(image))
    mu.mem_map(0x20000000, 0x10000)
    mu.mem_map(0x40000000, 0x30000)
    mu.mem_map(0xE0000000, 0x100000)
    state = {"backlight": False, "power_off": False, "regs": {}}

    def read(uc, access, addr, size, value, data):
        regs = state["regs"]
        if addr == RCC_CR:
            uc.mem_write(addr, struct.pack("<I", 0x03030000 | (regs.get(addr, 0) & 0xFFFF)))
        elif addr == RCC_CFGR:
            v = regs.get(addr, 0)
            uc.mem_write(addr, struct.pack("<I", (v & ~0xC) | ((v & 3) << 2)))
        elif addr == RCC_CSR:
            uc.mem_write(addr, struct.pack("<I", csr))
        elif addr == SPI1_SR:
            uc.mem_write(addr, struct.pack("<I", 2))
        elif addr == SYST_CSR:
            uc.mem_write(addr, struct.pack("<I", 0x10005))
        elif addr == GPIOA_IDR:
            uc.mem_write(addr, struct.pack("<I", 0x200))

    def write(uc, access, addr, size, value, data):
        state["regs"][addr] = value
        if addr == GPIOB_BSRR and value & (1 << 13):
            state["backlight"] = True
        if addr == GPIOA_BSRR and value & (1 << 26):
            state["power_off"] = True
            uc.emu_stop()

    for lo, hi in ((0x40000000, 0x40030000), (0xE000E000, 0xE000F000)):
        mu.hook_add(UC_HOOK_MEM_READ, read, begin=lo, end=hi)
        mu.hook_add(UC_HOOK_MEM_WRITE, write, begin=lo, end=hi)

    sp, reset = struct.unpack("<2L", bytes(image[:8]))
    mu.reg_write(UC_ARM_REG_SP, sp)
    try:
        mu.emu_start(reset, 0, count=budget)
    except UcError as e:
        state["error"] = str(e)
    state["pc"] = mu.reg_read(UC_ARM_REG_PC)
    return state


def main():
    good = bytearray(open(sys.argv[1], "rb").read())
    erased = lambda n: good[:n] + b"\xff" * (len(good) - n)
    flipped = bytearray(good)
    flipped[len(good) // 2] ^= 0x10
    bad_magic = bytearray(good)
    bad_magic[0x200] ^= 0xFF

    # name, image, budget, csr, expect_backlight, expect_power_off
    cases = [
        ("intact image starts and lights the backlight" + (" then switches off" if len(sys.argv) > 2 and sys.argv[2] == "off" else ""), good, 80_000_000, 0, True, len(sys.argv) > 2 and sys.argv[2] == "off"),
        ("tail erased after 2048 bytes", erased(2048), 5_000_000, 0, False, True),
        ("tail erased after 4096 bytes", erased(4096), 5_000_000, 0, False, True),
        ("only first 0x300 bytes written", erased(0x300), 5_000_000, 0, False, True),
        ("one flipped bit in the middle", flipped, 5_000_000, 0, False, True),
        ("header magic damaged", bad_magic, 5_000_000, 0, False, True),
        ("previous run ended in a watchdog reset", good, 5_000_000, 1 << 29, False, True),
    ]

    failed = 0
    for name, image, budget, csr, want_backlight, want_off in cases:
        r = run(image, budget, csr)
        ok = r["backlight"] == want_backlight and r["power_off"] == want_off and "error" not in r
        print(("PASS" if ok else "FAIL"), name, f"(backlight={r['backlight']}, power_off={r['power_off']})")
        failed += not ok
    sys.exit(1 if failed else 0)


main()
