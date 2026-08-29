/* ma_olecontrol.cpp — faithful host for the CRListBoxCtrl OCX on Linux.
 *
 * Real MFC instantiates the control from the dialog template and the client wrapper
 * (CRListBox : CWnd) talks to it via IDispatch (InvokeHelper/Get/SetProperty by
 * dispid). We host it directly: each client CWnd lazily owns one CRListBoxCtrl, and
 * the dispid is routed to the real control method by a hand-written switch (the dispid
 * scheme is sequential dispatch-map order — see RLISTBXC.CPP BEGIN_DISPATCH_MAP:
 * 30 properties = dispids 1..30, GetCount=31, AddString=32=0x20, ...).
 *
 * Compiled in "ole" mode (afxctl.h force-included, -ISRC/RLISTBOX). */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <map>
#include <vector>
#include "RListBxC.h"          /* CRListBoxCtrl */
/* S82: implemented in ma_olebutton.cpp (only that TU can see CRButtonCtrl). */
extern "C" int ma_button_title_hit(void* ctrl, int x, int y, int w, int h);
extern "C" int ma_button_is_title(void* ctrl);                          /* S199 */
extern "C" int ma_button_help_point(void* ctrl, int w, int h, int* lx, int* ly);   /* S98 */

/* typelib version symbols the control's UpdateRegistry references (would otherwise
   come from RLISTBOX.CPP, whose OLE-registration deps we don't host). */
/* extern const => external linkage (plain `const` at namespace scope is internal in
   C++, which wouldn't satisfy RListBox.h's `extern const GUID _tlid;` declaration). */
extern const GUID CDECL _tlid = { 0x90b5eda5, 0x666f, 0x11d1, { 0xa1, 0xf0, 0x44, 0x45, 0x53, 0x54, 0, 0 } };
extern const WORD _wVerMajor = 0x1;
extern const WORD _wVerMinor = 0x3;

/* ---- per-client hosted control registry (type-aware) --------------------- */
/* The front-end is a whole family of Rowan OCX controls (RListBox, RStatic, RButton,
   RTickBox, ...). Each client CWnd registers its CONTROL TYPE via CreateControl(CLSID)
   (driven from DDX_Control). We currently fully host only CRListBoxCtrl; other control
   types are recorded but not instantiated, so their InvokeHelper/Get/SetProperty calls
   no-op instead of being mis-routed to a listbox (which corrupted state / hung nav). */
enum { CT_NONE = 0, CT_LISTBOX, CT_STATIC, CT_BUTTON, CT_COMBO, CT_EDIT, CT_EDTBT, CT_TABS, CT_RADIO, CT_SCROLL, CT_SPIN, CT_OTHER };
/* S71: set to 1 only while an OOB-path listbox (Player Log tables) is being drawn, so
   CRListBoxCtrl::OnDraw skips its opaque black box fill and lets the composited dialog
   background show through (gold's translucency). The front-end menu/prefs listboxes never set
   it, so they keep the opaque box they rely on. Read from RLISTBXC.CPP. */
extern "C" { int ma_oob_lb_draw = 0; }
/* `relative`: control was positioned from the RT_DIALOG template (client-relative to its
   dialog) -> add the parent's screen origin when drawing. Game-positioned controls (the
   menu listbox via PositionRListBox) use absolute screen coords -> no parent add. */
/* S84: drawOx/drawOy record the offset this control was LAST DRAWN at by ma_ole_draw_toolbar.
   A toolbar-hosted control's screen position is the offset passed in at PAINT time (the map
   idle draws toolbar1 at 4,26 and toolbar2 at 4,52); the parent CRToolBar's own m_maX/m_maY are
   0, so a resolver that adds those lands ~50px off — which is why `#ID` recipes for toolbar
   buttons silently missed and had to be hand-computed. Record what paint did (same principle as
   the S82 click walk mirroring the paint walk) instead of re-deriving it. -1 = never drawn. */
/* S203 (PO-63): drawH records the height this control's paint ACTUALLY covered, which for a
   listbox is not its rect. The title menu's listbox is 105x100 and draws SEVEN rows of 28px =
   199px: measured on the capture, ink runs at y=215,238,266,294,322,350,378 against a control
   at y=210 h=100. Nothing clips it -- Windows clips a child to its parent window, this path does
   not -- and the gold title screen shows the whole list too, so drawing all seven is CORRECT.
   But every listbox hit test bounded the click by m_maH, so rows 4-6 were painted and could
   never be clicked. Row 4 is REPLAY, which is why the entire replay subsystem has no gate
   (PO-61) and why the PO could reach it with a real mouse only by... not being able to either.
   The paint walk and the click walk disagreeing about one fact, for the fourth time in this port
   (paint vs click collection S165, draw vs click type filter S164, GetRowFromY vs OnLButtonDown
   S166). Same cure as S84's drawOx: store what paint did, never re-derive it. -1 = never drawn. */
struct Hosted { int type; void* ctrl; void* parent; int relative; int id; int drawOx, drawOy; int drawH; int drawW; };
extern "C" int ma_evt_fire(void* dlg, const void* tinfo, int id, int dispid);
extern "C" { extern long ma_evtA0, ma_evtA1; }   /* event args (set before firing Select) */
static std::map<void*, Hosted>& hosted() { static std::map<void*, Hosted> m; return m; }

/* F2 — open-dropdown state (one at a time). g_dd_client is the client key of the combo whose
   list is open, or NULL. Geometry is captured during draw_all (the dropdown is drawn on top
   after the per-control loop) and reused to hit-test row clicks. */
/* S121 (PO-16): which hosted control has the keyboard. Set by CWnd::SetFocus and by clicking an
   edit; cleared when its dialog's controls are removed. */
static void* g_focus_client = 0;
static void* g_dd_client = 0;
static int   g_dd_ox, g_dd_oy, g_dd_w, g_dd_boxh, g_dd_rowh, g_dd_count, g_dd_hover = -1;
static void* combo_ctrl_of(void* client) {
    std::map<void*, Hosted>& m = hosted();
    std::map<void*, Hosted>::iterator it = m.find(client);
    return (it != m.end() && it->second.type == CT_COMBO) ? it->second.ctrl : 0;
}

/* per-type glue implemented in ma_olestatic.cpp (separate TU to avoid OCX-project
   header collisions) */
extern "C" void  ma_edit_char(void* ctrl, int ch);
extern "C" void  ma_edit_key(void* ctrl, int vk);
extern "C" const char* ma_edit_text(void* ctrl);
extern "C" void* ma_static_create(void* client);
extern "C" void  ma_static_set_string(void* ctrl, const char* s);
extern "C" void  ma_static_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_static_set_text(void* ctrl, const char* s);   /* S197 */
extern "C" void  ma_edit_set_text(void* ctrl, const char* s);     /* S197 */
extern "C" void  ma_static_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_static_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
extern "C" void* ma_button_create(void* client);
extern "C" void  ma_button_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_button_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_button_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
extern "C" void  ma_button_set_filenum(void* ctrl, long fn);
extern "C" void  ma_button_apply_icon(void* ctrl, int id);
extern "C" int   ma_button_resolve_art_names(void* ctrl);   /* S64: persisted art NAMES -> FileNums */
extern "C" int   ma_button_resolve_art_names_id(void* ctrl, int dbgid);
extern "C" int   ma_button_get_art(void* ctrl, long* n, long* p);   /* S62 trap 2 */
extern "C" void  ma_button_set_art(void* ctrl, long n, long p);     /* S62 trap 2 */
extern "C" void* ma_combo_create(void* client);
extern "C" void  ma_combo_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_combo_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_combo_invoke(void* ctrl, int dispid, int vt, void* pvRet, va_list ap);
extern "C" void  ma_combo_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
extern "C" void  ma_combo_click(void* ctrl);
extern "C" int   ma_combo_itemcount(void* ctrl);
extern "C" int   ma_combo_curindex(void* ctrl);
extern "C" int   ma_combo_select(void* ctrl, int row);
extern "C" void  ma_combo_dropdown_draw(void* ctrl, void* screenHdc, int sx, int sy, int w, int boxh, int hoverRow, int* out_rowh);
extern "C" void* ma_edit_create(void* client);
extern "C" void  ma_edit_set_string(void* ctrl, const char* s);
extern "C" void  ma_edit_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_edit_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_edit_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
extern "C" void  ma_button_set_string(void* ctrl, const char* s);
extern "C" void* ma_edtbt_create(void* client);
extern "C" void  ma_edtbt_set_string(void* ctrl, const char* s);
extern "C" void  ma_edtbt_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_edtbt_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_edtbt_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
/* S60 — RTabs (ma_oletabs.cpp): the Player Log / HTabBox tab bar */
extern "C" void* ma_tabs_create(void* client);
extern "C" void  ma_tabs_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_tabs_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_tabs_invoke(void* ctrl, int dispid, int vtRet, void* pvRet, va_list ap);
extern "C" void  ma_tabs_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
extern "C" long  ma_tabs_hit(void* ctrl, int x, int y);
extern "C" int   ma_tabs_click(void* ctrl, int x, int y);  /* S82: hit + SelectTab */
/* S170: RSpinBut, the last unhosted R* type. EPIC K step 8 adds a flight through the Squadron
   slot's Flights spin-box. See SRC/compat/ma_olespin.cpp. */
extern "C" void* ma_spin_create(void* client);
extern "C" void  ma_spin_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_spin_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_spin_invoke(void* ctrl, int dispid, int vtRet, void* pvRet, va_list ap);
extern "C" int   ma_spin_click(void* ctrl, void* parentWnd, int lx, int ly);
extern "C" void  ma_spin_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
extern "C" int   ma_spin_index(void* ctrl);
extern "C" int   ma_spin_arrow_point(void* ctrl, int down, int* lx, int* ly);
extern "C" int   ma_button_band_point(void* ctrl, int want, int w, int h, int* lx, int* ly);
extern "C" int   ma_button_disabled(void* ctrl);                       /* S186 */
/* S57 (BoB S124 §8f): template-membership draw filter + layer switch (ma_dlgtmpl.cpp) */
extern "C" int   ma_dlg_in_template(void* dlg, int id);
extern "C" int   ma_dlg_never_visible(void* dlg, int id);   /* S59: parked outside the dialog rect -> Windows-clipped, never paints */
extern "C" int   ma_pe_layer_on(void);
extern "C" int   ma_dlg_artnum(void* dlg, int id, long* outFn);   /* S58: tickbox-family filtered */

/* known control CLSIDs (compare on Data1) */
extern "C" {   /* S136: RRadio glue (ma_oleradio.cpp) */
void* ma_radio_create(void* client);
void  ma_radio_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_radio_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_radio_invoke(void* ctrl, int dispid, int vtRet, void* pvRet, va_list ap);
int   ma_radio_click(void* ctrl, int lx, int ly, int* outSel);
void  ma_radio_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
/* S140: RScrlBar glue (ma_olescroll.cpp) */
void* ma_scroll_create(void* client);
void  ma_scroll_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_scroll_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_scroll_invoke(void* ctrl, int dispid, int vtRet, void* pvRet, va_list ap);
int   ma_scroll_click(void* ctrl, void* parentWnd, int lx, int ly);
void  ma_scroll_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
void  ma_scroll_rect(void* ctrl, int* x, int* y, int* w, int* h);
int   ma_scroll_pos(void* ctrl);
int   ma_scroll_is_horz(void* ctrl);
}
static int clsid_is(const GUID* g, unsigned long d1) { return g && g->Data1 == d1; }

extern "C" void ma_ole_create(void* client, const void* clsidPtr, void* parent) {
    if (!client) return;
    if (getenv("MA_TRACE_OLE")) { const GUID* g=(const GUID*)clsidPtr; static int n=0; if(n++<400) fprintf(stderr,"[ole_create] client=%p clsid.Data1=%08lx parent=%p\n", client, g?(unsigned long)g->Data1:0, parent); }
    std::map<void*, Hosted>& m = hosted();
    if (m.find(client) != m.end()) { m[client].parent = parent; return; }   /* already; refresh parent */
    const GUID* clsid = (const GUID*)clsidPtr;
    Hosted h; h.type = CT_OTHER; h.ctrl = 0; h.parent = parent; h.relative = 0; h.id = 0;
    h.drawOx = h.drawOy = -1;   /* S84: not drawn yet */
    h.drawH = -1; h.drawW = -1; /* S203 / S317 */
    h.drawH = -1;               /* S203: not drawn yet */
    if (clsid_is(clsid, 0x48814009 /*RListBox*/)) {
        CRListBoxCtrl* c = new CRListBoxCtrl();
        c->m_hWnd = (HWND)client;                  /* non-null: OnDraw takes the real path */
        c->OnResetState();                         /* ClassWizard defaults (fore=white, etc.) */
        h.type = CT_LISTBOX; h.ctrl = c;
        /* ASan self-test (MA_ASAN_LISTBOX_SELFTEST=1): deterministically exercise the real
           CRListBoxCtrl::DeleteRow under ASan. DeleteRow frees a `new char[]` cell string;
           the S58 cross-port fix made that free `delete[]`. No game code calls DeleteRow
           (the shrink path uses SetNumberOfRows' own delete[] loop), so this is the only way
           to drive the fixed line. Runs once, on a throwaway control set up exactly like the
           live one (valid hwnd/parent), so AddString/DeleteRow take the real production path. */
        if (getenv("MA_ASAN_LISTBOX_SELFTEST")) {
            static bool ran = false;
            if (!ran) {
                ran = true;
                CRListBoxCtrl* t = new CRListBoxCtrl();
                t->m_hWnd = (HWND)client;
                t->OnResetState();
                if (parent) t->m_maParent = (CWnd*)parent;
                t->AddColumn(100);                 /* one column */
                t->AddString("asan-selftest-0", 0);/* new char[] cell -> column 0, row 0 */
                t->AddString("asan-selftest-1", 0);/* new char[] cell -> column 0, row 1 */
                fprintf(stderr, "[asan-selftest] CRListBoxCtrl: AddString x2 then DeleteRow x2 (S58 delete[])\n");
                t->DeleteRow(0);                   /* <-- the S58-fixed delete[] frees a cell */
                t->DeleteRow(0);
                /* `t` is intentionally leaked: COleControl's dtor is protected (MFC), the
                   live hosted controls are never freed either, and LSan is off in asan.sh.
                   The DeleteRow free we are validating already ran above. */
                fprintf(stderr, "[asan-selftest] CRListBoxCtrl DeleteRow OK\n");
            }
        }
    } else if (clsid_is(clsid, 0xc42bac3d /*RStatic*/)) {
        h.type = CT_STATIC; h.ctrl = ma_static_create(client);
    } else if (clsid_is(clsid, 0x78918646 /*RButton*/)) {
        h.type = CT_BUTTON; h.ctrl = ma_button_create(client);
    } else if (clsid_is(clsid, 0x737cb0c9 /*RCombo*/)) {
        h.type = CT_COMBO; h.ctrl = ma_combo_create(client);
    } else if (clsid_is(clsid, 0x499e2be6 /*REdit*/)) {
        h.type = CT_EDIT; h.ctrl = ma_edit_create(client);
    } else if (clsid_is(clsid, 0x461a1fe3 /*REdtBt — edit-button, e.g. prefs-Controls Calibrate*/)) {
        h.type = CT_EDTBT; h.ctrl = ma_edtbt_create(client);
    } else if (clsid_is(clsid, 0x4a1e1986 /*RTabs — HTabBox tab bar (IDJ_TABCTRL)*/)) {
        h.type = CT_TABS;  h.ctrl = ma_tabs_create(client);
    } else if (clsid_is(clsid, 0x5363ba22 /*RRadio — S136 (PO-28): the D.I.S. dialog's
                 intelligence filters. Unhosted until now, so CDIS::OnInitDialog's AddButton
                 calls went nowhere and the dialog showed blank bars where its captions
                 belong. */)) {
        h.type = CT_RADIO; h.ctrl = ma_radio_create(client);
    } else if (clsid_is(clsid, 0x505aee46 /*RScrlBar — S140: 8 of these on the campaign map alone,
                 unhosted, so every scrollable dialog listed more rows than it could show with no
                 way to reach them. */)) {
        h.type = CT_SCROLL; h.ctrl = ma_scroll_create(client);
    } else if (clsid_is(clsid, 0xc3270e66 /*RSpinBut — S170: the LAST unhosted R* type. The
                 wrapper (SRC/MFC/RSPINBUT.CPP) has compiled since bring-up, so every
                 InvokeHelper on one was a silent no-op and the control was never created,
                 drawn or clickable. EPIC K step 8 adds a flight through the Squadron slot's
                 Flights spin-box; found by BoB S197's cross-port note, which fixed the same
                 controls there (hosted but inert) and pointed out MA hosts none at all. */)) {
        h.type = CT_SPIN;  h.ctrl = ma_spin_create(client);
    }
    /* set the control's parent now so GetParent()->SendMessage(WM_GET*) works during
       early use (e.g. CRListBoxCtrl::UpdateScrollBar from AddString, before any draw). */
    if (h.ctrl && parent) ((CWnd*)h.ctrl)->m_maParent = (CWnd*)parent;
    m[client] = h;
}

/* the hosted listbox for this client, or NULL (other types / unregistered) */
static CRListBoxCtrl* get_ctrl(void* client, int /*create-unused*/) {
    std::map<void*, Hosted>& m = hosted();
    std::map<void*, Hosted>::iterator it = m.find(client);
    if (it == m.end() || it->second.type != CT_LISTBOX) return 0;
    return (CRListBoxCtrl*)it->second.ctrl;
}
static Hosted* get_hosted(void* client) {
    std::map<void*, Hosted>& m = hosted();
    std::map<void*, Hosted>::iterator it = m.find(client);
    return it == m.end() ? 0 : &it->second;
}
/* mark a control as template-positioned (client-relative) — called from DDX_Control.
   The listbox is always game-positioned (PositionRListBox, absolute), so never flag it. */
extern "C" void ma_ole_set_relative(void* client) {
    Hosted* h = get_hosted(client); if (h && h->type != CT_LISTBOX) h->relative = 1;
}
/* record the control's dialog id (from DDX_Control) so a click can fire its event by id */
extern "C" const void* ma_dlg_propbag(void* dlg, int id, int* outLen);   /* S62 */
extern "C" int ma_dlg_art_isplate(void* dlg, int id);                    /* S136 */
/* S162: `col` values below this carry a ROW index instead: col == MA_ROW_SENTINEL - row.
   Kept well clear of the real column numbers and of the -1 / -2 sentinels already in use. */
#define MA_ROW_SENTINEL (-100)
/* S170: the same encoding carries an optional COLUMN in the next byte up, so one int still
   addresses a whole CELL: col == MA_ROW_SENTINEL - row - 256*(column+1), column omitted = 0.
   Decode ONLY through these two macros -- the encoding is written in exactly one other place
   (the `:rN.C` parser in bob_video.cpp) and nowhere else. */
#define MA_RC_ROW(c)  ((MA_ROW_SENTINEL - (c)) & 0xFF)
#define MA_RC_COL(c)  (((MA_ROW_SENTINEL - (c)) >> 8) - 1)
extern "C" int ma_tabs_point(void* ctrl, int index, int* ox, int* oy);   /* S163 */
extern "C" void ma_gdi_set_clip(void*, int, int, int, int, int*);        /* S67 */
extern "C" void ma_gdi_restore_clip(void*, const int*);
extern "C" void ma_button_toggle_pressed(void* ctrl);                    /* S137 */
extern "C" int  ma_button_get_pressed(void* ctrl);

/* S62 (BoB S126 adoption, note 17 §3): replay this control's PERSISTED design-time
   property stream through its own DoPropExchange.
 *
 * Driven from ma_ole_set_id because that is the first moment (parent, id) are both
 * known — the id arrives after ma_ole_create on every path (DDX_Control and the
 * template-hosting wrapper alike), and (parent, id) is the bag's key.
 *
 * Two of BoB's three traps are handled here:
 *  - trap 2, persisted art indices: Normal/PressedFileNum in the bag are file-table
 *    indices from the AUTHORING install and are meaningless against the runtime table,
 *    so the button/edit-button hosts' art numbers are restored to their post-ctor boot
 *    values after the replay; art is resolved by NAME (the S57 FIL_ path) as before.
 *    Without this, BoB's first cut corrupted toolbar icons.
 *  - trap 1, COLORREF order: no conversion is applied here on purpose. MA's OLE_COLOR
 *    is already a 0x00BBGGRR COLORREF end to end (COleControl::TranslateColor passes it
 *    through), so the persisted value is already in the form the draw path wants —
 *    converting would be the "twice" BoB warns about.
 * Trap 3 (settled-state emulation) is a draw-path concern, not a load concern, and does
 * not apply until a screen shows the symptom.
 *
 * Fail-safe: an unattachable or malformed bag leaves the control exactly as its ctor
 * left it (MA's S58/S59 shape-(a) inits), so this can only add information. */
static void ma_px_replay(Hosted* h, void* client) {
    if (!h || !h->ctrl || !h->parent || h->id <= 0) return;
    int len = 0;
    const unsigned char* bag = (const unsigned char*)ma_dlg_propbag(h->parent, h->id, &len);
    if (!bag || len <= 0) return;

    COleControl* c = (COleControl*)h->ctrl;   /* every CR*Ctrl derives from it, offset 0 */

    /* trap 2: snapshot the boot art numbers so the design-time indices cannot stick */
    long artN = 0, artP = 0; int haveArt = 0;
    if (h->type == CT_BUTTON)              /* CRButtonCtrl only — REdtBt is a different class */
        haveArt = ma_button_get_art(h->ctrl, &artN, &artP);

    CPropExchange px;
    if (!px.Attach(bag, len)) return;
    c->DoPropExchange(&px);

    if (haveArt) ma_button_set_art(h->ctrl, artN, artP);
    /* S64: applying the PERSISTED ART NAMES here (ma_button_resolve_art_names) is
       implemented and correct in isolation, but is NOT wired in — measured regression.
       Doing it for every button resurrects the invisible system-box buttons ("Quit"/
       "Size") in the top-left 72x52 of every front-end screen, which is exactly the
       failure S58 documented when it narrowed the design-bag *caption* application to
       tickbox-class buttons only. The art-name path needs the same class narrowing, and
       the criterion is not yet established. Enable with MA_BTN_ART_NAMES=1 to experiment;
       the parity sweep is the arbiter. */
    if (haveArt && (h->id == 1001 /*IDJ_TITLE, see ma_ole_set_label*/ || getenv("MA_BTN_ART_NAMES")))
        ma_button_resolve_art_names_id(h->ctrl, h->id);

    if (getenv("MA_TRACE_PX")) {
        static int n = 0;
        /* S65: the cap was a fixed 60 and the boot path alone replays 58+ bags, so any
           later screen's controls fell off the end of the trace — which is how S64
           concluded "ma_px_replay never fires for id 1001" when in fact it was never
           printed. MA_TRACE_PX_MAX raises it. */
        static int cap = -1;
        if (cap < 0) { const char* c = getenv("MA_TRACE_PX_MAX"); cap = c ? atoi(c) : 60; }
        if (n++ < cap)
            fprintf(stderr, "[px] client=%p id=%d type=%d len=%d ver=%08lx ok=%d consumed=%d/%d fore=%06lx\n",
                    client, h->id, h->type, len, (unsigned long)px.m_dwVersion,
                    (int)px.m_bOk, px.m_nPos, px.m_nLen, (unsigned long)c->GetForeColor());
    }
}

extern "C" void ma_ole_set_id(void* client, int id) {
    Hosted* h = get_hosted(client);
    if (!h) return;
    h->id = id;
    ma_px_replay(h, client);
}
/* apply a label string parsed from RT_DLGINIT (DDX_Control) — statics, edits whose
   template carries a default caption (e.g. a default savename), and — S57, gated on
   the PE layer — buttons/edit-buttons (design-time String property: the Controls-tab
   tickbox glyph "3"; runtime SetCaption/SetString overwrites as on Windows). */
extern "C" void ma_ole_set_label(void* client, const char* text) {
    Hosted* h = get_hosted(client);
    if (!h || !h->ctrl) return;
    if (h->type == CT_STATIC) {
        if (getenv("MA_TRACE_BTNSTR")) fprintf(stderr, "[staticstr] id=%d parent=%p rect(%d,%d %dx%d) \"%s\"\n",
            h->id, h->parent, ((CWnd*)client)->m_maX, ((CWnd*)client)->m_maY,
            ((CWnd*)client)->m_maW, ((CWnd*)client)->m_maH, text ? text : "");
        ma_static_set_string(h->ctrl, text);
    }
    else if (h->type == CT_EDIT) ma_edit_set_string(h->ctrl, text);
    else if (h->type == CT_BUTTON && ma_pe_layer_on() && !getenv("MA_NO_BTN_STRING")) {
        /* S58 narrowing (BoB note 16 caveat, regression-proven here): a button's design-bag
           String is applied ONLY when the button is tickbox-class (carries FIL_ICON_TICKBOX*
           art — ma_dlg_artnum is already family-filtered). Everything else's caption is
           runtime-owned: applying broadly made invisible system-box buttons ("Quit"/"Size")
           materialise and doubled art-carried captions (title menu, toolbar). */
        long fn = 0;
        /* S65: IDJ_TITLE (1001) is a RESERVED ENGINE id — the dialog's title bar — in the
           same family as IDJ_TABCTRL and IDJ_PANEL0..9 that S61 already special-cases. Its
           caption is design-time by definition (IDD 276's bag carries "IDS_PLAYERLOG" +
           the literal "Player Log" + FIL_TITLEB_BMP art), so the S58 tickbox-only
           narrowing — which exists to stop runtime-owned captions being overwritten and
           system-box buttons materialising — should not apply to it. Scoped to this one
           reserved id rather than widened by a heuristic: template membership was tested
           as the general criterion and rejected (the system-box "Quit" button is a
           template control too). */
        /* S136 (PO-28): and a button whose design art is a PLATE rather than an icon. Its
           caption is design-time by construction -- there is nothing else for the plate to
           show. See ma_dlg_art_isplate for why "carries an IDS_ name" is NOT the test. */
        if (h->id == 1001 /*IDJ_TITLE*/ || ma_dlg_artnum(h->parent, h->id, &fn)
            || ma_dlg_art_isplate(h->parent, h->id)) {
            if (getenv("MA_TRACE_BTNSTR")) fprintf(stderr, "[btnstr] id=%d parent=%p \"%s\" (tickbox fn=0x%lx)\n", h->id, h->parent, text ? text : "", fn);
            ma_button_set_string(h->ctrl, text);
        }
    }
    else if (h->type == CT_EDTBT  && ma_pe_layer_on()) ma_edtbt_set_string(h->ctrl, text);
}
/* S57: apply the control's persisted "FIL_*" art (resolved FileNum) — tickbox art etc. */
extern "C" void ma_ole_set_artnum(void* client, long fn) {
    Hosted* h = get_hosted(client);
    if (!h || !h->ctrl || !ma_pe_layer_on() || getenv("MA_NO_BTN_ART")) return;
    if (getenv("MA_TRACE_BTNSTR")) fprintf(stderr, "[btnart] id=%d parent=%p fn=0x%lx\n", h->id, h->parent, fn);
    if (h->type == CT_BUTTON) ma_button_set_filenum(h->ctrl, fn);
}

/* DISPID constants (1-based dispatch-map order) */
enum {
    P_IsStripey=1, P_StripeColor, P_SelectColor, P_Lines, P_LineColor, P_DarkStripeColor,
    P_DarkBackColor, P_LockLeftColumn, P_LockTopRow, P_LockColor, P_DragAndDrop, P_FontNum,
    P_Blackboard, P_FontNum2, P_Lines2, P_HeaderColor, P_SelectWholeRows, P_FontPtr,
    P_ParentPointer, P_HilightRow, P_HilightCol, P_Border, P_Centred, P_HorzSeperation,
    P_VertSeperation, P_ToggleResizableColumns, P_ScrlBarOffset, P_ShadowSelectColour,
    P_ShadowLineColor, P_DrawBackgGound,
    F_GetCount=31, F_AddString, F_DeleteString, F_Clear, F_AddColumn, F_SetColumnWidth,
    F_AddPlayerNum, F_DeletePlayerNum, F_ReplacePlayerNum, F_ReplaceString, F_GetString,
    F_GetPlayerNum, F_GetRowFromY, F_UpdateScrollBar, F_GetListHeight, F_ResizeToFit,
    F_Shrink, F_GetColumnWidth, F_SetNumberOfRows, F_InsertRow, F_DeleteRow,
    F_SelectRecentlyFired, F_AddIconColumn, F_AddIcon, F_SetHorizontalOption, F_GetColFromX,
    F_GetRowColPlayerNum, F_SetColumnRightAligned, F_SetRowColour, F_SetIcon
};

extern "C" {

/* property GET — store result through pvRet per its declared C type */
void ma_ole_getprop(void* client, DISPID dispid, VARTYPE vt, void* pvRet) {
    Hosted* hh = get_hosted(client);
    if (hh && hh->type == CT_STATIC) { ma_static_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
    if (hh && hh->type == CT_BUTTON) { ma_button_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
    if (hh && hh->type == CT_RADIO)  { ma_radio_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
    if (hh && hh->type == CT_SCROLL) { ma_scroll_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
    if (hh && hh->type == CT_COMBO)  { ma_combo_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
    if (hh && hh->type == CT_SPIN)   { ma_spin_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
    if (hh && hh->type == CT_EDIT)   { ma_edit_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
    if (hh && hh->type == CT_EDTBT)  { ma_edtbt_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
    if (hh && hh->type == CT_TABS)   { ma_tabs_getprop(hh->ctrl,  (int)dispid, (int)vt, pvRet); return; }
    CRListBoxCtrl* c = get_ctrl(client, 1); if (!c || !pvRet) return;
    (void)vt;
    switch ((int)dispid) {
        case DISPID_FORECOLOR: *(OLE_COLOR*)pvRet = c->GetForeColor(); return;
        case DISPID_BACKCOLOR: *(OLE_COLOR*)pvRet = c->GetBackColor(); return;
        case P_IsStripey:      *(BOOL*)pvRet = c->GetIsStripey(); return;
        case P_StripeColor:    *(OLE_COLOR*)pvRet = c->GetStripeColor(); return;
        case P_SelectColor:    *(OLE_COLOR*)pvRet = c->GetSelectColor(); return;
        case P_Lines:          *(BOOL*)pvRet = c->GetLines(); return;
        case P_LineColor:      *(OLE_COLOR*)pvRet = c->GetLineColor(); return;
        case P_DarkStripeColor:*(OLE_COLOR*)pvRet = c->GetDarkStripeColor(); return;
        case P_DarkBackColor:  *(OLE_COLOR*)pvRet = c->GetDarkBackColor(); return;
        case P_LockLeftColumn: *(BOOL*)pvRet = c->GetLockLeftColumn(); return;
        case P_LockTopRow:     *(BOOL*)pvRet = c->GetLockTopRow(); return;
        case P_LockColor:      *(OLE_COLOR*)pvRet = c->GetLockColor(); return;
        case P_DragAndDrop:    *(BOOL*)pvRet = c->GetDragAndDrop(); return;
        case P_FontNum:        *(long*)pvRet = c->GetFontNum(); return;
        case P_Blackboard:     *(BOOL*)pvRet = c->GetBlackboard(); return;
        case P_FontNum2:       *(long*)pvRet = c->GetFontNum2(); return;
        case P_Lines2:         *(BOOL*)pvRet = c->GetLines2(); return;
        case P_HeaderColor:    *(OLE_COLOR*)pvRet = c->GetHeaderColor(); return;
        case P_SelectWholeRows:*(BOOL*)pvRet = c->GetSelectWholeRows(); return;
        case P_FontPtr:        *(long*)pvRet = c->GetFontPtr(); return;
        case P_ParentPointer:  *(long*)pvRet = c->GetParentPointer(); return;
        case P_HilightRow:     *(long*)pvRet = c->GetHilightRow(); return;
        case P_HilightCol:     *(long*)pvRet = c->GetHilightCol(); return;
        case P_Border:         *(BOOL*)pvRet = c->GetBorder(); return;
        case P_Centred:        *(BOOL*)pvRet = c->GetCentred(); return;
        case P_HorzSeperation: *(long*)pvRet = c->GetHorzSeperation(); return;
        case P_VertSeperation: *(long*)pvRet = c->GetVertSeperation(); return;
        case P_ToggleResizableColumns: *(BOOL*)pvRet = c->GetToggleResizableColumns(); return;
        case P_ScrlBarOffset:  *(short*)pvRet = c->GetScrlBarOffset(); return;
        case P_ShadowSelectColour: *(OLE_COLOR*)pvRet = c->GetShadowSelectColour(); return;
        case P_ShadowLineColor:*(OLE_COLOR*)pvRet = c->GetShadowLineColor(); return;
        case P_DrawBackgGound: *(BOOL*)pvRet = c->GetDrawBackgGound(); return;
    }
}

/* S197 (PO: Ins Wave shows "Player" where the time should be, and cannot be edited).
 *
 * CWnd::SetWindowTextA was `{ return TRUE; }` -- a stub that reports success and does nothing.
 * Only CRComboCtrl overrode it. So every `control->SetWindowText(...)` in the game silently went
 * nowhere and the control kept whatever text it already had.
 *
 * CWaveInsert::OnInitDialog sets the Time Over Target field exactly that way:
 *     edit->SetWindowText(CSprintf("%02i:%02i", time/60, time%60));
 * which is why the PO sees a stale "Player" instead of "08:30" -- the gold video's own value.
 *
 * Route it to the same place the OCX property set goes (SetText -> m_maText -> what OnDraw
 * renders), so the two ways of writing a control's text agree. Returns 1 if a hosted control took
 * it, so a plain CWnd keeps the old harmless no-op.
 *
 * This is the port's recurring shape: a Win32 stub that RETURNS SUCCESS. The value never arrives,
 * nothing errors, and the symptom surfaces far away as "the field shows the wrong thing".
 */
extern "C" int ma_ole_set_text(void* client, const char* s) {
    Hosted* hh = get_hosted(client);
    if (!hh) return 0;
    if (!s) s = "";
    /* delegate to the per-type glue: this router deliberately does not include the concrete
       control headers, and adding them here to reach one method would undo that. */
    switch (hh->type) {
        case CT_EDIT:   ma_edit_set_text(hh->ctrl, s);   return 1;
        case CT_STATIC: ma_static_set_text(hh->ctrl, s); return 1;
        default: break;
    }
    return 0;
}

/* property SET — single value follows in the va_list */
void ma_ole_setprop(void* client, DISPID dispid, VARTYPE vt, va_list ap) {
    Hosted* hh = get_hosted(client);
    if (hh && hh->type == CT_STATIC) { ma_static_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_BUTTON) { ma_button_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_RADIO)  { ma_radio_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_SCROLL) { ma_scroll_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_COMBO)  { ma_combo_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_SPIN)   { ma_spin_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_EDIT)   { ma_edit_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_EDTBT)  { ma_edtbt_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_TABS)   { ma_tabs_setprop(hh->ctrl,  (int)dispid, (int)vt, ap); return; }
    CRListBoxCtrl* c = get_ctrl(client, 1); if (!c) return;
    (void)vt;
    switch ((int)dispid) {
        case DISPID_FORECOLOR: c->SetForeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case DISPID_BACKCOLOR: c->SetBackColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case P_IsStripey:      c->SetIsStripey(va_arg(ap, int)); return;
        case P_StripeColor:    c->SetStripeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case P_SelectColor:    c->SetSelectColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case P_Lines:          c->SetLines(va_arg(ap, int)); return;
        case P_LineColor:      c->SetLineColor((unsigned long)va_arg(ap, unsigned long)); return;
        case P_DarkStripeColor:c->SetDarkStripeColor((unsigned long)va_arg(ap, unsigned long)); return;
        case P_DarkBackColor:  c->SetDarkBackColor((unsigned long)va_arg(ap, unsigned long)); return;
        case P_LockLeftColumn: c->SetLockLeftColumn(va_arg(ap, int)); return;
        case P_LockTopRow:     c->SetLockTopRow(va_arg(ap, int)); return;
        case P_LockColor:      c->SetLockColor((unsigned long)va_arg(ap, unsigned long)); return;
        case P_DragAndDrop:    c->SetDragAndDrop(va_arg(ap, int)); return;
        case P_FontNum:        c->SetFontNum(va_arg(ap, long)); return;
        case P_Blackboard:     c->SetBlackboard(va_arg(ap, int)); return;
        case P_FontNum2:       c->SetFontNum2(va_arg(ap, long)); return;
        case P_Lines2:         c->SetLines2(va_arg(ap, int)); return;
        case P_HeaderColor:    c->SetHeaderColor((unsigned long)va_arg(ap, unsigned long)); return;
        case P_SelectWholeRows:c->SetSelectWholeRows(va_arg(ap, int)); return;
        case P_FontPtr:        c->SetFontPtr(va_arg(ap, long)); return;
        case P_ParentPointer:  c->SetParentPointer(va_arg(ap, long)); return;
        case P_HilightRow:     c->SetHilightRow(va_arg(ap, long)); return;
        case P_HilightCol:     c->SetHilightCol(va_arg(ap, long)); return;
        case P_Border:         c->SetBorder(va_arg(ap, int)); return;
        case P_Centred:        c->SetCentred(va_arg(ap, int)); return;
        case P_HorzSeperation: c->SetHorzSeperation(va_arg(ap, long)); return;
        case P_VertSeperation: c->SetVertSeperation(va_arg(ap, long)); return;
        case P_ToggleResizableColumns: c->SetToggleResizableColumns(va_arg(ap, int)); return;
        case P_ScrlBarOffset:  c->SetScrlBarOffset((short)va_arg(ap, int)); return;
        case P_ShadowSelectColour: c->SetShadowSelectColour((unsigned long)va_arg(ap, unsigned long)); return;
        case P_ShadowLineColor:c->SetShadowLineColor((unsigned long)va_arg(ap, unsigned long)); return;
        case P_DrawBackgGound: c->SetDrawBackgGound(va_arg(ap, int)); return;
    }
}

/* method INVOKE — args in va_list, return through pvRet */
void ma_ole_invoke(void* client, DISPID dispid, WORD wFlags, VARTYPE vtRet, void* pvRet,
                   const BYTE* params, va_list ap) {
    (void)wFlags; (void)params;
    if (!client) return;   /* a method on a NULL control (e.g. combo's unopened dropdown listbox) — ignore */
    /* Combo methods (AddString/SetIndex/GetIndex/Clear/...) are low dispids 7-12 that would
       otherwise be mis-handled by the listbox path below — route them by type first. */
    { Hosted* hc = get_hosted(client);
      if (hc && hc->type == CT_COMBO) { ma_combo_invoke(hc->ctrl, (int)dispid, (int)vtRet, pvRet, ap); return; }
      /* S170: RSpinBut AddString/DeleteString/Clear are dispids 5-7 -- same low range, same reason. */
      if (hc && hc->type == CT_SPIN)  { ma_spin_invoke(hc->ctrl, (int)dispid, (int)vtRet, pvRet, ap); return; } }
    /* S60: RTabs methods are dispids 4-8, i.e. inside the same low range the listbox path
       below would misread — route by type first, exactly as the combo above. */
    { Hosted* ht = get_hosted(client);
      if (ht && ht->type == CT_TABS) { ma_tabs_invoke(ht->ctrl, (int)dispid, (int)vtRet, pvRet, ap); return; } }
    /* S136: RRadio's AddButton/Clear are dispids 5/6 -- the same low range, same reason. */
    { Hosted* hr = get_hosted(client);
      if (hr && hr->type == CT_RADIO) { ma_radio_invoke(hr->ctrl, (int)dispid, (int)vtRet, pvRet, ap); return; } }
    /* S140: RScrlBar's Move is dispid 10 -- inside the same low range. */
    { Hosted* hs = get_hosted(client);
      if (hs && hs->type == CT_SCROLL) {
          ma_scroll_invoke(hs->ctrl, (int)dispid, (int)vtRet, pvRet, ap);
          if ((int)dispid == 10) {      /* Move: mirror the rect back to the CLIENT the walk reads */
              int x=0,y=0,w=0,h=0; ma_scroll_rect(hs->ctrl,&x,&y,&w,&h);
              CWnd* cw = (CWnd*)client;
              if (cw) { cw->m_maX=x; cw->m_maY=y; cw->m_maW=w; cw->m_maH=h; }
          }
          return; } }
    /* Listbox-method dispids (>=F_GetCount=31) are unique to CRListBox — no other control
       has them. If such a method arrives for a client not yet hosted as a listbox, host it
       now: on a screen transition PositionRListBox can AddString BEFORE DDX_Control registers
       the (new panel's) listbox, which would otherwise drop the items (count stays 0). */
    if ((int)dispid >= F_GetCount) {
        Hosted* hh0 = get_hosted(client);
        if (!hh0 || hh0->type != CT_LISTBOX) {
            void* par = hh0 ? hh0->parent : 0;
            CRListBoxCtrl* nc = new CRListBoxCtrl();
            nc->m_hWnd = (HWND)client;
            nc->OnResetState();
            if (par) nc->m_maParent = (CWnd*)par;
            Hosted h; h.type = CT_LISTBOX; h.ctrl = nc; h.parent = par; h.relative = 0; h.id = 0;
            h.drawOx = h.drawOy = -1;   /* S84 */
            h.drawH = -1;               /* S317 */
            h.drawW = -1;               /* S317 */
    h.drawW = -1;               /* S317 */
            hosted()[client] = h;
        }
    }
    CRListBoxCtrl* c = get_ctrl(client, 1); if (!c) return;
    CWnd* clientWnd = (CWnd*)client;
    /* S317: ATTACH THE PARENT BEFORE ANY OF THESE RUN. ResizeToFit, Shrink, GetListHeight and
       GetRowFromY all pick their font with
           GetParent()->SendMessage(WM_GETGLOBALFONT, abs(m_FontNum2), NULL)
       and every one of them sizes something from the resulting TEXTMETRIC. With m_pParent unset
       that SendMessage answers 0, the control measures the DEFAULT font instead of the screen's
       global one, and PositionRListBox's Shrink+ResizeToFit writes back a rect computed at the
       WRONG font size: at 1920x1080 the title menu came out 105x100 while OnDraw -- which the
       draw pass does attach a parent for -- laid the same seven rows out at 292x305. The rect was
       under a third of the control, so the outer two thirds of every menu label was dead to the
       mouse (PO-67).
       This is the third time this exact attachment has had to be made: S203's comment records it
       for MaMouse, and ma_ole_menu_row_point's records it for GetRowFromY. Both fixed their own
       call site. The invoke dispatch -- where the GAME calls these methods -- was never done, so
       the rect was already wrong before any of those paths read it. Do it once, here, for every
       dispid. MA_NO_LBPARENT=1 reverts. */
    if (!getenv("MA_NO_LBPARENT")) {
        Hosted* hp = get_hosted(client);
        if (hp && hp->parent) c->m_pParent = (CWnd*)hp->parent;
    }
    /* the control sizes itself (ResizeToFit -> its own MoveWindow); mirror back to
       the client so PositionRListBox's GetClientRect/MoveWindow see the real size. */
    #define MA_SYNC_RECT() do { clientWnd->m_maX=c->m_maX; clientWnd->m_maY=c->m_maY; \
                                clientWnd->m_maW=c->m_maW; clientWnd->m_maH=c->m_maH; } while(0)
    switch ((int)dispid) {
        case F_GetCount:        if (pvRet) *(short*)pvRet = c->GetCount(); return;
        case F_AddString:       { char* s = va_arg(ap, char*); short i = (short)va_arg(ap, int); if (getenv("MA_TRACE_OLE")) fprintf(stderr, "[ole] AddString[%d] \"%s\"\n", i, s?s:"(null)"); c->AddString(s, i); return; }
        case F_DeleteString:    { short r = (short)va_arg(ap, int); short col = (short)va_arg(ap, int); c->DeleteString(r, col); return; }
        case F_Clear:           c->Clear(); return;
        case F_AddColumn:       c->AddColumn(va_arg(ap, long)); return;
        case F_SetColumnWidth:  { short i = (short)va_arg(ap, int); long w = va_arg(ap, long); c->SetColumnWidth(i, w); return; }
        case F_AddPlayerNum:    c->AddPlayerNum(va_arg(ap, long)); return;
        case F_DeletePlayerNum: { long r = c->DeletePlayerNum((short)va_arg(ap, int)); if (pvRet) *(long*)pvRet = r; return; }
        case F_ReplacePlayerNum:{ long p = va_arg(ap, long); short i = (short)va_arg(ap, int); long r = c->ReplacePlayerNum(p, i); if (pvRet) *(long*)pvRet = r; return; }
        case F_ReplaceString:   { char* s = va_arg(ap, char*); short a = (short)va_arg(ap, int); short b = (short)va_arg(ap, int); c->ReplaceString(s, a, b); return; }
        case F_GetString:       { short a = (short)va_arg(ap, int); short b = (short)va_arg(ap, int); long r = c->GetString(a, b); if (pvRet) *(long*)pvRet = r; return; }
        case F_GetPlayerNum:    { long r = c->GetPlayerNum((short)va_arg(ap, int)); if (pvRet) *(long*)pvRet = r; return; }
        case F_GetRowFromY:     { short r = c->GetRowFromY(va_arg(ap, long)); if (pvRet) *(short*)pvRet = r; return; }
        case F_UpdateScrollBar: c->UpdateScrollBar(); return;
        case F_GetListHeight:   { long r = c->GetListHeight(); if (pvRet) *(long*)pvRet = r; return; }
        case F_ResizeToFit:     c->ResizeToFit(); MA_SYNC_RECT(); return;
        case F_Shrink:          c->Shrink(); MA_SYNC_RECT(); return;
        case F_GetColumnWidth:  { long r = c->GetColumnWidth(va_arg(ap, long)); if (pvRet) *(long*)pvRet = r; return; }
        case F_SetNumberOfRows: c->SetNumberOfRows(va_arg(ap, long)); return;
        case F_InsertRow:       c->InsertRow(va_arg(ap, long)); return;
        case F_DeleteRow:       c->DeleteRow(va_arg(ap, long)); return;
        case F_SelectRecentlyFired: { BOOL r = c->SelectRecentlyFired(); if (pvRet) *(BOOL*)pvRet = r; return; }
        case F_AddIconColumn:   c->AddIconColumn(va_arg(ap, long)); return;
        case F_AddIcon:         { long a = va_arg(ap, long); short b = (short)va_arg(ap, int); c->AddIcon(a, b); return; }
        case F_SetHorizontalOption: c->SetHorizontalOption((short)va_arg(ap, int)); return;
        case F_GetColFromX:     { short r = c->GetColFromX(va_arg(ap, long)); if (pvRet) *(short*)pvRet = r; return; }
        case F_GetRowColPlayerNum: { long a = va_arg(ap, long); long b = va_arg(ap, long); long r = c->GetRowColPlayerNum(a, b); if (pvRet) *(long*)pvRet = r; return; }
        case F_SetColumnRightAligned: { long a = va_arg(ap, long); BOOL b = va_arg(ap, int); c->SetColumnRightAligned(a, b); return; }
        case F_SetRowColour:    { long a = va_arg(ap, long); long b = va_arg(ap, long); c->SetRowColour(a, b); return; }
        case F_SetIcon:         { long a = va_arg(ap, long); short b = (short)va_arg(ap, int); short d = (short)va_arg(ap, int); c->SetIcon(a, b, d); return; }
    }
}

/* Drive the control's OnDraw, compositing into the screen canvas at the client's
   MoveWindow position via the DC viewport origin. */
void ma_ole_draw(void* client, void* parentWnd, void* screenHdc) {
    CRListBoxCtrl* c = get_ctrl(client, 0);
    CWnd* clientWnd = (CWnd*)client;
    if (getenv("MA_TRACE_OLE")) {
        static int n = 0;
        if (n++ < 8) fprintf(stderr, "[ole_draw] client=%p ctrl=%p rect=(%d,%d %dx%d) count=%d\n",
            client, (void*)c, clientWnd->m_maX, clientWnd->m_maY, clientWnd->m_maW, clientWnd->m_maH,
            c ? c->GetCount() : -1);
    }
    if (!c) return;
    c->m_pParent = (CWnd*)parentWnd;
    /* mirror the client's window rect onto the control */
    c->m_maX = clientWnd->m_maX; c->m_maY = clientWnd->m_maY;
    c->m_maW = clientWnd->m_maW; c->m_maH = clientWnd->m_maH;
    int w = c->m_maW, h = c->m_maH;
    if (w <= 0 || h <= 0) return;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0;
    ma_gdi_set_viewport_org((void*)screenHdc, c->m_maX, c->m_maY, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org((void*)screenHdc, ox, oy, 0, 0);
}

/* Draw ALL hosted controls (listbox + statics + ...) into the screen canvas at their
   absolute position = parent-dialog screen origin + control client-relative pos. Called
   once per idle so every screen's controls render without per-screen wiring. */
/* A panel was destroyed (RDialog::DestroyPanel). Its child controls' clients are dialog members
   that won't be drawn again; drop their hosted-map entries so the per-frame draw_all/click scans
   don't grow unbounded across screen transitions. (The ctrl objects leak with the never-freed
   dialog — same pre-existing pattern as the no-op DestroyWindow — but the map stays bounded.) */
extern "C" void ma_gdi_clear_screen(void);
void ma_ole_remove_by_parent(void* parent) {
    /* S121: a destroyed panel must not leave the keyboard pointing at a freed control. */
    {
        std::map<void*, Hosted>& m = hosted();
        std::map<void*, Hosted>::iterator f = m.find(g_focus_client);
        if (f != m.end() && f->second.parent == parent) g_focus_client = 0;
    }
    if (!parent) return;
    std::map<void*, Hosted>& m = hosted();
    int n = 0;
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ) {
        if (it->second.parent == parent) {
            if (it->first == g_dd_client) { g_dd_client = 0; g_dd_hover = -1; }   /* F2: close orphaned dropdown */
            m.erase(it++); n++;
        } else ++it;
    }
    if (n && getenv("MA_TRACE_SIZE")) fprintf(stderr,"[hosted.remove] parent=%p removed=%d remaining=%zu\n", parent, n, m.size());
    /* PO-67 (2026-08-27): the canvas is cleared on map->panel (_wasMap) and 3D->panel (_was3d), and
       the S155 comment gives the reason -- "wherever the panel does not cover, the stale frame shows
       through". PANEL->PANEL was never covered. At 800x600 each prefs tab's art lands on the same
       rect, so the previous tab is overwritten and nobody noticed; maximized, the art lands
       differently and the previous tab's TEXT survives underneath, which is the "labels overlap and
       double up" the PO reported.
       The leftovers are stale PIXELS, not stale controls: MA_TRACE_GHOST shows the same three owners
       in both arms (CMIGView, CSSound, RFullPanelDial) with no ghost panel drawing, and this very
       function removes an identical 21 controls either way.
       A panel teardown is exactly a screen transition, so clear here. MA_NO_PANEL_CLEAR=1 reverts. */
    if (n > 0 && !getenv("MA_NO_PANEL_CLEAR")) ma_gdi_clear_screen();
}

/* S97 (PO-1): dialogs that are composited ONLY by ma_ole_draw_toolbar, never by the global pass.
   The campaign-map CRToolBars get this for free because their parent CDialog is created hidden, so
   ma_ole_draw_all skips them. CSystemBox's parent IS visible, so once its buttons were given art
   the global pass began drawing them a second time at their raw template origin -- a ghost cluster
   in the top-left corner that outlived the campaign and sat on the title screen. (It had been
   drawing all along; with no art it painted nothing, so nobody saw it.) Depending on a parent's
   hidden-ness for this is an accident; say it explicitly instead. */
static std::set<void*>& parent_scoped() { static std::set<void*> s; return s; }
extern "C" void ma_ole_set_parent_scoped(void* dialog) { if (dialog) parent_scoped().insert(dialog); }


/* S140: a listbox's SCROLLBARS are children of the LISTBOX, not of the dialog --
   CRListBoxCtrl::UpdateScrollBar does `m_pVertScrollBar->Create(..., this, 1000)`. Neither draw
   walk reaches them, because both walk controls whose parent is the DIALOG. So draw them here,
   with the listbox, at the listbox's origin plus the rect UpdateScrollBar placed them at (which
   is in listbox-client coordinates). Without this the bars were created, sized and positioned
   correctly and still never appeared. */
static void draw_listbox_scrollbars(void* listboxClient, void* listboxCtrl, void* screenHdc, int lx, int ly) {
    std::map<void*, Hosted>& m = hosted();
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        /* The bar's parent is whatever CRListBoxCtrl::UpdateScrollBar passed to Create -- which
           is the listbox CONTROL (`this`), not the client CWnd the registry is keyed by. Accept
           either: the first cut matched only the client key and found nothing at all. */
        if (h.type != CT_SCROLL || !h.ctrl) continue;
        if (h.parent != listboxClient && h.parent != listboxCtrl) continue;
        CWnd* cw = (CWnd*)it->first;
        if (!cw || !cw->m_maVisible) continue;
        if (cw->m_maW <= 0 || cw->m_maH <= 0) continue;
        if (getenv("MA_TRACE_SCROLL")) { static int n=0; if (n++<8)
            fprintf(stderr,"[scroll] draw ctrl=%p at (%d,%d) %dx%d (listbox at %d,%d)\n",
                    h.ctrl, lx + cw->m_maX, ly + cw->m_maY, cw->m_maW, cw->m_maH, lx, ly); }
        ma_scroll_draw(h.ctrl, h.parent, screenHdc, lx + cw->m_maX, ly + cw->m_maY, cw->m_maW, cw->m_maH);
    }
}

/* PO-67 / S317: WHERE PAINT PUT IT. S311 taught the draw pass to add the panel art origin
   (g_ma_panel_org_*) so the controls follow the centred artwork -- and gave the same offset to
   NO click walk. Every front-end hit-test therefore stayed at the pre-S311 coordinates while
   the pixels moved: the maximized front end drew perfectly and was completely dead to the
   mouse (`listbox id=2063 rect=(810,370,105,100) vs (1100,470) miss` -- the miss is exactly
   g_ma_panel_org). At 800x600 the origin is (0,0), so nothing moved and every gate stayed
   green; only port/panel_click.sh, which runs at a NON-default resolution for precisely this
   reason, could see it.
   One accessor, used by every walk, following S84's rule -- store what paint did, never
   re-derive it. drawOx/drawOy are recorded by BOTH paint passes (ma_ole_draw_all and
   ma_ole_draw_toolbar) and mean the same thing in each: the offset paint added to the
   control's own m_maX/m_maY. Falling back to the old arithmetic keeps a never-painted
   control behaving exactly as before. MA_NO_DRAWORG=1 reverts to the pre-S317 arithmetic. */
static void ma_ole_origin(const Hosted& h, CWnd* clientWnd, CWnd* parent, int* ox, int* oy)
{
    static int useDraw = -1;
    if (useDraw < 0) useDraw = getenv("MA_NO_DRAWORG") ? 0 : 1;
    if (useDraw && h.drawOx >= 0) {
        *ox = h.drawOx + clientWnd->m_maX;
        *oy = h.drawOy + clientWnd->m_maY;
        return;
    }
    int rel = h.relative && parent && h.type != CT_LISTBOX;
    *ox = (rel ? parent->m_maX : 0) + clientWnd->m_maX;
    *oy = (rel ? parent->m_maY : 0) + clientWnd->m_maY;
}

void ma_ole_draw_all(void* screenHdc) {
    std::map<void*, Hosted>& m = hosted();
    if (getenv("MA_TRACE_SIZE")) { static int f=0; if((f++ % 30)==0) fprintf(stderr,"[hosted.size] frame~%d entries=%zu\n", f, m.size()); }
    void* dd_ctrl = 0;   /* F2: the open dropdown's combo, captured below, drawn on top after the loop */
    /* S139 (PO-33): MA_TRACE_GHOST -- WHICH dialog is painting over this screen? A ghost panel is
       indistinguishable from a legitimate one in a screenshot, and the registry is keyed by
       control, not by owner. Print the distinct OWNERS this pass will draw, with their runtime
       class, once every 200 passes. */
    if (getenv("MA_TRACE_GHOST")) {
        static int gp = 0;
        /* 2026-08-27: the fixed 1-in-200 cadence means a capture at frame ~110 only ever shows
           PASS 1 -- the state before the dialogs are built, which is not the state being
           investigated. MA_TRACE_GHOST_EVERY=<n> sets the interval (1 = every pass). */
        static int gevery = 0;
        if (!gevery) { const char *ge = getenv("MA_TRACE_GHOST_EVERY");
                       gevery = (ge && atoi(ge) > 0) ? atoi(ge) : 200; }
        if ((gp++ % gevery) == 0) {
            std::map<void*, int> owners;
            for (std::map<void*, Hosted>::iterator gi = m.begin(); gi != m.end(); ++gi) {
                CWnd* cw = (CWnd*)gi->first; CWnd* pw = (CWnd*)gi->second.parent;
                if (!gi->second.ctrl || !cw || !cw->m_maVisible) continue;
                if (pw && !pw->m_maVisible) continue;
                if (gi->second.parent && parent_scoped().count(gi->second.parent)) continue;
                owners[gi->second.parent]++;
            }
            for (std::map<void*, int>::iterator oi = owners.begin(); oi != owners.end(); ++oi) {
                CWnd* pw = (CWnd*)oi->first;
                fprintf(stderr, "[ghost] pass %d owner=%p class=%s visible=%d controls=%d\n",
                        gp, oi->first, pw ? typeid(*pw).name() : "(none)",
                        pw ? pw->m_maVisible : -1, oi->second);
            }
        }
    }
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (!h.ctrl) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        CWnd* parent = (CWnd*)h.parent;
        if (!clientWnd) continue;                    /* defensive: never deref a NULL client key */
        if (getenv("MA_TRACE_LIST") && h.type==CT_LISTBOX && ((CRListBoxCtrl*)h.ctrl)->GetCount()!=7) { static int n=0; if(n++<10)
            fprintf(stderr,"[draw_all.lb] client=%p parent=%p clientVis=%d parentVis=%d rel=%d count=%d mX=%d mY=%d mW=%d mH=%d\n",
                it->first, h.parent, clientWnd->m_maVisible, parent?parent->m_maVisible:-1, h.relative, ((CRListBoxCtrl*)h.ctrl)->GetCount(),
                clientWnd->m_maX, clientWnd->m_maY, clientWnd->m_maW, clientWnd->m_maH); }
        /* S60: why-was-my-tab-bar-not-drawn probe (every gate below reports itself) */
        if (h.type == CT_TABS && getenv("MA_TRACE_TABS")) { static int nt=0; if (nt++<12)
            fprintf(stderr,"[tabs.draw_all] client=%p parent=%p vis=%d parentVis=%d rel=%d id=%d rect(%d,%d %dx%d) inTmpl=%d neverVis=%d\n",
                it->first, h.parent, clientWnd->m_maVisible, parent?parent->m_maVisible:-1, h.relative, h.id,
                clientWnd->m_maX, clientWnd->m_maY, clientWnd->m_maW, clientWnd->m_maH,
                (h.parent && h.id>0) ? ma_dlg_in_template(h.parent, h.id) : -1,
                (h.parent && h.id>0) ? ma_dlg_never_visible(h.parent, h.id) : -1); }
        /* skip hidden controls / controls whose parent dialog is hidden (ShowWindow(SW_HIDE)) */
        if (!clientWnd->m_maVisible) continue;
        if (parent && !parent->m_maVisible) continue;
        /* S97: this dialog is composited by the parent-scoped path only (see parent_scoped()) */
        if (h.parent && parent_scoped().count(h.parent)) continue;
        /* S57 template-membership filter (BoB S124 §8f): a template-positioned control
           ABSENT from the installed build's template for its dialog would never be
           created by the Windows dialog manager — don't draw it (kills source-only
           ghost/stray controls, e.g. Quick Mission's stray combo). Applied to the
           panel path only; the toolbar path (ma_ole_draw_toolbar) stays unfiltered. */
        if (h.relative && h.parent && h.id > 0 &&
            ma_dlg_in_template(h.parent, h.id) == 0) {
            if (getenv("MA_TRACE_BTNSTR")) { static int nf=0; if (nf++<60)
                fprintf(stderr, "[filter-skip] type=%d id=%d parent=%p rect(%d,%d %dx%d)\n",
                    h.type, h.id, h.parent, clientWnd->m_maX, clientWnd->m_maY, clientWnd->m_maW, clientWnd->m_maH); }
            continue;
        }
        /* S59 (parity #9): a template control parked fully OUTSIDE the dialog's own
           client rect is clipped away by Windows' parent-clipping and can never
           paint — whatever its show state (IDD 287's Cloud/Weather cluster at
           dlu x=367..389 on a 335-dlu dialog). Skip it like the membership filter. */
        if (h.relative && h.parent && h.id > 0 &&
            ma_dlg_never_visible(h.parent, h.id) == 1) {
            if (getenv("MA_TRACE_BTNSTR")) { static int nc=0; if (nc++<60)
                fprintf(stderr, "[clip-skip] type=%d id=%d parent=%p rect(%d,%d %dx%d)\n",
                    h.type, h.id, h.parent, clientWnd->m_maX, clientWnd->m_maY, clientWnd->m_maW, clientWnd->m_maH); }
            continue;
        }
        /* template controls are client-relative (add parent origin); game-positioned
           controls (menu listbox) are already absolute. */
        int rel = h.relative && parent && h.type != CT_LISTBOX;
        int px = rel ? parent->m_maX : 0;
        int py = rel ? parent->m_maY : 0;
        /* S311: follow the ART, not just the dialog window. The panel background is centred on the
           screen by the game (1280x1024 at (320,28) in a 1920x1080 window); the dialog window that
           owns these controls is placed from a different basis, so without this the whole control
           column -- tab bar, labels, combos -- renders on the black margin to the LEFT of the
           artwork it belongs on. S310 measured 226 px (prefs_3d), 230 (prefs_others), 156
           (quickmission).
           At 800x600 the art blits at (0,0), so this adds zero and the 800x600 references must stay
           byte-identical -- that is the check, not an assumption. MA_NO_PANEL_ORIGIN=1 reverts. */
        {   extern int g_ma_panel_org_x, g_ma_panel_org_y;
            static int useOrg = -1;
            if (useOrg < 0) useOrg = getenv("MA_NO_PANEL_ORIGIN") ? 0 : 1;
            /* Applied to game-positioned controls too, not just client-relative ones. The panel's
               TAB BAR is its listbox (IDC_RLISTBOX 2063) and `rel` deliberately excludes
               CT_LISTBOX, so the first version moved the labels and combos onto the art and left
               the tab bar stranded on the black margin -- visibly half-fixed. The listbox's
               coordinates are computed by the game against the screen, exactly like the art's, so
               it needs the same offset. Everything hosted belongs to the panel; nothing here is
               genuinely screen-absolute in a way the art is not. */
            if (useOrg) { px += g_ma_panel_org_x; py += g_ma_panel_org_y; } }
        /* S317: record it. This is the ONLY record of the panel origin a click walk can
           consult -- g_ma_panel_org_* is a moving global, and by the time a click arrives it
           may describe a different screen's art. */
        h.drawOx = px; h.drawOy = py;
        int ox = px + clientWnd->m_maX, oy = py + clientWnd->m_maY;
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (getenv("MA_TRACE_OLE")) { static int n=0; if(n++<200) { int cnt = (h.type==CT_LISTBOX) ? ((CRListBoxCtrl*)h.ctrl)->GetCount() : -1; fprintf(stderr,"[draw_all] type=%d client=%p parent=%p origin=(%d,%d) size=%dx%d vis=%d count=%d\n", h.type, it->first, h.parent, ox, oy, w, hh, clientWnd->m_maVisible, cnt); } }
        if (w <= 0 || hh <= 0) continue;
        if (h.type == CT_STATIC) {
            ma_static_draw(h.ctrl, parent, screenHdc, ox, oy, w, hh);
        } else if (h.type == CT_EDIT) {
            ma_edit_draw(h.ctrl, parent, screenHdc, ox, oy, w, hh);
        } else if (h.type == CT_EDTBT) {
            ma_edtbt_draw(h.ctrl, parent, screenHdc, ox, oy, w, hh);
        } else if (h.type == CT_TABS) {
            ma_tabs_draw(h.ctrl, parent, screenHdc, ox, oy, w, hh);
        } else if (h.type == CT_BUTTON) {
            ma_button_draw(h.ctrl, parent, screenHdc, ox, oy, w, hh);
        } else if (h.type == CT_COMBO) {
            ma_combo_draw(h.ctrl, parent, screenHdc, ox, oy, w, hh);
            if (it->first == g_dd_client) {            /* F2: remember this frame's box rect */
                g_dd_ox = ox; g_dd_oy = oy; g_dd_w = w; g_dd_boxh = hh; dd_ctrl = h.ctrl;
            }
        } else if (h.type == CT_LISTBOX) {
            CRListBoxCtrl* c = (CRListBoxCtrl*)h.ctrl;
            c->m_pParent = parent;
            c->m_maX = clientWnd->m_maX; c->m_maY = clientWnd->m_maY; c->m_maW = w; c->m_maH = hh;
            /* S203 (PO-63): record what this paint COVERS, before it happens. GetListHeight() is
               the control's own metric -- GetCount()*rowH + shadow, computed from the same
               TEXTMETRIC OnDraw lays the rows out with -- so it tracks any font change, which is
               the same reason ma_ole_menu_row_point resolves rows through it. Never shrink below
               the rect: a short list must still take clicks across its whole box. */
            { long _lh = c->GetListHeight(); h.drawH = (_lh > hh) ? (int)_lh : hh; }
            /* S317 (PO-67), the WIDTH half of S203. The rect this control carries was computed by
               PositionRListBox's Shrink+ResizeToFit during screen setup -- BEFORE the front end's
               global font table answers WM_GETGLOBALFONT, which at that moment returns NULL for
               every index (measured). So the game sized the box against the fallback 14px font and
               got 105x100, while every later paint runs with the real font and lays the same seven
               rows out 292 wide and 305 tall. OnDraw centres the rows on the rect's CENTRE and
               simply lets them overflow, so the visible menu is symmetric about a box under a
               third its size: the outer two thirds of every label was dead to the mouse, which is
               the half of PO-67 that survived the S317 origin fix.
               Re-run the control's OWN ResizeToFit here, where the font IS correct, and keep only
               the WIDTH it computes -- position is restored untouched, because ResizeToFit
               recomputes topleft from a GetClientRect the compat answers 0 for and would move the
               menu to (0,0). Height already comes from GetListHeight above. Once per control.
               MA_NO_LBREFIT=1 reverts. */
            if (h.drawW < 0 && !getenv("MA_NO_LBREFIT")) {
                int kx=c->m_maX, ky=c->m_maY, kw=c->m_maW, kh=c->m_maH;
                c->ResizeToFit();
                h.drawW = (c->m_maW > kw) ? c->m_maW : kw;
                c->m_maX=kx; c->m_maY=ky; c->m_maW=kw; c->m_maH=kh;
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr,"[lbrefit] id=%d rect w=%d -> painted w=%d (h=%d)\n",
                            h.id, kw, h.drawW, h.drawH);
            }
            CDC dc; dc.m_hDC = (HDC)screenHdc;
            int sx = 0, sy = 0;
            ma_gdi_set_viewport_org(screenHdc, ox, oy, &sx, &sy);
            CRect bounds(0, 0, w, hh);
            c->OnDraw(&dc, bounds, bounds);
            ma_gdi_set_viewport_org(screenHdc, sx, sy, 0, 0);
            /* S140: NOT here. Drawing the listbox scrollbars on the FRONT-END path put a
               vertical and a horizontal bar across the title screen's menu -- parity caught it
               (title, 2839px, bbox 530,210-635,310) and the gold has never had one there. The
               menu's content is measured as overflowing its box when it plainly fits, so the
               port's text metrics disagree with the original's by enough to trip
               UpdateScrollBar's `height > rcBounds.bottom-rcBounds.top` test. That is worth
               chasing, but it is a different defect from "campaign dialogs have no scrollbars",
               and the front-end screens are parity-locked and known good. The OOB/toolbar path
               below draws them, which is where they were missing. */
        }
    }
    /* F2: draw the open dropdown last so it sits on top of every other control. If the open
       combo wasn't drawn this frame (hidden / panel destroyed), close it. */
    if (g_dd_client) {
        if (dd_ctrl) {
            int rowh = 0;
            ma_combo_dropdown_draw(dd_ctrl, screenHdc, g_dd_ox, g_dd_oy, g_dd_w, g_dd_boxh, g_dd_hover, &rowh);
            g_dd_rowh = rowh;
            g_dd_count = ma_combo_itemcount(dd_ctrl);
        } else {
            g_dd_client = 0; g_dd_hover = -1;
        }
    }
}

/* Parent-scoped toolbar draw (BoB S88-92 recipe). Composites ONLY the hosted controls whose
   parent == `dialog`, at `dialog`'s screen origin (ox,oy) + each control's template-relative
   pos. Used for the campaign-map CRToolBars (their parent CDialog is created hidden, and the
   global ma_ole_draw_all would either skip them or, if the parent is forced visible, mix in
   the previous screen's still-registered controls -> stale bleed). */
/* S108 (PO-11): how many controls a given parent actually has hosted. The PO reports "many widgets
   are missing" on the campaign map, and the first question for each one is whether the port hosts
   its controls at all or merely fails to draw them -- two different fixes. Cheap, read-only. */
extern "C" int ma_ole_count_hosted(void* dialog) {
    std::map<void*, Hosted>& m = hosted();
    int n = 0;
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it)
        if (it->second.ctrl && it->second.parent == dialog) n++;
    return n;
}

/* S163: draw the open dropdown on top of everything. The front-end pass does this inline at the
   end of ma_ole_draw_all; the campaign map paints through ma_ole_draw_toolbar per dialog instead,
   so the OOB walk calls this ONCE after the whole tree -- a dropdown drawn per-dialog would be
   painted over by the next dialog in the walk, which is the one thing a dropdown must never be. */
extern "C" void ma_ole_draw_dropdown(void* screenHdc) {
    if (!g_dd_client) return;
    void* ctrl = combo_ctrl_of(g_dd_client);
    /* the open combo was not drawn this frame (hidden / dialog destroyed): close the list rather
       than leave a stale one hit-testable over the map. */
    if (!ctrl) { g_dd_client = 0; g_dd_hover = -1; return; }
    int rowh = 0;
    ma_combo_dropdown_draw(ctrl, screenHdc, g_dd_ox, g_dd_oy, g_dd_w, g_dd_boxh, g_dd_hover, &rowh);
    g_dd_rowh = rowh;
    g_dd_count = ma_combo_itemcount(ctrl);
}
/* S163: is a dropdown open, and did this click land in it? The OOB click walk asks BEFORE it
   offers the point to any control, for the same reason the paint draws it last: an open list
   covers whatever is under it. */
extern "C" int ma_ole_dropdown_take(int sx, int sy) {
    if (!g_dd_client) return 0;
    /* Same arithmetic and the same event as the front-end path in ma_ole_click -- deliberately
       not a second implementation of "which row is under the cursor": the two would drift. */
    void* ctrl = combo_ctrl_of(g_dd_client);
    if (ctrl && g_dd_rowh > 0) {
        int rx = g_dd_ox, ry = g_dd_oy + g_dd_boxh, rw = g_dd_w;
        int rb = ry + g_dd_count * g_dd_rowh;
        if (sx >= rx && sx < rx + rw && sy >= ry && sy < rb) {
            int row = (sy - ry) / g_dd_rowh;
            if (getenv("MA_TRACE_CLICK")) fprintf(stderr,"[ddclick] dropdown row %d of %d\n", row, g_dd_count);
            ma_combo_select(ctrl, row);
            std::map<void*, Hosted>& m = hosted();
            std::map<void*, Hosted>::iterator dit = m.find(g_dd_client);
            if (dit != m.end() && dit->second.parent && dit->second.id) {
                CWnd* dp = (CWnd*)dit->second.parent;
                ma_evt_fire(dp, &typeid(*dp), dit->second.id, 1 /*TextChanged*/);
            }
        } else if (getenv("MA_TRACE_CLICK")) {
            fprintf(stderr,"[ddclick] dismissed at (%d,%d)\n", sx, sy);
        }
    }
    /* a click anywhere else closes the list and is CONSUMED: Windows does not pass the
       dismissing click through to whatever is behind an open combo. */
    g_dd_client = 0; g_dd_hover = -1;
    return 1;
}

extern "C" void ma_ole_draw_toolbar(void* dialog, void* screenHdc, int ox, int oy) {
    std::map<void*, Hosted>& m = hosted();
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (h.type == CT_TABS && getenv("MA_TRACE_TABS")) { static int nt=0; if (nt++<12) {
            CWnd* cw = (CWnd*)it->first;
            fprintf(stderr,"[tabs.draw_tb] dialog=%p h.parent=%p match=%d vis=%d rect(%d,%d %dx%d)\n",
                dialog, h.parent, (int)(h.parent==dialog), cw?cw->m_maVisible:-1,
                cw?cw->m_maX:-1, cw?cw->m_maY:-1, cw?cw->m_maW:-1, cw?cw->m_maH:-1); } }
        if (!h.ctrl || h.parent != dialog) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        if (!clientWnd || !clientWnd->m_maVisible) continue;
        h.drawOx = ox; h.drawOy = oy;      /* S84: remember where paint actually put it */
        int cx = ox + clientWnd->m_maX, cy = oy + clientWnd->m_maY;
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        if (h.type == CT_STATIC)      ma_static_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
        else if (h.type == CT_EDIT)   ma_edit_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
        else if (h.type == CT_EDTBT)  ma_edtbt_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
        else if (h.type == CT_TABS)   ma_tabs_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
        else if (h.type == CT_BUTTON) { ma_button_apply_icon(h.ctrl, h.id); ma_button_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh); }
        else if (h.type == CT_RADIO)  ma_radio_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
        else if (h.type == CT_SCROLL) ma_scroll_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
        else if (h.type == CT_SPIN)   ma_spin_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
        else if (h.type == CT_COMBO)  { ma_combo_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
            /* S163: the OOB/toolbar draw must record the open combo's box the same way the
               front-end pass does, or the dropdown has nowhere to draw itself. */
            if (it->first == g_dd_client) { g_dd_ox = cx; g_dd_oy = cy; g_dd_w = w; g_dd_boxh = hh; } }
        else if (h.type == CT_LISTBOX) {
            /* S70 (parity #15, I4): the OOB dialog draw path had no listbox case, so the Player
               Log Career tab's Sorties/Combats/Kills/Losses table (an RListBox, IDC_RLISTBOXCTRL1
               in IDD_CAREER) never rendered even though CCareer::OnInitDialog populates it. Mirror
               ma_ole_draw's OnDraw-at-viewport-origin, but at the toolbar-offset rect (cx,cy). */
            CRListBoxCtrl* c = (CRListBoxCtrl*)h.ctrl;
            c->m_pParent = (CWnd*)dialog;
            c->m_maX = clientWnd->m_maX; c->m_maY = clientWnd->m_maY;
            c->m_maW = w; c->m_maH = hh;
            CDC dc; dc.m_hDC = (HDC)screenHdc;
            int lox = 0, loy = 0;
            ma_gdi_set_viewport_org(screenHdc, cx, cy, &lox, &loy);
            CRect lbounds(0, 0, w, hh);
            /* S71 (parity #15 chrome): OOB listboxes (the Player Log tables) composite over the
               dialog's already-painted background, so their opaque black fill hid gold's
               translucent look. This flag tells CRListBoxCtrl::OnDraw to skip the box fill on
               the OOB path ONLY — the front-end menu/prefs listboxes (drawn via ma_ole_draw_all,
               which never sets this) keep the opaque box they rely on (S70 regression when it
               was skipped globally). */
            /* S155 (PO-43): CLIP the listbox to its own rect. Windows clips a control's
               drawing to the control's window; this path never did, so a list with more rows
               than fit drew ALL of them -- straight past the bottom of the dialog, over the
               dialog's own buttons and on down the screen. The PO saw it on Intelligence:
               "buttons are not at the right place, not drawn right either" -- the buttons are
               where they belong, and the list is painted on top of them. The scrollbar (S140)
               ends at the dialog's true bottom, which is what shows the list is the thing
               overflowing. Same rule S67 applied to the button path, for the same reason. */
            ma_oob_lb_draw = 1;
            c->OnDraw(&dc, lbounds, lbounds);
            ma_oob_lb_draw = 0;
            ma_gdi_set_viewport_org(screenHdc, lox, loy, 0, 0);
            draw_listbox_scrollbars(it->first, h.ctrl, screenHdc, cx, cy);   /* S140 */
        }
    }
}

/* Hit-test a screen click (sx,sy) against `dialog`'s toolbar buttons at the SAME origin
   (ox,oy) ma_ole_draw_toolbar drew them, and fire the button's Clicked event to the dialog's
   ON_EVENT handler (OnClickedBases/Frag2/...). Returns 1 if a button was hit. */
/* S94 (PO-1): the union extent of a dialog's hosted controls, in template coordinates. Used to
   place the CSystemBox at the canvas's upper RIGHT without hardcoding its width -- the same rule
   the click recipes follow: derive from the controls' own metrics, never from a magic pixel. */
extern "C" int ma_ole_dialog_extent(void* dialog, int* outw, int* outh) {
    std::map<void*, Hosted>& m = hosted();
    int maxx = 0, maxy = 0, found = 0, visible = 0;
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (!h.ctrl || h.parent != dialog) continue;
        CWnd* cw = (CWnd*)it->first;
        if (!cw || cw->m_maW <= 0 || cw->m_maH <= 0) continue;
        if (cw->m_maX + cw->m_maW > maxx) maxx = cw->m_maX + cw->m_maW;
        if (cw->m_maY + cw->m_maH > maxy) maxy = cw->m_maY + cw->m_maH;
        found++;
        if (cw->m_maVisible) visible++;
    }
    if (outw) *outw = maxx;
    if (outh) *outh = maxy;
    if (getenv("MA_TRACE_SYSBOX")) { static int n=0; if (n++<3)
        fprintf(stderr,"[sysbox] controls=%d visible=%d extent=%dx%d\n", found, visible, maxx, maxy); }
    return found;
}


/* S140: route a click to a LISTBOX's scrollbars. They are children of the listbox, so the
   dialog's click walk never sees them -- the same gap as the draw walk. On a hit, run the
   control's own arithmetic and then fire the Scroll event the LISTBOX sinks:
       ON_EVENT(CRListBoxCtrl, 1000, 1 Scroll, OnScrollVert, VTS_I4)
       ON_EVENT(CRListBoxCtrl, 1001, 1 Scroll, OnScrollHorz, VTS_I4)
   The control's own DoFireScroll cannot deliver it: COleControl::FireEventV is a no-op here, so
   the host fires it, exactly as it does for every other hosted control's events. */
static int click_listbox_scrollbars(void* dialog, int ox, int oy, int sx, int sy) {
    std::map<void*, Hosted>& m = hosted();
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& lh = it->second;
        if (lh.type != CT_LISTBOX || !lh.ctrl || lh.parent != dialog) continue;
        CWnd* lcw = (CWnd*)it->first;
        if (!lcw || !lcw->m_maVisible) continue;
        int lx = ox + lcw->m_maX, ly = oy + lcw->m_maY;
        for (std::map<void*, Hosted>::iterator jt = m.begin(); jt != m.end(); ++jt) {
            Hosted& sh = jt->second;
            if (sh.type != CT_SCROLL || !sh.ctrl) continue;
            if (sh.parent != it->first && sh.parent != lh.ctrl) continue;
            CWnd* scw = (CWnd*)jt->first;
            if (!scw || !scw->m_maVisible || scw->m_maW <= 0 || scw->m_maH <= 0) continue;
            int bx = lx + scw->m_maX, by = ly + scw->m_maY;
            if (!(sx >= bx && sx < bx + scw->m_maW && sy >= by && sy < by + scw->m_maH)) continue;
            ma_scroll_click(sh.ctrl, lh.ctrl, sx - bx, sy - by);
            int horz = ma_scroll_is_horz(sh.ctrl);
            CWnd* lctrl = (CWnd*)lh.ctrl;
            ma_evtA0 = ma_scroll_pos(sh.ctrl); ma_evtA1 = 0;
            ma_evt_fire(lctrl, &typeid(*lctrl), horz ? 1001 : 1000, 1 /*Scroll*/);
            return 1;
        }
    }
    return 0;
}

extern "C" int ma_ole_toolbar_click(void* dialog, int ox, int oy, int sx, int sy) {
    /* S140: the listbox scrollbars sit ON TOP of the rows, so they get first refusal. */
    if (click_listbox_scrollbars(dialog, ox, oy, sx, sy)) return 1;
    std::map<void*, Hosted>& m = hosted();
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (!h.ctrl || h.parent != dialog) continue;
        /* S163: CT_COMBO joins the list. Combos inside an OOB dialog were DRAWN and inert --
           the same shape as S87 (listbox rows) and S140 (scroll bars), one control type later,
           and the widest one yet: the Wonju walkthrough's TASKS dialog alone drives five of them
           (Squadron / Attack Method / Attack Pattern / Group Formation / Escort Position), and
           the dossier's Damage tab needs one to reach its element list at all. */
        /* S200 (PO: "ins wave can't edit 8:30" -- reported THREE times).
           CT_EDIT joins the allowlist. A hosted EDIT inside an OOB dialog was skipped here
           outright, so a click on it was never dispatched and could never focus it -- and the
           keyboard in this port goes only to the control ma_ole_set_focus last named. The Ins Wave
           Time Over Target field is exactly that: an edit in an OOB dialog.
           This is the failure mode an allowlist has by construction -- it does not error, it does
           not warn, it silently omits, and the symptom is "that control does nothing". S163 added
           CT_COMBO for the same reason ("drawn and inert"), S140 the scroll bars, S87 the listbox
           rows. This is the fifth control type to be found missing from this one line, so the list
           itself is the recurring defect, not any of its entries. */
        if (h.type != CT_BUTTON && h.type != CT_TABS && h.type != CT_LISTBOX && h.type != CT_RADIO &&
            h.type != CT_SCROLL && h.type != CT_COMBO && h.type != CT_SPIN && h.type != CT_EDTBT &&
            h.type != CT_EDIT) continue;
        if (h.type == CT_BUTTON && !h.id) continue;        /* buttons route by id; tabs don't */
        /* S186 (PO-56): a DISABLED button swallows the click and fires nothing -- what Windows
           does. The router never checked, so greyed-out buttons dispatched to their handlers.
           Swallow rather than fall through: the click must not reach the map behind the
           dialog either. Checked AFTER the rect test below would be wrong (we must own the
           click), so the hit test happens first and this fires just before dispatch. */
        if (h.type == CT_BUTTON && !h.id) continue;        /* buttons route by id; tabs don't */
        if (h.type == CT_LISTBOX && !h.id) continue;       /* need an id to route Select */
        CWnd* clientWnd = (CWnd*)it->first;
        if (!clientWnd || !clientWnd->m_maVisible) continue;
        int cx = ox + clientWnd->m_maX, cy = oy + clientWnd->m_maY;
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        if (!(sx >= cx && sx < cx + w && sy >= cy && sy < cy + hh)) continue;
        /* S200: an EDIT takes the click by TAKING THE KEYBOARD. On Windows clicking an edit
           focuses it; here the keyboard goes only to whatever ma_ole_set_focus last named, and its
           only other caller is the game's own CWnd::SetFocus() -- which CCareer's name dialog
           calls (so typing the player name always worked) and CWaveInsert::OnInitDialog does not.
           Swallow the click rather than fall through: the point is inside a dialog that is painted
           over the map, and letting it reach the map behind is the S164 bug. */
        if (h.type == CT_EDIT) {
            ma_ole_set_focus(it->first);
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[tbclick] edit id=%d takes keyboard focus\n", h.id);
            return 1;
        }
        /* S82: a TAB BAR takes the click itself — the control's own rect list decides which tab,
           and its own SelectTab performs the switch (this is what MA_OOB_PLAYERLOG_TAB used to
           fake). If the point is inside the bar but on no tab, swallow it rather than let it
           fall through to whatever is behind the dialog. */
        /* S87: a LISTBOX inside an OOB dialog takes the click too. Until now this loop handled
           buttons and tabs only, so every ROW in Bases / Squads / D.I.S. / Intelligence was inert:
           the dialogs opened and listed real data that could not be selected. Drive the control's
           genuine OnLButtonDown/Up (MaMouse) so ITS OWN logic picks the row and column, then fire
           Select with BOTH args — the same rule that kept MA clear of BoB's hardcoded-column bug
           (their §8u). Note the front-end path (ma_ole_listbox_click) cannot serve these: it
           assumes absolutely-positioned listboxes, whereas an OOB dialog's are drawn at the
           walk's (ox,oy). */
        if (h.type == CT_LISTBOX) {
            CRListBoxCtrl* lc = (CRListBoxCtrl*)h.ctrl;
            long row = 0, col = 0;
            CWnd* par = (CWnd*)h.parent;
            lc->m_pParent = par;
            lc->m_maX = clientWnd->m_maX; lc->m_maY = clientWnd->m_maY;
            lc->m_maW = w; lc->m_maH = hh;
            lc->MaMouse(sx - cx, sy - cy, &row, &col);
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[tbclick] listbox id=%d local=(%d,%d) -> row=%ld col=%ld on %s\n",
                        h.id, sx-cx, sy-cy, row, col, par ? typeid(*par).name() : "(none)");
            if (par) {
                ma_evtA0 = row; ma_evtA1 = col;
                ma_evt_fire(par, &typeid(*par), h.id, 1 /*Select*/);
            }
            return 1;   /* the row took the click either way — don't fall through to the map */
        }
        if (h.type == CT_TABS) {
            if (ma_tabs_click(h.ctrl, sx - cx, sy - cy)) return 1;
            continue;
        }
        /* S163: open this combo's dropdown, exactly as the front-end path does. The box rect is
           recorded by the NEXT draw (ma_ole_draw_toolbar above), not computed here -- store what
           paint did, never re-derive it (S84). A combo with <=1 item has nothing to drop, so it
           keeps the old cycle behaviour. */
        if (h.type == CT_COMBO) {
            if (!h.id) continue;                        /* need an id to route TextChanged */
            if (ma_combo_itemcount(h.ctrl) > 1) {
                g_dd_client = it->first;
                g_dd_hover  = ma_combo_curindex(h.ctrl);
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[tbclick] combo id=%d open dropdown (%d items)\n",
                            h.id, ma_combo_itemcount(h.ctrl));
            } else {
                ma_combo_click(h.ctrl);
                CWnd* par = (CWnd*)h.parent;
                if (par) ma_evt_fire(par, &typeid(*par), h.id, 1 /*TextChanged*/);
            }
            return 1;
        }
        /* S136 (PO-28): a RADIO GROUP takes the click and fires Selected(index), which is what
           drives the D.I.S. dialog's Target/General and Latest/Priority intelligence filters
           (CDIS::OnSelectedRradioIntelltype/Intelltime). The hit-test uses the geometry the
           last PAINT recorded, so the two walks cannot drift apart. */
        /* S170: a SPIN BUTTON takes the click and runs its own arrow-strip test and up/down
           split -- both of which need BOTH local coordinates (the arrows are the right ~15px;
           up vs down is decided by y against the control's mid-height). It reports whether the
           value actually moved, so a click on a spinner already at its limit does not announce a
           change that did not happen. */
        if (h.type == CT_SPIN) {
            if (ma_spin_click(h.ctrl, h.parent, sx - cx, sy - cy)) {
                CWnd* par = (CWnd*)h.parent;
                if (par && h.id) {
                    /* ChooseSquad::OnTextChangedRspinbutctrl1 is declared VTS_BSTR, so the
                       thunk that matches it takes LPCSTR and reads ma_evtP -- NULL it rather
                       than leave the previous event's pointer for it to be handed. The handler
                       ignores the text and re-reads GetIndex() itself, which is why the index
                       still goes in A0 for any int-signature handler on another dialog. */
                    ma_evtA0 = ma_spin_index(h.ctrl); ma_evtA1 = 0; ma_evtP = 0;
                    ma_evt_fire(par, &typeid(*par), h.id, 1 /*Clicked/changed*/);
                }
            }
            return 1;   /* the spinner owns its rect either way -- do not fall through to the map */
        }
        /* S170: an EDIT-BUTTON (RedtBt) takes the click too. CREdtBtCtrl::OnLButtonUp fires
           Clicked for any press-and-release that did not become a drag -- the whole control
           is the button, there is no sub-rect to hit-test. CT_EDTBT was drawn and inert until
           now, and that is what actually blocked EPIC K step 8: the TASKS dialog's duty field
           (IDC_ACTYPE, "F84 (2)") is an RedtBt and it is the ONLY door to the ChooseSquad
           dialog that owns the Flights spin-box. Hosting the spin without this reaches
           nothing. */
        if (h.type == CT_EDTBT) {
            CWnd* par = (CWnd*)h.parent;
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[tbclick] edtbt id=%d local=(%d,%d) of %dx%d -> Clicked on %s\n",
                        h.id, sx-cx, sy-cy, w, hh, par ? typeid(*par).name() : "(none)");
            if (par && h.id) { ma_evtA0 = 0; ma_evtA1 = 0; ma_evtP = 0;
                               ma_evt_fire(par, &typeid(*par), h.id, 1 /*Clicked*/); }
            return 1;
        }
        /* S140: a SCROLL BAR takes the click and runs its own arrow/page/thumb arithmetic. */
        if (h.type == CT_SCROLL) {
            if (ma_scroll_click(h.ctrl, h.parent, sx - cx, sy - cy)) return 1;
            continue;
        }
        if (h.type == CT_RADIO) {
            int sel = -1;
            if (ma_radio_click(h.ctrl, sx - cx, sy - cy, &sel)) {
                CWnd* par = (CWnd*)h.parent;
                if (par && h.id) { ma_evtA0 = sel; ma_evtA1 = 0;
                                   ma_evt_fire(par, &typeid(*par), h.id, 1 /*Selected*/); }
                return 1;
            }
            continue;
        }
        /* S84: Authorise (2023) and Directives (2074) are NO LONGER DEFERRED — both blockers are
           fixed, and clicking them opens the real dialogs. History, because the recorded cause was
           wrong twice and that is worth remembering:
             - the note here blamed "CComit_e -> DirControl::AllocateAc". A symbolized backtrace
               named CSupply::OnInitDialog -> SortIntell -> Sort* -> Add*Mission, and the cause was
               a HALF-APPLIED for-scope hoist whose loop variable shadowed the hoisted one, so the
               table writes indexed uninitialised stack (S83 fixed one, S84 the other four — three
               in CSupply, five in DirControl, i.e. the original note had the right class for the
               *other* half of the bug).
             - underneath that sat a fatal duplicate open of FIL_ICON_MISSIONRESULTS (0x6a78):
               RDialog::OnGetFile holds its block PER DIALOG, so the map toolbar and the dialog's
               own identically-arted button each opened it. S84 serves the already-open block
               instead (fileman::MA_GetOpenFileData).
           MA_OOB_DEFER_DIALOGS=1 restores the old defer if either ever regresses. */
        if (getenv("MA_OOB_DEFER_DIALOGS") && (h.id == 2023 || h.id == 2074)) {
            /* S83: MA_OOB_NO_DEFER=1 lifts the guard so the crash can be reproduced/diagnosed
               deliberately. Since S82 routes real clicks, these two are user-reachable toolbar
               buttons that silently do nothing — the defer is now a visible gap, not a detail. */
            if (getenv("MA_TRACE_CLICK")) fprintf(stderr,"[tbclick] id=%d OOB-dialog deferred (deeper OnInitDialog crash; MA_OOB_NO_DEFER=1 to reproduce)\n", h.id);
            return 1;
        }
        if (h.type == CT_BUTTON && ma_button_disabled(h.ctrl)) {
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[tbclick] id=%d is DISABLED -- click swallowed, no event fired\n", h.id);
            return 1;
        }
        CWnd* parent = (CWnd*)dialog;
        const std::type_info* ti = &typeid(*parent);
        /* S82: a TITLE BAR is one of these buttons with the tick/close/help flags set, and only
           the genuine control knows which glyph band the point falls in. Ask it, and fire what
           its OnLButtonUp would have fired (OK / Cancel / Clicked). Buttons WITHOUT those flags
           — every toolbar button, i.e. every click path that already worked — skip this entirely
           and still fire plain Clicked, so their behaviour is unchanged. */
        int disp = ma_button_title_hit(h.ctrl, sx - cx, sy - cy, w, hh);
        if (disp >= 0) {
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[tbclick] id=%d TITLE local=(%d,%d) of %dx%d -> dispid %d (%s) on %s\n",
                        h.id, sx-cx, sy-cy, w, hh, disp,
                        disp==3?"OK":disp==2?"Cancel":disp==0?"Help":"Clicked", ti->name());
            if (disp == 0) {
                /* S98 (PO-4): route it. On Windows the "?" band sends WM_COMMANDHELP up the
                   window chain; the port has no message queue, so send it to the hosting dialog
                   directly and let the engine's own OnCommandHelp chain decide what to show. */
                LRESULT hr = parent->SendMessage(WM_COMMANDHELP, 0, 0);
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr,"[tbclick] id=%d HELP -> WM_COMMANDHELP on %s returned %ld\n",
                            h.id, ti->name(), (long)hr);
                return 1;
            }
        } else {
            disp = 1;                                      /* ordinary button: plain Clicked */
            /* S137 (PO-30): TOGGLE the button before firing, which is what the genuine control
               does -- CRButtonCtrl::OnLButtonUp is literally `m_bPressed=!m_bPressed;` and then
               fires. The port fired without toggling, so every handler that ASKS the button what
               state it is now in read a stale FALSE. CMapFilters::OnClickedFilter is exactly
               such a handler:
                   CRButton* but=(CRButton*)GetDlgItem(id);
                   bool pressed=(but->GetPressed()==1);
                   if (pressed) Save_Data.mapfilters |= ...; else Save_Data.mapfilters %= ...;
               so every filter click read "not pressed" and asked to CLEAR a filter that was
               already clear -- the map never changed, which is what the PO saw. It also gives
               the buttons their pressed ARTWORK, which the control picks by the same flag. */
            ma_button_toggle_pressed(h.ctrl);
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[tbclick] id=%d rect=(%d,%d,%d,%d) pressed=%d -> fire\n",
                        h.id, cx,cy,w,hh, ma_button_get_pressed(h.ctrl));
        }
        if (!ma_evt_fire(parent, ti, h.id, disp) && (disp == 3 || disp == 2)) {
            /* Nothing registered an OK handler for this title bar (the common case: the dialog
               overrides the virtual CDialog::OnOK instead of registering an ON_EVENT). Call the
               owning dialog's OnOK directly — VIRTUALLY, so the DERIVED override runs.
               BoB S145's trap is the opposite mistake: routing this to the panel WRAPPER's
               RDialog::OnOK, which EndDialog()s and silently skips the derived logic. `parent`
               here is the node that HOSTS the title bar, not the top-level panel. */
            /* S83: the engine registers these on the BASE class —
                 ON_EVENT(RDialog, IDJ_TITLE, 2 Cancel, OnCancel)
                 ON_EVENT(RDialog, IDJ_TITLE, 3 OK,     OnOK)      (RDIALOG.CPP:1176-1177)
               — and this sink matches the RUNTIME type exactly, so for a CPlyr_log (or any other
               derived dialog) BOTH entries are dead. That is BoB's §8z finding on MA's side. The
               virtual call is the right resolution rather than making the sink walk base classes:
               it reaches the DERIVED override, which is what the base registration was reaching on
               Windows anyway. S82 covered OK; Cancel (the ✕) was equally dead. */
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[tbclick] no %s handler registered -> virtual %s on %s\n",
                        disp==3?"OK":"Cancel", disp==3?"OnOK":"OnCancel", ti->name());
            if (disp == 3) ((CDialog*)parent)->OnOK();
            else           ((CDialog*)parent)->OnCancel();
        }
        return 1;
    }
    return 0;
}

/* Hit-test a screen click against hosted BUTTONS; fire the button's "Clicked" event to its
   parent dialog's eventsink handler (matched by control-id + the dialog's runtime type). */
int ma_ole_click(int sx, int sy) {
    std::map<void*, Hosted>& m = hosted();
    /* F2: a dropdown is open — this click either picks a row or dismisses the list (it does
       NOT fall through to the controls behind it). */
    if (g_dd_client) {
        void* ctrl = combo_ctrl_of(g_dd_client);
        if (ctrl && g_dd_rowh > 0) {
            int rx = g_dd_ox, ry = g_dd_oy + g_dd_boxh, rw = g_dd_w;
            int rb = ry + g_dd_count * g_dd_rowh;
            if (sx >= rx && sx < rx + rw && sy >= ry && sy < rb) {
                int row = (sy - ry) / g_dd_rowh;
                if (getenv("MA_TRACE_CLICK")) fprintf(stderr,"[click] dropdown row %d\n", row);
                ma_combo_select(ctrl, row);
                /* Route the change to the dialog's handler. The control's own
                   FireTextChanged -> COleControl::FireEvent goes through the (stubbed)
                   connection point and never reaches the dialog, so -- like the button
                   path below -- fire the TextChanged event (dispid 1) explicitly so e.g.
                   CSQuick1::OnTextChangedMisslists runs and reads the new combo index. */
                std::map<void*, Hosted>::iterator dit = m.find(g_dd_client);
                if (dit != m.end() && dit->second.parent && dit->second.id) {
                    CWnd* dp = (CWnd*)dit->second.parent;
                    ma_evt_fire(dp, &typeid(*dp), dit->second.id, 1 /*TextChanged*/);
                }
            }
        }
        g_dd_client = 0; g_dd_hover = -1;
        return 1;
    }
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (!h.ctrl) continue;
        /* S210 (PO-65: "no text appears when I type a replay name"). CT_EDIT WAS MISSING HERE, and
           that made S198's edit-focus code below UNREACHABLE -- it sits ~17 lines further down and
           the loop `continue`s past every edit before reaching it. So the front end's edit boxes
           could never take focus, however you clicked them: the replay Save screen's name field,
           and every other CT_EDIT on a full-screen panel.
           This is the S164 family for the fourth time -- a control type absent from a click walk's
           TYPE FILTER is drawn, looks alive, and is inert -- and the third dispatcher to need the
           same repair (S200 did the OOB allowlist, S198 the [tbclick] one, this is the front end).
           The tell was in S198's own comment: it moved this code here after finding the trace
           "firing ZERO times" elsewhere, and it fires zero times here too, one layer up.
           MA_NO_EDIT_CLICK=1 restores the old filter. */
        if (h.type != CT_BUTTON && h.type != CT_COMBO && h.type != CT_EDTBT
            && !(h.type == CT_EDIT && !getenv("MA_NO_EDIT_CLICK"))) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        CWnd* parent = (CWnd*)h.parent;
        if (!clientWnd || !clientWnd->m_maVisible || (parent && !parent->m_maVisible)) continue;
        /* S57: controls filtered out of the draw (not in the installed template) don't click either */
        if (h.relative && h.parent && h.id > 0 &&
            ma_dlg_in_template(h.parent, h.id) == 0) continue;
        /* S59: nor do Windows-clipped controls parked outside the dialog rect */
        if (h.relative && h.parent && h.id > 0 &&
            ma_dlg_never_visible(h.parent, h.id) == 1) continue;
        int ox, oy; ma_ole_origin(h, clientWnd, parent, &ox, &oy);   /* S317 */
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        if (getenv("MA_TRACE_CLICK") && h.type==CT_COMBO) fprintf(stderr,"[click] combo box=(%d,%d,%d,%d) vs (%d,%d) %s\n", ox,oy,w,hh,sx,sy, (sx>=ox&&sx<ox+w&&sy>=oy&&sy<oy+hh)?"HIT":"miss");
        if (getenv("MA_TRACE_CLICK") && h.type==CT_BUTTON) fprintf(stderr,"[click] button id=%d rect=(%d,%d,%d,%d) centre=(%d,%d)\n", h.id, ox,oy,w,hh, ox+w/2, oy+hh/2);
        if (!(sx >= ox && sx < ox + w && sy >= oy && sy < oy + hh)) continue;
        /* S198 (PO: "ins wave worked, but the 8:30 was not editable" -- twice).
           CLICKING AN EDIT MUST FOCUS IT. The keyboard goes to whichever hosted control
           ma_ole_set_focus last named, and its ONLY caller is the game's own CWnd::SetFocus().
           CCareer's name dialog calls it, which is why typing the player name always worked;
           CWaveInsert::OnInitDialog does not, so its Time Over Target field could never receive a
           keystroke however you clicked it.
           NB this is the SECOND place I put this. The first went into the [tbclick] dispatcher --
           a different function -- and the PO's log showed the trace firing ZERO times, which is
           the only reason I noticed. Note also that CT_EDIT has no case of its own below: an edit
           click falls through to the CT_BUTTON/CT_EDTBT "fire Clicked" arm, which an edit has no
           handler for. Focus first, then let it fall through exactly as before. */
        if (h.type == CT_EDIT) {
            ma_ole_set_focus(it->first);
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[click] edit id=%d takes keyboard focus\n", h.id);
        }
        if (h.type == CT_COMBO) {
            /* F2: open the dropdown list instead of cycling. <=1-item combos have nothing to
               drop down, so keep the old cycle behaviour as a fallback. */
            if (ma_combo_itemcount(h.ctrl) > 1) {
                g_dd_client = it->first;
                g_dd_hover  = ma_combo_curindex(h.ctrl);
                if (getenv("MA_TRACE_CLICK")) fprintf(stderr,"[click] combo open dropdown (%d items)\n", ma_combo_itemcount(h.ctrl));
            } else {
                ma_combo_click(h.ctrl);
            }
            /* same as the dropdown path: fire TextChanged to the dialog handler (the
               control's FireEvent connection-point path is stubbed). */
            if (parent && h.id) {
                const std::type_info* ti = &typeid(*parent);
                ma_evt_fire(parent, ti, h.id, 1 /*TextChanged*/);
            }
            return 1;
        }
        if (parent && h.id) {                       /* CT_BUTTON / CT_EDTBT (both fire Clicked, dispid 1) */
            const std::type_info* ti = &typeid(*parent);
            if (ma_evt_fire(parent, ti, h.id, 1 /*Clicked*/)) return 1;
        }
    }
    return 0;
}

/* Hit-test a screen click against every hosted, template-placed listbox that has a dialog id
   (i.e. a DDX_Control'd child-dialog listbox such as CLoad's IDC_RLISTBOXFILE — NOT the
   FullPanelDial menu listbox, whose id is 0 and which the caller handles separately). On a
   hit, resolve the row/col via MaMouse and fire the owning dialog's Select event (dispid 1,
   args via ma_evtA0/A1) so e.g. CLoad::OnSelectRlistboxfile runs (selects the save file). */
/* S63 — resolve a hosted menu listbox ROW to a canvas point, for font-independent
 * test recipes.
 *
 * Why this exists: every scripted capture/drive recipe (BOB_CLICKSEQ, and asan_all.sh's
 * mode recipes) encoded menu items as fixed pixel coordinates — "the title menu row at
 * y=231". S62 enabling the persisted-property reader changed the menu FontNum, the row
 * pitch went ~16px -> ~28px, and EVERY one of those recipes silently landed on the wrong
 * row: the `quickmission` capture came back showing Preferences and the campaign recipe
 * never reached the map. That invalidated the parity captures and the ASan drive recipes
 * together — i.e. the regression gate — which is why S62 had to ship the reader opt-in.
 *
 * Re-deriving the constants for the new pitch would buy exactly one sprint and re-break
 * on the next font, DPI or layout change. Instead resolve the row at click time through
 * the control's OWN GetRowFromY mapping: scan the listbox's height, ask it which row each
 * y belongs to, and return the midpoint of the band that answers `row`. No assumption
 * about row height, no duplicated layout maths — correct by construction under any font.
 *
 * Target selection: the visible hosted listbox with the most rows. The front-end menu is
 * the only populated listbox on the screens the recipes drive; MA_TRACE_CLICK reports the
 * choice so a wrong pick is visible rather than silent. */
extern "C" int ma_ole_menu_row_point(int row, int* outx, int* outy) {
    std::map<void*, Hosted>& m = hosted();
    Hosted* bestH = 0;
    CRListBoxCtrl* best = 0; CWnd* bestWnd = 0; CWnd* bestParent = 0; int bestCount = 0;
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (h.type != CT_LISTBOX || !h.ctrl) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        CWnd* parent = (CWnd*)h.parent;
        if (!clientWnd || !clientWnd->m_maVisible) continue;
        if (parent && !parent->m_maVisible) continue;
        if (clientWnd->m_maW <= 0 || clientWnd->m_maH <= 0) continue;
        CRListBoxCtrl* c = (CRListBoxCtrl*)h.ctrl;
        int n = (int)c->GetCount();
        if (n > bestCount) { bestCount = n; best = c; bestWnd = clientWnd; bestParent = parent; bestH = &h; }
    }
    if (!best || !bestWnd || row < 0 || row >= bestCount) {
        if (getenv("MA_TRACE_CLICK"))
            fprintf(stderr, "[clickrow] row=%d UNRESOLVED (best=%p count=%d)\n", row, (void*)best, bestCount);
        return 0;
    }
    /* GetRowFromY selects its font through GetParent()->SendMessage(WM_GETGLOBALFONT),
       so the parent must be attached exactly as the click path (MaMouse) does — without
       it the text metric, and therefore the row height, is not the one used to draw. */
    best->m_pParent = bestParent;
    best->m_maX = bestWnd->m_maX; best->m_maY = bestWnd->m_maY;
    best->m_maW = bestWnd->m_maW; best->m_maH = bestWnd->m_maH;
    /* Row height from the control's OWN metric. GetListHeight() is
       GetCount()*rowH + shadowOffset, computed from exactly the TEXTMETRIC (plus shadow
       and m_vertSeperation) that GetRowFromY divides by and that OnDraw lays rows out
       with — so it tracks any font change automatically, which is the whole point.
       GetRowFromY itself is NOT usable as the oracle here: it ends with
       `if (row > m_playerList.GetCount()) row = -1`, and the front-end menu leaves
       m_playerList empty, so it answers -1 for every row past the first. */
    long lh = best->GetListHeight();
    if (lh <= 0) {
        if (getenv("MA_TRACE_CLICK"))
            fprintf(stderr, "[clickrow] row=%d UNRESOLVED (GetListHeight=%ld count=%d)\n", row, lh, bestCount);
        return 0;
    }
    int rowH = (int)(lh / bestCount);
    if (rowH <= 0) return 0;
    int first = row * rowH, last = first + rowH - 1;
    /* S317: through the SAME origin every other walk uses. This resolver formed its point from
       the raw m_maX/m_maY, so under a centred panel (g_ma_panel_org != 0) every `rN` recipe
       pointed at the black margin the artwork had moved off. `rN` is the FIRST step of nearly
       every recipe in port/ -- "title -> Campaign" is `30,r3` -- so one wrong point here left the
       whole suite stranded on the title screen: 9 of 10 interaction gates failed maximized with
       `#1055 UNRESOLVED`, which reads like a broken dialog and is really a click that never
       landed. Measured, not reasoned: those 9 are green unmaximized. */
    int rox, roy; ma_ole_origin(*bestH, bestWnd, bestParent, &rox, &roy);
    if (outx) *outx = rox + bestWnd->m_maW / 2;
    if (outy) *outy = roy + (first + last) / 2;
    if (getenv("MA_TRACE_CLICK"))
        fprintf(stderr, "[clickrow] row=%d -> (%d,%d)  [listbox (%d,%d) %dx%d origin=(%d,%d), %d rows, listH=%ld rowH=%d]\n",
                row, outx?*outx:-1, outy?*outy:-1, bestWnd->m_maX, bestWnd->m_maY,
                bestWnd->m_maW, bestWnd->m_maH, rox, roy, bestCount, lh, rowH);
    return 1;
}

/* S63 — resolve a hosted control by its DIALOG CONTROL ID to a canvas point.
 *
 * The companion to ma_ole_menu_row_point for the parts of a recipe that are not menu
 * rows. The Load Game dialog's "Load" button was encoded as the fixed point (68,565);
 * with the persisted-property font it grew and (68,565) landed on "Back" instead — the
 * campaign recipe therefore stopped reaching the map even after the menu rows were fixed.
 * A control id is stable across any font or layout change, so `f,#<id>` recipes cannot
 * drift the way pixel coordinates do.
 *
 * Mirrors ma_ole_draw_all's origin rules: template-positioned (`relative`) controls are
 * offset by their parent's origin, listboxes are absolute. On failure with
 * MA_TRACE_CLICK set it lists the visible candidates, so an unknown id is diagnosable
 * instead of silent. */
/* S85: `parentClass` disambiguates a numeric id. RESOURCE.H reuses ids freely — FIVE symbols are
   2074 (IDC_DIRECTIVES, IDC_AUTHORISE4, IDC_FILTER_RED_TROOP, IDS_PILOTNAMES_74, IDC_DEVDESC) — so
   `#2074` matched whichever hosted control came first in map order (the map-filters toolbar's), and
   firing Clicked at a class with no handler for it is a SILENT NO-OP that reads exactly like "the
   feature is broken". Pass a class name (substring of the RTTI name, e.g. "CMainToolbar") to pick
   the intended host. NULL/empty keeps the old behaviour, but an ambiguous match now WARNS with all
   candidates instead of quietly choosing one. */
/* S173: `@Class#N` selects the Nth INSTANCE of a repeated sub-dialog. The frag screen hosts
   THREE `CFragPilot`s -- one per flight row -- each with the same control ids, so `@CFragPilot`
   is ambiguous with itself in the same way S171 found a reopened dialog to be. The instance
   is carried in the class string rather than a new parameter, so every existing caller and
   recipe form keeps working untouched.
   ORDER IS BY SCREEN POSITION (top-to-bottom, then left-to-right), NOT map order: map order is
   by pointer, i.e. by whatever the allocator did, and "the second flight row" has to mean the
   one the player sees second or the recipe is addressing luck again. */
static int ma_class_instance(const char* pc, char* out, size_t outn)
{
    if (out && outn) out[0] = 0;
    if (!pc || !*pc) return -1;
    const char* h = strchr(pc, '#');
    if (!h || !h[1]) { if (out && outn) { strncpy(out, pc, outn - 1); out[outn - 1] = 0; } return -1; }
    size_t n = (size_t)(h - pc);
    if (out && outn) { if (n > outn - 1) n = outn - 1; memcpy(out, pc, n); out[n] = 0; }
    return atoi(h + 1);
}

extern "C" int ma_ole_control_point_p(int id, int col, const char* parentClass, int* outx, int* outy) {
    std::map<void*, Hosted>& m = hosted();
    char _pcbuf[80];
    int  _wantinst = ma_class_instance(parentClass, _pcbuf, sizeof _pcbuf);
    if (_wantinst >= 0) parentClass = _pcbuf;
    /* Count visible candidates first, so ambiguity is reported rather than silently resolved.
       S171: this used to run ONLY when no @Class was given, on the assumption that a class name
       settles it. It does not. A dialog CLOSED AND REOPENED leaves the class ambiguous with
       ITSELF, and the loop below then takes whichever instance sorts first by pointer -- which
       on the second CFlt_Task was the DEAD one, so `#2149@CFlt_Task` opened the live dropdown
       and `:r1` then addressed a different control and reported "open the dropdown first".
       Count after the SAME filters the resolver uses, including the class. */
    {
        int cand = 0;
        for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
            Hosted& h = it->second; CWnd* cw = (CWnd*)it->first; CWnd* pw = (CWnd*)h.parent;
            if (!h.ctrl || h.id != id || !cw || !cw->m_maVisible) continue;
            if (pw && !pw->m_maVisible) continue;
            if (parentClass && *parentClass) { if (!pw || !strstr(typeid(*pw).name(), parentClass)) continue; }
            if (cw->m_maW > 0 && cw->m_maH > 0) cand++;
        }
        if (cand > 1 && _wantinst < 0) {
            fprintf(stderr, "[clickid] WARNING id=%d is AMBIGUOUS (%d visible hosts%s%s) — the recipe cannot say which:\n",
                    id, cand, (parentClass && *parentClass) ? " matching @" : " — add @Class",
                    (parentClass && *parentClass) ? parentClass : "");
            for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
                Hosted& h = it->second; CWnd* cw = (CWnd*)it->first; CWnd* pw = (CWnd*)h.parent;
                if (!h.ctrl || h.id != id || !cw || !cw->m_maVisible) continue;
                if (pw && !pw->m_maVisible) continue;
                if (parentClass && *parentClass) { if (!pw || !strstr(typeid(*pw).name(), parentClass)) continue; }
                if (cw->m_maW <= 0 || cw->m_maH <= 0) continue;
                fprintf(stderr, "[clickid]   candidate host=%s(%p) type=%d rect(%d,%d %dx%d)\n",
                        pw ? typeid(*pw).name() : "(none)", (void*)pw, h.type, cw->m_maX, cw->m_maY, cw->m_maW, cw->m_maH);
            }
        }
    }
    /* S173: when an instance was named, resolve WHICH candidate first -- by screen position --
       and then let the normal loop run against that one only. Doing it as a pre-pass keeps the
       rest of this function (columns, rows, dropdowns, title bands) completely unchanged. */
    void* _instclient = 0;
    if (_wantinst >= 0) {
        struct Cand { void* c; int y, x; };
        std::vector<Cand> cands;
        for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
            Hosted& h = it->second; CWnd* cw = (CWnd*)it->first; CWnd* pw = (CWnd*)h.parent;
            if (!h.ctrl || h.id != id || !cw || !cw->m_maVisible) continue;
            if (pw && !pw->m_maVisible) continue;
            if (cw->m_maW <= 0 || cw->m_maH <= 0) continue;
            if (parentClass && *parentClass) { if (!pw || !strstr(typeid(*pw).name(), parentClass)) continue; }
            int rel = h.relative && pw && h.type != CT_LISTBOX;
            Cand k; k.c = it->first;
            k.y = (rel ? pw->m_maY : 0) + cw->m_maY;
            k.x = (rel ? pw->m_maX : 0) + cw->m_maX;
            cands.push_back(k);
        }
        for (size_t a = 0; a + 1 < cands.size(); a++)
            for (size_t b = 0; b + 1 < cands.size() - a; b++)
                if (cands[b].y > cands[b+1].y || (cands[b].y == cands[b+1].y && cands[b].x > cands[b+1].x)) {
                    Cand t = cands[b]; cands[b] = cands[b+1]; cands[b+1] = t;
                }
        if (_wantinst >= (int)cands.size()) {
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr, "[clickid] id=%d @%s#%d -- only %d instance(s) on screen\n",
                        id, parentClass ? parentClass : "", _wantinst, (int)cands.size());
            return 0;
        }
        _instclient = cands[_wantinst].c;
        if (getenv("MA_TRACE_CLICK"))
            fprintf(stderr, "[clickid] id=%d @%s#%d of %d -> client=%p at (%d,%d)\n",
                    id, parentClass ? parentClass : "", _wantinst, (int)cands.size(),
                    _instclient, cands[_wantinst].x, cands[_wantinst].y);
    }
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (!h.ctrl || h.id != id) continue;
        if (_instclient && it->first != _instclient) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        CWnd* parent = (CWnd*)h.parent;
        if (!clientWnd || !clientWnd->m_maVisible) continue;
        if (parent && !parent->m_maVisible) continue;
        if (parentClass && *parentClass) {
            /* RTTI names are length-prefixed and mangled ("12CMainToolbar"), so match on substring
               and let the recipe say the plain class name. */
            if (!parent) continue;
            if (!strstr(typeid(*parent).name(), parentClass)) continue;
        }
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        /* S84's arithmetic, now shared with every other walk as ma_ole_origin (S317). The
           toolbar reason it was written for still holds: toolbar-hosted buttons live at the
           offset the map idle passes in (4,26 / 4,52), not at the parent's m_maX/m_maY
           (which are 0), so the old arithmetic put `#ID` recipes ~50px off. */
        int ox, oy; ma_ole_origin(h, clientWnd, parent, &ox, &oy);
        int cx = ox + w / 2;
        /* A horizontal listbox (e.g. the Load Game dialog's "Back Load" bar, id 2063) is
           ONE control whose items are columns, so its centre falls between them. When a
           column is named, find its band through the control's own GetColFromX — same
           technique as the row resolver, and equally font-proof. */
        if (col >= 0 && h.type == CT_LISTBOX) {
            CRListBoxCtrl* c = (CRListBoxCtrl*)h.ctrl;
            c->m_pParent = parent;
            c->m_maX = clientWnd->m_maX; c->m_maY = clientWnd->m_maY;
            c->m_maW = w; c->m_maH = hh;
            int first = -1, last = -1;
            for (int px = 0; px < w; px++) {
                if ((int)c->GetColFromX(px) == col) { if (first < 0) first = px; last = px; }
                else if (first >= 0) break;
            }
            if (first < 0) {
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[clickid] id=%d col=%d not mapped by GetColFromX (w=%d)\n", id, col, w);
                return 0;
            }
            cx = ox + (first + last) / 2;
        }
        int cy = oy + hh / 2;
        /* S170: a SPIN BUTTON has no usable centre -- its arrows live in the right-hand strip
           and up/down splits on mid-height, so `#ID` on its centre is a click the control
           correctly ignores. Recipe form `#ID@Class:0` = UP, `:1` = DOWN (bare `#ID` = UP).
           The point comes from ma_spin_arrow_point, i.e. from the control's own rect and the
           control's own constants -- see the note there. */
        if (h.type == CT_SPIN) {
            int alx = 0, aly = 0;
            if (!ma_spin_arrow_point(h.ctrl, col == 1 ? 1 : 0, &alx, &aly)) {
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[clickid] id=%d spin %dx%d has no arrow strip -- not clicked\n", id, w, hh);
                return 0;
            }
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr, "[clickid] id=%d spin %s local=(%d,%d) of %dx%d\n",
                        id, col == 1 ? "DOWN" : "UP", alx, aly, w, hh);
            cx = ox + alx; cy = oy + aly;
        }
        /* S162: recipe form `#ID@Class:rN` — the Nth ROW of a vertical listbox. The centre of a
           listbox is the middle row, so `#1055@CLoad` on the Authorize chooser was clicking
           "Fighter Bomber Strike" (row 2 of 3) while the walkthrough says explicitly to pick
           "Minimum Strike" and NOT that one. The mission still got created, so the recipe looked
           right and was testing the wrong thing -- S85's failure mode with a different control
           type. Resolved through the control's OWN GetRowFromY, exactly as the column form uses
           GetColFromX, so it survives a font or row-height change. */
        /* S163: on a COMBO, `:rN` addresses row N of its OPEN dropdown -- so a recipe spells the
           user's TWO clicks as two entries ("500,#2398" opens the list, "560,#2398:r0" picks a
           row), rather than one scaffold click that opens and selects in a single step and
           therefore never exercises the path a player takes (the S82 rule about scaffolds that
           stand in for the real route). The geometry comes from what PAINT recorded for the open
           list, never re-derived (S84). */
        if (col <= MA_ROW_SENTINEL && h.type == CT_COMBO) {
            int want = MA_RC_ROW(col);
            if (it->first != g_dd_client || g_dd_rowh <= 0) {
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[clickid] id=%d :r%d needs its dropdown OPEN first "
                                    "(add a plain #%d entry before this one)\n", id, want, id);
                return 0;
            }
            if (want < 0 || want >= g_dd_count) {
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[clickid] id=%d dropdown has %d rows, asked for %d\n", id, g_dd_count, want);
                return 0;
            }
            cx = g_dd_ox + g_dd_w / 2;
            cy = g_dd_oy + g_dd_boxh + want * g_dd_rowh + g_dd_rowh / 2;
            if (outx) *outx = cx;
            if (outy) *outy = cy;
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr, "[clickid] id=%d dropdown row %d -> (%d,%d)\n", id, want, cx, cy);
            return 1;
        }
        /* S163: the same `:rN` form on a TAB BAR means the Nth tab. One recipe form, "the Nth item
           of this control", resolved by whichever control type is hosting it. */
        if (col <= MA_ROW_SENTINEL && h.type == CT_TABS) {
            int want = MA_RC_ROW(col), tx = 0, ty = 0;
            if (!ma_tabs_point(h.ctrl, want, &tx, &ty)) {
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[clickid] id=%d has no tab %d (not laid out yet?)\n", id, want);
                return 0;
            }
            cx = ox + tx; cy = oy + ty;
        }
        else if (col <= MA_ROW_SENTINEL && h.type == CT_LISTBOX) {
            int want = MA_RC_ROW(col), wantcol = MA_RC_COL(col);
            CRListBoxCtrl* c = (CRListBoxCtrl*)h.ctrl;
            c->m_pParent = parent;
            c->m_maX = clientWnd->m_maX; c->m_maY = clientWnd->m_maY;
            c->m_maW = w; c->m_maH = hh;
            int first = -1, last = -1;
            for (int py = 0; py < hh; py++) {
                if ((int)c->GetRowFromY(py) == want) { if (first < 0) first = py; last = py; }
                else if (first >= 0) break;
            }
            if (first < 0) {
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[clickid] id=%d row=%d not mapped by GetRowFromY (h=%d)\n", id, want, hh);
                return 0;
            }
            cy = oy + (first + last) / 2;
            /* S170: ...and the COLUMN half, through the control's own GetColFromX -- the same
               technique the standalone `:C` form has used since S85, now usable together with
               a row so a recipe can name a CELL of a multi-column table. */
            if (wantcol >= 0) {
                int cfirst = -1, clast = -1;
                for (int px = 0; px < w; px++) {
                    if ((int)c->GetColFromX(px) == wantcol) { if (cfirst < 0) cfirst = px; clast = px; }
                    else if (cfirst >= 0) break;
                }
                if (cfirst < 0) {
                    if (getenv("MA_TRACE_CLICK"))
                        fprintf(stderr, "[clickid] id=%d col=%d not mapped by GetColFromX (w=%d)\n", id, wantcol, w);
                    return 0;
                }
                cx = ox + (cfirst + clast) / 2;
            }
        }
        /* S98 (PO-4): col == -2 means "the help glyph on this title bar" (recipe form `#ID@Class:?`).
           The band positions come from the button's art and move with the dialog's width and font,
           so the control's own hit-test is asked where it is -- the same rule as GetColFromX above,
           and as S95's map-icon scan. A recipe naming a pixel would be testing that pixel. */
        /* S170: `:-3` = the OK (tick) band, `:-4` = the Cancel (cross) band -- the two other
           glyphs on the same title bar, addressed the same way and through the same hit-test.
           No parser change: the generic `:%d` form already carries a negative column. */
        if ((col == -2 || col == -3 || col == -4) && h.type == CT_BUTTON) {
            int want = (col == -2) ? 0 : (col == -3) ? 3 : 2;
            const char* wname = (col == -2) ? "help" : (col == -3) ? "OK" : "Cancel";
            int lx = 0, ly = 0;
            if (!ma_button_band_point(h.ctrl, want, w, hh, &lx, &ly)) {
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[clickid] id=%d has no %s band (not a title bar?)\n", id, wname);
                return 0;
            }
            cx = ox + lx; cy = oy + ly;
        }
        if (outx) *outx = cx;
        if (outy) *outy = cy;
        if (getenv("MA_TRACE_CLICK"))
            fprintf(stderr, "[clickid] id=%d col=%d -> (%d,%d)  [type=%d rect(%d,%d %dx%d) rel=%d drawOx=%d]\n",
                    id, col, outx?*outx:-1, outy?*outy:-1, h.type, ox, oy, w, hh, h.relative,
                    h.drawOx);
        return 1;
    }
    if (getenv("MA_TRACE_CLICK")) {
        fprintf(stderr, "[clickid] id=%d UNRESOLVED; visible candidates:\n", id);
        for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
            Hosted& h = it->second; CWnd* cw = (CWnd*)it->first;
            if (!h.ctrl || !cw || !cw->m_maVisible || cw->m_maW <= 0) continue;
            CWnd* par = (CWnd*)h.parent; if (par && !par->m_maVisible) continue;
            int rel = h.relative && par && h.type != CT_LISTBOX;
            /* S171: name the PARENT CLASS. Without it an "UNRESOLVED" dump lists N identical
               candidates and cannot answer the only question being asked -- which dialog owns
               this one -- so the @Class qualifier the message tells you to add is a guess. */
            fprintf(stderr, "    id=%-5d type=%d at(%d,%d) %dx%d parent=%s\n", h.id, h.type,
                    (rel?par->m_maX:0)+cw->m_maX, (rel?par->m_maY:0)+cw->m_maY, cw->m_maW, cw->m_maH,
                    par ? typeid(*par).name() : "(none)");
        }
    }
    return 0;
}
/* Back-compat entry: unqualified lookup (still warns when the id is ambiguous). */
extern "C" int ma_ole_control_point(int id, int col, int* outx, int* outy) {
    return ma_ole_control_point_p(id, col, 0, outx, outy);
}

/* S210 (PO-65: "no text appears when I type a replay name"). THE EDIT IS DRAWN OVER THE LIST AND
   HIT-TESTED UNDER IT. On the replay Save/Load screen (CLoad) the Current File edit sits at
   (14,187) 202x26, which is wholly inside the file list's own rect (10,128) 294x260 -- the overlap
   is visible in any capture of that screen. The front-end dispatch tries ma_ole_mouse, then
   ma_ole_listbox_click, then ma_ole_click, so the LIST consumes the click and the edit's focus code
   never runs. That is why typing a replay name did nothing however you clicked: not a keyboard
   fault, a z-order one.
   TOPMOST GETS FIRST REFUSAL -- the rule MA already adopted for OOB dialogs (S82) and BoB had to
   learn again for its toolbars (their S188: hit-test in the REVERSE of the paint order). Applied
   here to the one overlap we have evidence for, rather than reordering the whole dispatch blind.
   MA_NO_EDIT_FIRST=1 reverts. */
extern "C" int ma_ole_edit_click(int sx, int sy) {
    if (getenv("MA_NO_EDIT_FIRST")) return 0;
    std::map<void*, Hosted>& m = hosted();
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (h.type != CT_EDIT || !h.ctrl) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        CWnd* parent = (CWnd*)h.parent;
        if (!clientWnd || !clientWnd->m_maVisible) continue;
        if (parent && !parent->m_maVisible) continue;
        int ox, oy; ma_ole_origin(h, clientWnd, parent, &ox, &oy);   /* S317 */
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        if (!(sx >= ox && sx < ox + w && sy >= oy && sy < oy + hh)) continue;
        ma_ole_set_focus(it->first);
        if (getenv("MA_TRACE_CLICK"))
            fprintf(stderr,"[click] edit id=%d takes keyboard focus (topmost, before the list)\n", h.id);
        return 1;
    }
    return 0;
}

extern "C" int ma_ole_listbox_click(int sx, int sy) {
    std::map<void*, Hosted>& m = hosted();
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (h.type != CT_LISTBOX || !h.ctrl || !h.id) continue;   /* need an id to route the event */
        CWnd* clientWnd = (CWnd*)it->first;
        CWnd* parent = (CWnd*)h.parent;
        if (!clientWnd || !clientWnd->m_maVisible) continue;
        if (parent && !parent->m_maVisible) continue;
        /* CT_LISTBOX is absolute-positioned -- but "absolute" became a lie in S311, which
           adds the panel art origin to the menu listbox too (and says so: the tab bar IS this
           control). S317 takes the offset from paint. */
        int ox, oy; ma_ole_origin(h, clientWnd, parent, &ox, &oy);
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        /* S203 (PO-63): hit-test the height PAINT covered, not the template rect. The title
           menu draws 199px of rows inside a 100px control, so bounding by m_maH made rows 4-6
           -- Replay, Credits, Quit -- unclickable by any route, injected or real. drawH is
           recorded by ma_ole_draw_all; fall back to the rect for a control never painted.
           This can only WIDEN what accepts a click, so it cannot move a pixel. */
        int hitH = (h.drawH > 0 && !getenv("MA_NO_DRAWH")) ? h.drawH : hh;
        /* S317: and the same for WIDTH. OnDraw centres the rows on the rect centre, so the
           painted band overflows EVENLY on both sides -- widen about the centre, not from the
           left edge, or the box lands one half-overflow to the right of the text. Height stays
           anchored at the top, which is where OnDraw starts the first row. */
        int hitW = (h.drawW > w && !getenv("MA_NO_DRAWH")) ? h.drawW : w;
        int hox  = ox - (hitW - w) / 2;
        if (getenv("MA_TRACE_CLICK"))
            fprintf(stderr,"[click] listbox id=%d rect=(%d,%d,%d,%d) drawH=%d hitH=%d drawW=%d hit=(%d..%d) vs (%d,%d) %s\n",
                    h.id, ox, oy, w, hh, h.drawH, hitH, h.drawW, hox, hox+hitW, sx, sy,
                    (sx>=hox&&sx<hox+hitW&&sy>=oy&&sy<oy+hitH)?"HIT":"miss");
        if (!(sx >= hox && sx < hox + hitW && sy >= oy && sy < oy + hitH)) continue;
        CRListBoxCtrl* c = (CRListBoxCtrl*)h.ctrl;
        long row = 0, col = 0;
        c->m_pParent = parent;
        c->m_maX = clientWnd->m_maX; c->m_maY = clientWnd->m_maY; c->m_maW = w; c->m_maH = hh;
        { int lx = sx - ox; if (lx < 0) lx = 0; if (lx >= w) lx = w - 1;   /* S317: clamp the
             overflow band onto the real rect -- the row is what matters, and a negative local x
             would run GetColFromX off the front of the column list. */
          c->MaMouse(lx, sy - oy, &row, &col); }
        if (getenv("MA_TRACE_CLICK")) fprintf(stderr,"[click] file-listbox id=%d hit local=(%d,%d) -> row=%ld col=%ld\n", h.id, sx-ox, sy-oy, row, col);
        if (parent) {
            ma_evtA0 = row; ma_evtA1 = col;
            const std::type_info* ti = &typeid(*parent);
            if (ma_evt_fire(parent, ti, h.id, 1 /*Select*/)) return 1;
        }
    }
    return 0;
}

/* Hit-test screen coords against the hosted listbox. Updates the highlight (hover)
   and returns 1 (with the hit row/col) when `clicked` and the point is inside — the
   caller then drives the panel's OnSelectRlistbox. */
int ma_ole_mouse(void* client, void* parentWnd, int sx, int sy, int clicked, long* outRow, long* outCol) {
    CRListBoxCtrl* c = get_ctrl(client, 0); if (!c) return 0;
    CWnd* clientWnd = (CWnd*)client;
    c->m_pParent = (CWnd*)parentWnd;
    c->m_maX = clientWnd->m_maX; c->m_maY = clientWnd->m_maY;
    c->m_maW = clientWnd->m_maW; c->m_maH = clientWnd->m_maH;
    /* S326 (PO 2026-08-28: "hovering the mouse causes the wrong menu item to highlight; you have to
       keep clicking the desired item several times before it activates").
       MY REGRESSION, introduced by flipping MA_MAXIMIZE on in S325. This is the HOVER path, and it
       was the ONE hit-test S317/S318 did not route through ma_ole_origin -- it subtracted the raw
       m_maX/m_maY. At 800x600 the panel origin is (0,0) so it was invisible; maximised it is
       (320,28), so the highlight tracked a point ~320 px from the cursor.
       That also explains the second half of the report: the CLICK path (ma_ole_listbox_click) WAS
       fixed, so a click eventually lands -- but the hover said otherwise, so it read as "click it
       several times". No gate caught this because panel_click tests CLICKING, not HOVERING. */
    { int hox, hoy; Hosted* hh = get_hosted(client);
      if (hh) { ma_ole_origin(*hh, clientWnd, (CWnd*)parentWnd, &hox, &hoy);
                c->m_maX = hox; c->m_maY = hoy; } }
    int lx = sx - c->m_maX, ly = sy - c->m_maY;
    if (clicked && getenv("MA_TRACE_OLE")) fprintf(stderr, "[ole_mouse] listbox rect=(%d,%d,%d,%d) click=(%d,%d) -> local=(%d,%d) %s\n", c->m_maX,c->m_maY,c->m_maW,c->m_maH, sx,sy, lx,ly, (lx<0||ly<0||lx>=c->m_maW||ly>=c->m_maH)?"OUTSIDE":"inside");
    if (lx < 0 || ly < 0 || lx >= c->m_maW || ly >= c->m_maH) return 0;   /* outside */
    long row = 0, col = 0;
    c->MaMouse(lx, ly, &row, &col);            /* sets m_iRowSel/Col + highlight */
    if (outRow) *outRow = row;
    if (outCol) *outCol = col;
    if (clicked && getenv("MA_TRACE_OLE")) fprintf(stderr, "[ole_click] local=(%d,%d) -> row=%ld col=%ld\n", lx, ly, row, col);
    return clicked ? 1 : 0;
}

} /* extern "C" */


/* ---- S121 (PO-16): keyboard delivery to hosted controls --------------------------------------
 * The front end had no keyboard route at all -- every hosted control was click-only, which is why
 * typing into the campaign profile name did nothing. Focus is recorded here; the SDL pump calls
 * ma_ole_key/ma_ole_char, and the game's own CREditCtrl does the editing.
 */
/* S199 (PO: "also make dialog title bars dragable - some dialogs are half off screen; if you
 * can't drag them, you can't use them").
 *
 * A dialog is MOVED by its title bar, and the port had no such thing: the title bar is a
 * CRButtonCtrl with the tick/close/help flags set (S82), it takes CLICKS on those glyph bands, and
 * a press anywhere else on it did nothing at all. Combined with dialogs that land partly
 * off-screen, that leaves a dialog you can see and cannot use.
 *
 * ma_ole_title_at: is (sx,sy) on a title bar, away from its glyphs? If so return the dialog tree's
 * ROOT node -- the top node carries the absolute placement (RDIALOG.CPP: children hold
 * parent-relative rects), so that is the one to move -- and its current origin.
 *
 * Returns NULL when the point is on a glyph band, so the tick and close keep working: a press
 * there is a click, not a drag.
 */
/* S200: is (sx,sy) on THIS dialog's title bar, away from its glyph bands?
 *
 * Takes the same (ox,oy) origin the caller passes to ma_ole_toolbar_click -- i.e. the node's own
 * MaXYOffset -- because that is how OOB dialogs are actually positioned. The first cut recomputed
 * origins the way ma_ole_click does and matched nothing: these dialogs never go through that
 * dispatcher at all. Two fixes in a row placed by assuming which click path a dialog uses; the
 * routing is in the log ([oobclick] -> [tbclick]) and should have been read first.
 *
 * Returns 1 and fills *rootOut with the dialog tree's ROOT (the top node carries the absolute
 * placement; children hold parent-relative rects). Returns 0 on a glyph band, so the tick, help
 * and close still take clicks rather than starting a drag.
 */
extern "C" int ma_ole_toolbar_title_at(void* dialog, int ox, int oy, int sx, int sy, void** rootOut)
{
    std::map<void*, Hosted>& m = hosted();
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (!h.ctrl || h.parent != dialog) continue;
        if (h.type != CT_BUTTON) continue;
        if (!ma_button_is_title(h.ctrl)) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        if (!clientWnd || !clientWnd->m_maVisible) continue;
        int cx = ox + clientWnd->m_maX, cy = oy + clientWnd->m_maY;
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        if (!(sx >= cx && sx < cx + w && sy >= cy && sy < cy + hh)) continue;
        if (ma_button_title_hit(h.ctrl, sx - cx, sy - cy, w, hh) >= 0) return 0;   /* a glyph: click */
        RDialog* root = (RDialog*)dialog;
        while (root && root->parent) root = root->parent;
        if (rootOut) *rootOut = (void*)root;
        return 1;
    }
    return 0;
}

/* Move a dialog tree to an absolute origin, clamped so its title bar can never leave the screen --
 * the whole point of the exercise is that an unreachable title bar makes a dialog unusable. */
extern "C" void ma_ole_move_dialog(void* rootp, int x, int y, int screenW, int screenH)
{
    RDialog* root = (RDialog*)rootp;
    if (!root) return;
    int w = root->m_maW > 0 ? root->m_maW : 0;
    const int keep = 24;                     /* leave at least this much of the bar reachable */
    if (x < -(w - keep)) x = -(w - keep);
    if (x > screenW - keep) x = screenW - keep;
    if (y < 0) y = 0;                        /* never above the top: the title bar goes first */
    if (y > screenH - keep) y = screenH - keep;
    root->m_maX = x;
    root->m_maY = y;
}

extern "C" void ma_ole_set_focus(void* client)
{
    std::map<void*, Hosted>& m = hosted();
    std::map<void*, Hosted>::iterator it = m.find(client);
    if (it == m.end()) return;                 /* not a hosted control: ignore, keep current focus */
    if (it->second.type != CT_EDIT) return;    /* only editable controls take the keyboard */
    g_focus_client = client;
    if (getenv("MA_TRACE_OLE"))
        fprintf(stderr, "[focus] edit control %p (id=%d) has the keyboard\n", client, it->second.id);
}

extern "C" int ma_ole_has_focus(void)
{
    return g_focus_client != 0;
}

/* Returns 1 if the keystroke was consumed by a focused control. */
extern "C" int ma_ole_char(int ch)
{
    if (!g_focus_client) return 0;
    std::map<void*, Hosted>& m = hosted();
    std::map<void*, Hosted>::iterator it = m.find(g_focus_client);
    if (it == m.end() || it->second.type != CT_EDIT || !it->second.ctrl) { g_focus_client = 0; return 0; }
    ma_edit_char(it->second.ctrl, ch);
    if (getenv("MA_TRACE_OLE"))
        fprintf(stderr, "[type] '%c' -> \"%s\"\n", (char)ch, ma_edit_text(it->second.ctrl));
    /* S251 (PO-68, ROOT CAUSE OF THE DESTRUCTIVE SAVE): typing updated the control's own text and
       told NOBODY. The R* controls raise TextChanged through COleControl::FireEvent, whose
       connection point is stubbed here, so the dialog never heard it -- exactly the reason the
       dropdown path in ma_ole_click fires its event by hand. CLoad::filename is a `CString&`
       BOUND TO THE CALLER'S selectedfile, and OnTextChangedSavename is the only writer of a typed
       name into it; with the event lost, `selectedfile` kept Save_Data.lastreplayname and the save
       overwrote THAT file. Measured twice on the PO's machine: saving "260825test" and later
       "260825test2" each landed on corpus-baseline.cam.
       Fire TextChanged (dispid 2 -- see CLoad's ON_EVENT map) with the control's text in ma_evtP,
       which the S251 LPTSTR thunk now delivers. MA_NO_EDIT_EVENT=1 reverts. */
    if (!getenv("MA_NO_EDIT_EVENT") && it->second.parent && it->second.id) {
        CWnd* dp = (CWnd*)it->second.parent;
        ma_evtP = (void*)ma_edit_text(it->second.ctrl);
        ma_evtA0 = 0; ma_evtA1 = 0;
        ma_evt_fire(dp, &typeid(*dp), it->second.id, 2 /*TextChanged*/);
        if (getenv("MA_TRACE_OLE"))
            fprintf(stderr, "[type] fired TextChanged id=%d -> \"%s\"\n",
                    it->second.id, (const char*)ma_evtP);
    }
    return 1;
}

extern "C" int ma_ole_key(int vk)
{
    if (!g_focus_client) return 0;
    std::map<void*, Hosted>& m = hosted();
    std::map<void*, Hosted>::iterator it = m.find(g_focus_client);
    if (it == m.end() || it->second.type != CT_EDIT || !it->second.ctrl) { g_focus_client = 0; return 0; }
    ma_edit_key(it->second.ctrl, vk);
    return 1;
}

extern "C" const char* ma_ole_focus_text(void)
{
    if (!g_focus_client) return "";
    std::map<void*, Hosted>& m = hosted();
    std::map<void*, Hosted>::iterator it = m.find(g_focus_client);
    if (it == m.end() || it->second.type != CT_EDIT || !it->second.ctrl) return "";
    return ma_edit_text(it->second.ctrl);
}
