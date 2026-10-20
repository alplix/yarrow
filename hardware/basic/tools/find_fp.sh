#!/bin/bash
# footprint names matching a pattern:  find_fp.sh Library pattern
ls /usr/share/kicad/footprints/$1.pretty | grep -E "$2" | head -12
