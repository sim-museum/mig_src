import sys, re
f, errline = sys.argv[1], int(sys.argv[2])
target = sys.argv[3] if len(sys.argv)>3 else None   # specific undeclared var
lines = open(f, encoding="latin-1").read().split("\n")
b = None
for i in range(errline-1, -1, -1):
    if lines[i][:1] == "{": b = i; break
if b is None: print("no brace"); sys.exit(1)
end = len(lines)
for i in range(b+1, len(lines)):
    if lines[i][:1] == "}": end = i; break

# Collect vars already declared at function-scope depth 0 (a plain declaration,
# not a for-init). If a var already has such a decl we must NOT add another
# (-> "redeclaration of int x"). Track brace depth so nested-block decls don't count.
TYPES = r"(?:int|long|short|char|unsigned|signed|float|double|bool|Bool|UWord|SWord|SLong|ULong|UByte|SByte|Word|Byte|DWORD|WORD|size_t|[A-Z]\w*)"
already = set()
depth = 0
for j in range(b+1, end):
    ln = lines[j]
    if depth == 0:
        for dm in re.finditer(r"(?:^\s*|[;{]\s*)"+TYPES+r"[\s\*&]+([A-Za-z_]\w*(?:\s*,\s*[A-Za-z_]\w*)*)\s*[;=]", ln):
            if "for" not in ln[:dm.start()].split(';')[-1]:
                for nm in re.findall(r"[A-Za-z_]\w*", dm.group(1)):
                    already.add(nm)
    depth += ln.count("{") - ln.count("}")

typed=[]; bare=set()
for j in range(b+1, end):
    m = re.search(r"for\s*\(\s*([A-Za-z_][\w:]*)\s+([A-Za-z_]\w*=.*?);", lines[j])
    if m:
        typ=m.group(1)
        for nm in re.finditer(r"([A-Za-z_]\w*)=", m.group(2)):
            if (typ,nm.group(1)) not in typed: typed.append((typ,nm.group(1)))
        lines[j]=re.sub(r"for(\s*)\("+re.escape(typ)+r"\s+", r"for\1(", lines[j], count=1)
    for mm in re.finditer(r"for\s*\(\s*([a-z_]\w*)=", lines[j]):  # bare for var
        bare.add(mm.group(1))
typednames={n for _,n in typed}
bare = {v for v in bare if v not in typednames}
# only force the reported var if it ACTUALLY appears in a for-loop in this function
# (else it's a function/global/type wrongly flagged 'not declared' -> don't fake-declare it)
if target and any(re.search(r"\bfor\b[^;]*\b"+re.escape(target)+r"\b", lines[j]) for j in range(b+1, end)):
    bare.add(target)
bare -= typednames
# drop anything already declared at function scope
typed = [(t,n) for (t,n) in typed if n not in already]
bare  = {n for n in bare if n not in already}
decls = [f"{t} {n};" for t,n in typed] + [f"int {n};" for n in sorted(bare) if n not in typednames]
if decls:
    lines.insert(b+1, "\t"+" ".join(decls)+"\t// Linux/GCC port: for-scope hoist")
    open(f,"w",encoding="latin-1").write("\n".join(lines))
    print("hoisted:", ", ".join(n for _,n in typed), " ".join(sorted(bare)))
else:
    # still rewrote any for-init type strips; persist those
    open(f,"w",encoding="latin-1").write("\n".join(lines))
    print("nothing to hoist (already declared:", ",".join(sorted(already&(typednames|bare))) or "none", ")")
