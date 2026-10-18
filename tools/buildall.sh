#!/bin/bash
# Builds, checks and tests the firmware of every language (or of the ones named): tools/buildall.sh [code ...]
# For each language: langcheck, the simulator (layout problems), the firmware, the emulator tests, a release copy.
cd "$(dirname "$0")/.."
MAKE=${MAKE:-mingw32-make}
langs=("$@")
if [ ${#langs[@]} -eq 0 ]; then
  for f in src/lang/*.h; do langs+=("$(basename "$f" .h)"); done
fi
bad=0
for l in "${langs[@]}"; do
  echo "=== $l"
  python tools/langcheck.py "$l" | tail -3 | grep -v "0 problem" && bad=1
  $MAKE sim YLANG=$l 2>&1 | grep -E "LAYOUT|DIFFERS|NOT STILL" | sort -u && bad=1
  $MAKE release YLANG=$l 2>&1 | grep -E "error|crc32"
  $MAKE test YLANG=$l 2>&1 | grep -E "FAIL|stack:" && true
done
echo "finished, problems: $bad"
exit $bad
