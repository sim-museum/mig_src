#!/usr/bin/env bash
# port/mp_engage_jip_tp.sh -- MPJOIN-2: join a TEAM PLAY game in flight, on the host's side (JSIDE=0).
# Joining on a side nobody occupied at launch is a known gap (the host never built that squadron's
# flight: [addplayer] ... NOT FOUND), recorded on the board, not gated.
exec env ARM=jip GT=tp JSIDE=0 "$(dirname "$0")/mp_engage.sh" "$@"
