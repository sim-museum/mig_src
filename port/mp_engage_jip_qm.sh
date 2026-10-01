#!/usr/bin/env bash
# port/mp_engage_jip_qm.sh -- MPQMJIP-1: join a QUICK MISSION already in flight (host kept level: a QM has
# no respawn). Before the SendACDetails sizing fix the host aborted ~10 s after the joiner arrived.
exec env ARM=jip GT=qm "$(dirname "$0")/mp_engage.sh" "$@"
