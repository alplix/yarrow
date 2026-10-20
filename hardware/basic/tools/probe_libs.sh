#!/bin/bash
# Searches the KiCad symbol libraries (run inside WSL with KiCad):  probe_libs.sh pattern...
for p in "$@"; do
    echo "--- $p"
    grep -n -o -E "\(symbol \"[^\"]*${p}[^\"]*\"" /usr/share/kicad/symbols/*.kicad_sym | sed 's#/usr/share/kicad/symbols/##' | head -12
done
