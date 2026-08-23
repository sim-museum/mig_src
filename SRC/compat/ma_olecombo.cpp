/* ma_olecombo.cpp — host glue for the CRComboCtrl OCX (the front-end's setting SELECTOR:
 * display driver / resolution / gamma / framerate / shading / reflections / weather, etc.).
 *
 * Compiled in its own TU ("olecombo" mode, -ISRC/RCOMBO) so the RCOMBO project headers don't
 * collide with the other OCX projects. Mirrors ma_olestatic.cpp; the shared (client -> control)
 * registry lives in ma_olecontrol.cpp. Dispatch dispids follow RCOMBOC.CPP map order:
 *   props   : 1 FontNum, 2 ListboxLength, 3 CircularStyle, 4 ShadowColor, 5 EndFileNum,
 *             6 FileNumMain  (+ stock FORECOLOR / CAPTION / ENABLED, negative dispids)
 *   methods : 7 AddString, 8 GetListbox, 9 SetIndex, 10 GetIndex, 11 Clear, 12 DeleteString
 * The combo shows InternalGetText() (the current item); SetIndex -> SetWindowText sets it. */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RComboC.h"          /* CRComboCtrl */

extern "C" {
void* ma_combo_create(void* client);
void  ma_combo_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_combo_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_combo_invoke(void* ctrl, int dispid, int vt, void* pvRet, va_list ap);
void  ma_combo_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
void  ma_combo_click(void* ctrl);
/* F2 — real dropdown list (vs cycle-on-click) */
int   ma_combo_itemcount(void* ctrl);
int   ma_combo_curindex(void* ctrl);
int   ma_combo_select(void* ctrl, int row);     /* SetIndex + fire change; 1 if applied */
void  ma_combo_dropdown_draw(void* ctrl, void* screenHdc, int sx, int sy, int w,
                             int boxh, int hoverRow, int* out_rowh);
void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
}

void* ma_combo_create(void* client) {
    CRComboCtrl* c = new CRComboCtrl();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw takes the real (parent) path */
    c->OnResetState();                 /* ClassWizard defaults (fore=white, shadow=black) */
    return c;
}

void ma_combo_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp; if (!c) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: c->SetForeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case DISPID_CAPTION:   c->SetText(va_arg(ap, char*)); return;
        case DISPID_ENABLED:   c->SetEnabled((BOOL)va_arg(ap, int)); return;
        case 1: c->SetFontNum(va_arg(ap, long)); return;
        case 2: c->SetListboxLength(va_arg(ap, long)); return;
        case 3: c->SetCircularStyle((BOOL)va_arg(ap, int)); return;
        case 4: c->SetShadowColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case 5: c->SetEndFileNum(va_arg(ap, long)); return;
        case 6: c->SetFileNumMain(va_arg(ap, long)); return;
    }
}

void ma_combo_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        /* S181 (PO-57): same missing read-back as the edit control -- setprop has handled
           DISPID_CAPTION since bring-up, getprop never did, so GetCaption() was empty on every
           hosted type. Fixed here too because the combo has the same InternalGetText accessor
           and the same wrapper contract (a CString* in pvRet). RStatic/RButton/REdtBt have the
           same gap but no confirmed caller yet -- left alone rather than changed blind. */
        case DISPID_CAPTION:   if (pvRet) *(CString*)pvRet = c->InternalGetText(); return;
        case DISPID_FORECOLOR: *(OLE_COLOR*)pvRet = c->GetForeColor(); return;
        case DISPID_ENABLED:   *(BOOL*)pvRet = c->GetEnabled(); return;
        case 1: *(long*)pvRet = c->GetFontNum(); return;
        case 2: *(long*)pvRet = c->GetListboxLength(); return;
        case 3: *(BOOL*)pvRet = c->GetCircularStyle(); return;
        case 4: *(OLE_COLOR*)pvRet = c->GetShadowColor(); return;
        case 5: *(long*)pvRet = c->GetEndFileNum(); return;
        case 6: *(long*)pvRet = c->GetFileNumMain(); return;
    }
}

void ma_combo_invoke(void* ctrlp, int dispid, int vt, void* pvRet, va_list ap) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp; if (!c) return;
    (void)vt;
    switch (dispid) {
        case 7: { char* s = va_arg(ap, char*);
                  if (getenv("MA_TRACE_OLE")) fprintf(stderr, "[combo] AddString \"%s\"\n", s?s:"(null)");
                  c->AddString(s ? s : ""); return; }
        case 8:  if (pvRet) *(long*)pvRet = c->GetListbox(); return;
        case 9:  { long row = va_arg(ap, long); c->SetIndex(row);
                   /* S171: what a combo READS BACK after the dialog refills it is the only
                      evidence that a selection survived a close/reopen -- the caption is drawn,
                      never logged, and GetIndex is only called when the game feels like it. */
                   if (getenv("MA_TRACE_OLE")) fprintf(stderr, "[combo] SetIndex %ld/%d \"%s\"\n",
                                                       row, (int)c->m_list.GetCount(), (const char*)c->InternalGetText());
                   return; }
        case 10: if (pvRet) *(long*)pvRet = c->GetIndex(); return;
        case 11: c->Clear(); return;
        case 12: { long row = va_arg(ap, long); c->DeleteString(row); return; }
    }
}

/* A click landed on the combo: advance to the next item (circular wrap). Most front-end combos
   are circular-style; the dropdown-listbox path isn't wired, so cycling is the interaction. */
void ma_combo_click(void* ctrlp) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp; if (!c) return;
    int n = c->m_list.GetCount(); if (n <= 0) return;
    long i = (c->GetIndex() + 1) % n;
    c->SetIndex(i);
    c->FireTextChanged(c->InternalGetText());   /* mirror the game's change notification */
    if (getenv("MA_TRACE_OLE")) fprintf(stderr,"[combo] click -> index %ld/%d \"%s\"\n", i, n, (const char*)c->InternalGetText());
}

/* ---- F2: real dropdown list ------------------------------------------------
   Click-to-cycle (ma_combo_click) stays as a fallback for <=1-item combos; for a
   real list the router (ma_olecontrol.cpp) opens a dropdown panel below the box,
   draws it on top each idle (ma_combo_dropdown_draw), and routes a row click back
   through ma_combo_select. The panel reuses the combo's own global font + forecolor
   so it matches the box; rows are text-only over a solid panel (readable over the
   art background the combos sit on). */

int ma_combo_itemcount(void* ctrlp) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp; return c ? (int)c->m_list.GetCount() : 0;
}

int ma_combo_curindex(void* ctrlp) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp; return c ? (int)c->GetIndex() : 0;
}

int ma_combo_select(void* ctrlp, int row) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp; if (!c) return 0;
    int n = (int)c->m_list.GetCount(); if (n <= 0 || row < 0 || row >= n) return 0;
    if ((int)c->GetIndex() == row) return 0;            /* no change */
    c->SetIndex(row);
    c->FireTextChanged(c->InternalGetText());           /* same notification as the cycle path */
    if (getenv("MA_TRACE_OLE")) fprintf(stderr,"[combo] dropdown select -> %d/%d \"%s\"\n", row, n, (const char*)c->InternalGetText());
    return 1;
}

/* Draw the open dropdown panel. sx,sy,w,boxh are the combo box's absolute screen rect
   (viewport origin is 0 here — the router draws this after the per-control loop). The row
   height is reported back via out_rowh so the router can hit-test rows with matching geometry. */
void ma_combo_dropdown_draw(void* ctrlp, void* screenHdc, int sx, int sy, int w,
                            int boxh, int hoverRow, int* out_rowh) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp;
    if (out_rowh) *out_rowh = 0;
    if (!c || w <= 0) return;
    int n = (int)c->m_list.GetCount(); if (n <= 0) return;

    CDC dc; dc.m_hDC = (HDC)screenHdc;
    CFont* pfont = 0;
    if (c->GetParent())
        pfont = (CFont*)c->GetParent()->SendMessage(WM_GETGLOBALFONT, abs((int)c->m_FontNum), NULL);
    CFont* pOldFont = pfont ? dc.SelectObject(pfont) : 0;
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextAlign(TA_LEFT | TA_TOP);
    TEXTMETRIC tm; dc.GetTextMetrics(&tm);
    int rowh = tm.tmHeight + 4; if (rowh < 12) rowh = 12;
    if (out_rowh) *out_rowh = rowh;

    int panelTop = sy + boxh;
    int panelH = n * rowh;
    /* solid backing panel + 1px frame so the list is readable over the menu artwork */
    dc.FillSolidRect(sx, panelTop, w, panelH, RGB(12, 14, 32));
    dc.FillSolidRect(sx, panelTop, w, 1, RGB(120, 130, 170));
    dc.FillSolidRect(sx, panelTop + panelH - 1, w, 1, RGB(120, 130, 170));
    dc.FillSolidRect(sx, panelTop, 1, panelH, RGB(120, 130, 170));
    dc.FillSolidRect(sx + w - 1, panelTop, 1, panelH, RGB(120, 130, 170));

    int cur = (int)c->GetIndex();
    POSITION pos = c->m_list.GetHeadPosition();
    for (int i = 0; i < n && pos; i++) {
        char* s = c->m_list.GetNext(pos);
        int ry = panelTop + i * rowh;
        int highlit = (i == hoverRow) || (hoverRow < 0 && i == cur);
        if (highlit)
            dc.FillSolidRect(sx + 1, ry, w - 2, rowh, RGB(48, 56, 110));
        dc.SetTextColor(highlit ? RGB(255, 240, 120) : c->TranslateColor(c->GetForeColor()));
        CRect rc(sx, ry, sx + w, ry + rowh);
        dc.ExtTextOut(sx + 5, ry + 2, ETO_CLIPPED, rc, s ? s : "", s ? (int)strlen(s) : 0, NULL);
    }
    if (pOldFont) dc.SelectObject(pOldFont);
}

void ma_combo_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    CRComboCtrl* c = (CRComboCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0;
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
}
