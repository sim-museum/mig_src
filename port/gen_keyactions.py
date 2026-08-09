#!/usr/bin/env python3
"""Regenerate SRC/compat/ma_keyactions.inc from SRC/H/KEYMAPS.H (S88, H2 key bindings)."""
import re, sys
src = open('SRC/H/KEYMAPS.H', encoding='latin-1').read()
ents, seen = [], set()
for n, name in re.findall(r'^KeyName\((\d+),\s*([A-Za-z_][A-Za-z_0-9]*)\)', src, re.M):
    if name in seen: continue
    seen.add(name); ents.append((int(n), name))
print("/* GENERATED - do not edit by hand. See port/gen_keyactions.py */")
print("static const struct { const char* name; int val; } ma_key_actions[] = {")
for n, name in ents: print('    { "%s", %d },' % (name, n*2))
print("};")
