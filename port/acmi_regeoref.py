#!/usr/bin/env python3
"""port/acmi_regeoref.py -- rewrite Lon/Lat in an existing .acmi using the S305 calibration.

Recordings published before S305 carry the guessed theatre origin (37.5N 127.0E, with the world
origin never subtracted), which lands Korea ~900 km NE in the Sea of Japan. The fix does NOT
require re-flying: every transform already carries the sim's own U|V in fields 7 and 8, and those
were always correct -- only the Lon/Lat derived from them were wrong. This recomputes columns 1
and 2 from columns 7 and 8 and leaves every other byte alone.

  usage: acmi_regeoref.py IN.acmi [OUT.acmi]        (default OUT = IN with .regeo.acmi)
"""
import re, sys, os

LAT0, LON0 = 31.986085, 119.500224      # lat/lon at world (0,0) -- MAPS.H KOREAMAPORIGINX/Y
MLAT, MLON = 110063.9, 84512.2          # metres per degree, solved from Kimpo + Sinuiju

T = re.compile(r'^(-?[0-9a-fA-F]+,T=)([^,\r\n]*)(.*)$')

def main(a):
    if not a: print(__doc__); return 2
    src = a[0]
    dst = a[1] if len(a) > 1 else os.path.splitext(src)[0] + '.regeo.acmi'
    n = 0
    with open(src, 'r', errors='ignore', newline='') as f, open(dst, 'w', newline='') as o:
        for line in f:
            m = T.match(line)
            if not m:
                # the header's own reference must move too, or Tacview centres the old spot
                if line.startswith('0,ReferenceLongitude='): line = '0,ReferenceLongitude=%.6f\r\n' % LON0
                elif line.startswith('0,ReferenceLatitude='): line = '0,ReferenceLatitude=%.6f\r\n' % LAT0
                o.write(line); continue
            fields = m.group(2).split('|')
            # syntax #4 is Lon|Lat|Alt|Roll|Pitch|Yaw|U|V|Heading -- without U/V there is nothing
            # to recompute from, so pass the record through untouched rather than invent one.
            if len(fields) < 8 or not fields[6] or not fields[7]:
                o.write(line); continue
            u, v = float(fields[6]), float(fields[7])
            fields[0] = '%.7f' % (LON0 + u / MLON)
            fields[1] = '%.7f' % (LAT0 + v / MLAT)
            o.write(m.group(1) + '|'.join(fields) + m.group(3) + '\n')
            n += 1
    print('%s -> %s  (%d transforms re-georeferenced)' % (src, dst, n))
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
