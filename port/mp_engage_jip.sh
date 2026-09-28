#!/usr/bin/env bash
# port/mp_engage_jip.sh -- MPJOIN-1: port/mp_engage.sh with ARM=jip (the joiner joins a game already
# in flight). A separate name so port/gates_all.sh lists it as its own gate.
exec env ARM=jip "$(dirname "$0")/mp_engage.sh" "$@"
