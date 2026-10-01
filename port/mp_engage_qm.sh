#!/usr/bin/env bash
# port/mp_engage_qm.sh -- MPQM-1: a two-player QUICK MISSION: host picks QM, reaches the Ready Room (which
# creates the session), both take frag seats (#2144 / #2145@CFragPilot) and fly the engage arm.
exec env ARM=engage GT=qm "$(dirname "$0")/mp_engage.sh" "$@"
