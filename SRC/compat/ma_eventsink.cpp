/* ma_eventsink.cpp — OCX control event routing (FireClicked/Selected/... -> dialog handler).
 *
 * Real MFC connects a control's events to the hosting dialog's ON_EVENT handlers via the
 * eventsink map + IConnectionPoint. On Linux the maps are no-ops, so a button's FireClicked
 * goes nowhere. We rebuild it: the redefined ON_EVENT macros (afxwin.h) register, for each
 * (dialog-CLASS, control-id, event-dispid), a thunk that casts the dialog and calls the
 * handler. At fire time ma_evt_fire matches by control-id + event + the dialog's RUNTIME type
 * (typeid) -- RTTI disambiguates the many dialogs that reuse the same IDC_ ids. */

#include <cstring>
#include <vector>
#include <typeinfo>
#include <stdio.h>
#include <stdlib.h>

/* event args set by the firing control before ma_evt_fire (read by the ma_evt_call thunks
   in afxwin.h for non-VTS_NONE handlers, e.g. Selected(int)/Select(int,int)) */
extern "C" { long ma_evtA0 = 0, ma_evtA1 = 0; void* ma_evtP = 0; }

struct EvtEntry { const std::type_info* ti; int id; int idLast; int dispid; void (*thunk)(void*); int passId; };
static std::vector<EvtEntry>& evtmap() { static std::vector<EvtEntry> v; return v; }

extern "C" void ma_evt_register(const void* tinfo, int id, int dispid, void (*thunk)(void*)) {
    EvtEntry e; e.ti = (const std::type_info*)tinfo; e.id = id; e.idLast = id; e.dispid = dispid; e.thunk = thunk; e.passId = 0;
    evtmap().push_back(e);
    if (getenv("MA_TRACE_OLE")) { static int n=0; if(++n<=3||(n%50)==0) fprintf(stderr,"[evt_register] #%d id=%d dispid=%d type=%s\n", n, id, dispid, e.ti?e.ti->name():"?"); }
    /* S168: MA_TRACE_EVTREG=<substring> prints every registration whose TYPE NAME contains the
       substring. The MA_TRACE_OLE line above is capped (`n<=3 || n%50==0`), so it answers "is
       registration happening at all" and cannot answer "did THIS class register" -- filter, don't
       cap, which this project has now booked five times. Registration happens once per class at
       static-init, so a filter here is bounded by the class's own entry count. */
    { const char* _w = getenv("MA_TRACE_EVTREG");
      if (_w && *_w && e.ti && strstr(e.ti->name(), _w))
          fprintf(stderr,"[evt_register] id=%d dispid=%d type=%s\n", id, dispid, e.ti->name()); }
}

/* S87: ON_EVENT_RANGE registers ONE handler for a span of ids (CBases' 30 airfield buttons,
   CMapFilters' layer filters). MFC hands such a handler the id that fired as its first argument,
   so these entries are flagged and ma_evt_fire supplies it. Registering per-id keeps the fire
   path a plain lookup — no range checks in the hot loop. */
extern "C" void ma_evt_register_range(const void* tinfo, int idFirst, int idLast, int dispid, void (*thunk)(void*)) {
    if (idLast < idFirst) return;
    /* S137 (PO-30): store the RANGE, do not expand it.
       This used to materialise one entry per id and REFUSE any span wider than 4096 -- and
       CMapFilters registers ON_EVENT_RANGE(CMapFilters, 1, 9999, Clicked, OnClickedFilter),
       a span of 9998. So the map-filter toolbar's ONE handler was silently discarded at
       registration and every one of its 30 buttons was dead: the click routed, the button
       highlighted, and nothing listened. The PO reported it as "red and blue buttons at
       upper right do nothing -- they're supposed to filter map icons".
       A cap that silently drops legitimate work is the same fault the trace caps kept
       committing; the answer is the same one, applied to the data structure instead. */
    EvtEntry e; e.ti = (const std::type_info*)tinfo; e.id = idFirst; e.idLast = idLast;
    e.dispid = dispid; e.thunk = thunk; e.passId = 1;
    evtmap().push_back(e);
    if (getenv("MA_TRACE_OLE"))
        fprintf(stderr,"[evt_register_range] ids %d..%d dispid=%d type=%s\n", idFirst, idLast, dispid,
                ((const std::type_info*)tinfo)->name());
}

/* dlg = the dialog instance; tinfo = &typeid(*dlg) (passed by the caller, which has the
   concrete pointer). Call every handler whose class matches the dialog's runtime type. */
extern "C" int ma_evt_fire(void* dlg, const void* tinfo, int id, int dispid) {
    const std::type_info* dt = (const std::type_info*)tinfo;
    std::vector<EvtEntry>& v = evtmap();
    int fired = 0;
    for (size_t i = 0; i < v.size(); i++) {
        if (id >= v[i].id && id <= v[i].idLast && v[i].dispid == dispid && v[i].ti && dt && *v[i].ti == *dt) {
            if (getenv("MA_TRACE_OLE")) fprintf(stderr,"[evt_fire] id=%d dispid=%d type=%s -> HANDLER CALLED%s\n", id, dispid, dt->name(), v[i].passId?" (range)":"");
            long savedA0 = ma_evtA0;
            if (v[i].passId) ma_evtA0 = id;   /* a range handler's first arg is the id that fired */
            v[i].thunk(dlg); fired = 1;
            ma_evtA0 = savedA0;
        }
    }
    /* S168: an UNMATCHED fire says so, unconditionally-ish (MA_TRACE_CLICK, which every recipe run
       already sets), and lists what IS registered for that id. A click that reaches the control,
       toggles its artwork and then finds no handler is indistinguishable from a working button
       whose handler does nothing -- and "-> fire" is printed BEFORE this call, so the log actively
       reads like success. Same rule as S85's ambiguous-id report: the failure mode is that nobody
       was looking. */
    if (!fired && getenv("MA_TRACE_CLICK")) {
        fprintf(stderr, "[evt_fire] NO HANDLER for id=%d dispid=%d on type=%s\n",
                id, dispid, dt ? dt->name() : "(null)");
        int shown = 0;
        for (size_t i = 0; i < v.size() && shown < 6; i++)
            if (id >= v[i].id && id <= v[i].idLast && v[i].ti) {
                fprintf(stderr, "[evt_fire]   registered: id=%d..%d dispid=%d type=%s\n",
                        v[i].id, v[i].idLast, v[i].dispid, v[i].ti->name());
                shown++;
            }
        if (!shown) fprintf(stderr, "[evt_fire]   nothing at all is registered for id=%d\n", id);
        /* how many entries does this TYPE have at all? distinguishes "this id is not registered"
           from "this class's whole sink map never registered" -- two very different bugs. */
        int forType = 0;
        for (size_t i = 0; i < v.size(); i++) if (v[i].ti && dt && *v[i].ti == *dt) forType++;
        fprintf(stderr, "[evt_fire]   registry holds %d entr(y|ies) for %s, %d in total\n",
                forType, dt ? dt->name() : "(null)", (int)v.size());
    }
    return fired;
}
