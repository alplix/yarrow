#!/bin/bash
# Electrical rules check of the Basic board's schematic (run inside WSL with KiCad):  run_erc.sh
cd "$(dirname "$0")/../kicad" || exit 1
kicad-cli sch erc --severity-all --format report --output erc_report.txt yarrow_counter_basic.kicad_sch
echo "exit $?"
grep -c "^\[" erc_report.txt
grep "^\[" erc_report.txt | sed 's/\[\(.*\)\]: .*/\1/' | sort | uniq -c | sort -rn
