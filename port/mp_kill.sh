#!/usr/bin/env bash
# port/mp_kill.sh -- MPKILL-1: port/mp_engage.sh with ARM=kill (the host hits the joiner, the joiner then
# crashes; the host must be credited with the kill and both scoreboards must agree).
exec env ARM=kill "$(dirname "$0")/mp_engage.sh" "$@"
