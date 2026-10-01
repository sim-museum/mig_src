#!/usr/bin/env bash
# port/mp_chat.sh -- MPCHAT-1: port/mp_engage.sh with ARM=chat (each side types a Ready Room line and
# presses RETURN; the other must receive it). A separate name so port/gates_all.sh lists it as its own gate.
exec env ARM=chat "$(dirname "$0")/mp_engage.sh" "$@"
