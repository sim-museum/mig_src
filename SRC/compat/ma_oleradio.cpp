/* ma_oleradio.cpp — host glue for the CRRadioCtrl OCX (S136, PO-28).
 *
 * WHY. The PO, on the campaign map: "a lot of map dialogs lack button text". The D.I.S.
 * dialog draws three blank bars where its intelligence filters should be. They are not
 * blank buttons -- they are RRadio controls, and RRadio was simply not in the router's
 * CLSID table, so nothing hosted them and nothing drew their captions. `CDIS::OnInitDialog`
 * has always populated them:
 *
 *     pradio=GETDLGITEM(IDC_RRADIO_INTELLTYPE);
 *     string.LoadString(IDS_TARGET);  pradio->AddButton(string);
 *     string.LoadString(IDS_GENERAL); pradio->AddButton(string);
 *
 * and every one of those AddButton calls went to a control that did not exist.
 *
 * Own TU ("oleradio" mode, -ISRC/RRADIO) so the OCX projects' headers do not collide, and
 * stateless glue over a CRRadioCtrl* the router (ma_olecontrol.cpp) hands it -- the same
 * shape as ma_olebutton/ma_olestatic. Dispatch dispids follow RRADIOC.CPP map order:
 * 1 FontNum, 2 Cols, 3 CurrentSelection, 4 ColumnWidth, 5 AddButton, 6 Clear; + stock
 * ForeColor. Confirmed against the client wrapper (RRADIO.CPP), which calls
 * InvokeHelper(0x5) for AddButton and InvokeHelper(0x6) for Clear.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RRadioC.h"           /* CRRadioCtrl */

extern "C" {
void* ma_radio_create(void* client);
void  ma_radio_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_radio_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_radio_invoke(void* ctrl, int dispid, int vtRet, void* pvRet, va_list ap);
int   ma_radio_click(void* ctrl, int lx, int ly, int* outSel);
void  ma_radio_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
void  ma_gdi_set_clip(void*, int, int, int, int, int*);
void  ma_gdi_restore_clip(void*, const int*);
}

void* ma_radio_create(void* client) {
    CRRadioCtrl* c = new CRRadioCtrl();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw takes the real (parent) path */
    c->OnResetState();
    return c;
}

void ma_radio_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    CRRadioCtrl* c = (CRRadioCtrl*)ctrlp; if (!c) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: c->SetForeColor((OLE_COLOR)va_arg(ap, unsigned long)); return;
        case 1: c->SetFontNum(va_arg(ap, long)); return;
        case 2: c->SetCols(va_arg(ap, long)); return;
        case 3: c->SetCurrentSelection(va_arg(ap, long)); return;
        case 4: c->SetColumnWidth(va_arg(ap, long)); return;
    }
}

void ma_radio_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    CRRadioCtrl* c = (CRRadioCtrl*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        case DISPID_FORECOLOR: *(OLE_COLOR*)pvRet = c->GetForeColor(); return;
        case 1: *(long*)pvRet = c->GetFontNum(); return;
        case 2: *(long*)pvRet = c->GetCols(); return;
        case 3: *(long*)pvRet = c->GetCurrentSelection(); return;
        case 4: *(long*)pvRet = c->GetColumnWidth(); return;
    }
}

void ma_radio_invoke(void* ctrlp, int dispid, int vtRet, void* pvRet, va_list ap) {
    CRRadioCtrl* c = (CRRadioCtrl*)ctrlp; if (!c) return;
    (void)vtRet; (void)pvRet;
    switch (dispid) {
        case 5: { char* s = va_arg(ap, char*);      /* AddButton(BSTR) — BSTR is char* here */
                  if (getenv("MA_TRACE_RADIO"))
                      fprintf(stderr, "[radio] AddButton ctrl=%p \"%s\"\n", ctrlp, s ? s : "(null)");
                  c->AddButton(s ? s : ""); return; }
        case 6: c->Clear(); return;
    }
}

/* Geometry recorded by the LAST PAINT, so the click walk cannot drift from the paint walk
   (S82's rule, relearned here rather than rediscovered). CRRadioCtrl lays its buttons out on
   a grid whose column stride is m_ColumnWidth*tm.tmAveCharWidth and whose row stride is
   tm.tmHeight+2 -- both depend on the font the PARENT hands the control at draw time, so a
   hit-test that recomputed them from the control rect would be a second, divergent source of
   truth. */
struct MaRadioGeom { int colStride, rowStride, cols, count; };
static MaRadioGeom* geom_for(void* ctrl, int create) {
    static void*        keys[64];
    static MaRadioGeom  vals[64];
    static int          n = 0;
    for (int i = 0; i < n; i++) if (keys[i] == ctrl) return &vals[i];
    if (!create || n >= 64) return 0;
    keys[n] = ctrl; vals[n].colStride = vals[n].rowStride = 0; vals[n].cols = 1; vals[n].count = 0;
    return &vals[n++];
}

/* Hit-test a click against the button grid and report the new selection. The control has its
   own OnLButtonDown, but it runs in an MFC message context this port does not provide (GetDC,
   SetCapture, InvalidateControl, FireSelected through the window plumbing) -- the same reason
   the host supplies combo cycling and edit-box typing rather than calling the control's
   handler (S121). */
int ma_radio_click(void* ctrlp, int lx, int ly, int* outSel) {
    CRRadioCtrl* c = (CRRadioCtrl*)ctrlp; if (!c) return 0;
    MaRadioGeom* g = geom_for(ctrlp, 0);
    if (!g || g->count <= 0 || g->colStride <= 0 || g->rowStride <= 0) return 0;
    int col = lx / g->colStride, row = ly / g->rowStride;
    if (col < 0 || col >= g->cols || row < 0) return 0;
    long sel = (long)(row * g->cols + col);
    if (sel < 0 || sel >= g->count) return 0;
    if (getenv("MA_TRACE_RADIO"))
        fprintf(stderr, "[radio] click ctrl=%p local(%d,%d) -> row %d col %d -> selection %ld of %d\n",
                ctrlp, lx, ly, row, col, sel, g->count);
    c->SetCurrentSelection(sel);
    if (outSel) *outSel = (int)sel;
    return 1;
}

void ma_radio_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    CRRadioCtrl* c = (CRRadioCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    /* Record the grid the paint is about to use, with the same font the control will draw
       with -- the parent's global font, which is what OnDraw asks for. */
    { CWnd* par = (CWnd*)parentWnd;
      if (par) dc.SelectObject((CFont*)par->SendMessage(WM_GETGLOBALFONT, abs(c->m_FontNum), 0));
      TEXTMETRIC tm; memset(&tm, 0, sizeof(tm)); dc.GetTextMetrics(&tm);
      MaRadioGeom* g = geom_for(ctrlp, 1);
      if (g) {
          g->cols      = (int)(c->m_Cols > 0 ? c->m_Cols : 1);
          g->colStride = (int)(c->m_ColumnWidth * tm.tmAveCharWidth);
          g->rowStride = (int)(tm.tmHeight + 2);
          g->count     = (int)c->m_list.GetCount();
          if (g->colStride <= 0) g->colStride = w > 0 ? w / g->cols : 0;
      } }
    int ox = 0, oy = 0, clipSaved[5];
    ma_gdi_set_clip(screenHdc, sx, sy, sx + w, sy + h, clipSaved);
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
    ma_gdi_restore_clip(screenHdc, clipSaved);
}
