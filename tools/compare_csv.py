#!/usr/bin/env python3
"""Compares two sample CSV files line by line."""
import sys

a = open(sys.argv[1]).read().split()
b = open(sys.argv[2]).read().split()
if a == b:
    print(f"host decoder matches the device decoder: {len(a) - 1} samples")
    sys.exit(0)
n = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
print(f"MISMATCH at line {n}: expected {a[n] if n < len(a) else None}, got {b[n] if n < len(b) else None} ({len(a)} vs {len(b)} lines)")
sys.exit(1)
