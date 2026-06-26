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
#include <map>
#include "RListBxC.h"          /* CRListBoxCtrl */

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
enum { CT_NONE = 0, CT_LISTBOX, CT_STATIC, CT_BUTTON, CT_COMBO, CT_EDIT, CT_OTHER };
/* `relative`: control was positioned from the RT_DIALOG template (client-relative to its
   dialog) -> add the parent's screen origin when drawing. Game-positioned controls (the
   menu listbox via PositionRListBox) use absolute screen coords -> no parent add. */
struct Hosted { int type; void* ctrl; void* parent; int relative; int id; };
extern "C" int ma_evt_fire(void* dlg, const void* tinfo, int id, int dispid);
extern "C" { extern long ma_evtA0, ma_evtA1; }   /* event args (set before firing Select) */
static std::map<void*, Hosted>& hosted() { static std::map<void*, Hosted> m; return m; }

/* F2 — open-dropdown state (one at a time). g_dd_client is the client key of the combo whose
   list is open, or NULL. Geometry is captured during draw_all (the dropdown is drawn on top
   after the per-control loop) and reused to hit-test row clicks. */
static void* g_dd_client = 0;
static int   g_dd_ox, g_dd_oy, g_dd_w, g_dd_boxh, g_dd_rowh, g_dd_count, g_dd_hover = -1;
static void* combo_ctrl_of(void* client) {
    std::map<void*, Hosted>& m = hosted();
    std::map<void*, Hosted>::iterator it = m.find(client);
    return (it != m.end() && it->second.type == CT_COMBO) ? it->second.ctrl : 0;
}

/* per-type glue implemented in ma_olestatic.cpp (separate TU to avoid OCX-project
   header collisions) */
extern "C" void* ma_static_create(void* client);
extern "C" void  ma_static_set_string(void* ctrl, const char* s);
extern "C" void  ma_static_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_static_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_static_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
extern "C" void* ma_button_create(void* client);
extern "C" void  ma_button_setprop(void* ctrl, int dispid, int vt, va_list ap);
extern "C" void  ma_button_getprop(void* ctrl, int dispid, int vt, void* pvRet);
extern "C" void  ma_button_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
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

/* known control CLSIDs (compare on Data1) */
static int clsid_is(const GUID* g, unsigned long d1) { return g && g->Data1 == d1; }

extern "C" void ma_ole_create(void* client, const void* clsidPtr, void* parent) {
    if (!client) return;
    if (getenv("MA_TRACE_OLE")) { const GUID* g=(const GUID*)clsidPtr; static int n=0; if(n++<400) fprintf(stderr,"[ole_create] client=%p clsid.Data1=%08lx parent=%p\n", client, g?(unsigned long)g->Data1:0, parent); }
    std::map<void*, Hosted>& m = hosted();
    if (m.find(client) != m.end()) { m[client].parent = parent; return; }   /* already; refresh parent */
    const GUID* clsid = (const GUID*)clsidPtr;
    Hosted h; h.type = CT_OTHER; h.ctrl = 0; h.parent = parent; h.relative = 0; h.id = 0;
    if (clsid_is(clsid, 0x48814009 /*RListBox*/)) {
        CRListBoxCtrl* c = new CRListBoxCtrl();
        c->m_hWnd = (HWND)client;                  /* non-null: OnDraw takes the real path */
        c->OnResetState();                         /* ClassWizard defaults (fore=white, etc.) */
        h.type = CT_LISTBOX; h.ctrl = c;
    } else if (clsid_is(clsid, 0xc42bac3d /*RStatic*/)) {
        h.type = CT_STATIC; h.ctrl = ma_static_create(client);
    } else if (clsid_is(clsid, 0x78918646 /*RButton*/)) {
        h.type = CT_BUTTON; h.ctrl = ma_button_create(client);
    } else if (clsid_is(clsid, 0x737cb0c9 /*RCombo*/)) {
        h.type = CT_COMBO; h.ctrl = ma_combo_create(client);
    } else if (clsid_is(clsid, 0x499e2be6 /*REdit*/)) {
        h.type = CT_EDIT; h.ctrl = ma_edit_create(client);
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
extern "C" void ma_ole_set_id(void* client, int id) {
    Hosted* h = get_hosted(client); if (h) h->id = id;
}
/* apply a label string parsed from RT_DLGINIT (DDX_Control) — statics, and edits whose
   template carries a default caption (e.g. a default savename) */
extern "C" void ma_ole_set_label(void* client, const char* text) {
    Hosted* h = get_hosted(client);
    if (!h || !h->ctrl) return;
    if (h->type == CT_STATIC) ma_static_set_string(h->ctrl, text);
    else if (h->type == CT_EDIT) ma_edit_set_string(h->ctrl, text);
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
        /* skip hidden controls / controls whose parent dialog is hidden (ShowWindow(SW_HIDE)) */
        if (!clientWnd->m_maVisible) continue;
        if (parent && !parent->m_maVisible) continue;
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
        if (h.type != CT_BUTTON && h.type != CT_COMBO) continue;
        CWnd* clientWnd = (CWnd*)it->first;
        CWnd* parent = (CWnd*)h.parent;
        if (!clientWnd || !clientWnd->m_maVisible || (parent && !parent->m_maVisible)) continue;
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
        if (parent && h.id) {                       /* CT_BUTTON */
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
