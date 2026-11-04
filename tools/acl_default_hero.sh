#!/bin/sh
# Regenerate SRC/H/MA_ACL_HERO.H (the built-in default hero doctrine) from an ACL hero file.
# usage: tools/acl_default_hero.sh [~/acl/heroes/sabre_ma14.acl]
set -e
SRC=${1:-$HOME/acl/heroes/sabre_ma14.acl}
cd "$(dirname "$0")/.."
python3 - "$SRC" <<'PY'
import sys
src = open(sys.argv[1]).read()
assert ')ACL"' not in src
hdr = ('/* ACL-ENGINE (PO 11-03: "make sabre_ma14 the default"): the built-in hero doctrine every UN AI fighter flies when\n'
       '   MA_ACL_HERO is unset. A copy of ~/acl/heroes/sabre_ma14.acl, compiled in so the AppImage needs no data file (and an\n'
       '   install never keeps an old copy). Regenerate with tools/acl_default_hero.sh after changing the hero. */\n'
       '#ifndef MA_ACL_HERO_H\n#define MA_ACL_HERO_H\n'
       'static const char ma_acl_default_hero[] = R"ACL(' + src + ')ACL";\n#endif\n')
open('SRC/H/MA_ACL_HERO.H', 'w').write(hdr)
PY
