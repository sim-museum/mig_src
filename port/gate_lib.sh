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
