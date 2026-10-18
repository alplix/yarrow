#!/usr/bin/env python3
"""Compares a translation with the Turkish source: python tools/langcheck.py en [de fr ...]
Every macro must exist in both, lists and the guide must have the same shape, and nothing may be empty."""
import os
import re
import sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')


def load(code):
    text = open(os.path.join(root, "src/lang/%s.h" % code), encoding="utf-8").read()
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    macros = {}
    for m in re.finditer(r"^#define\s+(\w+)\s+((?:.*\\\n)*.*)$", text, flags=re.M):
        macros[m.group(1)] = m.group(2)
    return macros


def shape(value):
    """number of string literals and of zero fillers, so that lists and the guide keep their layout"""
    rest = STRING.sub('""', value)
    return (len(STRING.findall(value)), len(re.findall(r"(?<![\w\"])0(?![\w\"])", rest)))


def main():
    codes = sys.argv[1:] or ["en"]
    tr = load("tr")
    bad = 0
    for code in codes:
        other = load(code)
        for name in sorted(tr):
            if name not in other:
                print("%s: missing %s" % (code, name))
                bad += 1
                continue
            if STRING.search(tr[name]) and shape(tr[name]) != shape(other[name]):
                print("%s: %s has a different shape (%s against %s)" % (code, name, shape(other[name]), shape(tr[name])))
                bad += 1
            if name != "L_DAY_SUFFIX" and any(s.strip() == "" for s in STRING.findall(other[name])) and not any(
                    s.strip() == "" for s in STRING.findall(tr[name])):
                print("%s: %s contains an empty string" % (code, name))
                bad += 1
        # numbers are safety information (the dose table, the thresholds): they must survive a translation unchanged
        en = load("en")
        for name in sorted(tr):
            if name in other and name in en and name != "GUIDE_DATA_EMERGENCY":
                a, b = STRING.findall(en[name]), STRING.findall(other[name])
                if len(a) != len(b):
                    continue
                for x, y in zip(a, b):
                    if "112" in x and "911" in x:
                        continue  # the emergency number is local
                    nums = re.findall(r"\d+(?:[.,]\d+)?", x)
                    if any(n.replace(",", ".") not in y.replace(",", ".") for n in nums):
                        print("%s: %s: numbers of '%s' are not all in '%s'" % (code, name, x[:40], y[:50]))
                        bad += 1
        for name in sorted(other):
            if name not in tr:
                print("%s: %s does not exist in the source" % (code, name))
                bad += 1
    print("langcheck: %d problem(s)" % bad)
    return 1 if bad else 0


sys.exit(main())
