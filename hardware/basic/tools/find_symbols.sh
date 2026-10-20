#!/bin/bash
# Which KiCad library holds which symbol. Run inside the WSL distro that has KiCad.
cd /usr/share/kicad/symbols
for s in "$@"; do
    echo "$s: $(grep -l "^\s*(symbol \"$s\"" *.kicad_sym | tr '\n' ' ')"
done
