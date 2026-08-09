#!/usr/bin/env python3
"""port/tools/hlp_probe.py — read the internal structure of a WinHelp .hlp file.

Reconnaissance for PO-4: the "?" button on a dialog title bar. S98 routed the click through the
engine's own chain to CWinApp::WinHelp(HID_BASE_RESOURCE + IDD_x) -- which is where the port runs
out of road, because there is no WinHelp viewer on Linux.

This tool answers "is it worth building one?" with facts instead of a guess. For the shipped
English/TEXT/MIG.HLP the answer is yes: 44 topics with titles like "Dossier", "Map Screen",
"Main Toolbar", "Weather", "Squadron Information" -- i.e. documentation for exactly the screens
the play-tester was pressing "?" on -- and 35 |CTXOMAP entries mapping the context ids the game
passes straight to the topic offsets.

What is NOT done here: |TOPIC decompression. This file is WinHelp 4 (it has |PhrImage + |PhrIndex,
Hall compression) so topic text needs LZ77 + phrase expansion before it reads as English.

  python3 port/tools/hlp_probe.py <file.hlp> [--titles] [--ctxmap]
"""
import struct, sys

def read(path):
    return open(path, 'rb').read()

def internal_files(d):
    magic, dirstart, freechain, entire = struct.unpack_from('<iiii', d, 0)
    if magic != 0x00035F3F:
        raise SystemExit(f"not a WinHelp .hlp file (magic {magic:#x})")
    o = dirstart + 9
    pagesize, = struct.unpack_from('<H', d, o + 4)
    rootpage, = struct.unpack_from('<h', d, o + 26)
    pages = o + 38
    p = pages + rootpage * pagesize
    _unk, n, _prev, _next = struct.unpack_from('<hhhh', d, p)
    q = p + 8
    out = {}
    for _ in range(n):
        e = d.index(b'\0', q); name = d[q:e].decode('latin-1'); q = e + 1
        off, = struct.unpack_from('<i', d, q); q += 4
        out[name] = off
    return out

def btree_strings(d, off):
    """leaf entries of a string-keyed B+ tree (|TTLBTREE): [(title, topic_offset)]"""
    o = off + 9
    pagesize, = struct.unpack_from('<H', d, o + 4)
    rootpage, = struct.unpack_from('<h', d, o + 26)
    p = o + 38 + rootpage * pagesize
    _unk, n, _prev, _next = struct.unpack_from('<hhhh', d, p)
    q = p + 8
    out = []
    for _ in range(n):
        e = d.index(b'\0', q); name = d[q:e].decode('latin-1'); q = e + 1
        v, = struct.unpack_from('<i', d, q); q += 4
        out.append((name, v))
    return out

def ctxomap(d, off):
    o = off + 9
    n, = struct.unpack_from('<h', d, o)
    return [struct.unpack_from('<ii', d, o + 2 + i * 8) for i in range(n)]

def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    d = read(sys.argv[1])
    files = internal_files(d)
    print(f"{len(d)} bytes, {len(files)} internal files")
    for k, v in sorted(files.items()):
        print(f"  {k:<14} @ {v}")
    if '--titles' in sys.argv and '|TTLBTREE' in files:
        t = btree_strings(d, files['|TTLBTREE'])
        print(f"\n{len(t)} topic titles:")
        for name, o in t:
            print(f"  {o:>8}  {name}")
    if '--ctxmap' in sys.argv and '|CTXOMAP' in files:
        c = ctxomap(d, files['|CTXOMAP'])
        print(f"\n{len(c)} context-id -> topic mappings:")
        for cid, o in c:
            print(f"  ctx {cid:>7} -> topic @ {o}")

if __name__ == '__main__':
    main()
