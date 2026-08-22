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
void  ma_gdi_set_clip(void*, int, int, int, int, int*);   /* S67 */
void  ma_gdi_restore_clip(void*, const int*);            /* S67 */
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

/* Control-id -> toolbar-icon (FIL_ICON_*) table (BoB S88-92 recipe: the .rc DLGINIT doesn't
   differentiate the buttons, so map each by function). Filter/border buttons: left blank for now. */
/* S64 — resolve the button's PERSISTED art NAMES to runtime FileNums.
 *
 * The completion of BoB trap 2. The persisted numeric Normal/PressedFileNum are
 * authoring-install indices and are snapshotted/restored away by ma_px_replay, so the
 * only sound source of a button's design-time art is the NAME it also persists
 * (`NormalFileNumString` / `PressedFileNumString`). But `PX_String` writes those members
 * DIRECTLY — it never runs the dispatch setter — so nothing was converting them, and
 * GetFileNum() (which the setters would have called) was itself a stub returning 0.
 * With GetFileNum now implemented against the F_GRAFIX.G table, do the conversion
 * explicitly after the replay. Only overwrites when the name resolves, so a button whose
 * art is runtime-owned keeps its boot value. */
extern "C" int ma_fil_lookup(const char* name);
extern "C" int ma_button_resolve_art_names_id(void* ctrlp, int dbgid);
extern "C" int ma_button_resolve_art_names(void* ctrlp) { return ma_button_resolve_art_names_id(ctrlp, -1); }
extern "C" int ma_button_resolve_art_names_id(void* ctrlp, int dbgid) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c) return 0;
    int applied = 0;
    CString n = c->GetNormalFileNumString();
    if (!n.IsEmpty()) { int fn = ma_fil_lookup((LPCSTR)n); if (fn > 0) { c->SetNormalFileNum(fn); applied = 1; } }
    CString pstr = c->GetPressedFileNumString();
    if (!pstr.IsEmpty()) { int fp = ma_fil_lookup((LPCSTR)pstr); if (fp > 0) { c->SetPressedFileNum(fp); applied = 1; } }
    if (getenv("MA_TRACE_TITLEBTN"))
        fprintf(stderr, "[titlebtn] id=%d close=%d tick=%d shadow=%d movesparent=%d\n",
                dbgid, (int)c->GetCloseButton(), (int)c->GetTickButton(),
                (int)c->GetShowShadow(), (int)c->GetMovesParent());
    if (getenv("MA_TRACE_FILENUM") && (!n.IsEmpty() || !pstr.IsEmpty()))
        fprintf(stderr, "[btnartname] id=%d normal=\"%s\" pressed=\"%s\" applied=%d\n",
                dbgid, (LPCSTR)n, (LPCSTR)pstr, applied);
    return applied;
}

/* S82: which glyph band of a dialog TITLE BAR did this point hit? Lives here because only the
   olebutton TU can see CRButtonCtrl (each OCX ships its own header set; ma_olecontrol.cpp is
   built in "ole" mode against RLISTBOX). Returns -1 for an ordinary button — every existing
   toolbar/dialog button — so the caller keeps its old plain-Clicked behaviour untouched.
   Otherwise returns the dispid the control's own OnLButtonUp would have fired:
   3=OK(tick) 2=Cancel(close) 1=Clicked 0=Help. */
/* S98 (PO-4): where is this title bar's "?" glyph? Found by asking the control's OWN hit-test,
   scanning the bar right-to-left for the first point it reports as the help band -- never by a
   pixel a human read off a screenshot. The glyph positions come from the button's art and move
   with the dialog, its width and the font (S95's map-icon lesson, and S96 moved the screen edge
   twice inside one sprint). Returns 1 and the LOCAL point, or 0 if this control has no help band. */
/* S170: the general form of ma_button_help_point -- find the point on this title bar whose
   own hit-test reports `want` (0 Help, 1 Clicked, 2 Cancel, 3 OK). The OK band is how a
   player closes an OOB dialog and commits it (ChooseSquad::OnOK recalculates the route and
   refreshes the parent), so a recipe that cannot press it can only ever prove that a control
   moved, never that the change reached the mission. */
extern "C" int ma_button_band_point(void* ctrlp, int want, int w, int h, int* lx, int* ly) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp;
    if (!c || !c->MaHasTitleButtons() || w <= 0 || h <= 0) return 0;
    int y = h / 2;
    for (int x = w - 1; x >= 0; x--) {
        if (c->MaButtonHit(x, y, w, h) == want) { if (lx) *lx = x; if (ly) *ly = y; return 1; }
    }
    return 0;
}

extern "C" int ma_button_help_point(void* ctrlp, int w, int h, int* lx, int* ly) {
    return ma_button_band_point(ctrlp, 0 /*Help*/, w, h, lx, ly);
}

extern "C" int ma_button_title_hit(void* ctrlp, int x, int y, int w, int h) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp;
    if (!c || !c->MaHasTitleButtons()) return -1;
    return c->MaButtonHit(x, y, w, h);
}

extern "C" void ma_button_apply_icon(void* ctrlp, int id) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c) return;
    long fn = 0;
    switch (id) {
        case 2080: fn=0x6a63; break;  /* IDC_BASES      -> FIL_ICON_BASES */
        case 2065: fn=0x6a66; break;  /* IDC_SQUADS     -> FIL_ICON_SQUADRONS */
        case 2069: fn=0x6a69; break;  /* IDC_WEATHER    -> FIL_ICON_WEATHER */
        case 2072: fn=0x6a6c; break;  /* IDC_DIS        -> FIL_ICON_DIS */
        case 2074: fn=0x6607; break;  /* IDC_DIRECTIVES -> FIL_ICON_DIRECTIVES */
        case 1905: fn=0x6a96; break;  /* IDC_FRAG2      -> FIL_ICON_FRAG */
        case 2064: fn=0x6a7b; break;  /* IDC_PLAYERLOG  -> FIL_ICON_PLAYERLOG */
        case 2058: fn=0x6a7e; break;  /* IDC_OVERVIEW   -> FIL_ICON_OVERVIEW */
        case 2140: fn=0x6a75; break;  /* IDC_PACKAGES   -> FIL_ICON_MISSIONFOLDER */
        case 2023: fn=0x6a78; break;  /* IDC_AUTHORISE  -> FIL_ICON_MISSIONRESULTS */
        /* S97 (PO-1): the CSystemBox cluster at the top right -- the widgets the play-tester went
           looking for and could not find, because the box drew but every button in it was blank.
           The art was in the game all along and is named after the controls themselves. Confirmed
           against the Wine gold (#7, top-right corner): maximise and restore stacked in a 24-wide
           left column, and a large X on the right, which is IDC_FILES -> CMainFrame::OnBye() --
           the exit.
           The two small ids do NOT take the art whose NAME matches them: FIL_ICON_THUMBNAIL /
           FIL_ICON_ZOOMIN render as unrelated map glyphs here. The art was identified by
           comparing renders against the gold crop (probe hook MA_BTN_ART), and the result is
           self-consistent -- IDC_ZOOMIN drives OnGoBig/OnGoNormal, which IS the screen-size
           widget. Matching a name to an id would have shipped the wrong pictures. */
        case 4:    fn=0x6a99; break;  /* IDC_THUMBNAIL (top)    -> 0x6a99, minimise glyph */
        case 7:    fn=0x6a9c; break;  /* IDC_ZOOMIN    (bottom) -> FIL_ICON_SCREENSIZE     */
        case 10:   fn=0x6aa0; break;  /* IDC_FILES     (large)  -> FIL_ICON_CLOSE1, the exit */
        default: return;
    }
    /* S97 probe hook: MA_BTN_ART="id=0xNNNN,id=0xNNNN" overrides the table at runtime. Finding the
       right art means comparing renders against the gold shot, and rebuilding for each candidate
       turns a two-minute question into an hour. Off unless set. */
    const char* ov = getenv("MA_BTN_ART");
    if (ov) {
        const char* p2 = ov;
        while (p2 && *p2) {
            int oid = 0; unsigned ofn = 0;
            if (sscanf(p2, "%d=%i", &oid, &ofn) == 2 && oid == id) { fn = (long)ofn; break; }
            const char* c2 = strchr(p2, ','); p2 = c2 ? c2 + 1 : 0;
        }
    }
    if (c->GetNormalFileNum() != fn) c->SetNormalFileNum(fn);
}
extern "C" void ma_button_set_filenum(void* ctrlp, long fn) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (c) c->SetNormalFileNum(fn);
}
/* S57: apply the RT_DLGINIT design-time String property (e.g. the Controls-tab tickbox
   glyph "3"); runtime SetString/SetCaption dispatches overwrite it, as on Windows. */
extern "C" void ma_button_set_string(void* ctrlp, const char* s) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (c) c->SetString(s ? s : "");
}
void ma_button_draw(void* ctrlp, void* parentWnd, void* screenHdc, int sx, int sy, int w, int h) {
    /* S67: no fixed cap -- an early-screen control would otherwise consume it before the
       screen under investigation appears (the S65 trace-cap trap). Filter, don't cap. */
    if (getenv("MA_TRACE_TITLEW") && w > 300)
        fprintf(stderr, "[btndraw] ctrl=%p at(%d,%d) %dx%d\n", ctrlp, sx, sy, w, h);

    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c || w <= 0 || h <= 0) return;
    c->m_maParent = (CWnd*)parentWnd;
    c->m_maX = sx; c->m_maY = sy; c->m_maW = w; c->m_maH = h;
    CDC dc; dc.m_hDC = (HDC)screenHdc;
    int ox = 0, oy = 0;
    /* S67: clip this control's drawing to its own rect, as Windows does. CRButtonCtrl's
       picture path (RBUTTONC.CPP:1145) blits its DIB at NATURAL SIZE straight to the DC,
       so art larger than the control painted over its neighbours -- the Player Log's
       IDJ_TITLE art is ~550px wide on a 336px control and ran ~213px past the dialog,
       over the map. */
    int clipSaved[5];
    ma_gdi_set_clip(screenHdc, sx, sy, sx + w, sy + h, clipSaved);
    ma_gdi_set_viewport_org(screenHdc, sx, sy, &ox, &oy);
    CRect bounds(0, 0, w, h);
    c->OnDraw(&dc, bounds, bounds);
    ma_gdi_set_viewport_org(screenHdc, ox, oy, 0, 0);
    ma_gdi_restore_clip(screenHdc, clipSaved);
}

/* click routing for buttons (FireClicked/OK/Cancel -> dialog event sink) is a follow-on;
   rendering is wired first. */
int ma_button_click(void* ctrlp, void* parentWnd, int lx, int ly) {
    (void)ctrlp; (void)parentWnd; (void)lx; (void)ly; return 0;
}

/* S62 (BoB trap 2): snapshot/restore the button's art indices around a property-stream
   replay. The persisted Normal/PressedFileNum are file-table indices from the AUTHORING
   install and are meaningless against the runtime table — BoB's first cut without this
   corrupted their toolbar icons. Art is resolved by NAME instead (the S57 FIL_ path). */
extern "C" int ma_button_get_art(void* ctrlp, long* n, long* p) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c) return 0;
    if (n) *n = c->GetNormalFileNum();
    if (p) *p = c->GetPressedFileNum();
    return 1;
}
extern "C" void ma_button_set_art(void* ctrlp, long n, long p) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c) return;
    c->SetNormalFileNum(n);
    c->SetPressedFileNum(p);
}

/* S137 (PO-30): the toggle the port's click path was missing. CRButtonCtrl::OnLButtonUp does
   `m_bPressed=!m_bPressed;` before firing Clicked, and handlers that ask the button what state
   it is now in -- CMapFilters::OnClickedFilter is the reported case -- read a stale FALSE
   without it. Kept here rather than in the router so the router never touches control internals. */
extern "C" void ma_button_toggle_pressed(void* ctrlp) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; if (!c) return;
    c->SetPressed(c->GetPressed() ? FALSE : TRUE);
}
extern "C" int ma_button_get_pressed(void* ctrlp) {
    CRButtonCtrl* c = (CRButtonCtrl*)ctrlp; return c ? (c->GetPressed() ? 1 : 0) : 0;
}
