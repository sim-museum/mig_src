/* ma_olespin.cpp — host glue for the CRSpinButCtrl OCX (S170).
 *
 * WHY. RSpinBut was the last R* control type with no host in this port: RSPINBUT.CPP (the
 * wrapper) has been compiled since bring-up, so every InvokeHelper on one of these was a silent
 * no-op, and the control itself was never created, never drawn and never clickable.
 *
 * It is not a corner. The PO's Wonju walkthrough, step 8, is:
 *     "add a third flight -- either via the Squadron slot's FLIGHTS SPIN-BOX, or by clicking
 *      the 3rd flight slot (Off Duty) and choosing the 1000lb bombs payload"
 * so EPIC K walks into it, and the campaign's TASKS/Profile dialogs are built from these.
 *
 * Found by the sibling port, not by us: BoB S197 fixed its own spin buttons (they were hosted
 * but took no clicks) and its note pointed out that MA hosts no spin type AT ALL -- same outcome
 * for the player, opposite cause. BoB's SRC/RSPINBUT/bob_ole_rspinbut.cpp is the reference, and
 * its S142 header documents the two traps below, both of which this control has here too.
 *
 * Own TU ("olespin" mode, -ISRC/RSPINBUT) so the OCX projects' headers do not collide -- the same
 * arrangement every other ma_ole*.cpp uses.
 *
 * Dispids from the WRAPPER (SRC/MFC/RSPINBUT.CPP), confirmed against RSPINBTC.H:
 *   1 RepeatDelay  2 Index  3 FontNum  4 CurrentValue   (properties)
 *   5 AddString(BSTR)  6 DeleteString(I4)  7 Clear
 *   8 SetPriceOption(I4,I4,I4)->BOOL   9 SetValueOption(I4,I4,I4)->BOOL
 *   10 SetPassWord(BOOL)  11 SetSearchValueOption(...)->BOOL  12 SetPlayerNegPriceOption(...)->BOOL
 *
 * THE CLICK drives the control's OWN OnLButtonDown, as the scroll bar host does (S140) and for
 * the same reason: the handler only needs GetClientRect, SetCapture, SetTimer and RedrawWindow --
 * and in this port GetClientRect ALREADY reports the control's own rect (compat CWnd carries
 * m_maX/m_maY/m_maW/m_maH), while the other three are safe no-ops. So the arrow-strip test and the
 * up/down split stay in the one place that already has them.
 *   (BoB could not do that until S197: its compat GetClientRect answered with the whole SDL
 *    window, so the handler's `if (point.x < rect.right-15) return;` rejected every click. Same
 *    handler, same port family, opposite outcome -- because MA has the per-window rect and BoB
 *    did not. See §8-BoB197.)
 *
 * TWO CRSpinBut-SPECIFIC TRAPS (RSPINBTC.CPP), both verified present in this tree:
 *  (1) m_bDrawing is a STATIC (class-wide) reentrancy flag: OnDraw does
 *          if (m_bDrawing || !m_hWnd) return;  m_bDrawing = TRUE;
 *      and the clear at the end sits on a path a failed art lookup can skip. One spinner that
 *      bails therefore latches the flag for EVERY spinner on EVERY later frame. With a grid of
 *      them that is the difference between a table and one lonely box. Cleared here before each
 *      draw, host-side, so it cannot latch even if the art lookup fails. No game-code edit.
 *  (2) It has no m_FirstSweep, so unlike its siblings it re-blits the parent artwork on every
 *      draw. Harmless here -- DrawBitmap fetches art via WM_GETFILE and no-ops unless it gets a
 *      'BM' blob back -- but it is why this host clips to the control's rect.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "RSPINBTC.H"          /* CRSpinButCtrl */

/* S170: CRSpinButCtrl declares its dispatch methods and the two mouse handlers under
 * `protected:` — unlike RCombo/RListBox, whose headers leave that block public because
 * BEGIN_OLEFACTORY reopens `public:` and they never re-specify. On Windows the generated
 * dispatch map reaches them from inside the class; the port has no dispatch map, so the
 * router calls them directly. A thin derived accessor republishes exactly the members the
 * host needs — no game source is edited. */
struct MaSpinBut : public CRSpinButCtrl {
	using CRSpinButCtrl::OnLButtonDown;
	using CRSpinButCtrl::OnLButtonUp;
	using CRSpinButCtrl::GetRepeatDelay;   using CRSpinButCtrl::SetRepeatDelay;
	using CRSpinButCtrl::GetIndex;         using CRSpinButCtrl::SetIndex;
	using CRSpinButCtrl::GetFontNum;       using CRSpinButCtrl::SetFontNum;
	using CRSpinButCtrl::GetCurrentValue;  using CRSpinButCtrl::SetCurrentValue;
	using CRSpinButCtrl::AddString;        using CRSpinButCtrl::DeleteString;
	using CRSpinButCtrl::Clear;            using CRSpinButCtrl::SetPassWord;
	using CRSpinButCtrl::SetPriceOption;   using CRSpinButCtrl::SetValueOption;
	using CRSpinButCtrl::SetSearchValueOption;
	using CRSpinButCtrl::SetPlayerNegPriceOption;
};

extern "C" {
void* ma_spin_create(void* client);
void  ma_spin_setprop(void* ctrl, int dispid, int vt, va_list ap);
void  ma_spin_getprop(void* ctrl, int dispid, int vt, void* pvRet);
void  ma_spin_invoke(void* ctrl, int dispid, int vtRet, void* pvRet, va_list ap);
int   ma_spin_click(void* ctrl, void* parentWnd, int lx, int ly);
int   ma_spin_arrow_point(void* ctrl, int down, int* lx, int* ly);
void  ma_spin_draw(void* ctrl, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h);
int   ma_spin_index(void* ctrl);
void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
void  ma_gdi_set_clip(void*, int, int, int, int, int*);
void  ma_gdi_restore_clip(void*, const int*);
}

void* ma_spin_create(void* client) {
    MaSpinBut* c = new MaSpinBut();
    c->m_hWnd = (HWND)client;          /* non-null: OnDraw bails on !m_hWnd (and skips the arrows) */
    c->OnResetState();
    return c;
}

void ma_spin_setprop(void* ctrlp, int dispid, int vt, va_list ap) {
    MaSpinBut* c = (MaSpinBut*)ctrlp; if (!c) return;
    (void)vt;
    switch (dispid) {
        case 1: c->SetRepeatDelay(va_arg(ap, long));  return;
        case 2: c->SetIndex(va_arg(ap, long));        return;
        case 3: c->SetFontNum(va_arg(ap, long));      return;
        case 4: c->SetCurrentValue(va_arg(ap, long)); return;
        default: return;
    }
}

void ma_spin_getprop(void* ctrlp, int dispid, int vt, void* pvRet) {
    MaSpinBut* c = (MaSpinBut*)ctrlp; if (!c || !pvRet) return;
    (void)vt;
    switch (dispid) {
        case 1: *(long*)pvRet = c->GetRepeatDelay();  return;
        case 2: *(long*)pvRet = c->GetIndex();        return;
        case 3: *(long*)pvRet = c->GetFontNum();      return;
        case 4: *(long*)pvRet = c->GetCurrentValue(); return;
        default: return;
    }
}

void ma_spin_invoke(void* ctrlp, int dispid, int vtRet, void* pvRet, va_list ap) {
    MaSpinBut* c = (MaSpinBut*)ctrlp; if (!c) return;
    (void)vtRet;
    switch (dispid) {
        case 5: { const char* s = va_arg(ap, const char*); if (s) c->AddString(s);
                  if (getenv("MA_TRACE_SPIN")) fprintf(stderr, "[spin] AddString ctrl=%p \"%s\" -> count=%d\n",
                                                       ctrlp, s ? s : "(null)", (int)c->m_list.GetCount());
                  return; }
        case 6: { long i = va_arg(ap, long); c->DeleteString(i); return; }
        case 7: c->Clear(); return;
        /* The three *Option setters are how the game gives a spinner its RANGE. They matter:
           OnTimer's guards are `m_index >= 1` going down and `m_index <= m_list.GetCount()-2`
           going up, so a spinner whose range was never set cannot move in either direction --
           which looks exactly like "the click does nothing". */
        case 8: { long mn = va_arg(ap, long), mx = va_arg(ap, long), cu = va_arg(ap, long);
                  BOOL r = c->SetPriceOption(mn, mx, cu); if (pvRet) *(BOOL*)pvRet = r; return; }
        case 9: { long mn = va_arg(ap, long), mx = va_arg(ap, long), cu = va_arg(ap, long);
                  BOOL r = c->SetValueOption(mn, mx, cu); if (pvRet) *(BOOL*)pvRet = r; return; }
        case 10: { BOOL b = (BOOL)va_arg(ap, int); c->SetPassWord(b); return; }
        case 11: { long mn = va_arg(ap, long), mx = va_arg(ap, long), cu = va_arg(ap, long);
                   BOOL r = c->SetSearchValueOption(mn, mx, cu); if (pvRet) *(BOOL*)pvRet = r; return; }
        case 12: { long mn = va_arg(ap, long), mx = va_arg(ap, long), cu = va_arg(ap, long);
                   BOOL r = c->SetPlayerNegPriceOption(mn, mx, cu); if (pvRet) *(BOOL*)pvRet = r; return; }
        default: return;
    }
}

int ma_spin_index(void* ctrlp) {
    MaSpinBut* c = (MaSpinBut*)ctrlp; return c ? (int)c->GetIndex() : 0;
}

/* Returns 1 if the control took the click AND its value moved, 0 otherwise -- the caller fires
   the dialog's TextChanged only when something actually changed, so a click on a spinner already
   at its limit does not announce a change that did not happen. */
int ma_spin_click(void* ctrlp, void* parentWnd, int lx, int ly) {
    MaSpinBut* c = (MaSpinBut*)ctrlp; if (!c) return 0;
    c->m_maParent = (CWnd*)parentWnd;
    long bi = c->GetIndex(), bv = c->GetCurrentValue();
    c->OnLButtonDown(0, CPoint(lx, ly));   /* the genuine arrow-strip test + up/down split */
    c->OnLButtonUp(0, CPoint(lx, ly));     /* releases capture, stops the repeat */
    long ai = c->GetIndex(), av = c->GetCurrentValue();
    if (getenv("MA_TRACE_SPIN"))
        fprintf(stderr, "[spin] click ctrl=%p local(%d,%d) rect=%dx%d index %ld -> %ld value %ld -> %ld "
                        "(list has %d entries; UP needs index <= count-2, DOWN needs index >= 1)\n",
                ctrlp, lx, ly, c->m_maW, c->m_maH, bi, ai, bv, av, (int)c->m_list.GetCount());
    return (ai != bi) || (av != bv);
}

/* Where a recipe (or a synthetic click) must aim to work the spinner, expressed in the
   control's OWN geometry and kept in the control's own TU. CRSpinButCtrl::OnLButtonDown
   (RSPINBTC.CPP:483) ignores anything left of `rect.right - 15` and splits up/down on
   `rect.bottom / 2`; those two numbers are restated here and NOWHERE else, because a
   resolver that carried its own copy is exactly the two-paths-disagreeing-about-one-fact
   shape that has cost this port several sprints. Returns 0 if the control is too small to
   have an arrow strip at all -- then there is no point to aim at and the caller must say so
   rather than click the middle and report a working spinner. */
int ma_spin_arrow_point(void* ctrlp, int down, int* lx, int* ly) {
    MaSpinBut* c = (MaSpinBut*)ctrlp; if (!c) return 0;
    int w = c->m_maW, h = c->m_maH;
    if (w <= 15 || h < 2) return 0;
    if (lx) *lx = w - 8;                     /* inside the right-hand 15px arrow strip */
    if (ly) *ly = down ? (h * 3) / 4 : h / 4; /* below / above the mid-height split */
    return 1;
}

void ma_spin_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    MaSpinBut* c = (MaSpinBut*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    c->m_bDrawing = FALSE;                 /* trap (1): class-wide static, latches if a draw bails */
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0, clipSaved[5];
    ma_gdi_set_clip(screenHdc, sx, sy, sx + w, sy + h, clipSaved);
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
    ma_gdi_restore_clip(screenHdc, clipSaved);
}
