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
#include "RListBxC.h"          /* CRListBoxCtrl */
/* S82: implemented in ma_olebutton.cpp (only that TU can see CRButtonCtrl). */
extern "C" int ma_button_title_hit(void* ctrl, int x, int y, int w, int h);
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
enum { CT_NONE = 0, CT_LISTBOX, CT_STATIC, CT_BUTTON, CT_COMBO, CT_EDIT, CT_EDTBT, CT_TABS, CT_OTHER };
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
struct Hosted { int type; void* ctrl; void* parent; int relative; int id; int drawOx, drawOy; };
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
/* S57 (BoB S124 §8f): template-membership draw filter + layer switch (ma_dlgtmpl.cpp) */
extern "C" int   ma_dlg_in_template(void* dlg, int id);
extern "C" int   ma_dlg_never_visible(void* dlg, int id);   /* S59: parked outside the dialog rect -> Windows-clipped, never paints */
extern "C" int   ma_pe_layer_on(void);
extern "C" int   ma_dlg_artnum(void* dlg, int id, long* outFn);   /* S58: tickbox-family filtered */

/* known control CLSIDs (compare on Data1) */
static int clsid_is(const GUID* g, unsigned long d1) { return g && g->Data1 == d1; }

extern "C" void ma_ole_create(void* client, const void* clsidPtr, void* parent) {
    if (!client) return;
    if (getenv("MA_TRACE_OLE")) { const GUID* g=(const GUID*)clsidPtr; static int n=0; if(n++<400) fprintf(stderr,"[ole_create] client=%p clsid.Data1=%08lx parent=%p\n", client, g?(unsigned long)g->Data1:0, parent); }
    std::map<void*, Hosted>& m = hosted();
    if (m.find(client) != m.end()) { m[client].parent = parent; return; }   /* already; refresh parent */
    const GUID* clsid = (const GUID*)clsidPtr;
    Hosted h; h.type = CT_OTHER; h.ctrl = 0; h.parent = parent; h.relative = 0; h.id = 0;
    h.drawOx = h.drawOy = -1;   /* S84: not drawn yet */
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
        if (h->id == 1001 /*IDJ_TITLE*/ || ma_dlg_artnum(h->parent, h->id, &fn)) {
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
    if (hh && hh->type == CT_COMBO)  { ma_combo_getprop(hh->ctrl, (int)dispid, (int)vt, pvRet); return; }
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

/* property SET — single value follows in the va_list */
void ma_ole_setprop(void* client, DISPID dispid, VARTYPE vt, va_list ap) {
    Hosted* hh = get_hosted(client);
    if (hh && hh->type == CT_STATIC) { ma_static_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_BUTTON) { ma_button_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
    if (hh && hh->type == CT_COMBO)  { ma_combo_setprop(hh->ctrl, (int)dispid, (int)vt, ap); return; }
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
      if (hc && hc->type == CT_COMBO) { ma_combo_invoke(hc->ctrl, (int)dispid, (int)vtRet, pvRet, ap); return; } }
    /* S60: RTabs methods are dispids 4-8, i.e. inside the same low range the listbox path
       below would misread — route by type first, exactly as the combo above. */
    { Hosted* ht = get_hosted(client);
      if (ht && ht->type == CT_TABS) { ma_tabs_invoke(ht->ctrl, (int)dispid, (int)vtRet, pvRet, ap); return; } }
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
            hosted()[client] = h;
        }
    }
    CRListBoxCtrl* c = get_ctrl(client, 1); if (!c) return;
    CWnd* clientWnd = (CWnd*)client;
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

void ma_ole_draw_all(void* screenHdc) {
    std::map<void*, Hosted>& m = hosted();
    if (getenv("MA_TRACE_SIZE")) { static int f=0; if((f++ % 30)==0) fprintf(stderr,"[hosted.size] frame~%d entries=%zu\n", f, m.size()); }
    void* dd_ctrl = 0;   /* F2: the open dropdown's combo, captured below, drawn on top after the loop */
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
            CDC dc; dc.m_hDC = (HDC)screenHdc;
            int sx = 0, sy = 0;
            ma_gdi_set_viewport_org(screenHdc, ox, oy, &sx, &sy);
            CRect bounds(0, 0, w, hh);
            c->OnDraw(&dc, bounds, bounds);
            ma_gdi_set_viewport_org(screenHdc, sx, sy, 0, 0);
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
        else if (h.type == CT_COMBO)  ma_combo_draw(h.ctrl, dialog, screenHdc, cx, cy, w, hh);
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
            ma_oob_lb_draw = 1;
            c->OnDraw(&dc, lbounds, lbounds);
            ma_oob_lb_draw = 0;
            ma_gdi_set_viewport_org(screenHdc, lox, loy, 0, 0);
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

extern "C" int ma_ole_toolbar_click(void* dialog, int ox, int oy, int sx, int sy) {
    std::map<void*, Hosted>& m = hosted();
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (!h.ctrl || h.parent != dialog) continue;
        if (h.type != CT_BUTTON && h.type != CT_TABS && h.type != CT_LISTBOX) continue;
        if (h.type == CT_BUTTON && !h.id) continue;        /* buttons route by id; tabs don't */
        if (h.type == CT_LISTBOX && !h.id) continue;       /* need an id to route Select */
        CWnd* clientWnd = (CWnd*)it->first;
        if (!clientWnd || !clientWnd->m_maVisible) continue;
        int cx = ox + clientWnd->m_maX, cy = oy + clientWnd->m_maY;
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        if (!(sx >= cx && sx < cx + w && sy >= cy && sy < cy + hh)) continue;
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
            if (getenv("MA_TRACE_CLICK"))
                fprintf(stderr,"[tbclick] id=%d rect=(%d,%d,%d,%d) -> fire\n", h.id, cx,cy,w,hh);
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
        if (h.type != CT_BUTTON && h.type != CT_COMBO && h.type != CT_EDTBT) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        CWnd* parent = (CWnd*)h.parent;
        if (!clientWnd || !clientWnd->m_maVisible || (parent && !parent->m_maVisible)) continue;
        /* S57: controls filtered out of the draw (not in the installed template) don't click either */
        if (h.relative && h.parent && h.id > 0 &&
            ma_dlg_in_template(h.parent, h.id) == 0) continue;
        /* S59: nor do Windows-clipped controls parked outside the dialog rect */
        if (h.relative && h.parent && h.id > 0 &&
            ma_dlg_never_visible(h.parent, h.id) == 1) continue;
        int rel = h.relative && parent;
        int ox = (rel ? parent->m_maX : 0) + clientWnd->m_maX;
        int oy = (rel ? parent->m_maY : 0) + clientWnd->m_maY;
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        if (getenv("MA_TRACE_CLICK") && h.type==CT_COMBO) fprintf(stderr,"[click] combo box=(%d,%d,%d,%d) vs (%d,%d) %s\n", ox,oy,w,hh,sx,sy, (sx>=ox&&sx<ox+w&&sy>=oy&&sy<oy+hh)?"HIT":"miss");
        if (getenv("MA_TRACE_CLICK") && h.type==CT_BUTTON) fprintf(stderr,"[click] button id=%d rect=(%d,%d,%d,%d) centre=(%d,%d)\n", h.id, ox,oy,w,hh, ox+w/2, oy+hh/2);
        if (!(sx >= ox && sx < ox + w && sy >= oy && sy < oy + hh)) continue;
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
        if (n > bestCount) { bestCount = n; best = c; bestWnd = clientWnd; bestParent = parent; }
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
    if (outx) *outx = bestWnd->m_maX + bestWnd->m_maW / 2;
    if (outy) *outy = bestWnd->m_maY + (first + last) / 2;
    if (getenv("MA_TRACE_CLICK"))
        fprintf(stderr, "[clickrow] row=%d -> (%d,%d)  [listbox (%d,%d) %dx%d, %d rows, listH=%ld rowH=%d]\n",
                row, outx?*outx:-1, outy?*outy:-1, bestWnd->m_maX, bestWnd->m_maY,
                bestWnd->m_maW, bestWnd->m_maH, bestCount, lh, rowH);
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
extern "C" int ma_ole_control_point_p(int id, int col, const char* parentClass, int* outx, int* outy) {
    std::map<void*, Hosted>& m = hosted();
    /* Count visible candidates first, so ambiguity is reported rather than silently resolved. */
    if (!parentClass || !*parentClass) {
        int cand = 0;
        for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
            Hosted& h = it->second; CWnd* cw = (CWnd*)it->first; CWnd* pw = (CWnd*)h.parent;
            if (!h.ctrl || h.id != id || !cw || !cw->m_maVisible) continue;
            if (pw && !pw->m_maVisible) continue;
            if (cw->m_maW > 0 && cw->m_maH > 0) cand++;
        }
        if (cand > 1) {
            fprintf(stderr, "[clickid] WARNING id=%d is AMBIGUOUS (%d visible hosts) — add @Class to the recipe:\n", id, cand);
            for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
                Hosted& h = it->second; CWnd* cw = (CWnd*)it->first; CWnd* pw = (CWnd*)h.parent;
                if (!h.ctrl || h.id != id || !cw || !cw->m_maVisible) continue;
                if (pw && !pw->m_maVisible) continue;
                if (cw->m_maW <= 0 || cw->m_maH <= 0) continue;
                fprintf(stderr, "[clickid]   candidate host=%s type=%d rect(%d,%d %dx%d)\n",
                        pw ? typeid(*pw).name() : "(none)", h.type, cw->m_maX, cw->m_maY, cw->m_maW, cw->m_maH);
            }
        }
    }
    for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
        Hosted& h = it->second;
        if (!h.ctrl || h.id != id) continue;
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
        int rel = h.relative && parent && h.type != CT_LISTBOX;
        int ox, oy;
        if (h.drawOx >= 0) {
            /* S84: this control has been drawn by ma_ole_draw_toolbar — use the offset PAINT
               used. Toolbar-hosted buttons live at the offset the map idle passes in (4,26 /
               4,52), not at the parent's m_maX/m_maY (which are 0), so the old arithmetic put
               `#ID` recipes ~50px off and every toolbar recipe had to be hand-computed. */
            ox = h.drawOx + clientWnd->m_maX;
            oy = h.drawOy + clientWnd->m_maY;
        } else {
            ox = (rel ? parent->m_maX : 0) + clientWnd->m_maX;
            oy = (rel ? parent->m_maY : 0) + clientWnd->m_maY;
        }
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
        /* S98 (PO-4): col == -2 means "the help glyph on this title bar" (recipe form `#ID@Class:?`).
           The band positions come from the button's art and move with the dialog's width and font,
           so the control's own hit-test is asked where it is -- the same rule as GetColFromX above,
           and as S95's map-icon scan. A recipe naming a pixel would be testing that pixel. */
        if (col == -2 && h.type == CT_BUTTON) {
            int lx = 0, ly = 0;
            if (!ma_button_help_point(h.ctrl, w, hh, &lx, &ly)) {
                if (getenv("MA_TRACE_CLICK"))
                    fprintf(stderr, "[clickid] id=%d has no help band (not a title bar?)\n", id);
                return 0;
            }
            cx = ox + lx; cy = oy + ly;
        }
        if (outx) *outx = cx;
        if (outy) *outy = cy;
        if (getenv("MA_TRACE_CLICK"))
            fprintf(stderr, "[clickid] id=%d col=%d -> (%d,%d)  [type=%d rect(%d,%d %dx%d) rel=%d]\n",
                    id, col, outx?*outx:-1, outy?*outy:-1, h.type, ox, oy, w, hh, rel);
        return 1;
    }
    if (getenv("MA_TRACE_CLICK")) {
        fprintf(stderr, "[clickid] id=%d UNRESOLVED; visible candidates:\n", id);
        for (std::map<void*, Hosted>::iterator it = m.begin(); it != m.end(); ++it) {
            Hosted& h = it->second; CWnd* cw = (CWnd*)it->first;
            if (!h.ctrl || !cw || !cw->m_maVisible || cw->m_maW <= 0) continue;
            CWnd* par = (CWnd*)h.parent; if (par && !par->m_maVisible) continue;
            int rel = h.relative && par && h.type != CT_LISTBOX;
            fprintf(stderr, "    id=%-5d type=%d at(%d,%d) %dx%d\n", h.id, h.type,
                    (rel?par->m_maX:0)+cw->m_maX, (rel?par->m_maY:0)+cw->m_maY, cw->m_maW, cw->m_maH);
        }
    }
    return 0;
}
/* Back-compat entry: unqualified lookup (still warns when the id is ambiguous). */
extern "C" int ma_ole_control_point(int id, int col, int* outx, int* outy) {
    return ma_ole_control_point_p(id, col, 0, outx, outy);
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
        int ox = clientWnd->m_maX, oy = clientWnd->m_maY;        /* CT_LISTBOX is absolute-positioned */
        int w = clientWnd->m_maW, hh = clientWnd->m_maH;
        if (w <= 0 || hh <= 0) continue;
        if (!(sx >= ox && sx < ox + w && sy >= oy && sy < oy + hh)) continue;
        CRListBoxCtrl* c = (CRListBoxCtrl*)h.ctrl;
        long row = 0, col = 0;
        c->m_pParent = parent;
        c->m_maX = clientWnd->m_maX; c->m_maY = clientWnd->m_maY; c->m_maW = w; c->m_maH = hh;
        c->MaMouse(sx - ox, sy - oy, &row, &col);
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
