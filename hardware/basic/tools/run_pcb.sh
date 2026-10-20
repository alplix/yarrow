#!/bin/bash
# Places the board, runs the design rule check and renders a picture (run inside WSL with KiCad 9, after run_all.sh).
cd "$(dirname "$0")" || exit 1
python3 make_pcb.py ../kicad || exit 1
cd ../kicad || exit 1
kicad-cli pcb drc --severity-all --format report --output drc_report.txt yarrow_counter_basic.kicad_pcb
echo "violations by type:"
grep "^\[" drc_report.txt | sed 's/\[\(.*\)\]: .*/\1/' | sort | uniq -c | sort -rn
kicad-cli pcb export svg --layers "F.Cu,B.Cu,F.Silkscreen,Edge.Cuts,F.Courtyard" --mode-single --page-size-mode 2 --output pcb.svg yarrow_counter_basic.kicad_pcb > /dev/null
convert -density 150 -background white -flatten pcb.svg "${1:-/tmp/pcb.png}"
ls -la "${1:-/tmp/pcb.png}"
