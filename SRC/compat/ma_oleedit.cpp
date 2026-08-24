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
void  ma_edit_char(void* ctrl, int ch);
void  ma_edit_key(void* ctrl, int vk);
const char* ma_edit_text(void* ctrl);
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

/* S197: set the control's text directly. CWnd::SetWindowTextA used to be a no-op stub that
   returned TRUE, so CWaveInsert::OnInitDialog's
       edit->SetWindowText(CSprintf("%02i:%02i", ...))
   never arrived and the Time Over Target field kept a stale "Player". Same destination as the
   DISPID_CAPTION property set below, so both ways of writing the text agree. */
extern "C" void ma_edit_set_text(void* ctrlp, const char* s) {
    CREditCtrl* c = (CREditCtrl*)ctrlp;
    if (c) c->SetText(s ? s : "");
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
        /* S181 (PO-57): READ BACK the typed text. setprop has handled DISPID_CAPTION since
           bring-up and getprop never did -- on ANY hosted control type -- so `GetCaption()`
           returned an empty CString everywhere. That is why the campaign-start name dialog
           lost the player's name: `CCareer` does
               buffer = editbox->GetCaption();
               if (buffer.GetLength() <= PLAYERNAMELEN-1) strcpy(MMC.PlayerName, buffer);
           and an EMPTY buffer passes that length test, so the failed read-back did not fall
           through to a default -- it overwrote the name with nothing. The PO typed "Test"
           every time and the roster showed a blank seat.
           The wrapper passes a CString* (REDIT.CPP:92 GetProperty(DISPID_CAPTION, VT_BSTR,
           &result)), so assign into it. */
        case DISPID_CAPTION:   if (pvRet) *(CString*)pvRet = c->InternalGetText(); return;
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

/* ---- S121 (PO-16): keyboard entry ------------------------------------------------------------
 * The game's own CREditCtrl already implements everything -- OnChar inserts, OnKeyDown handles
 * backspace/arrows, it tracks m_curPos, draws a caret and fires TextChanged. Nothing here needs to
 * reimplement an edit box; the port only ever failed to DELIVER the keystrokes. These are the
 * delivery points, called from the OCX router for whichever control holds focus.
 */
/* The control's own OnChar/OnKeyDown cannot be called here: they run in an MFC message context
 * this port does not provide -- they measure through CWnd::GetDC(), invalidate, and drive a caret
 * timer, and each of those is a separate null in a windowless host (two were fixed on the way to
 * finding this out, both worth keeping: an empty CString measured as strlen(NULL), and an unknown
 * DC dereferenced in ma_gdi_get_text_extent).
 *
 * So the host supplies the editing behaviour, exactly as it already does for the other hosted
 * controls -- ma_ole_click CYCLES a combo rather than invoking CRComboCtrl::OnLButtonDown. The
 * text itself still lives in the game's control, so its own OnDraw renders it and the dialog reads
 * it back through the normal property path.
 */
void ma_edit_char(void* ctrlp, int ch)
{
    CREditCtrl* c = (CREditCtrl*)ctrlp;
    if (!c || ch < 32 || ch > 126) return;
    CString t = c->InternalGetText();
    if (t.GetLength() >= 63) return;            /* PLAYERNAMELEN-ish guard */
    char add[2]; add[0] = (char)ch; add[1] = 0;
    t += add;
    c->SetText(t);
}

void ma_edit_key(void* ctrlp, int vk)
{
    CREditCtrl* c = (CREditCtrl*)ctrlp;
    if (!c) return;
    if (vk == 8) {                              /* VK_BACK */
        CString t = c->InternalGetText();
        int n = t.GetLength();
        if (n > 0) c->SetText(t.Left(n - 1));
    }
}

const char* ma_edit_text(void* ctrlp)
{
    CREditCtrl* c = (CREditCtrl*)ctrlp;
    if (!c) return "";
    static CString t;
    t = c->InternalGetText();
    return (const char*)t;
}
