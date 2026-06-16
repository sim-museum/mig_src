/* ma_olestatic.cpp — host glue for the CRStaticCtrl OCX (the dominant front-end text
 * control: m_IDC_RSTATICCTRL / SDETAIL / SCLAIMS).
 *
 * Compiled in its own TU ("olestatic" mode, -ISRC/RSTATIC) so the RSTATIC and RLISTBOX
 * project headers (each pulls its own resource.h / app class) never collide. The shared
 * (client -> control) registry lives in ma_olecontrol.cpp; this file is stateless glue
 * that operates on a CRStaticCtrl* the router hands it. Dispatch dispids follow the
 * RSTATICC.CPP map order: 1 UpdateCaption, 2 FontNum, 3 String, 4 ResourceNumber,
 * 5 PictureFileNum, 6 Central, 7 ShadowColor; + stock Caption/ForeColor (negative). */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "RStaticC.h"          /* CRStaticCtrl */

extern "C" {
void* ma_static_create(void* client);
void  ma_static_set_string(void* ctrl, const char* s);
void  ma_static_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_static_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_static_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
}

void* ma_static_create(void* client) {
    CRStaticCtrl* c = new CRStaticCtrl();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw takes the real (parent) path */
    c->OnResetState();                 /* ClassWizard defaults */
    return c;
}

/* set the label text directly (from the RT_DLGINIT-parsed caption) */
void ma_static_set_string(void* ctrlp, const char* s) {
    CRStaticCtrl* c = (CRStaticCtrl*)ctrlp; if (!c) return;
    c->SetString(s ? s : "");
}

void ma_static_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    CRStaticCtrl* c = (CRStaticCtrl*)ctrlp; if (!c) return;
    (void)vt;
    if (getenv("MA_TRACE_STATIC")) { static int n=0; if(n++<40)
        fprintf(stderr,"[static_set] ctrl=%p dispid=%d vt=%d\n", ctrlp, dispid, vt); }
    switch (dispid) {
        case DISPID_FORECOLOR: c->SetForeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case DISPID_CAPTION:   c->SetText(va_arg(ap, char*)); return;
        case 2: c->SetFontNum(va_arg(ap, long)); return;
        case 3: { char* s = va_arg(ap, char*); c->SetString(s ? s : ""); return; }   /* String */
        case 4: c->SetResourceNumber(va_arg(ap, long)); return;
        case 5: c->SetPictureFileNum(va_arg(ap, long)); return;
        case 6: c->SetCentral(va_arg(ap, int)); return;
        case 7: c->SetShadowColor((unsigned long)va_arg(ap, unsigned long)); return;
    }
}

void ma_static_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    CRStaticCtrl* c = (CRStaticCtrl*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: *(OLE_COLOR*)pvRet = c->GetForeColor(); return;
        case 2: *(long*)pvRet = c->GetFontNum(); return;
        case 4: *(long*)pvRet = c->GetResourceNumber(); return;
        case 5: *(long*)pvRet = c->GetPictureFileNum(); return;
        case 6: *(BOOL*)pvRet = c->GetCentral(); return;
    }
}

void ma_static_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    CRStaticCtrl* c = (CRStaticCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;          /* CRStaticCtrl::OnDraw uses GetParent() */
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0;
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
}
