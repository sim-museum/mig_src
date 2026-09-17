#!/usr/bin/env bash
# port/campaign_text.sh — gate for N4/PO-72: the campaign BACKGROUND and OBJECTIVES screens
# actually put their instruction text on the glass.
#
# WHY THIS EXISTS. N4 reads "campaign instruction text and next-mission instructions after 3D
# exit are missing". Five sprints guessed at which screen it meant without naming one. Sprint 2
# (2026-09-17) found the screens the item is most plausibly about: the campaign phase screen's
# BACKGROUND and OBJECTIVES buttons, whose whole content is one line in CAMPBACK.CPP --
#     CRStatic* text = GETDLGITEM(IDC_SDETAIL1); text->SetString(LoadResString(strnum));
# Sprint 3 ran them and both render in full. This gate keeps them rendering.
#
# WHY IT ASSERTS PIXELS, NOT THE LOG. "[LoadString] id=835 n=779" proves the string was read out
# of miglang.dll. It proves NOTHING about the panel: the same line appears whether the text is
# drawn, drawn off-panel, clipped to one line, or painted in the background colour. The log line
# is exactly the half-truth this port keeps getting caught by, so the gate scores INK in the
# panel rect and uses the bare phase screen -- same resolution, same background art, no CCampBack
# open -- as its CONTROL. Control must be ~0; both text screens must be far above it.
#
# WHY THE TWO SCREENS CROSS-CHECK EACH OTHER. IDS_CAMPDESC0 is 779 chars and IDS_CAMPOBJ0 is 145,
# a 5.4x ratio. Measured ink was 25664 vs 4956 = 5.2x. So the gate also asserts the ink RATIO
# tracks the character ratio: a failure that paints a fixed blob (a placeholder, a stuck string,
# a solid fill) passes a threshold test and fails this one.
#
# NB Mig.exe carries its own copy of ids 835-844 as untranslated placeholders ("!!CAMPDESC0!!")
# and bob_load_string does NOT fall back to it. So an empty panel means the lookup failed, while
# "!!CAMPDESC0!!" on screen would mean the resource handle points at the wrong module.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
RES="${RES:-1920x1080}"
OUT="${OUT:-$HOME/ma-camptext}"     # NEVER /tmp: captures are game data (7.6 GB tmpfs)
mkdir -p "$OUT"

# Single Player (row 1) -> Campaign (row 2); the button row is one horizontal menu,
# Back|Film|Background|Objectives|Begin = columns 0..4.
NAV="30,r1;80,r2"

run() { # $1=label $2=extra-click-or-empty
  local ppm="$OUT/$1.ppm" log="$OUT/$1.log"
  # PO-90 S2: clear the previous capture. Written in the same session as the BoB rotation that
  # spent four sprints on exactly this -- bob_parity.sh printed "PASS: 14 screen(s)
  # byte-identical" with /bin/true as the game because $OUT persisted -- and this gate shipped
  # with the same hole. A run that captures nothing would score the LAST run's pixels and pass.
  rm -f "$ppm"
  ( cd "$RUNDIR" && timeout -k 5 -s KILL 150 env SDL_VIDEODRIVER=dummy \
      BOB_RUN_INIT=1 MA_DISABLE_3D=1 MA_IGNORE_SAVE_DATE=1 MA_FORCE_RES="$RES" \
      MA_TRACE_STR=400 BOB_DRIVE_C="$BOB_DRIVE_C" \
      BOB_CLICKSEQ="$NAV${2:+;$2}" MA_SHOT=260 MA_SHOT_PATH="$ppm" "$WMIG" ) >"$log" 2>&1
  [ -s "$ppm" ] || { echo "CANNOT MEASURE: $1 produced no capture (see $log)"; return 2; }
  return 0
}

ink() { python3 - "$1" <<'PY'
import sys
p=sys.argv[1]; f=open(p,'rb')
assert f.readline().strip()==b'P6'
l=f.readline()
while l.startswith(b'#'): l=f.readline()
w,h=map(int,l.split()); f.readline(); d=f.read(w*h*3)
n=0
for y in range(170,min(460,h)):
    b=y*w*3
    for x in range(370,min(890,w)):
        o=b+x*3
        if (d[o]*30+d[o+1]*59+d[o+2]*11)//100 > 90: n+=1
print(n)
PY
}

echo "campaign instruction text @ $RES  -> $OUT"
run control    ""            || exit 2
run background "140,#2063:2" || exit 2
run objectives "140,#2063:3" || exit 2

C=$(ink "$OUT/control.ppm"); B=$(ink "$OUT/background.ppm"); O=$(ink "$OUT/objectives.ppm")
echo "  ink: control=$C  background=$B  objectives=$O"

# the strings must come from miglang.dll, not Mig.exe's placeholders
for s in background:835:779 objectives:840:145; do
  n="${s%%:*}"; rest="${s#*:}"; id="${rest%%:*}"; len="${rest##*:}"
  grep -aq "\[LoadString\] id=$id n=$len " "$OUT/$n.log" \
    || { echo "FAIL: $n did not load id=$id at n=$len (wrong module, or the string changed)"; exit 1; }
done
grep -aq '!!CAMP' "$OUT/background.log" "$OUT/objectives.log" \
  && { echo "FAIL: a !!CAMPxxx!! placeholder reached the panel -- resource handle is on Mig.exe"; exit 1; }

rc=0
[ "$C" -lt 200 ] || { echo "FAIL: control panel is not blank ($C) -- the region or res moved, gate is not measuring text"; rc=1; }
[ "$B" -gt 8000 ] || { echo "FAIL: BACKGROUND text not on the glass (ink=$B)"; rc=1; }
[ "$O" -gt 1500 ] || { echo "FAIL: OBJECTIVES text not on the glass (ink=$O)"; rc=1; }
# 779/145 = 5.4; allow 3.5..8.0 for wrap and font differences
if [ "$O" -gt 0 ]; then
  r=$(( B * 10 / O ))
  [ "$r" -ge 35 ] && [ "$r" -le 80 ] \
    || { echo "FAIL: ink ratio ${r}/10 outside 3.5..8.0 -- ink does not track the 779/145 char ratio"; rc=1; }
  echo "  ratio: ${r}/10 (expect ~54/10 from 779/145 chars)"
fi
[ $rc -eq 0 ] && echo "PASS: both campaign instruction screens render their text"
exit $rc
