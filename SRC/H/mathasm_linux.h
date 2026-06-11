//------------------------------------------------------------------------------
// mathasm_linux.h - portable C reimplementations of the MSVC/Watcom inline-asm
// math primitives in mathasm.h, for the Linux native port (BOB_LINUX).
// Semantics taken from the Watcom #pragma aux variants (the clearest spec).
// Included by mathasm.h under #if defined(BOB_LINUX).
//------------------------------------------------------------------------------
#ifndef MATHASM_LINUX_Included
#define MATHASM_LINUX_Included

#include <math.h>
#include <string.h>

#ifndef __fastcall
#define __fastcall
#endif
#ifndef _fastcall
#define _fastcall
#endif

// --- bit test/reset/set/complement on a memory bit-string (dword granular) ---
static inline bool BITRESET(void* p, ULong bit) {
    ULong* a=(ULong*)p; ULong m=1u<<(bit&31); ULong i=bit>>5;
    bool old=(a[i]&m)!=0; a[i]&=~m; return old;
}
static inline ULong BITRESETI(ULong v, ULong bit) { return v & ~(1u<<(bit&31)); }

static inline bool BITSET(void* p, ULong bit) {
    ULong* a=(ULong*)p; ULong m=1u<<(bit&31); ULong i=bit>>5;
    bool old=(a[i]&m)!=0; a[i]|=m; return old;
}
static inline ULong BITSETI(ULong v, ULong bit) { return v | (1u<<(bit&31)); }

static inline bool BITTEST(const void* p, ULong bit) {
    const ULong* a=(const ULong*)p; return (a[bit>>5]>>(bit&31))&1u;
}
static inline bool BITTESTI(int v, ULong bit) { return ((ULong)v>>(bit&31))&1u; }

static inline bool BITCOMP(void* p, ULong bit) {
    ULong* a=(ULong*)p; ULong m=1u<<(bit&31); ULong i=bit>>5;
    bool old=(a[i]&m)!=0; a[i]^=m; return old;
}
static inline ULong BITCOMPI(ULong v, ULong bit) { return v ^ (1u<<(bit&31)); }

// --- bit scan: lowest/highest set bit of `bits`, else `errcode` ---
static inline ULong BITSCANLOWEST(ULong bits, ULong errcode=0) {
    return bits ? (ULong)__builtin_ctz(bits) : errcode;
}
static inline ULong BITSCANHIGHEST(ULong bits, ULong errcode=0) {
    return bits ? (ULong)(31 - __builtin_clz(bits)) : errcode;
}

// --- 64-bit intermediate multiply-shift / multiply-divide (fixed point) ---
static inline ULong MULSHUNS(ULong a, ULong b, UByte sh) {
    return (ULong)(((unsigned long long)a * b) >> (sh & 31));
}
static inline SLong MULSHSIN(SLong a, SLong b, UByte sh) {
    return (SLong)(((unsigned long long)((long long)a * (long long)b)) >> (sh & 31));
}
static inline ULong SHDIVUNS(ULong num1, UByte sh, ULong den) {
    return (ULong)(((unsigned long long)num1 << (sh & 63)) / den);
}
static inline ULong SHDIVSIN(SLong num1, UByte sh, ULong den) {
    return (ULong)(((long long)num1 << (sh & 63)) / (SLong)den);
}
static inline ULong MULDIVUNS(ULong a, ULong b, ULong c) {
    return (ULong)(((unsigned long long)a * b) / c);
}
static inline SLong MULDIVSIN(SLong a, SLong b, SLong c) {
    return (SLong)(((long long)a * (long long)b) / c);
}

// --- branchless sign get/apply (sign mask is 0 or -1) ---
static inline SWord mathlib_w_getsign(SWord num) { return (SWord)(num >> 15); }
static inline SWord mathlib_w_applysign(SWord num1, SWord num2) {
    return (SWord)((num1 ^ num2) - num2);
}
static inline SLong mathlib_l_getsign(SLong num) { return (SLong)(num >> 31); }
static inline SLong mathlib_l_applysign(SLong num1, SLong num2) {
    return (num1 ^ num2) - num2;
}

// --- x87 FPU control word (precision control) ---
static inline UWord GETFPCW() {
    unsigned short v; __asm__ __volatile__("fnstcw %0" : "=m"(v)); return v;
}
static inline void SETFPCW(UWord w) {
    unsigned short v = w; __asm__ __volatile__("fldcw %0" : : "m"(v));
}
static inline void SETPREC(int p) {
    unsigned short v = GETFPCW(); v = (unsigned short)((v & 0xf0ff) | ((unsigned)p << 8));
    SETFPCW(v);
}
static inline int GETPREC() { return (GETFPCW() & 0x0f00) >> 8; }

// --- block dword copy (rep movsd copies s -> d) ---
static inline void repmovsd(void* s, void* d, int len) {
    memcpy(d, s, (size_t)len * 4);
}

static inline double SQUARE_ROOT(double w) { return __builtin_sqrt(w); }

#endif // MATHASM_LINUX_Included
