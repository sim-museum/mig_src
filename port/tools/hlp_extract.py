#!/usr/bin/env python3
"""port/tools/hlp_extract.py — extract topic TEXT from a WinHelp .hlp file.

Completes the reconnaissance in hlp_probe.py: that tool showed MIG.HLP holds 44 topics documenting
exactly the screens the "?" button is pressed on. This one gets the words out, so the port can
show them (PO-4).

Pipeline, in the order the format demands:
  |TOPIC  -> fixed-size blocks, each a 12-byte header + LZ77-compressed payload
          -> TOPICLINK records inside the decompressed stream
          -> record type 0x02 = topic header (title), 0x20/0x23 = displayable text
  text    -> phrase-compressed; the phrase table comes from |PhrImage + |PhrIndex (Hall, WinHelp 4)
             or |Phrases (WinHelp 3.1). Both are handled.

STATUS — read this before using the output:

  SOLVED and independently verified
    * container / internal-file B+ tree        (11 files, names as the format specifies)
    * LZ77                                     (|PhrImage decompresses to a clean alphabetical
                                                word list: about, against, aircraft, airfield...)
    * |PhrIndex bit reader                     (732 phrases with exact boundaries; the reader is
                                                LSB-first over 32-bit DWORDs, not MSB-first bytes)
    * |TOPIC link chain via TopicPos           (43 topic headers found; |TTLBTREE lists 44)

  NOT SOLVED
    * the Hall text opcode table. The current guess emits REAL WORDS IN THE WRONG ORDER. It is
      left in place, behind --hall-guess, purely so the next attempt starts from something to
      disprove -- it must not be shipped into the game.

⚠ ON ORACLES. The first oracle here scored extracted text against a list of common English words
and reported 0.484 "PLAUSIBLE" for output that is gibberish -- because a wrong phrase decoder emits
real dictionary words, just in the wrong order, and that is exactly what the metric rewards. AN
ORACLE THAT THE FAILURE MODE CAN SATISFY IS NOT AN ORACLE. --verify now uses an independent
reference instead: |TTLBTREE holds each topic's real title, and correct topic text contains its own
title. That check fails today, correctly.

  python3 port/tools/hlp_extract.py <file.hlp> [--list] [--topic N] [--json OUT] [--verify]
                                               [--hall-guess]
"""
import struct, sys, json, re

# ---------------------------------------------------------------- container

def internal_files(d):
    magic, dirstart, _free, _entire = struct.unpack_from('<iiii', d, 0)
    if magic != 0x00035F3F:
        raise SystemExit(f"not a WinHelp .hlp file (magic {magic:#x})")
    o = dirstart + 9
    pagesize, = struct.unpack_from('<H', d, o + 4)
    rootpage, = struct.unpack_from('<h', d, o + 26)
    p = o + 38 + rootpage * pagesize
    _unk, n, _prev, _next = struct.unpack_from('<hhhh', d, p)
    q = p + 8
    out = {}
    for _ in range(n):
        e = d.index(b'\0', q); out[d[q:e].decode('latin-1')] = struct.unpack_from('<i', d, e + 1)[0]
        q = e + 5
    return out

def filedata(d, off):
    """payload of an internal file (9-byte FILEHEADER: reserved, used, flags)"""
    _reserved, used, _flags = struct.unpack_from('<iiB', d, off)
    return d[off + 9: off + 9 + used]

# ---------------------------------------------------------------- LZ77

def lz77(src):
    """WinHelp's LZ77: a bit-mask byte, then 8 items — literal, or a 2-byte (len,dist) pair."""
    out = bytearray()
    i, n = 0, len(src)
    while i < n:
        mask = src[i]; i += 1
        for b in range(8):
            if i >= n: break
            if mask & (1 << b):
                if i + 1 >= n: break
                code = src[i] | (src[i + 1] << 8); i += 2
                length = ((code >> 12) & 0xF) + 3
                dist = (code & 0xFFF) + 1
                if dist > len(out):          # corrupt / not actually compressed
                    return bytes(out)
                for _ in range(length):
                    out.append(out[-dist])
            else:
                out.append(src[i]); i += 1
    return bytes(out)

# ---------------------------------------------------------------- phrases

class Bits:
    """WinHelp's bit reader for |PhrIndex.

    NOT MSB-first-per-byte, which is the natural guess and produces a phrase table that is
    *almost* right — the phrase IMAGE decodes into correct alphabetical word fragments, and only
    the boundaries between them land in the wrong places ("aboutagainstaircraf" / "tair"). It
    reads 32-bit little-endian DWORDs and takes bits from bit 0 upward, and multi-bit values
    accumulate LSB-first as well."""
    def __init__(self, data, start=0):
        self.d, self.p = data, start
        self.value, self.mask = 0, 0
    def bit(self):
        self.mask <<= 1
        if self.mask == 0 or self.mask > 0x80000000:
            if self.p + 4 <= len(self.d):
                self.value = struct.unpack_from('<I', self.d, self.p)[0]
            else:
                chunk = self.d[self.p:self.p + 4] + b'\0' * 4
                self.value = struct.unpack_from('<I', chunk, 0)[0]
            self.p += 4
            self.mask = 1
        return 1 if (self.value & self.mask) else 0
    def bits(self, n):
        v = 0
        for i in range(n):
            if self.bit(): v |= (1 << i)
        return v

def phrases_hall(d, files):
    """WinHelp 4: |PhrIndex holds bit-packed phrase lengths, |PhrImage the concatenated text."""
    if '|PhrIndex' not in files or '|PhrImage' not in files: return None
    idx = filedata(d, files['|PhrIndex'])
    img = filedata(d, files['|PhrImage'])
    magic, entries, _csize, phrimagesize, phrimagecompressedsize, _z, packed, tail = \
        struct.unpack_from('<iiiiiiHH', idx, 0)
    # Documented as "always 0x4A01"; MIG.HLP has 1. The reliable tell is the trailing 0x4A00,
    # so accept either rather than rejecting a file the format actually permits.
    if magic not in (1, 0x4A01) and tail != 0x4A00: return None
    bits_ = packed & 0xF
    if phrimagesize != phrimagecompressedsize:
        img = lz77(img)
    bs = Bits(idx, 28)
    offsets, cur = [0], 0
    for _ in range(entries):
        n = 1
        while bs.bit(): n += (1 << bits_)
        if bits_ > 0: n += bs.bits(bits_)
        cur += n
        offsets.append(cur)
    return [img[offsets[i]:offsets[i + 1]] for i in range(entries)]

def phrases_31(d, files):
    """WinHelp 3.1: |Phrases = count, then offsets, then (optionally LZ77) concatenated text."""
    if '|Phrases' not in files: return None
    p = filedata(d, files['|Phrases'])
    n, magic = struct.unpack_from('<hh', p, 0)
    body = p[4:] if magic == 0x0100 else p[2:]
    off = 2 if magic == 0x0100 else 0
    offs = [struct.unpack_from('<H', p, off + 2 + 2 * i)[0] for i in range(n + 1)]
    base = off + 2 + 2 * (n + 1)
    text = p[base:]
    if magic == 0x0100:
        text = lz77(text)
    return [text[offs[i] - offs[0]:offs[i + 1] - offs[0]] for i in range(n)]

def phrase_expand(raw, table):
    """Bytes 1..15 introduce a phrase reference; everything else is a literal."""
    if not table: return raw
    out = bytearray()
    i, n = 0, len(raw)
    while i < n:
        c = raw[i]
        if 1 <= c <= 15 and i + 1 < n:
            j = c * 256 + raw[i + 1] - 256; i += 2
            k = j >> 1
            if k < len(table): out += table[k]
            if j & 1: out += b' '
        else:
            out.append(c); i += 1
    return bytes(out)

# ---------------------------------------------------------------- topics

def topic_blocks(d, files, blocksize=4096):
    """Decompress every |TOPIC block, keeping them SEPARATE (see topic_read)."""
    raw = filedata(d, files['|TOPIC'])
    blocks = []
    for off in range(0, len(raw), blocksize):
        blk = raw[off:off + blocksize]
        if len(blk) <= 12: break
        blocks.append(lz77(blk[12:]))
    return blocks

def topic_read(blocks, pos, count, decompsize=0x4000):
    """Read `count` bytes starting at TopicPos `pos`, spanning blocks if necessary.

    TopicPos is an address in a LOGICAL space of fixed `decompsize` blocks, even though each block
    usually decompresses to far less. Concatenating the decompressed blocks and walking linearly
    therefore desynchronises the moment a block boundary is crossed -- which looked like "only 6 of
    44 topics exist" rather than like an addressing bug."""
    out = bytearray()
    p = pos - 12
    while count > 0:
        b, o = divmod(p, decompsize)
        if b >= len(blocks): break
        blk = blocks[b]
        if o >= len(blk):            # past this block's real data: jump to the next block
            p = (b + 1) * decompsize
            continue
        take = min(count, len(blk) - o)
        out += blk[o:o + take]
        p += take; count -= take
    return bytes(out)

def topic_records(d, files):
    """Yield (recordtype, data1, data2) by FOLLOWING the link chain from TopicPos 12."""
    blocks = topic_blocks(d, files)
    if not blocks: return
    pos, seen = 12, set()
    while True:
        if pos in seen: break            # defensive: a malformed chain must not spin
        seen.add(pos)
        hdr = topic_read(blocks, pos, 21)
        if len(hdr) < 21: break
        blocksz, datalen2, _prev, nxt, datalen1, rectype = struct.unpack_from('<iiiiiB', hdr, 0)
        if datalen1 < 21 or blocksz <= 0: break
        body = topic_read(blocks, pos, max(blocksz, datalen1 + max(datalen2, 0)))
        d1 = body[21:datalen1]
        d2 = body[datalen1:datalen1 + max(datalen2, 0)]
        yield rectype, d1, d2
        if nxt <= 0: break
        pos = nxt

def btree_titles(d, files):
    """|TTLBTREE leaf entries: (title, topic offset). Used as the decode oracle."""
    off = files['|TTLBTREE']
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

def hall_guess_decode(raw, table):
    """UNSOLVED. Best current guess at the Hall opcode table; emits real words in the wrong order.
    Kept only as a starting point to disprove -- never call this for anything user-visible."""
    out = bytearray(); i = 0; n = len(raw)
    while i < n:
        c = raw[i]
        if c & 1:
            k = c >> 1; i += 1
            if k < len(table): out += table[k]
            out += b' '
        elif c & 2:
            if i + 1 >= n: break
            k = (c >> 2) + (raw[i + 1] << 6); i += 2
            if k < len(table): out += table[k]
        elif c & 4:
            out += b' ' * ((c >> 3) + 1); i += 1
        elif c & 8:
            m = (c >> 4) + 1; i += 1; out += raw[i:i + m]; i += m
        else:
            out.append(c); i += 1
    return bytes(out)

def extract(path, hall_guess=False):
    d = open(path, 'rb').read()
    files = internal_files(d)
    table = phrases_hall(d, files) or phrases_31(d, files)
    topics, cur = [], None
    for rectype, d1, d2 in topic_records(d, files):
        if rectype == 0x02:                       # TOPICHEADER: starts a new topic
            if cur and cur['text'].strip(): topics.append(cur)
            cur = {'text': ''}
        elif rectype in (0x20, 0x23):             # displayable text
            txt = hall_guess_decode(d2, table) if hall_guess else phrase_expand(d2, table)
            txt = txt.replace(b'\0', b'\n').decode('latin-1')
            if cur is None: cur = {'text': ''}
            cur['text'] += txt
    if cur and cur['text'].strip(): topics.append(cur)
    for t in topics:
        lines = [l.strip() for l in t['text'].split('\n')]
        lines = [l for l in lines if l]
        t['title'] = lines[0] if lines else ''
        t['text'] = '\n'.join(lines)
    return topics, table

def title_oracle(topics, titles):
    """Independent check: |TTLBTREE knows each topic's real title, and a correctly decoded topic
    contains its own title. Unlike a word-frequency score, a wrong decoder cannot accidentally
    satisfy this -- it has to produce the right words in the right place."""
    hits = 0
    checked = 0
    for t, want in zip(topics, titles):
        want = want.strip()
        if not want or len(want) < 4: continue
        checked += 1
        if want.lower() in t['text'][:400].lower(): hits += 1
    return hits, checked

def main():
    if len(sys.argv) < 2: raise SystemExit(__doc__)
    path = sys.argv[1]
    d = open(path, 'rb').read()
    files = internal_files(d)
    topics, table = extract(path, hall_guess='--hall-guess' in sys.argv)
    titles = [n for n, _ in btree_titles(d, files)] if '|TTLBTREE' in files else []
    print(f"{len(topics)} topics decoded, phrase table {len(table) if table else 0} entries, "
          f"{len(titles)} titles in |TTLBTREE")
    if '--list' in sys.argv:
        for i, t in enumerate(topics):
            print(f"  [{i:>3}] {t['title'][:70]}")
    if '--topic' in sys.argv:
        n = int(sys.argv[sys.argv.index('--topic') + 1])
        print('-' * 60); print(topics[n]['text']); print('-' * 60)
    if '--verify' in sys.argv:
        hits, checked = title_oracle(topics, titles)
        print(f"title oracle: {hits}/{checked} topics contain their own |TTLBTREE title")
        if checked and hits >= checked * 0.6:
            print("  -> topic text decode looks CORRECT")
        else:
            print("  -> topic text decode is WRONG (the Hall opcode table is unsolved).")
            print("     Everything upstream of it -- container, LZ77, phrase table, link chain --")
            print("     is verified; do not read a high word-frequency score as success.")
    if '--json' in sys.argv:
        out = sys.argv[sys.argv.index('--json') + 1]
        json.dump(topics, open(out, 'w'), indent=1)
        print(f"wrote {out}")

if __name__ == '__main__':
    main()
