#!/bin/bash
# ACL-ENGINE story 2: build the MA ACL runtime natively and smoke-test it on an ACL file + doctrine.
# usage: tools/acl_cpp_test.sh <file.acl> <doctrine>
set -e
D=$(cd "$(dirname "$0")/.." && pwd)
OUT=${TMPDIR:-/tmp}/acl_cpp_test
g++ -O1 -Wall -Wno-misleading-indentation -DACL_TEST -I"$D/SRC/H" "$D/SRC/compat/ma_acl.cpp" -o "$OUT"
"$OUT" "$@"
