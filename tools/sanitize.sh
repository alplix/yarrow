#!/bin/bash
# Runs every host unit test of `make check` again, built with AddressSanitizer and UndefinedBehaviorSanitizer (any out-of-range
# access, use after free, signed overflow, bad shift, misaligned access stops the test with a report). Needs gcc on Linux:
#   bash tools/sanitize.sh
# The compile lines are taken from the Makefile, so a new test is picked up by itself.
cd "$(dirname "$0")/.." || exit 1
mkdir -p build/san
fail=0
while IFS= read -r line; do
    cmd=${line#$'\t'}
    cmd=${cmd//\$(BUILD)/build}
    cmd=${cmd//\$(MON_SRCS)/monitor/yarrow_lock.c monitor/yarrow_src.c monitor/yarrow_src_yarrow.c monitor/yarrow_src_rah.c monitor/yarrow_src_gmc.c monitor/yarrow_hid.c}
    cmd=${cmd//\$(MON)/monitor/yarrow_bin.c monitor/yarrow_trickle.c}
    cmd=${cmd//\$(HIDLIBS)/}
    out=$(echo "$cmd" | sed -n 's/.* -o \(build\/tests\/[a-z_0-9]*\)$/\1/p')
    [ -n "$out" ] || continue
    san=${out/tests/san}
    cmd=$(echo "$cmd" | sed "s#-O2#-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer#; s#-o $out#-o $san#")
    if ! eval "$cmd" 2> build/san/compile.log; then
        echo "BUILD FAILED: $san"; head -5 build/san/compile.log; fail=1; continue
    fi
    if ASAN_OPTIONS=detect_leaks=0 "$san" > build/san/run.log 2>&1; then
        echo "ok    $(basename $san): $(tail -1 build/san/run.log | cut -c1-80)"
    else
        echo "FAIL  $(basename $san)"; grep -E "ERROR|runtime error|FAIL" build/san/run.log | head -5; fail=1
    fi
done < <(grep -P '^\tgcc .*tests/test_' Makefile)
exit $fail
