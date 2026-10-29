#!/usr/bin/env python3
"""MiG Alley .X8 image maps (IFF FORM/PBM: 8-bit chunky, ByteRun1) <-> PNG.

  ma_x8.py topng  IN.X8 OUT.png [PALETTE]   decode, colouring with the game palette (gpal.pal: one "r g b" line each)
  ma_x8.py fromx8 IN.png TEMPLATE.X8 OUT.X8 re-encode an edited PNG with TEMPLATE's header (size, transparency);
                                            pixels are matched to the nearest palette colour, index 254 = transparent
  ma_x8.py check  IN.X8                     decode + re-encode, report whether the bytes round-trip exactly

MA-RED-ALL-1 (e): the Yak-9 cockpit is the F-51 cockpit (CPTF51) re-textured, so its .X8 maps need editing.
"""
import struct, sys

GPAL = '/home/admin/ma-gates/scratch/rowan/mig/gpal.pal'


def chunks(b, off, end):
    while off + 8 <= end:
        tag, n = b[off:off + 4], struct.unpack('>I', b[off + 4:off + 8])[0]
        yield tag, off + 8, n
        off += 8 + n + (n & 1)


def unpackbits(src, size):
    out, i = bytearray(), 0
    while len(out) < size:
        c = src[i]; i += 1
        if c < 128:
            out += src[i:i + c + 1]; i += c + 1
        elif c > 128:
            out += bytes([src[i]]) * (257 - c); i += 1
    return bytes(out[:size])


def packbits_row(row):
    """ByteRun1 for one row: runs of 3+ equal bytes as repeats, the rest as literals (Deluxe Paint's encoding)."""
    out, i, n = bytearray(), 0, len(row)
    while i < n:
        j = i
        while j + 1 < n and row[j + 1] == row[i] and j - i < 127:
            j += 1
        if j - i >= 2 or (j - i == 1 and (i == 0 or True) and j + 1 == n):
            out += bytes([257 - (j - i + 1), row[i]]); i = j + 1
            continue
        k = i
        while k < n and k - i < 128:
            if k + 2 < n and row[k] == row[k + 1] == row[k + 2]:
                break
            k += 1
        out += bytes([k - i - 1]) + bytes(row[i:k]); i = k
    return bytes(out)


def read(path):
    b = open(path, 'rb').read()
    assert b[:4] == b'FORM' and b[8:12] == b'PBM ', path
    hdr = body = None
    for tag, o, n in chunks(b, 12, len(b)):
        if tag == b'BMHD': hdr = b[o:o + n]
        if tag == b'BODY': body = b[o:o + n]
    w, h = struct.unpack('>HH', hdr[:4]); comp = hdr[10]
    rw = w + (w & 1)
    pix = unpackbits(body, rw * h) if comp == 1 else body[:rw * h]
    return b, hdr, w, h, bytes(b''.join(pix[y * rw:y * rw + w] for y in range(h)))


def write(hdr, w, h, pix):
    rw = w + (w & 1)
    body = b''.join(packbits_row(pix[y * w:(y + 1) * w] + (b'\0' if rw != w else b'')) for y in range(h))
    if hdr[10] != 1: body = b''.join(pix[y * w:(y + 1) * w] + (b'\0' if rw != w else b'') for y in range(h))
    def ck(t, d): return t + struct.pack('>I', len(d)) + d + (b'\0' if len(d) & 1 else b'')
    inner = b'PBM ' + ck(b'BMHD', hdr) + ck(b'BODY', body)
    return b'FORM' + struct.pack('>I', len(inner)) + inner


def palette(path=GPAL):
    p = [tuple(int(v) for v in l.split()[:3]) for l in open(path) if len(l.split()) >= 3]
    return (p + [(0, 0, 0)] * 256)[:256]


def main():
    cmd = sys.argv[1]
    if cmd == 'topng':
        from PIL import Image
        _, hdr, w, h, pix = read(sys.argv[2])
        pal = palette(sys.argv[4] if len(sys.argv) > 4 else GPAL)
        im = Image.frombytes('P', (w, h), pix); im.putpalette([c for rgb in pal for c in rgb]); im.save(sys.argv[3])
    elif cmd == 'fromx8':
        from PIL import Image
        _, hdr, w, h, _ = read(sys.argv[3])
        im = Image.open(sys.argv[2])
        if im.mode == 'P':
            pix = im.tobytes()
        else:
            pal = palette(); im = im.convert('RGBA'); cache = {}; out = bytearray()
            for r, g, b_, a in im.getdata():
                if a < 128: out.append(254); continue
                k = (r, g, b_)
                if k not in cache:
                    cache[k] = min((i for i in range(256) if i != 254), key=lambda i: (pal[i][0] - r) ** 2 + (pal[i][1] - g) ** 2 + (pal[i][2] - b_) ** 2)
                out.append(cache[k])
            pix = bytes(out)
        assert im.size == (w, h), (im.size, w, h)
        open(sys.argv[4], 'wb').write(write(hdr, w, h, pix))
    elif cmd == 'check':
        raw, hdr, w, h, pix = read(sys.argv[2])
        again = write(hdr, w, h, pix)
        same_pix = read_bytes(again) == pix
        print('%s %dx%d bytes-identical=%s pixels-identical=%s' % (sys.argv[2], w, h, again == raw, same_pix))


def read_bytes(b):
    import tempfile, os
    fd, p = tempfile.mkstemp(); os.write(fd, b); os.close(fd)
    try: return read(p)[4]
    finally: os.unlink(p)


if __name__ == '__main__':
    main()
