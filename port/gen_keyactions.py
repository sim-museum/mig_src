#!/usr/bin/env python3
"""Regenerate SRC/compat/ma_keyactions.inc from SRC/H/KEYMAPS.H (S88, H2 key bindings)."""
import re, sys
src = open('SRC/H/KEYMAPS.H', encoding='latin-1').read()
ents, seen = [], set()
# S379 (PO-78): accept a leading `/**/`. That is an EMPTY comment -- it comments nothing out, so
# `/**/KeyName(112,OUTREVLOCKTOG)` is live code, and the old line-anchored pattern skipped 31 such
# declarations. They were then reported as "actions with no name", which cost a sprint to a wrong
# conclusion ("39 keys silently do nothing"; retracted -- they were bound and dispatched all along,
# just nameless in the dump and the rebinding file). Genuinely dead lines carry `//DeadCode`/`//`
# and must STAY excluded (33 of them), so the prefix admitted here is only the empty comment.
for n, name in re.findall(r'^(?:\s*/\*\*/)?\s*KeyName\((\d+),\s*([A-Za-z_][A-Za-z_0-9]*)\)', src, re.M):
    if name in seen: continue
    seen.add(name); ents.append((int(n), name))
print("/* GENERATED - do not edit by hand. See port/gen_keyactions.py */")
print("static const struct { const char* name; int val; } ma_key_actions[] = {")
for n, name in ents: print('    { "%s", %d },' % (name, n*2))
print("};")
