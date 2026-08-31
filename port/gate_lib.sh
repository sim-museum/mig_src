# port/gate_lib.sh — shared assertions for the click-driven gates. Source it, do not run it.
#
# S171: `flak_suppression.sh` reported **PASS on a run that segfaulted**. Every assertion it made
# was true — the evidence was all in the log before the crash — and the gate never looked at how
# the run ended. Only `oob_sweep.sh` checked exit status, and it is the one gate whose whole job is
# to count crashes. A gate that cannot fail on a crash is not a gate; it is a log grep.
#
# The binary prints "=== CRASH: signal N (tid …) fault_addr=… ===" followed by a backtrace, so the
# LOG is the authority rather than the exit status: `timeout` and the MA_SHOT exit path both muddy
# the status, and a crash on a worker thread need not change it at all.

# assert_no_crash <logfile> — echoes a line and returns 1 if the run crashed.
assert_no_crash() {
    local log="$1"
    if [ ! -s "$log" ]; then
        echo "  the run produced no log at all — FAIL"; return 1
    fi
    if grep -aq "=== CRASH: signal" "$log"; then
        echo "  $(grep -a '=== CRASH: signal' "$log" | head -1 | sed 's/^=== /CRASHED: /;s/ ===$//') — FAIL"
        # name the top frames: an unsymbolized address list is not a diagnosis
        local bin frames
        bin="${WMIG:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/build/wmig}"
        frames=$(grep -a "wmig() \[0x" "$log" | head -6 | sed 's/.*\[\(0x[0-9a-f]*\)\].*/\1/')
        if [ -n "$frames" ] && command -v addr2line >/dev/null 2>&1 && [ -x "$bin" ]; then
            echo "$frames" | while read -r a; do
                echo "      $a  $(addr2line -f -C -e "$bin" "$a" 2>/dev/null | head -1)"
            done
        fi
        return 1
    fi
    return 0
}

# assert_recipe_ran <logfile> — a recipe entry that can never resolve holds every entry after it
# (S171). Without this a truncated run reads as "the values never changed".
assert_recipe_ran() {
    local log="$1" rc=0
    if grep -aq "\[clickseq\] STALLED" "$log"; then
        echo "  $(grep -a '\[clickseq\] STALLED' "$log" | head -1 | sed 's/^\[clickseq\] //') — FAIL"; rc=1
    fi
    if grep -aq "\[clickid\] WARNING id=.* AMBIGUOUS" "$log"; then
        echo "  $(grep -a AMBIGUOUS "$log" | head -1 | sed 's/^\[clickid\] //') — FAIL"; rc=1
    fi
    return $rc
}

# assert_clean_start — refuse to run while another wmig is alive.
#
# S177: `damage_elements` reported "the tab bar never took a click" in a suite run and PASSED
# standalone minutes later. It was not a regression: the suite had been SIGKILLed twice to free the
# display, leaving a stray `wmig` that still held the run directory when the next gate started, so
# that gate's clicks went nowhere. The gate then reported a CONTENT failure ("the tab bar is
# broken") for an ENVIRONMENT problem -- which is the same family as S171's "PASS on a crashed run":
# a gate that cannot tell its own preconditions apart from its subject.
#
# REFUSE, do not kill. A stray wmig may be the user's own game on the display, and a gate is never
# entitled to close it. Exit 2 (not 1) so a suite can tell "could not run" from "failed".
assert_clean_start() {
    local other
    other=$(pgrep -x wmig 2>/dev/null | tr '\n' ' ')
    if [ -n "$other" ]; then
        echo "  REFUSING TO RUN: wmig already running (pid ${other%% })."
        echo "  A previous gate may have been killed, or this is an interactive session."
        echo "  This gate drives a pinned save and would fight it. Stop that process first."
        return 1
    fi
    return 0
}

# ---- S378: SAFE STASH/RESTORE -----------------------------------------------------------------
# On 2026-08-30 the player's campaign save was destroyed by a gate's own protection. /tmp hit its
# quota mid-run; parity_2d's pin_save copied "Auto Save.sav" to a backup that came out EMPTY, and
# unpin_save then wrote that 0-byte file back over the real save. Nothing checked, nothing warned.
# compass_wrap later stashed the already-zeroed file and would have restored it forever -- a
# perfect backup of a destroyed file.
#
# The backup step is exactly where a failure has to be FATAL: everything downstream is licensed by
# it. `cp` without a size check is not a backup, it is a hope.
#
#   ma_safe_backup  <original> <backup>   -> non-zero if the backup cannot be trusted
#   ma_safe_restore <backup> <original>   -> refuses to restore an empty or missing backup
ma_safe_backup() {
    local src="$1" dst="$2" a b
    [ -f "$src" ] || return 0                      # nothing to protect
    a=$(stat -c%s "$src" 2>/dev/null || echo 0)
    if [ "${a:-0}" -eq 0 ]; then
        echo "  ⚠️  $src is ALREADY 0 bytes -- refusing to stash it over a good backup." >&2
        echo "      Something has destroyed it; do not let a gate enshrine that." >&2
        return 1
    fi
    cp -a "$src" "$dst" || { echo "  ⚠️  could not copy $src -> $dst" >&2; return 1; }
    b=$(stat -c%s "$dst" 2>/dev/null || echo 0)
    if [ "$b" != "$a" ]; then
        echo "  ⚠️  BACKUP IS SHORT: $dst is $b bytes, $src is $a (disk full?)." >&2
        rm -f "$dst"                               # a truncated backup is worse than none
        return 1
    fi
    return 0
}
ma_safe_restore() {
    local bak="$1" dst="$2" b
    [ -f "$bak" ] || return 0
    b=$(stat -c%s "$bak" 2>/dev/null || echo 0)
    if [ "${b:-0}" -eq 0 ]; then
        echo "  ⚠️  REFUSING to restore a 0-byte backup over $dst -- leaving the file alone." >&2
        return 1
    fi
    cp -f "$bak" "$dst"
}
