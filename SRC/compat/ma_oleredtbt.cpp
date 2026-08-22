/* ma_oleredtbt.cpp — host glue for the CREdtBtCtrl OCX (REDTBT: the "edit button" —
 * a caption + arrow pushbutton, e.g. the prefs-Controls "Calibrate" button, S57/I2).
 *
 * Own TU ("oleredtbt" mode, -ISRC/REDTBT) so the REDTBT project headers don't collide
 * with the other OCX projects'. Stateless glue on a CREdtBtCtrl* the router
 * (ma_olecontrol.cpp) hands it — same shape as ma_olebutton.cpp. Dispatch dispids =
 * REDTBTC.CPP map order: 1 FontNum, 2 DragAndDropID, 3 SetPictureFileNum; + stock
 * Caption/ForeColor (negative). The caption reaches captiontext via the compat
 * COleControl::SetText -> OnTextChanged (SController::OnInitDialog does
 * calib->SetCaption(RESSTRING(CALIBRATE))). Clicks fire event dispid 1 (Clicked)
 * from ma_ole_click, reaching ON_EVENT(SController, IDC_CALIB, 1, OnClickedCalib). */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "REdtBtC.h"           /* CREdtBtCtrl */

extern "C" {
void* ma_edtbt_create(void* client);
void  ma_edtbt_set_string(void* ctrl, const char* s);
void  ma_edtbt_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_edtbt_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_edtbt_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
}

void* ma_edtbt_create(void* client) {
    CREdtBtCtrl* c = new CREdtBtCtrl();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw takes the real (parent) path */
    c->OnResetState();
    return c;
}

/* apply a design-time caption from RT_DLGINIT (usually none — REdtBt captions are set
   at runtime via SetCaption) */
void ma_edtbt_set_string(void* ctrlp, const char* s) {
    CREdtBtCtrl* c = (CREdtBtCtrl*)ctrlp; if (!c) return;
    c->SetText(s ? s : "");
}

void ma_edtbt_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    CREdtBtCtrl* c = (CREdtBtCtrl*)ctrlp; if (!c) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: c->SetForeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case DISPID_CAPTION:   { char* s = va_arg(ap, char*);
                                 /* S171: the TASKS duty field ("F84 (1/4)" / "Off Duty") is this
                                    caption, and it is the only readout that says WHICH squadron a
                                    duty slot holds. It is drawn, never logged, so without this a
                                    gate can see that a flight count moved but not that the flak
                                    slot got the squadron the walkthrough names. */
                                 if (getenv("MA_TRACE_OLE"))
                                     fprintf(stderr, "[edtbt] caption ctrl=%p \"%s\"\n", ctrlp, s ? s : "");
                                 c->SetText(s); return; }
        case 1: c->SetFontNum(va_arg(ap, long)); return;
        case 2: c->SetDragAndDropID(va_arg(ap, long)); return;
        case 3: c->SetPictureFileNum(va_arg(ap, long)); return;
    }
}

void ma_edtbt_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    CREdtBtCtrl* c = (CREdtBtCtrl*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: *(OLE_COLOR*)pvRet = c->GetForeColor(); return;
        case 1: *(long*)pvRet = c->GetFontNum(); return;
        case 2: *(long*)pvRet = c->GetDragAndDropID(); return;
    }
}

void ma_edtbt_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    CREdtBtCtrl* c = (CREdtBtCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0;
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
}
