#!/bin/bash
# Regenerates the schematic, runs ERC and exports the net list (run inside WSL with KiCad).
cd "$(dirname "$0")" || exit 1
python3 make_schematic.py ../kicad || exit 1
bash run_erc.sh
kicad-cli sch export netlist --format kicadxml --output ../kicad/yarrow_counter_basic.net.xml ../kicad/yarrow_counter_basic.kicad_sch
