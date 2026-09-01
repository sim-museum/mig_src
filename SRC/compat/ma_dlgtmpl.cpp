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
int   ma_dlg_enum_kind(void* dlg, int kind, int* ids, int maxn);   /* S60 */
int   ma_dlg_kind(void* dlg, int id);                              /* S60 */
int   ma_dlg_own_size(void* dlg, int* w, int* h);                  /* S60 */
const void* ma_dlg_propbag(void* dlg, int id, int* outLen);        /* S62 */
int   ma_dlg_artnum(void* dlg, int id, long* outFn);
int   ma_fil_lookup(const char* name);                             /* S64 */
int   ma_pe_layer_on(void);
}

/* the S57 layer master switch (also consulted by afxwin.h/ma_olecontrol.cpp consumers) */
extern "C" int ma_pe_layer_on(void) {
    static int v = -1;
    if (v < 0) v = getenv("MA_NO_PE_RSRC") ? 0 : 1;
    return v;
}

/* control-class kind from the template's "{CLSID}" class string (classify by Data1).
   S60: the taxonomy moved to ma_dlgkind.h so afxwin.h's template-hosting consumer
   compares against the same values; these aliases keep the local code unchanged. */
#include "ma_dlgkind.h"
enum { K_UNKNOWN = MA_K_UNKNOWN, K_RSTATIC = MA_K_RSTATIC, K_RCOMBO = MA_K_RCOMBO,
       K_RLISTBOX = MA_K_RLISTBOX, K_RBUTTON = MA_K_RBUTTON, K_REDIT = MA_K_REDIT,
       K_REDTBT = MA_K_REDTBT, K_RTABS = MA_K_RTABS, K_RSCRLBAR = MA_K_RSCRLBAR };

struct Rect4 { int x, y, w, h; unsigned char kind;
               /* S59: template-visibility routing (parity #9 root cause).
                  tvis    = the control's WS_VISIBLE style bit (Windows creates
                            !WS_VISIBLE controls HIDDEN; a runtime ShowWindow can
                            still show them — e.g. IDD 287 id=2023 "I.D." label).
                  clipped = the control's dlu rect lies fully OUTSIDE the dialog's
                            own client rect — Windows clips children to the parent,
                            so it can NEVER paint (designers park dead controls
                            there, e.g. IDD 287's Cloud/Weather cluster at
                            dlu x=367..389 on a 335-dlu-wide dialog). */
               unsigned char tvis; unsigned char clipped; };
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
/* S60: each parsed dialog's OWN client size in px (from the template's cx/cy).
   Windows sizes a dialog window from its template; this port never did, so every
   RDialog came back 0x0 from GetClientRect — which is what collapsed the Player
   Log's layout (MakeParentDialog derives the whole tree's geometry from it). */
static std::map<void*, std::pair<int,int> >& dlgsize() {
    static std::map<void*, std::pair<int,int> > m; return m;
}
/* S62 (BoB S126 adoption, note 17 §3): the RAW persisted property stream for each
   control, kept verbatim so the hosts can replay it through the genuine
   DoPropExchange via the real CPropExchange in afxwin.h. Until now MA extracted only
   the ANSI strings it could recognise (caption / IDS_ / FIL_) and threw the rest away,
   so every design-time property — fonts, colours, alignments, the persisted version
   that gates the controls' own tail branches — was lost and each control booted from an
   empty exchange. */
static std::map<std::pair<void*, int>, std::string>& bagmap() {
    static std::map<std::pair<void*, int>, std::string> m; return m;
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
        /* S62: keep the whole record verbatim, before any string mining */
        if (s57 && size > 0)
            bagmap()[std::make_pair(dlg, (int)id)] = std::string((const char*)data, size);
        /* S136: MA_TRACE_DLGBAG — what design-time text and art each control carries. The
           caption policy (S57 broad -> S58 tickbox-only) has been argued twice from guesses
           about which controls own their caption at runtime; this prints the evidence. */
        if (getenv("MA_TRACE_DLGBAG"))
            fprintf(stderr, "[dlgbag] dlg=%p id=%d label=\"%s\" ids=\"%s\" art=\"%s\"\n",
                    dlg, (int)id, label.c_str(), ids.c_str(), art.c_str());
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
   (e.g. the Controls tab tickboxes: FIL_ICON_TICKBOX1 -> 0x6a81). 0 = none/unknown.
   S58 narrowing (BoB note 16's first-cut-regression caveat, hit here too): the S57
   broad application of persisted art/String to EVERY button regressed live screens —
   toolbar/system-box buttons whose art+caption are runtime-managed drew their
   design-bag state (all prefs tabs red/highlight art, invisible "Quit"/"Size"
   system-box buttons materialising at (0,0)). Only the TICKBOX family genuinely
   needs its design-time art (+glyph caption) — the game never sets it at runtime
   (parity #7's missing checkboxes). So restrict to FIL_ICON_TICKBOX*; everything
   else stays runtime-owned. MA_BTN_ART_ALL=1 re-widens for A/B archaeology. */
/* S64 — resolve a "FIL_*" art NAME to its FileNum from the F_GRAFIX.G table.
 *
 * Exported so `GetFileNum()` (port_link_stubs.cpp) can stop being a stub. The R* controls'
 * string-file setters (SetNormalFileNumString / SetPressedFileNumString) call GetFileNum
 * to turn a persisted art NAME into a runtime FileNum — which is precisely the "resolve
 * art by name" half of BoB's trap 2, the safe counterpart to the persisted numeric
 * FileNums that are meaningless authoring-install indices. With GetFileNum stubbed to 0
 * every name-resolved button lost its artwork; the Player Log's IDJ_TITLE title bar is
 * the visible case (no FIL_ entry in the template artmap — its art comes from the
 * persisted NormalFileNumString instead). */
extern "C" int ma_fil_lookup(const char* name) {
    if (!name || !*name) return 0;
    syms_load();
    int fn = symLookup(g_fils, g_nfils, name);
    return fn > 0 ? fn : 0;
}

/* S109 (PO-14): the design-time bag carries BOTH a control's art and its caption, and until now
   one predicate gated both -- which is why the art could not be widened without the captions coming
   with it. They are different questions:
     ma_dlg_artnum_any() : does this control have design-time ART?    (used to apply art)
     ma_dlg_artnum()     : ...and is it TICKBOX-family?               (used to gate the CAPTION)
   S57 applied the caption broadly and had to be reverted: system-box buttons whose caption is
   runtime-owned materialised as "Quit"/"Size", and art-carried captions doubled up. That finding is
   about captions only; the art itself is inert design data. Splitting them lets the map-filter
   toolbar's 30 buttons get their icons (PO-11) while every runtime-owned caption stays untouched. */
/* S136 (PO-28): is this button's design-time art a PLATE rather than an ICON?
 *
 * The caption policy has been argued twice from guesses. The evidence (MA_TRACE_DLGBAG) is
 * that nearly every button carries an IDS_ name, so "has a string resource" cannot be the
 * test -- the system box's buttons carry IDS_THUMBNAILMAP / IDS_ZOOMIN / IDS_LOADSAVE and the
 * map filters carry IDS_AIRFIELD / IDS_SUPPLY / ..., which are TOOLTIPS, not captions. Drawing
 * those is precisely the S57 regression ("Quit"/"Size" materialising on icon buttons).
 *
 * What separates them is the ART. An icon button's art is FIL_ICON_*, and its picture IS its
 * label. A text button's art is a plate to write on -- the D.I.S. dialog's three buttons carry
 * FIL_MAP_DIS_BUTTON and are drawn as empty bevelled bars because their captions
 * (IDS_NOTES / IDS_FOOTAGE / IDS_INTELL) were never applied. FIL_NULL is excluded: no art at
 * all is not a plate, and those controls' captions are runtime-owned.
 */
extern "C" int ma_dlg_art_isplate(void* dlg, int id) {
    if (!ma_pe_layer_on()) return 0;
    std::map<std::pair<void*, int>, std::string>& m = artmap();
    std::map<std::pair<void*, int>, std::string>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end()) return 0;
    const std::string& a = it->second;
    if (a.compare(0, 9, "FIL_ICON_") == 0) return 0;      /* icon button: the picture is the label */
    if (a.compare(0, 7, "FIL_NUL") == 0) return 0;        /* FIL_NULL / FIL_NUL: no art at all */
    return 1;
}

extern "C" int ma_dlg_artnum_any(void* dlg, int id, long* outFn) {
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

extern "C" int ma_dlg_artnum(void* dlg, int id, long* outFn) {
    if (!ma_pe_layer_on()) return 0;
    std::map<std::pair<void*, int>, std::string>& m = artmap();
    std::map<std::pair<void*, int>, std::string>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end()) return 0;
    if (!getenv("MA_BTN_ART_ALL") &&
        it->second.compare(0, 16, "FIL_ICON_TICKBOX") != 0) return 0;
    syms_load();
    int fn = symLookup(g_fils, g_nfils, it->second.c_str());
    if (fn <= 0) return 0;
    if (outFn) *outFn = (long)fn;
    return 1;
}

/* Dialog-base-units for the template font (MS Sans Serif 8pt ~ 6x13). MapDialogRect:
   px_x = dlu_x * baseX / 4, px_y = dlu_y * baseY / 8. */
/* S321 (PO-67 root cause). THESE TWO CONSTANTS ARE THE BLOCKER, and they have a twin.
   On Windows the dialog base units come from the DIALOG'S ACTUAL FONT (tmAveCharWidth /
   tmHeight), so MapDialogRect scales every template rect with the font BY CONSTRUCTION. This
   port pins them at 6x13 forever, while the front end picks its font from a RESOLUTION LADDER
   (`RFullPanelDial::OnGetGlobalFont`, FULLPANE.CPP:3612):

       currentres >= 1280 -> g_AllFonts[n][3]      (measured ~43 px tall)
       currentres >= 1024 -> g_AllFonts[n][2]
       currentres >=  800 -> g_AllFonts[n][1]      (measured ~14 px tall)
       else               -> g_AllFonts[n][0]

   So TEXT scales with the layout and RECTS do not. That is S320's "rects are
   resolution-independent while fonts are resolution-dependent", and this is its root.

   THE TWIN: the game has its own scaler, `RDialog::ScaleDialog` -- whose comment says exactly
   what it is for ("changing the dialog size at different resolutions as required in the front
   end") -- and it builds its `scalinglookup` table by calling `MapDialogRect`. Compat's
   `CWnd::MapDialogRect` (afxwin.h) is `{}`, a NO-OP. So the table comes back as the identity,
   ScaleDialog's own guard `scalinglookup[511][0][1]!=scalinglookup[511][0][0]` fails, and the
   game's scaler is INERT. Both mechanisms the engine provides for scaling dialogs are disabled,
   which is the same family as BoB S37/S315: a stubbed compat function silently disabling a real
   game mechanism, so the defect it guarded against happens.

   MA_TRACE_DLGBASE=1 reports the constants against the font actually in use, so the fix is
   measured rather than guessed. NOT YET CHANGING THE CONVERSION -- parity_2d's 800x600 gold
   references are byte-identical under 6x13, so any derived value MUST reproduce 6x13 at 800 or
   it is wrong. That check is the next sprint's gate. */
enum { DLG_BASE_X = 6, DLG_BASE_Y = 13 };
extern "C" void ma_gdi_font_metrics(void*, int*, int*);
/* Reports the font the RESOLUTION LADDER RETURNED (hfont), not the DC's current one. */
extern "C" void ma_dlgbase_report(int currentres, void* hfont)
{
    if (!getenv("MA_TRACE_DLGBASE")) return;
    static int seen = -1;
    if (currentres == seen) return;
    seen = currentres;
    int fh = 0, fw = 0;
    ma_gdi_font_metrics(hfont, &fh, &fw);
    fprintf(stderr,
        "[dlgbase] currentres=%d  ladder font h=%d w=%d  ->  implied base (%d,%d)"
        "   vs hardcoded (%d,%d)   would scale rects x%.2f\n",
        currentres, fh, fw, fw, fh, DLG_BASE_X, DLG_BASE_Y,
        DLG_BASE_Y ? (double)fh / (double)DLG_BASE_Y : 0.0);
}
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
    /* S60: the two coclasses the front-end declares in templates but no dialog class
       DDX_Control-binds. RTabs is the Player Log's tab bar (IDJ_TABCTRL in IDD 130) and
       is hosted this sprint; RScrlBar is classified for trace/audit only (it IS reached
       via DDX from the listbox, but is not hosted — backlog). */
    if (!strncasecmp(cls+1, "4A1E1986", 8)) return K_RTABS;
    if (!strncasecmp(cls+1, "505AEE46", 8)) return K_RSCRLBAR;
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
    short dcx, dcy;                                /* S59: dialog client size in dlus (clip bound) */
    if (ex) { style = rd32(d + 12); cdit = rd16(d + 16); dcx = (short)rd16(d + 22); dcy = (short)rd16(d + 24); p = d + 26; }
    else    { style = rd32(d);      cdit = rd16(d + 8);  dcx = (short)rd16(d + 14); dcy = (short)rd16(d + 16); p = d + 18; }
    p = skip_sz_or_ord(p, end);                    /* menu */
    p = skip_sz_or_ord(p, end);                    /* window class */
    p = skip_sz_or_ord(p, end);                    /* title */
    if (style & 0x40 /*DS_SETFONT*/) {
        p += 2;                                    /* point size */
        if (ex) p += 4;                            /* weight + italic + charset */
        while (p + 2 <= end && rd16(p) != 0) p += 2;
        p += 2;                                    /* font name terminator */
    }
    if (trace) fprintf(stderr, "[dlg] IDD %u dlg=%p: %d items (sz=%u%s)\n", idd, dlg, cdit, sz, ex ? ", EX" : "");
    for (int i = 0; i < cdit && p < end; i++) {
        p = align4(d, p);
        short x, y, cx, cy; unsigned id; unsigned cstyle;
        if (ex) {
            if (p + 24 > end) break;
            cstyle = rd32(p + 8);                  /* EX item: helpID, exStyle, style */
            x = (short)rd16(p + 12); y  = (short)rd16(p + 14);
            cx = (short)rd16(p + 16); cy = (short)rd16(p + 18);
            id = rd32(p + 20);
            p += 24;
        } else {
            if (p + 18 > end) break;
            cstyle = rd32(p);                      /* classic item: style, exStyle */
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
        r.tvis = (cstyle & 0x10000000u /*WS_VISIBLE*/) ? 1 : 0;
        r.clipped = (x >= dcx || y >= dcy || x + cx <= 0 || y + cy <= 0) ? 1 : 0;
        dlgmap()[std::make_pair(dlg, (int)id)] = r;
        if (trace) fprintf(stderr, "[dlg]   id=%u dlu(%d,%d,%d,%d) -> px(%d,%d,%d,%d) kind=%d style=%08x vis=%d clip=%d\n",
                           id, x, y, cx, cy, r.x, r.y, r.w, r.h, (int)r.kind, cstyle, (int)r.tvis, (int)r.clipped);
    }
    dlgsize()[dlg] = std::make_pair(dlu_x(dcx), dlu_y(dcy));   /* S60 */
    if (trace) fprintf(stderr, "[dlg] IDD %u own size dlu(%d,%d) -> px(%d,%d)\n",
                       idd, (int)dcx, (int)dcy, dlu_x(dcx), dlu_y(dcy));
    /* PO-89/S414: NOTHING ever erases dlgmap()/tmplloaded(), and both are keyed by the RAW
       DIALOG POINTER. So if one address ever carries two templates -- a dialog rebound, or a
       freed dialog's address reused by a new one -- the older template's control ids stay
       reachable under that key forever, and ma_dlg_in_template() answers "yes" for a control
       that belongs to a screen that is gone. Say it out loud when it happens. */
    {
        std::map<void*,int>& tl = tmplloaded();
        std::map<void*,int>::iterator prev = tl.find(dlg);
        if (prev != tl.end() && prev->second != (int)idd)
            fprintf(stderr, "[tmpl.rebind] dlg=%p IDD %d -> %d  (the old template's ids are STILL"
                            " in dlgmap under this pointer)\n", dlg, prev->second, (int)idd);
        else if (getenv("MA_TRACE_TMPLBIND"))
            /* PO-90/S423: report the MAP SIZES here too. These two are the ones with no erase
               anywhere in this file, so "how fast do they grow?" is the whole open question --
               and the answer has to be a number, not "forever". */
            fprintf(stderr, "[tmpl.bind] dlg=%p IDD %d   dlgmap=%zu tmplloaded=%zu\n",
                    dlg, (int)idd, dlgmap().size(), tl.size());
        fflush(stderr);
    }
    tmplloaded()[dlg] = (int)idd;
    parse_dlginit(idd, dlg);     /* also record per-control label/IDS/art text from RT_DLGINIT */
}

/* S62: the raw persisted property stream for (dialog, control), or NULL. */
extern "C" const void* ma_dlg_propbag(void* dlg, int id, int* outLen) {
    if (outLen) *outLen = 0;
    if (!ma_pe_layer_on()) return 0;
    /* S62: OPT-IN, default OFF. The reader itself is correct — all 58 bags on the boot
       path parse clean (ok=1, <=8 bytes of documented editor slop) and the payoff is
       real and gold-verified: Preferences goes from white-serif labels to gold's BLUE
       labels + YELLOW values in one step.

       S63: ON BY DEFAULT. S62's two blockers are cleared:
         (a) the uninitialised read was root-caused — CRButtonCtrl::GetParentWndInfo (x2)
             and CRStaticCtrl (x1) assign the WM_GETSTRING out-param buffer WITHOUT
             checking the returned length. `workspace[0]=99` is the IN capacity, and when
             no parent in the tree handles WM_GETSTRING SendMessage returns 0 having
             written nothing — leaving literal 'c' (0x63, exactly the first garbage byte)
             followed by uninitialised stack, adopted as the control's caption. Latent
             until this reader gave m_ResourceNumber genuine values. Now buffered, zeroed
             and only adopted when strsize>0; verified zero non-ASCII text draws and
             byte-identical across runs.
         (b) the fixed-pixel recipes were replaced with font-independent forms —
             BOB_CLICKSEQ "f,rN" (menu row, resolved via the listbox's own metric) and
             "f,#ID[:COL]" (hosted control by dialog id, column via GetColFromX). The
             pitch may now change freely; the recipes track it.
       `MA_NO_DLGINIT_PROPS=1` reverts to the empty-exchange behaviour. */
    if (getenv("MA_NO_DLGINIT_PROPS")) return 0;
    std::map<std::pair<void*, int>, std::string>& m = bagmap();
    std::map<std::pair<void*, int>, std::string>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end() || it->second.empty()) return 0;
    if (outLen) *outLen = (int)it->second.size();
    return (const void*)it->second.data();
}

/* S60: the dialog's own client size in px, as declared by its template. */
extern "C" int ma_dlg_own_size(void* dlg, int* w, int* h) {
    if (!ma_pe_layer_on()) return 0;
    std::map<void*, std::pair<int,int> >& m = dlgsize();
    std::map<void*, std::pair<int,int> >::iterator it = m.find(dlg);
    if (it == m.end() || it->second.first <= 0 || it->second.second <= 0) return 0;
    if (w) *w = it->second.first;
    if (h) *h = it->second.second;
    return 1;
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
    int in = dlgmap().count(std::make_pair(dlg, id)) ? 1 : 0;
    /* S243 (PO-71): this filter drops a control from BOTH the draw and the click walk when the
       INSTALLED Mig.exe's template does not list it. That is usually right -- it is how the port
       matches the shipped build -- but it makes a missing control completely invisible to
       diagnosis: no draw, no hit-test, no trace, nothing to grep for. The PO chased a Replay
       button that the port had silently filtered away, and the only symptom was "id=1049 never
       appears anywhere". Say so out loud. MA_TRACE_DLG=1. */
    if (!in && getenv("MA_TRACE_DLG")) {
        static std::map<std::pair<void*,int>,int> seen;
        std::pair<void*,int> k(dlg,id);
        if (!seen.count(k)) { seen[k] = 1;
            fprintf(stderr,"[tmpl] FILTERED OUT: id=%d is not in the installed template for dlg=%p"
                           " -- it will not draw and cannot be clicked\n", id, dlg);
            fflush(stderr); }
    }
    return in;
}

/* S59 (parity #9 root cause): the template's WS_VISIBLE bit for (dlg, id).
   Windows creates !WS_VISIBLE template controls HIDDEN (shown only by a later
   runtime ShowWindow) — route it as the control's INITIAL m_maVisible.
   1 = template-visible; 0 = created hidden; -1 = no template info. */
extern "C" int ma_dlg_template_visible(void* dlg, int id) {
    if (!ma_pe_layer_on()) return -1;
    std::map<std::pair<void*, int>, Rect4>& m = dlgmap();
    std::map<std::pair<void*, int>, Rect4>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end()) return -1;
    return it->second.tvis ? 1 : 0;
}

/* S59 (parity #9 root cause): 1 = the control's template rect lies fully outside
   the dialog's own client rect — Windows clips children to the parent window, so
   the control can NEVER paint, whatever its show state (designers park dead
   controls off the dialog edge, e.g. IDD 287's Cloud/Weather cluster).
   0 = inside/overlapping; -1 = no template info. */
extern "C" int ma_dlg_never_visible(void* dlg, int id) {
    if (!ma_pe_layer_on()) return -1;
    std::map<std::pair<void*, int>, Rect4>& m = dlgmap();
    std::map<std::pair<void*, int>, Rect4>::iterator it = m.find(std::make_pair(dlg, id));
    if (it == m.end()) return -1;
    return it->second.clipped ? 1 : 0;
}

/* S57: the template's RStatic control ids for a dialog — the label statics the game
   never DDX_Control-binds (on Windows the dialog manager creates EVERY template item;
   DDX-driven creation silently misses them, e.g. ~6 of the prefs-Others row labels).
   Returns the count written to ids[]. */
/* S60: enumerate this dialog's template controls of ONE kind. ma_dlg_enum_statics is
   now the K_RSTATIC special case of it (kept as its own symbol — S57 callers and the
   membership audits use it by name). */
extern "C" int ma_dlg_enum_kind(void* dlg, int kind, int* ids, int maxn) {
    if (!ma_pe_layer_on()) return 0;
    int n = 0;
    std::map<std::pair<void*, int>, Rect4>& m = dlgmap();
    for (std::map<std::pair<void*, int>, Rect4>::iterator it = m.begin(); it != m.end() && n < maxn; ++it)
        if (it->first.first == dlg && it->second.kind == kind)
            ids[n++] = it->first.second;
    return n;
}

/* S60: the parsed kind of one template control (0/K_UNKNOWN if absent or unclassified) */
extern "C" int ma_dlg_kind(void* dlg, int id) {
    if (!ma_pe_layer_on()) return K_UNKNOWN;
    std::map<std::pair<void*, int>, Rect4>& m = dlgmap();
    std::map<std::pair<void*, int>, Rect4>::iterator it = m.find(std::make_pair(dlg, id));
    return it == m.end() ? K_UNKNOWN : (int)it->second.kind;
}

extern "C" int ma_dlg_enum_statics(void* dlg, int* ids, int maxn) {
    return ma_dlg_enum_kind(dlg, K_RSTATIC, ids, maxn);
}
