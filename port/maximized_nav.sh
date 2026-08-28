#!/usr/bin/env bash
# port/maximized_nav.sh — port/panel_click.sh, run MAXIMISED. The layout the PO actually sees.
#
# WHY THIS EXISTS (PO-67, S312/S314/S317/S318/S319). S312 flipped MA_MAXIMIZE on by default, every
# gate stayed green, and the shipped build had a front end that DREW correctly and ignored every
# click; S314 reverted it. The suite ran unmaximised, where the panel-art origin is (0,0) and the
# whole class of defect is invisible -- nothing could contradict the flip. S318 measured the
# maximised suite for the first time: 1 of 10 gates passed, nine failing on ONE missed click.
#
# WHY IT IS A WRAPPER AND NOT ITS OWN RECIPE. The first cut of this gate drove the navigation with
# `rN`/`#id` recipes and PASSED ITS OWN NEGATIVE CONTROL (MA_NO_DRAWORG=1, i.e. the S317 fix
# reverted, still reported PASS). Those recipes are resolved INTERNALLY: the point producer and the
# hit-test read the same coordinate frame, so they move together and agree no matter how wrong that
# frame is. An injected click therefore cannot see a paint-vs-click mismatch -- which is exactly the
# defect the PO hit and the only one that matters here.
#
# panel_click.sh is the one gate that CAPTURES A FRAME, LOCATES THE MENU BY ITS PIXELS, and clicks
# that pixel in a real GL window. That makes the drawn position and the hit-test independent, which
# is the whole test. So this gate is a thin wrapper -- same locator, same real window, MA_MAXIMIZE=1
# -- rather than a second implementation that would drift from it.
#
# PASS = with MA_MAXIMIZE=1, clicking the pixel the menu is DRAWN at reaches OnSelectRlistbox.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
echo "maximised front-end click — panel_click under MA_MAXIMIZE=1"
MA_MAXIMIZE=1 OUT="${OUT:-/tmp/ma_maxnav}" exec "$ROOT/port/panel_click.sh" "${1:-1920x1080}"
