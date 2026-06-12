#!/usr/bin/env bash
# Compile one MFC fragment .cpp individually (approach B), with the unity prelude
# (stdafx.h + _mfc.h) force-included as a PCH-equivalent so the fragment sees the
# full MFC + Rowan-control + game-header environment. Usage: ccx_mfc.sh FILE [out.o]
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec g++ -m32 -fno-pie -fpermissive -fno-strict-aliasing -fno-delete-null-pointer-checks \
    -fcommon -fpack-struct=1 -w -DNDEBUG -DFF_LINUX -DMA_LINUX -D_LINUX \
    -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -Dstrcmpi=strcasecmp \
    -I "$ROOT/SRC/compat" -I "$ROOT/SRC/H" -I "$ROOT/SRC/MFC" \
    -include stdafx.h -include _mfc.h \
    -c "$1" -o "${2:-/dev/null}"
