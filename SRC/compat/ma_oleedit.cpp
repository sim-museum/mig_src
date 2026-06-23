/* ma_oleedit.cpp — host glue for the CREditCtrl OCX (REDIT): the single-line text-entry
 * control used by the load/save dialog (IDC_SAVENAME, the savename field) and other
 * name-entry screens (visitorsbook, paintshop variant name, ...).
 *
 * Compiled in its own TU ("oleedit" mode, -ISRC/REDIT) so the REDIT project headers
 * (its own resource.h / app class) never collide with the other OCX projects. The shared
 * (client -> control) registry lives in ma_olecontrol.cpp; this file is stateless glue
 * operating on a CREditCtrl* the router hands it. Dispatch dispids follow REDITCTL.CPP's
 * map order: 1 FontNum; + stock ForeColor/Caption/Enabled (negative dispids). Text is
 * stored via SetText (-> m_maText), which CREditCtrl::OnDraw renders via InternalGetText. */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "REditCtl.h"          /* CREditCtrl */

extern "C" {
void* ma_edit_create(void* client);
void  ma_edit_set_string(void* ctrl, const char* s);
void  ma_edit_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_edit_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_edit_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
}

void* ma_edit_create(void* client) {
    CREditCtrl* c = new CREditCtrl();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw takes the real (parent) path */
    c->OnResetState();                 /* ClassWizard defaults */
    return c;
}

/* set the entry text directly (e.g. a default savename from the game) */
void ma_edit_set_string(void* ctrlp, const char* s) {
    CREditCtrl* c = (CREditCtrl*)ctrlp; if (!c) return;
    c->SetText(s ? s : "");
}

void ma_edit_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    CREditCtrl* c = (CREditCtrl*)ctrlp; if (!c) return;
    (void)vt;
    if (getenv("MA_TRACE_EDIT")) { static int n=0; if(n++<40)
        fprintf(stderr,"[edit_set] ctrl=%p dispid=%d vt=%d\n", ctrlp, dispid, vt); }
    switch (dispid) {
        case DISPID_FORECOLOR: c->SetForeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case DISPID_CAPTION:   c->SetText(va_arg(ap, char*)); return;
        case DISPID_ENABLED:   c->SetEnabled((BOOL)va_arg(ap, int)); return;
        case 1: c->SetFontNum(va_arg(ap, long)); return;
    }
}

void ma_edit_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    CREditCtrl* c = (CREditCtrl*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: *(OLE_COLOR*)pvRet = c->GetForeColor(); return;
        case DISPID_ENABLED:   *(BOOL*)pvRet = c->GetEnabled(); return;
        case 1: *(long*)pvRet = c->GetFontNum(); return;
    }
}

void ma_edit_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    CREditCtrl* c = (CREditCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;          /* CREditCtrl::OnDraw uses GetParent() */
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0;
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
}
