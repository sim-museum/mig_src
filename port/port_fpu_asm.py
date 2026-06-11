#!/usr/bin/env python3
# Port common single-variable x87 FPU _asm blocks to libm/__builtin.
# Tolerant of optional trailing semicolons (source is inconsistent).
import sys, re
C = r"(?:[ \t]*//[^\n]*\n)*"
def asm(body): return r"_asm\s*\{?\s*" + body + r"\s*\}"
SUBS = [
  (asm(r"fld\s+(\w+);?\s*fsqrt;?\s*fld1;?\s*fpatan;?\s*fstp\s+\1;?"),
   r"\1 = (Float)__builtin_atan2(__builtin_sqrt((double)\1), 1.0);"),
  (asm(r"fldpi;?\s*fstp\s+(\w+);?"), r"\1 = 3.14159265358979323846;"),
  (asm(r"fld\s+(\w+);?\s*fcos;?\s*"+C+r"fstp\s+\1;?"), r"\1 = (Float)__builtin_cos((double)\1);"),
  (asm(r"fld\s+(\w+);?\s*fsin;?\s*"+C+r"fstp\s+\1;?"), r"\1 = (Float)__builtin_sin((double)\1);"),
  (asm(r"fld\s+(\w+);?\s*fsqrt;?\s*fstp\s+\1;?"), r"\1 = (Float)__builtin_sqrt((double)\1);"),
  (asm(r"fild\s+(\w+);?\s*fsqrt;?\s*fstp\s+(\w+);?"), r"\2 = (double)__builtin_sqrt((double)\1);"),
  (asm(r"fld\s+(\w+);?\s*fsqrt;?\s*fstp\s+(\w+);?"), r"\2 = (double)__builtin_sqrt((double)\1);"),
]
total=0
for f in sys.argv[1:]:
    s=open(f,encoding="latin-1").read(); n=0
    for pat,rep in SUBS:
        s,k=re.subn(pat, rep, s); n+=k
    if n: open(f,"w",encoding="latin-1").write(s); print(f"{f}: {n}"); total+=n
print("total:",total)
