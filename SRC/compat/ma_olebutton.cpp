/* ma_olebutton.cpp — host glue for the CRButtonCtrl OCX (the most-used front-end control:
 * menu/dialog buttons, ~28 per destination screen).
 *
 * Own TU ("olebutton" mode, -ISRC/RBUTTON) so the RBUTTON/RLISTBOX/RSTATIC project headers
 * don't collide. Stateless glue on a CRButtonCtrl* the router (ma_olecontrol.cpp) hands it.
 * Dispatch dispids = RBUTTONC.CPP map order: 1 UpdateCaption, 2 MovesParent, 3 FontNum,
 * 4 CloseButton, 5 TickButton, 6 ShowShadow, 7 ShadowColor, 8 String, 9 ResourceNumber,
 * 10 NormalFileNum, 11 PressedFileNum, ..., 14 Pressed, 15 Disabled; + stock Caption/ForeColor. */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "RButtonC.h"          /* CRButtonCtrl */

extern "C" {
void* ma_button_create(void* client);
void  ma_button_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_button_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_button_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
int   ma_button_click(void* ctrl, void* parentWnd, int lx, int ly);
void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
}

void* ma_button_create(void* client) {
    CRButtonCtrl* c = new CRButtonCtrl();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw takes the real (parent) path */
    c->OnResetState();
    return c;
}

void ma_button_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: c->SetForeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case DISPID_CAPTION:   c->SetText(va_arg(ap, char*)); return;
        case 3:  c->SetFontNum(va_arg(ap, long)); return;
        case 8:  { char* s = va_arg(ap, char*); c->SetString(s ? s : ""); return; }
        case 9:  c->SetResourceNumber(va_arg(ap, long)); return;
        case 10: { long fn = va_arg(ap, long); if (getenv("MA_TRACE_OLE")) { static int n=0; if(n++<12) fprintf(stderr,"[btn] SetNormalFileNum=%ld (0x%lx)\n", fn, fn); } c->SetNormalFileNum(fn); return; }
        case 11: c->SetPressedFileNum(va_arg(ap, long)); return;
        case 14: c->SetPressed(va_arg(ap, int)); return;
        case 15: c->SetDisabled(va_arg(ap, int)); return;
    }
}

void ma_button_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: *(OLE_COLOR*)pvRet = c->GetForeColor(); return;
        case 3:  *(long*)pvRet = c->GetFontNum(); return;
        case 14: *(BOOL*)pvRet = c->GetPressed(); return;
        case 15: *(BOOL*)pvRet = c->GetDisabled(); return;
    }
}

extern "C" void ma_button_set_filenum(void* ctrlp, long fn) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (c) c->SetNormalFileNum(fn);
}
void ma_button_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0;
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
}

/* click routing for buttons (FireClicked/OK/Cancel -> dialog event sink) is a follow-on;
   rendering is wired first. */
int ma_button_click(void* ctrlp, void* parentWnd, int lx, int ly) {
    (void)ctrlp; (void)parentWnd; (void)lx; (void)ly; return 0;
}
