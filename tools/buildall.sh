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
  out=$(python tools/langcheck.py "$l" | tail -3 | grep -v "0 problem"); [ -n "$out" ] && { echo "$out"; bad=1; }
  out=$($MAKE sim YLANG=$l 2>&1 | grep -E "LAYOUT|DIFFERS|NOT STILL" | sort -u); [ -n "$out" ] && { echo "$out"; bad=1; }
  $MAKE release YLANG=$l 2>&1 | grep -E "error|crc32"
  out=$($MAKE test YLANG=$l 2>&1 | grep -E "FAIL|stack:"); echo "$out"; echo "$out" | grep -q FAIL && bad=1
done
echo "finished, problems: $bad"
exit $bad
