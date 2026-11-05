#!/bin/sh
# Regenerate SRC/H/MA_ACL_HERO.H (the built-in default hero doctrine) from an ACL hero file.
# usage: tools/acl_default_hero.sh [~/acl/heroes/ma_period1.acl]
set -e
SRC=${1:-$HOME/acl/heroes/ma_period1.acl}
cd "$(dirname "$0")/.."
python3 - "$SRC" <<'PY'
import sys
src = open(sys.argv[1]).read()
assert ')ACL"' not in src
hdr = ('/* ACL-ENGINE (PO 11-04: Shaw flies both sides): the built-in doctrine every AI fighter flies when MA_ACL_HERO is\n'
       '   unset -- "hero" for the UN, "hero_red" for the MiGs. A copy of ~/acl/heroes/' + sys.argv[1].rsplit('/', 1)[-1] + ', compiled in so the\n'
       '   AppImage needs no data file (and an install never keeps an old copy). Regenerate with tools/acl_default_hero.sh. */\n'
       '#ifndef MA_ACL_HERO_H\n#define MA_ACL_HERO_H\n'
       'static const char ma_acl_default_hero[] = R"ACL(' + src + ')ACL";\n#endif\n')
open('SRC/H/MA_ACL_HERO.H', 'w').write(hdr)
PY
