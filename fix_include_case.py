#!/usr/bin/env python3
"""Create symlinks for case-mismatched #include directives (Linux port).

The original VC6 tree is case-insensitive; Linux is not. This scans every
#include in SRC/ and, when the target exists only case-insensitively, creates
a symlink with the requested spelling pointing at the real file. Symlinks are
gitignored (see .gitignore). Re-run after adding files with new-cased includes.
Adapted from the FreeFalcon port's fix_include_case.py.
"""
import os, re

ROOT = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(ROOT, 'SRC')

INCLUDE_DIRS = [SRC, os.path.join(SRC, 'H'), os.path.join(SRC, 'compat')]

SCAN_EXT = ('.cpp', '.h', '.c', '.hpp', '.inl', '.m', '.cc')
inc_re = re.compile(r'^\s*#\s*include\s+["<]([^">]+)[">]', re.MULTILINE)

def case_resolve(base, rel):
    parts = rel.replace('\\', '/').split('/')
    cur, out, exact = base, [], True
    for p in parts:
        if p == '..':
            cur = os.path.dirname(cur); out.append('..'); continue
        if p == '.':
            continue
        if not os.path.isdir(cur):
            return None
        try:
            entries = os.listdir(cur)
        except OSError:
            return None
        if p in entries:
            out.append(p); cur = os.path.join(cur, p); continue
        match = next((e for e in entries if e.lower() == p.lower()), None)
        if match is None:
            return None
        exact = False; out.append(match); cur = os.path.join(cur, match)
    return ('/'.join(out), exact)

created = scanned = 0
for root, dirs, files in os.walk(SRC):
    dirs[:] = [d for d in dirs if d != '.git']
    for f in files:
        if os.path.splitext(f)[1].lower() not in SCAN_EXT:
            continue
        scanned += 1
        try:
            text = open(os.path.join(root, f), encoding='utf-8', errors='replace').read()
        except OSError:
            continue
        for m in inc_re.finditer(text):
            target = m.group(1).replace('\\', '/')
            if target.startswith(('sys/', 'GL/', 'SDL2/', 'AL/', 'bits/')):
                continue
            bases = [root] + INCLUDE_DIRS
            found_exact = False; found_ci = None
            for b in bases:
                if os.path.exists(os.path.join(b, target)):
                    found_exact = True; break
                r = case_resolve(b, target)
                if r and not r[1] and found_ci is None:
                    found_ci = (b, r[0])
            if found_exact or found_ci is None:
                continue
            b, resolved = found_ci
            cur = b
            for tp, rp in zip(target.split('/'), resolved.split('/')):
                if tp != rp:
                    linkpath = os.path.join(cur, tp)
                    try:
                        os.symlink(rp, linkpath); created += 1
                    except FileExistsError:
                        pass
                    except OSError as e:
                        print(f"FAIL {linkpath}: {e}")
                cur = os.path.join(cur, rp)

print(f"scanned {scanned} files, created {created} symlinks")
