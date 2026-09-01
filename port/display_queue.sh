#!/usr/bin/env bash
# port/display_queue.sh -- run every MA measurement that has been waiting on the display, in one
# unattended pass, in priority order.
#
# WHY THIS EXISTS. Eighteen sprints ran while the PO's own BoB session held the single display
# (gl-lock serialises it, correctly). Work accumulated that is not analysis but MEASUREMENT, and
# some of it GATES A CHANGE ALREADY IN THE TREE: S399-S403 made ETO_CLIPPED actually clip, which
# means the S67 clip now bites on buttons, tabs, combos and spins for the FIRST TIME. That is a
# regression risk against every front-end screen, and parity_2d is the gate that would see it.
#
# So the order here is deliberate: REGRESSION CHECKS FIRST, new investigations after. If the display
# frees only briefly, the thing that runs is the thing protecting work already committed.
#
# Each step writes its own log and the script keeps going on failure -- a red parity_2d must not
# stop the leak census from being collected in the same window.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${OUT:-$ROOT/port/out/display_queue}"; mkdir -p "$OUT"
run() {  # run <name> <timeout> <command...>
    local name="$1" tmo="$2"; shift 2
    echo "=== $name ==="
    timeout "$tmo" "$@" > "$OUT/$name.log" 2>&1
    local rc=$?
    printf '  %-22s exit=%s   (%s)\n' "$name" "$rc" "$OUT/$name.log"
    tail -4 "$OUT/$name.log" | sed 's/^/    /'
}

echo "MA display queue -- regression checks first"

# 1. THE REGRESSION CHECK for S399-S403. parity_2d compares whole front-end screens against gold,
#    so it is the only thing that can show text vanishing from a control now that the clip bites.
run parity_2d 1800 bash "$ROOT/port/parity_2d.sh"

# 2. PO-77's own A/B: does the replay-save list still bleed? S155 measured 75,456 px of film-strip
#    art coming through; with ETO_CLIPPED honoured that should now be clipped away.
run po77_default 900 bash "$ROOT/port/parity_2d.sh" replaysave
MA_NO_ETOCLIP=1 run po77_control 900 bash "$ROOT/port/parity_2d.sh" replaysave

# 3. PO-82-leak: is the handle count a fixed working set or unbounded churn? The instrument is in
#    (S393) and reports maxHandle + live registry size next to the texture counters.
run po82_leak 600 bash "$ROOT/port/texfail.sh"

# 4. PO-76: drive the HOST path. S396 found it -- CreateCommsGame, menu index 1 of selectservice.
#    Watch for Open(CREATE) in the DirectPlay trace.
echo "=== po76_host ==="
( cd "$HOME/sgl/TUE/MigAlley/WP/drive_c/rowan/mig" && \
  timeout 400 env BOB_RUN_INIT=1 BOB_DRIVE_C="$HOME/sgl/TUE/MigAlley/WP/drive_c" \
      MA_TRACE_DPLAY=1 BOB_CLICKSEQ="40,r1;95,r1" \
      "$ROOT/build/wmig" ) > "$OUT/po76_host.log" 2>&1
echo "  po76_host exit=$?"
grep -a "Open(CREATE)\|EnumConnections\|InitializeConnection" "$OUT/po76_host.log" | head -4 | sed 's/^/    /'

echo
echo "DONE -- logs in $OUT"
