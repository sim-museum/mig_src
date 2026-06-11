#!/usr/bin/env python3
# Apply BoB's portable (BOB_LINUX) versions of the 10 sdlong/MulDiv asm functions
# to MYVECTOR.H (same functions as VECTOR.H; MYVECTOR.H is an alternate variant).
import re
p = 'SRC/H/MYVECTOR.H'
s = open(p, 'r', encoding='latin-1').read()

HELPER = '''#if defined(BOB_LINUX)
static inline long long bob_sd_get(const void* q)
{ const SLong* z=(const SLong*)q; return ((long long)z[0]<<32) | (unsigned long long)(ULong)z[1]; }
static inline void bob_sd_set(void* q, long long v)
{ SLong* z=(SLong*)q; z[0]=(SLong)(v>>32); z[1]=(SLong)(ULong)(unsigned long long)v; }
#endif
'''

PORT = {
 'LMulDiv': 'inline SLong LMulDiv( SLong num, SLong num2, SLong num3 )\n{ return (SLong)((long long)num*(long long)num2/(long long)num3); }',
 'MakeSign': 'inline SLong MakeSign(SLong num)\n{ return (SLong)(num >> 31); }',
 'SDLongAdd': 'inline void SDLongAdd(sdlong& num, sdlong& num2)\n{ bob_sd_set(&num, bob_sd_get(&num) + bob_sd_get(&num2)); }',
 'SDLongSub': 'inline void SDLongSub(sdlong& num1, sdlong& num2)\n{ bob_sd_set(&num1, bob_sd_get(&num1) - bob_sd_get(&num2)); }',
 'SDLongMul': 'inline void SDLongMul(sdlong& num1, sdlong& num2)\n{ bob_sd_set(&num1, bob_sd_get(&num1) * bob_sd_get(&num2)); }',
 'SDLongAbs': 'inline void SDLongAbs(sdlong& num)\n{ long long v=bob_sd_get(&num); bob_sd_set(&num, v<0?-v:v); }',
 'SDLongSHL': 'inline void SDLongSHL(sdlong& num1,SLong num2 )\n{ long long v=bob_sd_get(&num1); if(num2>0) v=(num2>=64)?0:(v<<num2); bob_sd_set(&num1,v); }',
 'SDLongSHR': 'inline void SDLongSHR(sdlong& num1,SLong num2)\n{ unsigned long long v=(unsigned long long)bob_sd_get(&num1); if(num2>0) v=(num2>=64)?0:(v>>num2); bob_sd_set(&num1,(long long)v); }',
 'SDLongSGN': 'inline SLong SDLongSGN(sdlong& num)\n{ return bob_sd_get(&num) < 0 ? 1 : 0; }',
 'SDLong2Long': 'inline Bool SDLong2Long(sdlong& num)\n{ long long v=bob_sd_get(&num); long long a=v<0?-v:v; return (a < 0x80000000LL) ? BOOL_TRUE : BOOL_FALSE; }',
}

def wrap(text, name, portable):
    m = re.search(r'\binline\b[^\n;{]*\b' + re.escape(name) + r'\s*\(', text)
    if not m:
        print(f"  !! {name}: not found"); return text, 0
    b = text.find('{', m.end()); depth=0; i=b
    while i < len(text):
        if text[i]=='{': depth+=1
        elif text[i]=='}':
            depth-=1
            if depth==0: break
        i+=1
    end=i+1; orig=text[m.start():end]
    if 'asm' not in orig:
        print(f"  ?? {name}: no asm, skip"); return text, 0
    block='#if defined(BOB_LINUX)\n'+portable+'\n#else\n'+orig+'\n#endif'
    print(f"  ok {name}")
    return text[:m.start()]+block+text[end:], 1

# insert helper after the sdlong forward decl
if 'bob_sd_get' not in s:
    s = re.sub(r'(class\s+sdlong\s*;\s*\n)', r'\1'+HELPER, s, count=1)

n=0
for name, body in PORT.items():
    s, k = wrap(s, name, body); n+=k
open(p,'w',encoding='latin-1').write(s)
print(f"transplanted {n} functions into MYVECTOR.H")
