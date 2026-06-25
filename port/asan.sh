#!/usr/bin/env bash
# AddressSanitizer diagnostic runner for the Mig Alley native port.
#
#   port/asan.sh build            # ASan rebuild  -> /tmp/wmig-asan (port/build-asan/)
#   port/asan.sh run [args...]    # run /tmp/wmig-asan with report-and-continue ASAN_OPTIONS
#   port/asan.sh <args...>        # shorthand for `run <args...>`
#
# The ASan build is a heap-corruption ORACLE (BoB's single most productive hardening
# tool on this same engine: caught new[]/delete mismatches, OOB writes, double-frees).
# It builds to a SEPARATE tree so the production wmig is never clobbered.
#
# ASAN_OPTIONS rationale:
#   halt_on_error=0          report EVERY invalid access and keep running (one flight
#                            surfaces all bugs), paired with -fsanitize-recover=address.
#   detect_odr_violation=0   the unity-build twins + -Wl,--allow-multiple-definition
#                            create benign duplicate globals; ODR checks are pure noise here.
#   detect_leaks=0           the engine intentionally never frees its globals; LSan = noise.
#   abort_on_error=1         still core-dump on a genuinely fatal (non-recoverable) fault.
#   log_path=/tmp/wmig-asan.log  each pid's report -> /tmp/wmig-asan.log.<pid> AND stderr.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

export ASAN_OPTIONS="halt_on_error=0:detect_odr_violation=0:detect_leaks=0:abort_on_error=1:log_path=/tmp/wmig-asan.log:print_stats=0:symbolize=1"

case "${1:-run}" in
  build)
    ASAN=1 exec port/rebuild.sh
    ;;
  run)
    shift
    [ -x /tmp/wmig-asan ] || { echo "no /tmp/wmig-asan — run: port/asan.sh build" >&2; exit 1; }
    exec /tmp/wmig-asan "$@"
    ;;
  *)
    [ -x /tmp/wmig-asan ] || { echo "no /tmp/wmig-asan — run: port/asan.sh build" >&2; exit 1; }
    exec /tmp/wmig-asan "$@"
    ;;
esac
