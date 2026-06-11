#!/usr/bin/env python3
# Wrap MATRIX.CPP's 7 inline-asm functions with BoB's portable BOB_LINUX versions.
# Keeps Mig Alley's own game logic; only the asm bodies get a Linux branch.
import re, sys
p = 'SRC/MATH/MATRIX.CPP'
s = open(p, 'r', encoding='latin-1').read()

# name-regex -> portable replacement body
PORT = {
 'ASMTransform': '''extern "C" void XASMTransform(void);
inline SLong ASMTransform(MATRIX_PTR a,SLong& b,SLong& c,SLong& d)
{
\tSLong ret;
\t__asm__ volatile("call XASMTransform" : "=a"(ret)
\t\t: "a"(a), "d"(&b), "b"(&c), "c"(&d) : "esi","edi","cc","memory");
\treturn ret;
}''',
 'ASMDoBigXProd': '''extern "C" void XASMDoBigXProd(void);
inline Bool ASMDoBigXProd(SLong a,SLong b,SLong c,SLong d)
{
\tint ret;
\t__asm__ volatile("call XASMDoBigXProd" : "=a"(ret)
\t\t: "a"(a),"d"(b),"b"(c),"c"(d) : "esi","edi","cc","memory");
\treturn (Bool)(ret & 0xff);
}''',
 'ASMBody2Screen': '''inline void ASMBody2Screen(SLong& num1,SLong& num2,SLong& num3,SLong& num4,SLong& num5,SLong& num6)
{
\tdouble K = 8388608.0 / (__builtin_fabs((double)num3) * (double)num4);
\tnum5 = (SLong)__builtin_lrint((double)num1 * K);
\tnum6 = (SLong)__builtin_lrint(-(double)num2 * K);
}''',
 'fpSin_Cos': '''inline void fpSin_Cos(ANGLES angle, Float& sin_ang, Float& cos_ang)
{
\tdouble rad = (double)((int)angle & 0xFFFF) * 3.14159265358979323846 / 32768.0;
\tsin_ang = (Float)__builtin_sin(rad);
\tcos_ang = (Float)__builtin_cos(rad);
}''',
 'fpTan': '''inline void fpTan(ANGLES ang,Float& tanAng)
{
\tdouble rad = (double)((int)ang & 0xFFFF) * 3.14159265358979323846 / 32768.0;
\ttanAng = (Float)(__builtin_sin(rad) / __builtin_cos(rad));
}''',
 'TestOFlowY': '''inline SLong TestOFlowY(SLong a)
{
\tlong long d2 = (long long)a * 2;
\treturn (d2 >= -2147483648LL && d2 <= 2147483647LL) ? (SLong)d2 : a;
}''',
}

def wrap_function(text, name, portable):
    # find:  inline <...> name (   ... up to the opening brace, then balance braces
    m = re.search(r'\binline\b[^\n;{]*\b' + re.escape(name) + r'\s*\(', text)
    if not m:
        print(f"  !! {name}: signature not found"); return text, False
    start = m.start()
    # find first '{' at/after the signature
    b = text.find('{', m.end())
    if b < 0:
        print(f"  !! {name}: no opening brace"); return text, False
    depth = 0; i = b
    while i < len(text):
        if text[i] == '{': depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0: break
        i += 1
    end = i + 1
    original = text[start:end]
    if 'asm' not in original:
        print(f"  ?? {name}: no asm in body, skipping"); return text, False
    block = '#if defined(BOB_LINUX)\n' + portable + '\n#else\n' + original + '\n#endif'
    print(f"  ok {name}: wrapped ({end-start} chars)")
    return text[:start] + block + text[end:], True

n = 0
for name, body in PORT.items():
    s, ok = wrap_function(s, name, body)
    n += ok

# GetScale: insert bob's portable static before scaleto16bit's extern decl.
GETSCALE = '''#if defined(BOB_LINUX)
static SWord GetScale(SLong x,SLong y,SLong z)
{
\tULong m = (ULong)(x<0?-x:x) | (ULong)(y<0?-y:y) | (ULong)(z<0?-z:z);
\tint hb = m ? (31 - __builtin_clz(m)) : 0;
\tint scale = hb - 14;
\treturn (SWord)(scale < 0 ? 0 : scale);
}
#endif
'''
m = re.search(r'UWord\s+matrix::scaleto16bit\s*\(', s)
if m and 'static SWord GetScale' not in s:
    s = s[:m.start()] + GETSCALE + s[m.start():]
    print("  ok GetScale: inserted before scaleto16bit"); n += 1

open(p, 'w', encoding='latin-1').write(s)
print(f"transplanted {n} functions")
