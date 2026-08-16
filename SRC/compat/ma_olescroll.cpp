/* ma_olescroll.cpp — host glue for the CRScrlBarCtrl OCX (S140).
 *
 * WHY. A CLSID census of the campaign map (S136) found 8 x 0x505aee46 -- RScrlBar -- with no
 * entry in the router's table. So every scrollable campaign dialog (Intelligence, Squadrons,
 * Bases, the mission log) listed more rows than it could show and offered no way to reach them:
 * the bar the game placed there was simply never created, never drawn, and never clickable.
 * RDIALOG.CPP has carried the note "WM_ACTIVEXSCROLL -> OnActiveXScroll (RScrlBar not hosted
 * yet)" since S83; this closes both halves.
 *
 * Own TU ("olescroll" mode, -ISRC/RSCRLBAR) so the OCX projects' headers do not collide.
 * Dispatch dispids follow RSCRLBRC.CPP map order: 1 MinValue, 2 MaxValue, 3 StepSize,
 * 4 HorzAlign, 5 parentPointer, 6 PageSize, 7 ScrollPos, 8 UseMessagesInsteadOfEvents,
 * 9 FileNumOffset, 10 Move.
 *
 * The CLICK calls the control's OWN OnLButtonDown. That is unusual for this port -- the host
 * normally supplies the behaviour (combo cycling, edit typing, radio selection) because the
 * genuine handler reaches for an MFC message context we do not provide. Here it does not: the
 * handler needs GetClientRect (which the host fills from the draw rect), SetCapture, SetTimer and
 * RedrawWindow, and the last three are safe no-ops in compat. So the arrow / page / thumb
 * arithmetic stays in the one place that already had it, and the click walk cannot drift from the
 * paint walk because they are literally the same code.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "RScrlBrC.h"          /* CRScrlBarCtrl */

extern "C" {
void* ma_scroll_create(void* client);
void  ma_scroll_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_scroll_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_scroll_invoke(void* ctrl, int dispid, int vtRet, void* pvRet, va_list ap);
int   ma_scroll_click(void* ctrl, void* parentWnd, int lx, int ly);
void  ma_scroll_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
void  ma_scroll_rect(void* ctrl, int* x, int* y, int* w, int* h);
int   ma_scroll_pos(void* ctrl);
int   ma_scroll_is_horz(void* ctrl);
void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
void  ma_gdi_set_clip(void*, int, int, int, int, int*);
void  ma_gdi_restore_clip(void*, const int*);
}

void* ma_scroll_create(void* client) {
    CRScrlBarCtrl* c = new CRScrlBarCtrl();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw takes the real (parent) path */
    c->OnResetState();
    return c;
}

void ma_scroll_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    CRScrlBarCtrl* c = (CRScrlBarCtrl*)ctrlp; if (!c) return;
    (void)vt;
    switch (dispid) {
        case 1: c->SetMinValue(va_arg(ap, long)); return;
        case 2: c->SetMaxValue(va_arg(ap, long)); return;
        case 3: c->SetStepSize(va_arg(ap, long)); return;
        case 4: c->SetHorzAlign(va_arg(ap, int)); return;
        /* parentPointer is a CWnd* passed as a long -- this is how the bar learns which dialog
           to send WM_ACTIVEXSCROLL to (DoFireScroll). Without it the notification goes nowhere
           and the list never moves, however well the bar draws. */
        case 5: c->SetParentPointer(va_arg(ap, long)); return;
        case 6: c->SetPageSize(va_arg(ap, long)); return;
        case 7: c->SetScrollPos(va_arg(ap, long)); return;
        case 8: c->SetUseMessagesInsteadOfEvents(va_arg(ap, int)); return;
        case 9: c->SetFileNumOffset(va_arg(ap, long)); return;
    }
}

void ma_scroll_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    CRScrlBarCtrl* c = (CRScrlBarCtrl*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        case 1: *(long*)pvRet = c->GetMinValue(); return;
        case 2: *(long*)pvRet = c->GetMaxValue(); return;
        case 3: *(long*)pvRet = c->GetStepSize(); return;
        case 4: *(BOOL*)pvRet = c->GetHorzAlign(); return;
        case 5: *(long*)pvRet = c->GetParentPointer(); return;
        case 6: *(long*)pvRet = c->GetPageSize(); return;
        case 7: *(long*)pvRet = c->GetScrollPos(); return;
        case 8: *(BOOL*)pvRet = c->GetUseMessagesInsteadOfEvents(); return;
        case 9: *(long*)pvRet = c->GetFileNumOffset(); return;
    }
}

void ma_scroll_invoke(void* ctrlp, int dispid, int vtRet, void* pvRet, va_list ap) {
    CRScrlBarCtrl* c = (CRScrlBarCtrl*)ctrlp; if (!c) return;
    (void)vtRet; (void)pvRet;
    if (dispid == 10) {                       /* Move(left, top, right, bottom) */
        long l = va_arg(ap, long), t = va_arg(ap, long);
        long r = va_arg(ap, long), b = va_arg(ap, long);
        c->Move(l, t, r, b);                  /* -> SetRectInContainer, which records the rect */
        if (getenv("MA_TRACE_SCROLL"))
            fprintf(stderr, "[scroll] Move ctrl=%p -> (%ld,%ld)-(%ld,%ld)\n", ctrlp, l, t, r, b);
    }
}

/* The bar is NOT a template control -- CRListBoxCtrl::UpdateScrollBar creates it at runtime and
   places it with Move(). The draw walk reads the CLIENT CWnd's rect, not the control's, so the
   router mirrors this back after a Move; without it every bar had a zero rect and the draw path's
   `if (w<=0||h<=0) return` silently dropped all 26 of them. Same mirror the listbox path does
   (MA_SYNC_RECT) and for the same reason. */
void ma_scroll_rect(void* ctrlp, int* x, int* y, int* w, int* h) {
    CRScrlBarCtrl* c = (CRScrlBarCtrl*)ctrlp; if (!c) return;
    if (x) *x = c->m_maX; if (y) *y = c->m_maY;
    if (w) *w = c->m_maW; if (h) *h = c->m_maH;
}

int ma_scroll_pos(void* ctrlp) {
    CRScrlBarCtrl* c = (CRScrlBarCtrl*)ctrlp; return c ? (int)c->GetScrollPos() : 0;
}
int ma_scroll_is_horz(void* ctrlp) {
    CRScrlBarCtrl* c = (CRScrlBarCtrl*)ctrlp; return c ? (c->GetHorzAlign() ? 1 : 0) : 0;
}

int ma_scroll_click(void* ctrlp, void* parentWnd, int lx, int ly) {
    CRScrlBarCtrl* c = (CRScrlBarCtrl*)ctrlp; if (!c) return 0;
    c->m_maParent = (CWnd*)parentWnd;
    long before = c->GetScrollPos();
    c->OnLButtonDown(0, CPoint(lx, ly));     /* the genuine arrow / page / thumb arithmetic */
    c->OnLButtonUp(0, CPoint(lx, ly));       /* releases capture and clears the drag offset */
    if (getenv("MA_TRACE_SCROLL"))
        fprintf(stderr, "[scroll] click ctrl=%p local(%d,%d) pos %ld -> %ld (min %ld max %ld page %ld)\n",
                ctrlp, lx, ly, before, c->GetScrollPos(),
                c->GetMinValue(), c->GetMaxValue(), c->GetPageSize());
    return 1;                                 /* the bar took the click either way */
}

void ma_scroll_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    CRScrlBarCtrl* c = (CRScrlBarCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0, clipSaved[5];
    ma_gdi_set_clip(screenHdc, sx, sy, sx + w, sy + h, clipSaved);
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
    ma_gdi_restore_clip(screenHdc, clipSaved);
}
