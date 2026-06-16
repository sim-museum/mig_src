/* ma_eventsink.cpp — OCX control event routing (FireClicked/Selected/... -> dialog handler).
 *
 * Real MFC connects a control's events to the hosting dialog's ON_EVENT handlers via the
 * eventsink map + IConnectionPoint. On Linux the maps are no-ops, so a button's FireClicked
 * goes nowhere. We rebuild it: the redefined ON_EVENT macros (afxwin.h) register, for each
 * (dialog-CLASS, control-id, event-dispid), a thunk that casts the dialog and calls the
 * handler. At fire time ma_evt_fire matches by control-id + event + the dialog's RUNTIME type
 * (typeid) -- RTTI disambiguates the many dialogs that reuse the same IDC_ ids. */

#include <vector>
#include <typeinfo>
#include <stdio.h>
#include <stdlib.h>

/* event args set by the firing control before ma_evt_fire (read by the ma_evt_call thunks
   in afxwin.h for non-VTS_NONE handlers, e.g. Selected(int)/Select(int,int)) */
extern "C" { long ma_evtA0 = 0, ma_evtA1 = 0; void* ma_evtP = 0; }

struct EvtEntry { const std::type_info* ti; int id; int dispid; void (*thunk)(void*); };
static std::vector<EvtEntry>& evtmap() { static std::vector<EvtEntry> v; return v; }

extern "C" void ma_evt_register(const void* tinfo, int id, int dispid, void (*thunk)(void*)) {
    EvtEntry e; e.ti = (const std::type_info*)tinfo; e.id = id; e.dispid = dispid; e.thunk = thunk;
    evtmap().push_back(e);
    if (getenv("MA_TRACE_OLE")) { static int n=0; if(++n<=3||(n%50)==0) fprintf(stderr,"[evt_register] #%d id=%d dispid=%d type=%s\n", n, id, dispid, e.ti?e.ti->name():"?"); }
}

/* dlg = the dialog instance; tinfo = &typeid(*dlg) (passed by the caller, which has the
   concrete pointer). Call every handler whose class matches the dialog's runtime type. */
extern "C" int ma_evt_fire(void* dlg, const void* tinfo, int id, int dispid) {
    const std::type_info* dt = (const std::type_info*)tinfo;
    std::vector<EvtEntry>& v = evtmap();
    int fired = 0;
    for (size_t i = 0; i < v.size(); i++) {
        if (v[i].id == id && v[i].dispid == dispid && v[i].ti && dt && *v[i].ti == *dt) {
            if (getenv("MA_TRACE_OLE")) fprintf(stderr,"[evt_fire] id=%d dispid=%d type=%s -> HANDLER CALLED\n", id, dispid, dt->name());
            v[i].thunk(dlg); fired = 1;
        }
    }
    return fired;
}
