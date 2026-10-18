#!/usr/bin/env python3
"""Small hand corrections to a language file: python tools/langfix.py de S_RECORDED="Tage gespeichert" S_T_X="..."
Each argument is MACRO="new text" for a macro that holds a single string."""
import os
import re
import sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
code = sys.argv[1]
path = os.path.join(root, "src/lang/%s.h" % code)
text = open(path, encoding="utf-8").read()
for arg in sys.argv[2:]:
    name, _, value = arg.partition("=")
    value = value.strip('"')
    pat = re.compile(r'^(#define\s+%s\s+)"(?:[^"\\]|\\.)*"' % re.escape(name), re.M)
    if not pat.search(text):
        sys.exit("%s: no single-string macro %s" % (code, name))
    text = pat.sub(lambda m: m.group(1) + '"' + value.replace('"', '\\"') + '"', text, count=1)
open(path, "w", encoding="utf-8", newline="\n").write(text)
print("%s: %d correction(s)" % (code, len(sys.argv) - 2))
