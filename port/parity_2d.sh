#!/usr/bin/env bash
# port/parity_2d.sh — the standing 2D screen-parity regression gate (S80).
#
# Re-captures the standing front-end screens GL-free (MA_SHOT under SDL_VIDEODRIVER=dummy)
# and compares each against its committed reference in port/ref/native/, reporting the
# number of differing PIXELS (0 = byte-identical render).
#
# WHY THIS EXISTS: the recipes (click sequence + shot idle) had been re-derived by hand from
# prose in port/scrum/screen-parity.md every sprint. That is the same trap S62/S63 hit with
# hardcoded pixel coordinates — a gate you must reconstruct from memory is a gate that
# silently stops being run the same way twice. Recipes live here now, in one place.
#
#   port/parity_2d.sh [screen ...]     (default: all)
#
# NOTE #7 prefs_controls is deliberately NOT in the set: that capture embeds live joystick
# state, so it is environment-dependent and not a valid regression oracle (S62/S64).
set -u
# S118 (PO-12 phase 4): hardware is now a PLAYER SETTING, so an unpinned gate would test
# whichever renderer settings.mig happens to hold. Pin the software path here -- that is
# what these references were captured from. MA_NO_HARDWARE=0 runs the same gate on the
# hardware path.
export MA_NO_HARDWARE="${MA_NO_HARDWARE:-1}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WMIG="${WMIG:-$ROOT/build/wmig}"
BOB_DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/MigAlley/WP/drive_c}"
RUNDIR="$BOB_DRIVE_C/rowan/mig"
# S302: SECOND RESOLUTION. Every reference in ref/native was captured at 800x600, and the gate pins
# settings.mig to that size -- so it is byte-identical whether or not a display-resolution layout
# change is correct (S300 measured exactly this: MA_MAXIMIZE=1 passed 5/5 while being a no-op).
# PARITY_RES=1080 runs the same recipes at 1920x1080 with the maximise fix on, against their own
# reference set.
#
# ⚠️ THESE ARE NOT GOLD-PARITY REFERENCES AND MUST NOT BE DESCRIBED AS SUCH. ref/native came from
# the real game; nobody has a 1920x1080 gold capture of these screens. ref/native1080 is captured
# from THIS PORT, so it is a REGRESSION oracle -- it answers "did this change?" and cannot answer
# "is this right?". Anything it blesses is blessed only against the port's own past behaviour.
# Recorded here because a reference set whose provenance is forgotten becomes a false authority,
# and this repo already has a case (S290) of a stale reference silently outranking a real fix.
PARITY_RES="${PARITY_RES:-800}"
if [ "$PARITY_RES" = "1080" ]; then
  REF="$ROOT/port/ref/native1080"
  RESENV="MA_FORCE_RES=1920x1080 MA_MAXIMIZE=1"
else
  REF="$ROOT/port/ref/native"
  # S312: PIN MA_MAXIMIZE=0. The default is now ON, and ma_gdi_canvas() reports the DISPLAY size,
  # so without this pin the "800x600" arm would maximise to 1920x1080 and every capture here would
  # stop matching ref/native -- which came from the real game at 800x600. The pin keeps this arm
  # measuring the layout its references were taken at. The shipping configuration is covered by
  # PARITY_RES=1080 against ref/native1080.
  RESENV="MA_MAXIMIZE=0"
fi
OUT="${OUT:-/tmp/parity2d}"
TMO="${TMO:-60}"

# screen | click sequence (BOB_CLICKSEQ, font-independent forms) | shot idle | extra env
# Recipes: screen-parity.md "S63 — recipes are now FONT-INDEPENDENT". 2063 == IDC_RLISTBOX,
# the panel's own menu listbox; a horizontal tab bar is its COLUMNS (:N), a vertical menu its
# rows (rN). Never write a tab/menu coordinate as pixels here — a font change moves them and
# breaks every recipe at once (S62/S63).
RECIPES="
title|:|30|
prefs_3d|40,r0|70|
prefs_others|40,r0;70,#2063:6|110|
quickmission|40,r1;60,r1|90|
campaign_map|30,r3;65,#1055;100,#2063:1|150|MA_IGNORE_SAVE_DATE=1
"
# campaign_map renders live campaign SAVE state (date readout, frontline, unit icons), so it is
# only an oracle if the save is pinned. S80 measured an 8095px drift caused by this repo's own
# MA_CAMP_FLY/MA_CAMP_LOOP runs advancing the campaign, and excluded the screen. S81 restored it:
# PIN below installs the committed pristine save before the capture, so the screen is
# reproducible again. (Root cause of the drift going unnoticed for so long: the port was
# writing/reading the autosave under the TRUNCATED name "Auto Save.sa" — fixed in S81.)
# The player's own save is stashed and restored around the capture; the gate must not eat it.
PIN="$ROOT/port/ref/save/campaign_pristine.sav"
SAVEDIR="$RUNDIR/SaveGame"
# S378: this pair DESTROYED the player's campaign save on 2026-08-30. /tmp hit its quota, the
# backup copy came out empty, and unpin wrote that 0-byte file back over the real save. Both
# halves now go through ma_safe_backup/ma_safe_restore (port/gate_lib.sh), which refuse a short
# or empty backup rather than passing it on. If the backup cannot be trusted, the pin does NOT
# happen -- the capture is worth less than the save.
pin_save() {
  [ -f "$PIN" ] || return 0
  if ! ma_safe_backup "$SAVEDIR/Auto Save.sav" "$OUT/player_autosave.bak"; then
      echo "  SKIPPING the campaign_map pin: the player's save could not be backed up safely." >&2
      PIN_SKIPPED=1; return 1
  fi
  PIN_SKIPPED=0
  cp -f "$PIN" "$SAVEDIR/Auto Save.sav"
}
unpin_save() {
  [ "${PIN_SKIPPED:-0}" = "1" ] && return 0        # never pinned, so nothing to put back
  ma_safe_restore "$OUT/player_autosave.bak" "$SAVEDIR/Auto Save.sav"
}

# S103: PIN settings.mig around EVERY capture, for the same reason campaign_map's save is pinned.
# Until S103 the port never loaded settings.mig (the only reader is SaveData::InitPreferences,
# which nothing called), so the Preferences screens rendered a fixed never-initialised Save_Data
# and were accidentally state-independent. Now that preferences really load, "Gamma Correction"
# and every other combo shows whatever the player last chose -- and an oracle that renders mutable
# player state is not an oracle (S80). Every screen is pinned, not just the two prefs ones: any
# panel may read a setting, and a gate that pins only where it currently matters silently stops
# being valid the day that changes. The player's own file is restored afterwards -- a gate must
# never eat the player's settings (S81 learned that with the campaign save).
PIN_SET="$ROOT/port/ref/save/settings_pristine.mig"
pin_settings() {
  [ -f "$PIN_SET" ] || return 0
  [ -f "$SAVEDIR/settings.mig" ] && cp -a "$SAVEDIR/settings.mig" "$OUT/player_settings.bak"
  cp -f "$PIN_SET" "$SAVEDIR/settings.mig"
}
unpin_settings() {
  if [ -f "$OUT/player_settings.bak" ]; then cp -a "$OUT/player_settings.bak" "$SAVEDIR/settings.mig"
  else rm -f "$SAVEDIR/settings.mig"; fi
}

mkdir -p "$OUT"
[ -x "$WMIG" ] || { echo "no binary at $WMIG" >&2; exit 2; }

# S210: REFUSE to run while a wmig this gate did not start is up -- and never kill it.
# This gate does `pkill -x wmig` after each capture, and on 2026-08-25 that killed the PO's live
# session in the middle of a play-test. gate_lib.sh has had assert_clean_start() and the right
# policy written down since it was created ("a stray wmig may be the user's own game on the
# display, and a gate is never entitled to close it") -- 14 gates source it and call it, and this
# one, the most frequently run of the lot, did neither. Exit 2 = "could not run", not "failed".
. "$ROOT/port/gate_lib.sh" 2>/dev/null || true
if command -v assert_clean_start >/dev/null 2>&1; then
  assert_clean_start || exit 2
elif pgrep -x wmig >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: wmig already running (pid $(pgrep -x wmig | tr '\n' ' '))."
  echo "  It may be the player's own session; this gate would kill it. Stop it first."
  exit 2
fi

want() { [ "$#" -eq 0 ] && return 0; for w in "$@"; do [ "$w" = "$SCREEN" ] && return 0; done; return 1; }

FAIL=0; RAN=0
echo "2D parity gate — refs: $REF"
while IFS='|' read -r SCREEN SEQ SHOT XENV; do
  [ -z "${SCREEN:-}" ] && continue
  want "$@" || continue
  [ "$SEQ" = ":" ] && SEQ=""
  ppm="$OUT/$SCREEN.ppm"
  rm -f "$ppm"
  case "$SCREEN" in campaign_map) pin_save ;; esac
  pin_settings
  ( cd "$RUNDIR" && timeout -k 5 -s KILL "$TMO" env \
      SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" MA_DISABLE_3D=1 \
      BOB_CLICKSEQ="$SEQ" MA_SHOT="$SHOT" MA_SHOT_PATH="$ppm" $RESENV $XENV \
      "$WMIG" ) >"$OUT/$SCREEN.log" 2>&1
  pkill -x "$(basename "$WMIG")" 2>/dev/null
  unpin_settings
  case "$SCREEN" in campaign_map) unpin_save ;; esac
  RAN=$((RAN+1))
  if [ ! -s "$ppm" ]; then echo "  $SCREEN: FAIL (no capture — see $OUT/$SCREEN.log)"; FAIL=1; continue; fi
  python3 - "$ppm" "$REF/$SCREEN.png" "$SCREEN" <<'PY'
import sys
from PIL import Image
cap, ref, name = sys.argv[1], sys.argv[2], sys.argv[3]
a = Image.open(cap).convert('RGB')
try:
    b = Image.open(ref).convert('RGB')
except FileNotFoundError:
    print("  %-14s NO REFERENCE (%s) — capture is %dx%d" % (name, ref, a.size[0], a.size[1])); sys.exit(3)
if a.size != b.size:
    print("  %-14s FAIL size %s vs ref %s" % (name, a.size, b.size)); sys.exit(1)
diff = sum(1 for p, q in zip(a.getdata(), b.getdata()) if p != q)
print("  %-14s %s  (%d px differ of %d)" % (name, "OK byte-identical" if diff == 0 else "DIFF", diff, a.size[0]*a.size[1]))
sys.exit(0 if diff == 0 else 1)
PY
  [ $? -eq 0 ] || FAIL=1
done <<EOF
$RECIPES
EOF

echo "----------------------------------------"
if [ "$FAIL" -eq 0 ]; then echo "PASS: $RAN screen(s) byte-identical to the committed references"; exit 0
else echo "FAIL: a screen differs from its reference (captures in $OUT)"; exit 1; fi
