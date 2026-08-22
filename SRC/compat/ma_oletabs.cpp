/* ma_oletabs.cpp — host glue for the CRTabsCtrl OCX (the Rowan tab bar).
 *
 * S60 (parity #15, I4). The Player Log is built by MAINTBAR.CPP:315 as
 *   MakeTopDialog(Place(x,y), DialList(DialBox(CPlyr_log), HTabBox(IdList(IDS_CAREER,
 *   IDS_MISSIONLOG, IDS_LASTMISSION), DialBox(CCareer), DialBox(CMisn_log),
 *   DialBox(CLastMissionLog))))
 * and the HTabBox arm is driven by RDialog::AddChildren(…, childtype, titles)
 * (RDIALOG.CPP:612), which opens with
 *   CRTabs* t = (CRTabs*)GetDlgItem(IDJ_TABCTRL); t->SetHorzAlign(…);
 * and ends each child with RDialog::AttachTabToTabControl -> t->AddTab(text, this).
 * IDJ_TABCTRL (1002) is declared in the IDD_EMPTYPAGE (130) template as an RTabs OCX,
 * but NO dialog class DDX_Control-binds it — on Windows the dialog manager instantiates
 * every template item, so GetDlgItem just finds it. This port only created controls that
 * the game explicitly DDX_Control'd, so the tab control never existed, GetDlgItem
 * returned NULL, AttachTabToTabControl took its "No tab control exists" early-out, and
 * the Player Log rendered as a bare pilot-photo blit. afxwin.h's template-driven hosting
 * (ma_host_template_controls) now creates it; this file is the dispatch/draw glue.
 *
 * Compiled in its own TU ("rtabs" mode, -ISRC/RTABS) for the same reason as every other
 * R* mode: each OCX project ships its own stdafx.h/resource.h. The (client -> control)
 * registry lives in ma_olecontrol.cpp; this file is stateless glue over a CRTabsCtrl*.
 *
 * Dispatch ids follow the RTABSCTL.CPP map order: 1 FirstTab, 2 HorzAlign, 3 FontNum,
 * 4 AddTab, 5 Clear, 6 CalculateHeight, 7 CalcWidestWord, 8 SelectTab; + stock ForeColor.
 *
 * Tab ART: CRTabsCtrl::OnDraw's one-shot m_bInit block does
 * m_TabUp.LoadBitmap(IDB_TABUP) / m_TabDown.LoadBitmap(IDB_TABDOWN) — bitmaps that live
 * in RTabs.ocx's OWN PE resources (AfxGetInstanceHandle inside the control = the OCX, not
 * Mig.exe), which is why the compat CBitmap::LoadBitmap no-op cannot serve them. RTabs.ocx
 * ships in the install dir, so we preload both RT_BITMAPs from it through the existing PE
 * resource layer and hand the control ready-made memory DCs, then clear m_bInit so OnDraw
 * skips its own load. Same principle as the S57 PE-resource ruling: the installed
 * (BDG-patched) binaries are the oracle. If the OCX is missing the control still draws its
 * tab TEXT — the art blits degrade to no-ops (the widths are only ever used as BitBlt
 * source offsets, never as divisors), so a missing OCX costs art, not tabs.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "RTabsCtl.h"          /* CRTabsCtrl */

extern "C" {
void* ma_tabs_create(void* client);
void  ma_tabs_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_tabs_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_tabs_invoke(void* ctrl, int dispid, int vtRet, void* pvRet, va_list ap);
void  ma_tabs_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
long  ma_tabs_hit(void* ctrl, int x, int y);

void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
void* ma_gdi_create_dc(void);
void* ma_gdi_create_bitmap(int w, int h);
void* ma_gdi_select_bitmap(void* hdc, void* hbmp);
void  ma_gdi_set_dibits(void*, int, int, int, int, const void*, const void*);
void* bob_LoadLibrary(const char* path);
const void* bob_res_get(void* h, unsigned type, unsigned id, unsigned* outSize);
}

#define RT_BITMAP_T 2
#define IDB_TABUP_RES   213     /* SRC/RTABS/RESOURCE.H */
#define IDB_TABDOWN_RES 214

/* ---- tab art from RTabs.ocx's own PE resources ------------------------- */

/* Load RT_BITMAP `id` out of RTabs.ocx into a fresh memory DC.
   An RT_BITMAP payload is a BITMAPINFOHEADER + colour table + pixel bits (no
   BITMAPFILEHEADER), which is exactly what ma_gdi_set_dibits consumes. */
static void* load_ocx_bitmap_dc(unsigned id, int* outW, int* outH) {
    *outW = *outH = 0;
    static void* s_ocx = 0;
    static int   s_tried = 0;
    if (!s_tried) { s_tried = 1; s_ocx = bob_LoadLibrary("RTabs.ocx"); }
    if (!s_ocx) return 0;

    unsigned sz = 0;
    const unsigned char* dib = (const unsigned char*)bob_res_get(s_ocx, RT_BITMAP_T, id, &sz);
    if (!dib || sz < 40) return 0;

    int biSize     = *(const int*)(dib + 0);
    int biW        = *(const int*)(dib + 4);
    int biH        = *(const int*)(dib + 8);
    int bpp        = *(const unsigned short*)(dib + 14);
    int biClrUsed  = *(const int*)(dib + 32);
    int h = biH < 0 ? -biH : biH;
    if (biW <= 0 || h <= 0 || biSize < 40) return 0;

    /* pixel bits start after the header + colour table */
    int nclr = biClrUsed ? biClrUsed : (bpp <= 8 ? (1 << bpp) : 0);
    unsigned bitsOff = (unsigned)biSize + (unsigned)nclr * 4u;
    if (bitsOff >= sz) return 0;

    void* hdc  = ma_gdi_create_dc();
    void* hbmp = ma_gdi_create_bitmap(biW, h);
    if (!hdc || !hbmp) return 0;
    ma_gdi_select_bitmap(hdc, hbmp);
    ma_gdi_set_dibits(hdc, 0, 0, biW, h, dib + bitsOff, dib);
    *outW = biW; *outH = h;
    return hdc;
}

/* Give the control its art up front and disarm OnDraw's own (no-op) load. */
static void prime_tab_art(CRTabsCtrl* c) {
    if (!c || !c->m_bInit) return;
    int w = 0, h = 0;
    void* up = load_ocx_bitmap_dc(IDB_TABUP_RES, &w, &h);
    if (up) { c->m_TabUpDC.m_hDC = (HDC)up; c->m_TabUpWidth = w; c->m_TabUpHeight = h; }
    else    { c->m_TabUpWidth = 0; c->m_TabUpHeight = 0; }
    void* dn = load_ocx_bitmap_dc(IDB_TABDOWN_RES, &w, &h);
    if (dn) { c->m_TabDownDC.m_hDC = (HDC)dn; c->m_TabDownWidth = w; c->m_TabDownHeight = h; }
    else    { c->m_TabDownWidth = 0; c->m_TabDownHeight = 0; }
    if (getenv("MA_TRACE_TABS"))
        fprintf(stderr, "[tabs] art up=%p (%dx%d) down=%p (%dx%d)\n",
                up, c->m_TabUpWidth, c->m_TabUpHeight, dn, c->m_TabDownWidth, c->m_TabDownHeight);
    c->m_bInit = FALSE;        /* OnDraw's m_bInit block would undo all of the above */
}

/* ---- glue -------------------------------------------------------------- */

void* ma_tabs_create(void* client) {
    CRTabsCtrl* c = new CRTabsCtrl();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw takes the real (parent) path */
    c->OnResetState();                 /* ClassWizard defaults */
    /* S59 uninit-PX audit, checked for this control: CRTabsCtrl's ctor already inits its
       only DoPropExchange-persisted member (m_FontNum = 0, the PX_Long default) along with
       m_iCurrentSelection/m_firstTab/m_bHorzAlign/m_bInit — so the S58 heap-garbage class
       does not apply here. The art dimensions are the one uninitialised group, and
       prime_tab_art sets all four on every path. */
    return c;
}

void ma_tabs_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    CRTabsCtrl* c = (CRTabsCtrl*)ctrlp; if (!c) return;
    (void)vt;
    if (getenv("MA_TRACE_TABS")) { static int n=0; if(n++<40)
        fprintf(stderr,"[tabs_set] ctrl=%p dispid=%d vt=%d\n", ctrlp, dispid, vt); }
    switch (dispid) {
        case DISPID_FORECOLOR: c->SetForeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case 1: c->SetFirstTab(va_arg(ap, long)); return;
        case 2: c->SetHorzAlign((BOOL)va_arg(ap, int)); return;
        case 3: c->SetFontNum(va_arg(ap, long)); return;
    }
}

void ma_tabs_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    CRTabsCtrl* c = (CRTabsCtrl*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: *(OLE_COLOR*)pvRet = c->GetForeColor(); return;
        case 1: *(long*)pvRet = c->GetFirstTab(); return;
        case 2: *(BOOL*)pvRet = c->GetHorzAlign(); return;
        case 3: *(long*)pvRet = c->GetFontNum(); return;
    }
}

void ma_tabs_invoke(void* ctrlp, int dispid, int vtRet, void* pvRet, va_list ap) {
    CRTabsCtrl* c = (CRTabsCtrl*)ctrlp; if (!c) return;
    (void)vtRet;
    switch (dispid) {
        case 4: {                                  /* AddTab(BSTR text, I4 pWnd) */
            char* s = va_arg(ap, char*);
            long  w = va_arg(ap, long);
            if (getenv("MA_TRACE_TABS")) fprintf(stderr, "[tabs] AddTab ctrl=%p \"%s\" wnd=%ld\n", ctrlp, s?s:"(null)", w);
            c->AddTab(s ? s : "", w);
            return;
        }
        case 5: c->Clear(); return;
        case 6: { long t = va_arg(ap, long); long r = c->CalculateHeight(t); if (pvRet) *(long*)pvRet = r; return; }
        case 7: { long r = c->CalcWidestWord(); if (pvRet) *(long*)pvRet = r; return; }
        case 8: { long t = va_arg(ap, long); long r = c->SelectTab(t);       if (pvRet) *(long*)pvRet = r; return; }
    }
}

void ma_tabs_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    CRTabsCtrl* c = (CRTabsCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;          /* OnDraw uses GetParent() for rect + font */
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    prime_tab_art(c);
    if (getenv("MA_TRACE_TABS")) { static int n=0; if (n++<6)
        fprintf(stderr, "[tabs.draw] ctrl=%p at(%d,%d) %dx%d tabs=%d horz=%d sel=%d\n",
                ctrlp, sx, sy, w, h, (int)c->m_textList.GetCount(), (int)c->m_bHorzAlign, c->m_iCurrentSelection); }
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0;
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
}

/* Control-relative hit test -> the RDialog* of the tab under (x,y), or 0.
   CRTabsCtrl::OnDraw fills m_rectList/m_tabList in lockstep while laying the rows out,
   which is exactly what its own OnLButtonDown consumes; we reuse that instead of
   re-deriving the layout. Returns the tab's window pointer (what SelectTab expects). */
/* S82: hit + select in one call — the click half of the tab bar. Until now ma_tabs_hit had no
   caller at all (declared, never used): OOB dialogs received no clicks, so switching a Player Log
   tab was only possible through the MA_OOB_PLAYERLOG_TAB scaffold hook. Returns 1 if a tab took
   the click. Selection goes through the control's own SelectTab, not a reimplementation. */
extern "C" int ma_tabs_click(void* ctrlp, int x, int y) {
    CRTabsCtrl* c = (CRTabsCtrl*)ctrlp; if (!c) return 0;
    long tab = ma_tabs_hit(ctrlp, x, y);
    if (!tab) return 0;
    if (getenv("MA_TRACE_TABS")) fprintf(stderr,"[tabs] click (%d,%d) -> SelectTab(%p)\n", x, y, (void*)tab);
    c->SelectTab(tab);
    return 1;
}

/* S163: the CENTRE of the Nth tab's own rect, for the `#ID@Class:rN` recipe form. Same rule as the
   listbox row and column resolvers -- ask the control where its Nth item was DRAWN (m_rectList is
   filled by OnDraw in lockstep with m_tabList, which is what its own OnLButtonDown consumes) rather
   than deriving a pixel. Naming a 3-tab bar without an index resolves to the control's centre, i.e.
   the MIDDLE tab, which is right by luck for a 3-tab dossier and wrong for every other count. */
extern "C" int ma_tabs_point(void* ctrlp, int index, int* ox, int* oy) {
    CRTabsCtrl* c = (CRTabsCtrl*)ctrlp; if (!c || index < 0) return 0;
    POSITION rp = c->m_rectList.GetHeadPosition();
    for (int i = 0; rp; i++) {
        CRect r = c->m_rectList.GetNext(rp);
        if (i == index) {
            if (ox) *ox = (r.left + r.right) / 2;
            if (oy) *oy = (r.top + r.bottom) / 2;
            return 1;
        }
    }
    return 0;
}

long ma_tabs_hit(void* ctrlp, int x, int y) {
    CRTabsCtrl* c = (CRTabsCtrl*)ctrlp; if (!c) return 0;
    POSITION rp = c->m_rectList.GetHeadPosition();
    POSITION tp = c->m_tabList.GetHeadPosition();
    while (rp && tp) {
        CRect r = c->m_rectList.GetNext(rp);
        int   t = c->m_tabList.GetNext(tp);
        if (x >= r.left && x < r.right && y >= r.top && y < r.bottom) return (long)t;
    }
    return 0;
}
