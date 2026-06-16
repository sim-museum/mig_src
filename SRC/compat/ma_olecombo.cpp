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
#include "RComboC.h"          /* CRComboCtrl */

extern "C" {
void* ma_combo_create(void* client);
void  ma_combo_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_combo_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_combo_invoke(void* ctrl, int dispid, int vt, void* pvRet, va_list ap);
void  ma_combo_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
void  ma_combo_click(void* ctrl);
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
        case 9:  { long row = va_arg(ap, long); c->SetIndex(row); return; }
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
