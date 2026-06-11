#!/usr/bin/env bash
# Compile one TU with the Phase-1 (BoB-foundation) toolchain. Usage: port/ccx.sh SRC/MATH/_MATH.CPP
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec g++ -m32 -fno-pie -fpermissive -fno-strict-aliasing -fno-delete-null-pointer-checks \
    -fcommon -fpack-struct=1 -w -DNDEBUG -DFF_LINUX -DMA_LINUX -D_LINUX \
    -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -Dstrcmpi=strcasecmp \
    -I "$ROOT/SRC/compat" -I "$ROOT/SRC/H" -I "$ROOT/SRC/MFC" \
    -c "$1" -o "${2:-/dev/null}"
