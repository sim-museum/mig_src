#!/usr/bin/env python3
"""FUNC-SWEEP-MA: a BOB_KEYSEQ list (with BOB_KEYSEQ_FRAMES=1: "frame,dik[,moddik]") that taps every keyboard
binding in controls.cfg once. Held back (end/pause/leave the flight; test separately): EXITKEY, EJECTPILOT,
PAUSEKEY, KEY_CONFIGMENU, GOTOMAPKEY, ACCELKEY, ACCELKEY2, SUICIDE. CapsLock-state and controller rows skipped.
Usage: keysweep_gen.py controls.cfg [start_frame] [step_frames] [mapfile] > keyseq.txt"""
import re, sys
HOLD = {'EXITKEY','EJECTPILOT','PAUSEKEY','KEY_CONFIGMENU','GOTOMAPKEY','ACCELKEY','ACCELKEY2','SUICIDE'}
MOD = {0: 0, 2: 0x38, 3: 0x38, 4: 0x1D, 5: 0x1D, 6: 0x2A, 7: 0x2A}
t = int(sys.argv[2]) if len(sys.argv) > 2 else 600
step = int(sys.argv[3]) if len(sys.argv) > 3 else 30
out, mp, seen = [], [], set()
for ln in open(sys.argv[1], encoding='utf-8', errors='replace'):
    m = re.match(r'^(\w+)\s*=\s*(0x[0-9A-Fa-f]+)(?:\s*,\s*(\d+))?', ln)
    if not m: continue
    act, sc, st = m.group(1), int(m.group(2), 16), int(m.group(3) or 0)
    if act in HOLD or act.startswith('KeySrc_') or sc >= 0x100 or st not in MOD: continue
    key = (sc, MOD[st])
    if key in seen: continue                 # one tap per physical chord
    seen.add(key)
    out.append('%d,%d,%d' % ((t, sc, MOD[st]) if MOD[st] else (t, sc, 0)) if MOD[st] else '%d,%d' % (t, sc))
    mp.append('%d\t0x%02x\tstate%d\t%s' % (t, sc, st, act)); t += step
print(';'.join(out))
open(sys.argv[4] if len(sys.argv) > 4 else '/dev/stderr', 'w').write('\n'.join(mp) + '\n')
