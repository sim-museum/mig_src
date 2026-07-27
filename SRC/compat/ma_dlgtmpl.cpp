/* ma_dlgtmpl.cpp — RT_DIALOG template parser for the Linux port.
 *
 * Real MFC's CDialog::Create(IDD) loads the dialog's RT_DIALOG template from the module
 * and creates+positions each control. Our compat CDialog::Create is otherwise a no-op, so
 * the OCX controls have no position. This parses the DLGTEMPLATE (from the INSTALLED
 * build's miglang.dll / Mig.exe via bob_res_get — the BDG 0.85F patched data, i.e. the
 * PO's parity oracle) and records each control's client-relative rect (dialog units ->
 * pixels) keyed by (dialog, control-id); DDX_Control then MoveWindow's the control into
 * place.
 *
 * S57 (BoB S124 cross-port, note 14 / lessons §8f):
 *  - classic creation-data advance fixed (a nonzero classic count INCLUDES its own size
 *    WORD; the old `2+cd` advance was the EX semantics and would desync mid-template),
 *    and DLGTEMPLATEEX is now parsed too;
 *  - each control's CLASS ("{CLSID}" for the R* OCXes) is captured -> kind, enabling
 *    template-driven static hosting (ma_dlg_enum_statics) and the template-membership
 *    draw filter (ma_dlg_in_template);
 *  - RT_DLGINIT extraction also keeps the persisted "IDS_*" resource-string name and the
 *    "FIL_*" art name; ma_dlg_label resolves IDS_* -> RESOURCE.H id -> the language DLL's
 *    (BDG-patched) string table, exactly what CRStaticCtrl's WM_GETSTRING does on Windows
 *    — the DLGINIT literal is design-time only and goes stale ("Input Device:" vs the
 *    shipped "Input Devices:"). Literal stays as fallback; IDS_NONE is the no-resource
 *    sentinel and never resolved.
 *  - MA_NO_PE_RSRC=1 reverts the whole S57 layer (old parse semantics, no kind/IDS/art
 *    capture, and — via ma_pe_layer_on() — no static hosting / membership filter / button
 *    caption+art application in the consumers). MA never had a .rc-text fallback: unlike
 *    BoB, the per-IDD parse has read the installed build's PE resources since Phase 4,
 *    and Mig.exe (also BDG-patched) already backfills anything miglang.dll lacks.
 */

#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strncasecmp (PE class GUID case varies) */
#include <stdio.h>
#include <map>
#include <string>
#include <utility>

#ifndef MA_SRC_DIR
#define MA_SRC_DIR "."
#endif

extern "C" {
const void* bob_res_get(void* h, unsigned type, unsigned id, unsigned* outSize);
int   bob_load_string(void* h, unsigned id, char* buf, int maxlen);
void  ma_dlg_load_template(unsigned idd, void* dlg);
int   ma_dlg_rect(void* dlg, int id, int* x, int* y, int* w, int* h);
int   ma_dlg_label(void* dlg, int id, char* out, int outsz);
int   ma_dlg_in_template(void* dlg, int id);
int   ma_dlg_enum_statics(void* dlg, int* ids, int maxn);
int   ma_dlg_artnum(void* dlg, int id, long* outFn);
int   ma_pe_layer_on(void);
}

/* the S57 layer master switch (also consulted by afxwin.h/ma_olecontrol.cpp consumers) */
extern "C" int ma_pe_layer_on(void) {
    static int v = -1;
    if (v < 0) v = getenv("MA_NO_PE_RSRC") ? 0 : 1;
    return v;
}

/* control-class kind from the template's "{CLSID}" class string (classify by Data1) */
enum { K_UNKNOWN = 0, K_RSTATIC, K_RCOMBO, K_RLISTBOX, K_RBUTTON, K_REDIT, K_REDTBT };

struct Rect4 { int x, y, w, h; unsigned char kind; };
typedef unsigned char u8;

/* per-(dialog-instance, control-id) tables; the instance key naturally scopes repeated
   control ids to their own dialog (the BoB S94/S123 shared-id lesson, by construction) */
static std::map<std::pair<void*, int>, std::string>& labelmap() {
    static std::map<std::pair<void*, int>, std::string> m; return m;
}
static std::map<std::pair<void*, int>, std::string>& idsmap() {     /* "IDS_*" names */
    static std::map<std::pair<void*, int>, std::string> m; return m;
}
static std::map<std::pair<void*, int>, std::string>& artmap() {     /* "FIL_*" names */
    static std::map<std::pair<void*, int>, std::string> m; return m;
}
static std::map<std::pair<void*, int>, Rect4>& dlgmap() {
    static std::map<std::pair<void*, int>, Rect4> m; return m;
}
static std::map<void*, int>& tmplloaded() {   /* dialogs with a parsed PE template */
    static std::map<void*, int> m; return m;
}

/* ---- RESOURCE.H (#define IDS_* n) + F_GRAFIX.G (FIL_* =0xNNNN) symbol tables ----
   Plain C fixed arrays (BoB bob_dlgtemplate.cpp shape): resolved once, on demand.
   Source location: $MA_RC_DIR override, else the compile-time MA_SRC_DIR. When neither
   exists (installed build without a source checkout) resolution degrades to the DLGINIT
   literals — same text as pre-S57. */
#define MAX_SYMS 20000
#define SYM_LEN  48
struct Sym { char name[SYM_LEN]; int id; };
static Sym g_syms[MAX_SYMS]; static int g_nsyms = 0;
static Sym g_fils[4096];     static int g_nfils = 0;
static int g_symloaded = 0;

static int symLookup(Sym* t, int n, const char* name) {
    for (int i = 0; i < n; i++) if (strcmp(t[i].name, name) == 0) return t[i].id;
    return -1;
}
static void symAdd(Sym* t, int* n, int maxn, const char* name, long val) {
    if (*n >= maxn || val < 0) return;
    strncpy(t[*n].name, name, SYM_LEN - 1); t[*n].name[SYM_LEN - 1] = 0;
    t[*n].id = (int)val; (*n)++;
}
static void parseResourceH(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char sym[256]; long val;
        if (sscanf(line, " #define %255s %ld", sym, &val) == 2 && strlen(sym) < SYM_LEN)
            symAdd(g_syms, &g_nsyms, MAX_SYMS, sym, val);
    }
    fclose(f);
}
/* "	FIL_ICON_TICKBOX1		=0x6a81," — FIL_* equates from the graphics file table */
static void parseFilG(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char sym[256]; long val;
        char* p = strstr(line, "FIL_");
        if (!p) continue;
        if (sscanf(p, "%255[A-Za-z0-9_] =0x%lx", sym, &val) == 2 ||
            sscanf(p, "%255[A-Za-z0-9_]=0x%lx", sym, &val) == 2 ||
            sscanf(p, "%255[A-Za-z0-9_] = 0x%lx", sym, &val) == 2) {
            if (strlen(sym) < SYM_LEN) symAdd(g_fils, &g_nfils, 4096, sym, val);
        }
    }
    fclose(f);
}
static void syms_load(void) {
    if (g_symloaded) return;
    g_symloaded = 1;
    const char* base = getenv("MA_RC_DIR"); if (!base) base = MA_SRC_DIR;
    char path[1024];
    snprintf(path, sizeof path, "%s/MFC/RESOURCE.H", base); parseResourceH(path);
    snprintf(path, sizeof path, "%s/H/F_GRAFIX.G",   base); parseFilG(path);
    if (getenv("MA_TRACE_DLG"))
        fprintf(stderr, "[dlg] symtabs: %d RESOURCE.H defines, %d FIL_ equates (from %s)\n",
                g_nsyms, g_nfils, base);
}

/* Parse the dialog's RT_DLGINIT (240) resource and record each control's label text. Each entry
   is { WORD id; WORD msg(0x0376); DWORD size; BYTE data[size] } where data = a license string
   (DWORD wchar-count + UTF-16) followed by the control's persisted properties. Rowan's controls
   persist ANSI length-prefixed strings: the "IDS_<symbol>" resource-string name, the "FIL_*"
   art name, and the design-time literal (e.g. "Display Driver:"). We keep all three, keyed by
   (dlg,id): literal -> labelmap, IDS_* -> idsmap, FIL_* -> artmap. */
static void parse_dlginit(unsigned idd, void* dlg) {
    unsigned sz = 0;
    const u8* d = (const u8*)bob_res_get(0, 240 /*RT_DLGINIT*/, idd, &sz);
    if (!d || !sz) return;
    int s57 = ma_pe_layer_on();
    unsigned p = 0;
    while (p + 8 <= sz) {
        unsigned id   = (unsigned)(d[p] | (d[p+1] << 8));
        unsigned size = (unsigned)(d[p+4] | (d[p+5]<<8) | (d[p+6]<<16) | ((unsigned)d[p+7]<<24));
        const u8* data = d + p + 8;
        if (p + 8 + size > sz) break;
        p += 8 + size;
        /* skip the license string: DWORD wchar-count + count*2 bytes */
        unsigned off = 0;
        if (size >= 4) { unsigned slen = data[0] | (data[1]<<8) | (data[2]<<16) | ((unsigned)data[3]<<24);
                         off = 4 + (slen > size ? 0 : slen * 2); }
        /* scan the remaining bytes for length-prefixed ANSI strings */
        std::string label, ids, art;
        for (unsigned i = off; i < size; ) {
            unsigned L = data[i];
            if (L >= 1 && L <= 64 && i + 1 + L <= size) {
                int printable = 1;
                for (unsigned k = 0; k < L; k++) { u8 c = data[i+1+k]; if (c < 32 || c >= 127) { printable = 0; break; } }
                if (printable) { std::string s((const char*)data + i + 1, L);
                                 if      (s.compare(0, 4, "IDS_") == 0) { if (s57) ids = s; }
                                 else if (s.compare(0, 4, "FIL_") == 0) { if (s57) art = s; }
                                 else if (s57 && s.compare(0, 9, "Copyright") == 0) { /* licence residue */ }
                                 else label = s;   /* the human label (keep the last) */
                                 i += 1 + L; continue; }
            }
            i++;
        }
        if (!label.empty()) labelmap()[std::make_pair(dlg, (int)id)] = label;
        if (!ids.empty() && ids != "IDS_NONE") idsmap()[std::make_pair(dlg, (int)id)] = ids;
        if (!art.empty()) artmap()[std::make_pair(dlg, (int)id)] = art;
    }
}

extern "C" int ma_dlg_label(void* dlg, int id, char* out, int outsz) {
    if (!out || outsz <= 0) return 0;
    out[0] = 0;
    /* S57: faithful runtime path first — resolve the persisted IDS_* name through
       RESOURCE.H and the language DLL's (BDG-patched) string table, exactly what the
       genuine control's WM_GETSTRING(ResourceNumber) does on Windows. */
    if (ma_pe_layer_on()) {
        std::map<std::pair<void*, int>, std::string>& im = idsmap();
        std::map<std::pair<void*, int>, std::string>::iterator ii = im.find(std::make_pair(dlg, id));
        if (ii != im.end()) {
            syms_load();
            int sid = symLookup(g_syms, g_nsyms, ii->second.c_str());
            if (sid > 0 && bob_load_string(0, (unsigned)sid, out, outsz) > 0 && out[0]) return 1;
        }
    }
    std::map<std::pair<void*, int>, std::string>& m = labelmap();
    std::map<std::pair<void*, int>, std::string>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end()) return 0;
    strncpy(out, it->second.c_str(), outsz - 1); out[outsz - 1] = 0;
    return 1;
}

/* the control's persisted "FIL_*" art equate, resolved to a FileNum via F_GRAFIX.G
   (e.g. the Controls tab tickboxes: FIL_ICON_TICKBOX1 -> 0x6a81). 0 = none/unknown. */
extern "C" int ma_dlg_artnum(void* dlg, int id, long* outFn) {
    if (!ma_pe_layer_on()) return 0;
    std::map<std::pair<void*, int>, std::string>& m = artmap();
    std::map<std::pair<void*, int>, std::string>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end()) return 0;
    syms_load();
    int fn = symLookup(g_fils, g_nfils, it->second.c_str());
    if (fn <= 0) return 0;
    if (outFn) *outFn = (long)fn;
    return 1;
}

/* Dialog-base-units for the template font (MS Sans Serif 8pt ~ 6x13). MapDialogRect:
   px_x = dlu_x * baseX / 4, px_y = dlu_y * baseY / 8. */
enum { DLG_BASE_X = 6, DLG_BASE_Y = 13 };
static inline int dlu_x(int v) { return v * DLG_BASE_X / 4; }
static inline int dlu_y(int v) { return v * DLG_BASE_Y / 8; }

static inline unsigned short rd16(const u8* p) { return (unsigned short)(p[0] | (p[1] << 8)); }
static inline unsigned rd32(const u8* p) { return (unsigned)(p[0] | (p[1]<<8) | (p[2]<<16) | ((unsigned)p[3]<<24)); }

/* advance past a sz_Or_Ord field (0x0000 = none, 0xFFFF + WORD ordinal, else WCHAR* nul-term);
   optionally narrows the string into out. */
static const u8* skip_sz_or_ord(const u8* p, const u8* end, char* out = 0, int outsz = 0) {
    if (out && outsz > 0) out[0] = 0;
    if (p + 2 > end) return end;
    unsigned short w = rd16(p);
    if (w == 0x0000) return p + 2;
    if (w == 0xFFFF) {                              /* ordinal */
        if (out) snprintf(out, outsz, "#%u", p + 4 <= end ? rd16(p + 2) : 0);
        return p + 4;
    }
    int k = 0;
    while (p + 2 <= end && rd16(p) != 0) {
        if (out && k < outsz - 1) { unsigned short c = rd16(p); out[k++] = (c < 256) ? (char)c : '?'; out[k] = 0; }
        p += 2;
    }
    return p + 2;                                   /* skip terminator */
}
static const u8* align4(const u8* base, const u8* p) {
    size_t off = (size_t)(p - base);
    off = (off + 3) & ~((size_t)3);
    return base + off;
}

/* R* coclass CLSIDs as they appear in template class strings (match on Data1, case varies) */
static int classifyClass(const char* cls) {
    if (!cls || cls[0] != '{') return K_UNKNOWN;
    if (!strncasecmp(cls+1, "C42BAC3D", 8)) return K_RSTATIC;
    if (!strncasecmp(cls+1, "737CB0C9", 8)) return K_RCOMBO;
    if (!strncasecmp(cls+1, "48814009", 8)) return K_RLISTBOX;
    if (!strncasecmp(cls+1, "78918646", 8)) return K_RBUTTON;
    if (!strncasecmp(cls+1, "499E2BE6", 8)) return K_REDIT;
    if (!strncasecmp(cls+1, "461A1FE3", 8)) return K_REDTBT;
    return K_UNKNOWN;
}

extern "C" void ma_dlg_load_template(unsigned idd, void* dlg) {
    if (getenv("MA_TRACE_DLGINIT")) {
        unsigned isz = 0; const u8* di = (const u8*)bob_res_get(0, 240 /*RT_DLGINIT*/, idd, &isz);
        fprintf(stderr, "[dlginit] idd=%u DLGINIT %s sz=%u: ", idd, di?"FOUND":"none", isz);
        for (unsigned j=0; di && j<48 && j<isz; j++) fprintf(stderr, "%02x ", di[j]);
        fprintf(stderr, "\n");
        const char* want = getenv("MA_DUMP_DLGINIT");
        if (want && di && idd == (unsigned)atoi(want)) {
            FILE* f = fopen("/tmp/dlginit.bin", "wb"); if (f) { fwrite(di, 1, isz, f); fclose(f);
                fprintf(stderr, "[dlginit] wrote %u bytes for idd=%u to /tmp/dlginit.bin\n", isz, idd); }
        }
    }
    int s57 = ma_pe_layer_on();
    unsigned sz = 0;
    const u8* d = (const u8*)bob_res_get(0, 5 /*RT_DIALOG*/, idd, &sz);
    int trace = getenv("MA_TRACE_DLG") ? 1 : 0;
    if (!d || sz < 18) { if (trace) fprintf(stderr, "[dlg] IDD %u dlg=%p NOT FOUND in resource module (sz=%u)\n", idd, dlg, sz); return; }
    const u8* end = d + sz;

    /* extended template (DLGTEMPLATEEX) starts with dlgVer=1, signature=0xFFFF */
    int ex = (rd16(d) == 1 && rd16(d + 2) == 0xFFFF);
    if (ex && !s57) { if (trace) fprintf(stderr, "[dlg] %u: extended template (unsupported pre-S57)\n", idd); return; }
    unsigned style; int cdit; const u8* p;
    if (ex) { style = rd32(d + 12); cdit = rd16(d + 16); p = d + 26; }
    else    { style = rd32(d);      cdit = rd16(d + 8);  p = d + 18; }
    p = skip_sz_or_ord(p, end);                    /* menu */
    p = skip_sz_or_ord(p, end);                    /* window class */
    p = skip_sz_or_ord(p, end);                    /* title */
    if (style & 0x40 /*DS_SETFONT*/) {
        p += 2;                                    /* point size */
        if (ex) p += 4;                            /* weight + italic + charset */
        while (p + 2 <= end && rd16(p) != 0) p += 2;
        p += 2;                                    /* font name terminator */
    }
    if (trace) fprintf(stderr, "[dlg] IDD %u: %d items (sz=%u%s)\n", idd, cdit, sz, ex ? ", EX" : "");
    for (int i = 0; i < cdit && p < end; i++) {
        p = align4(d, p);
        short x, y, cx, cy; unsigned id;
        if (ex) {
            if (p + 24 > end) break;
            x = (short)rd16(p + 12); y  = (short)rd16(p + 14);
            cx = (short)rd16(p + 16); cy = (short)rd16(p + 18);
            id = rd32(p + 20);
            p += 24;
        } else {
            if (p + 18 > end) break;
            x = (short)rd16(p + 8),  y  = (short)rd16(p + 10);
            cx = (short)rd16(p + 12), cy = (short)rd16(p + 14);
            id = rd16(p + 16);
            p += 18;
        }
        char cls[64];
        const u8* clsp = p;
        p = skip_sz_or_ord(p, end, cls, sizeof cls);   /* class */
        const u8* titlep = p;
        p = skip_sz_or_ord(p, end);                    /* title */
        /* creation data: WORD count (0 = none). S57 fix: a nonzero CLASSIC count
           INCLUDES the size WORD itself; an EX count EXCLUDES it (BoB §8f — the old
           unconditional `2+cd` advance desyncs a classic template mid-parse). */
        unsigned short cd = 0; const u8* cdp = 0;
        if (p + 2 <= end) {
            cd = rd16(p); cdp = p + 2;
            if (!s57)    p += 2 + cd;                          /* pre-S57 behaviour */
            else if (!cd) p += 2;
            else          p += ex ? 2u + cd : (cd >= 2 ? (unsigned)cd : 2u);
        }
        if (getenv("MA_TRACE_DLGCTL")) {
            char ttl[128]={0}; int k=0;
            const u8* tp=titlep; if(rd16(tp)!=0xFFFF){ while(rd16(tp)!=0 && k<127){ttl[k++]=(char)tp[0];tp+=2;} } else { ttl[0]='#'; }
            (void)clsp;
            fprintf(stderr,"[dlgctl] id=%u class=\"%s\" title=\"%s\" cdlen=%u cd[0..7]=", id, cls, ttl, cd);
            for(unsigned j=0;j<cd&&j<8&&cdp;j++) fprintf(stderr,"%02x ", cdp[j]);
            fprintf(stderr,"\n");
        }
        Rect4 r; r.x = dlu_x(x); r.y = dlu_y(y); r.w = dlu_x(cx); r.h = dlu_y(cy);
        r.kind = (unsigned char)(s57 ? classifyClass(cls) : K_UNKNOWN);
        dlgmap()[std::make_pair(dlg, (int)id)] = r;
        if (trace) fprintf(stderr, "[dlg]   id=%u dlu(%d,%d,%d,%d) -> px(%d,%d,%d,%d) kind=%d\n",
                           id, x, y, cx, cy, r.x, r.y, r.w, r.h, (int)r.kind);
    }
    tmplloaded()[dlg] = (int)idd;
    parse_dlginit(idd, dlg);     /* also record per-control label/IDS/art text from RT_DLGINIT */
}

extern "C" int ma_dlg_rect(void* dlg, int id, int* x, int* y, int* w, int* h) {
    std::map<std::pair<void*, int>, Rect4>& m = dlgmap();
    std::map<std::pair<void*, int>, Rect4>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end()) return 0;
    if (x) *x = it->second.x; if (y) *y = it->second.y;
    if (w) *w = it->second.w; if (h) *h = it->second.h;
    return 1;
}

/* S57: is (dlg, id) part of the installed build's template for this dialog?
   1 = yes; 0 = the dialog HAS a parsed template but the control is not in it (the
   Windows dialog manager would never create it — don't draw it); -1 = no template
   parsed for this dialog (game-positioned controls etc. — no filtering possible). */
extern "C" int ma_dlg_in_template(void* dlg, int id) {
    if (!ma_pe_layer_on()) return -1;
    if (tmplloaded().find(dlg) == tmplloaded().end()) return -1;
    return dlgmap().count(std::make_pair(dlg, id)) ? 1 : 0;
}

/* S57: the template's RStatic control ids for a dialog — the label statics the game
   never DDX_Control-binds (on Windows the dialog manager creates EVERY template item;
   DDX-driven creation silently misses them, e.g. ~6 of the prefs-Others row labels).
   Returns the count written to ids[]. */
extern "C" int ma_dlg_enum_statics(void* dlg, int* ids, int maxn) {
    if (!ma_pe_layer_on()) return 0;
    int n = 0;
    std::map<std::pair<void*, int>, Rect4>& m = dlgmap();
    for (std::map<std::pair<void*, int>, Rect4>::iterator it = m.begin(); it != m.end() && n < maxn; ++it)
        if (it->first.first == dlg && it->second.kind == K_RSTATIC)
            ids[n++] = it->first.second;
    return n;
}
