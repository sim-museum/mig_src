#!/usr/bin/env bash
# Compile a single game translation unit with the Linux/SDL2 port toolchain.
# Usage: port/cc.sh SRC/GENERAL/STATIC.CPP   (prints errors; -o /dev/null)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec g++ -m32 -c -std=gnu++98 -fpermissive -fno-strict-aliasing -w \
    -fno-operator-names -fno-exceptions \
    -include "$ROOT/port/include/ma_prelude.h" \
    -DLINUX -D__MA_PORT__ \
    -I "$ROOT/port/include" \
    -I "$ROOT/port/inc_ci" \
    "$1" -o "${2:-/dev/null}"
