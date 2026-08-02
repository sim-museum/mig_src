/* afxctl.h — MFC ActiveX-control framework shim for the Linux port.
 *
 * The menu listbox is a real OCX (CRListBoxCtrl : COleControl, SRC/RLISTBOX/). We host
 * it faithfully: this header provides just enough of <afxctl.h> for the control TU to
 * compile. The dispatch/event/proppage maps stay no-ops (afxwin.h) — we route the
 * client's InvokeHelper(dispid) to the control via a hand-written switch (the dispid
 * scheme is sequential map order: AddString=0x20), not MFC's generic marshaller. */
#ifndef __AFXCTL_H__
#define __AFXCTL_H__

#include "afxwin.h"

/* ---- low-level OLE-control defines the control TU needs ----------------- */
#ifndef CDECL
#define CDECL
#endif
#ifndef BASED_CODE
#define BASED_CODE
#endif
#ifndef afxRegApartmentThreading
#define afxRegApartmentThreading 1
#endif
#ifndef ETO_CLIPPED
#define ETO_CLIPPED 0x0004
#endif
#ifndef AFX_IDS_VERB_PROPERTIES
#define AFX_IDS_VERB_PROPERTIES 0xFD38
#endif
#ifndef ID_HELP
#define ID_HELP 9
#endif
#ifndef OLEMISC_RECOMPOSEONRESIZE
#define OLEMISC_RECOMPOSEONRESIZE     0x00000001
#define OLEMISC_INSIDEOUT             0x00000080
#define OLEMISC_ACTIVATEWHENVISIBLE   0x00000100
#define OLEMISC_CANTLINKINSIDE        0x00000010
#define OLEMISC_SETCLIENTSITEFIRST    0x00020000
#endif
#ifndef ON_OLEVERB
#define ON_OLEVERB(idsVerbName, memberFxn)
#define ON_STDOLEVERB(iVerb, memberFxn)
#endif

#ifndef LPCOLESTR
typedef const wchar_t* LPCOLESTR;
#endif
static inline BSTR     SysAllocString(const wchar_t*)   { return (BSTR)0; }
static inline unsigned RegisterClipboardFormat(const char*) { return 0; }

/* OLE property page base (RListBxP.h: CRListBoxPropPage : COlePropertyPage) */
#ifndef MA_COLEPROPERTYPAGE_DEFINED
#define MA_COLEPROPERTYPAGE_DEFINED
class COlePropertyPage : public CDialog {
public:
    COlePropertyPage(UINT = 0, UINT = 0) {}
    void SetModifiedFlag(BOOL = TRUE) {}
    void SetDialogResource(HGLOBAL) {}
    void SetPageName(LPCSTR) {}
};
#endif

/* OLE drag&drop (CRListBoxCtrl::OnBeginDrag) — stubbed; no DnD on the Linux front-end */
#ifndef MA_OLEDND_DEFINED
#define MA_OLEDND_DEFINED
typedef DWORD DROPEFFECT;
#define DROPEFFECT_NONE 0
#define DROPEFFECT_COPY 1
#define DROPEFFECT_MOVE 2
class COleDataSource {
public:
    void CacheGlobalData(unsigned, HGLOBAL, void* = 0) {}
    DROPEFFECT DoDragDrop(DWORD = 3, LPCRECT = 0, void* = 0) { return DROPEFFECT_NONE; }
};
#endif

/* (FIL_SFX_OFFICE_* are real enums in F_SOUNDS.G; the OCX's selection-sound lines
   are guarded out under MA_LINUX in RLISTBXC.CPP rather than #defined here, which
   would corrupt the enum in every other TU that includes files.g.) */

/* Minimal class factory base; we host the control by direct `new`, but the
   ClassWizard factory methods (UpdateRegistry/VerifyUserLicense/GetLicenseKey)
   are compiled and need a base with m_clsid / m_lpszProgID. */
#ifndef MA_COLEOBJECTFACTORY_DEFINED
#define MA_COLEOBJECTFACTORY_DEFINED
class COleObjectFactory {
public:
    CLSID m_clsid;
    const char* m_lpszProgID;
    COleObjectFactory() : m_lpszProgID(0) { CLSID z = {0}; m_clsid = z; }
    virtual ~COleObjectFactory() {}
    virtual BOOL UpdateRegistry(BOOL) { return TRUE; }
};
#endif

/* Real OLE-factory declaration macros (override afxwin.h's no-ops): declare the
   nested CRListBoxCtrlFactory with the members the .CPP defines against. */
#undef BEGIN_OLEFACTORY
#undef END_OLEFACTORY
#undef IMPLEMENT_OLECREATE_EX
#define BEGIN_OLEFACTORY(class_name) \
public: \
    class class_name##Factory : public COleObjectFactory { \
    public: \
        virtual BOOL UpdateRegistry(BOOL bRegister);
#define END_OLEFACTORY(class_name) \
    }; \
    friend class class_name##Factory; \
    static class_name##Factory factory;
#define IMPLEMENT_OLECREATE_EX(class_name, progid, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    class_name::class_name##Factory class_name::factory;

/* The ActiveX module object (CRListBoxApp : COleControlModule). */
#ifndef MA_COLECONTROLMODULE_DEFINED
#define MA_COLECONTROLMODULE_DEFINED
class COleControlModule : public CWinApp {
public:
    BOOL InitInstance() { return TRUE; }
    int  ExitInstance() { return 0; }
};
#endif

/* CPropExchange persistence helpers.
 * S62 (BoB S126 adoption): these used to be no-ops returning TRUE, so every control
 * booted with its default for every persisted property. With a stream attached
 * (CPropExchange::Attach, fed from the DLGINIT property bag) each PX_* now READS its
 * field from the stream, in DoPropExchange SOURCE ORDER — that ordering IS the wire
 * format, which is why the calls must stay sequential and why a mid-stream error has to
 * poison the rest (m_bOk drops, every remaining PX_* falls back to its default) rather
 * than resync and read misaligned values.
 * Widths: PX_Bool=BYTE, PX_Short=WORD, PX_Long/PX_Color=DWORD, PX_String=CString archive.
 * Unattached, every one of these still just writes the default — the old behaviour. */
#ifndef PX_Color
static inline BOOL PX_Color(CPropExchange* px, LPCSTR, OLE_COLOR& v, OLE_COLOR d = 0) {
    DWORD t; if (px && px->m_bOk && px->ReadU32(t)) { v = (OLE_COLOR)t; return TRUE; }
    v = d; return TRUE;
}
static inline BOOL PX_Bool (CPropExchange* px, LPCSTR, BOOL& v, BOOL d = 0) {
    BYTE t; if (px && px->m_bOk && px->ReadU8(t)) { v = t ? TRUE : FALSE; return TRUE; }
    v = d; return TRUE;
}
/* some R* controls declare a persisted bool as `short` — same BYTE on the wire */
static inline BOOL PX_Bool (CPropExchange* px, LPCSTR, short& v, BOOL d = 0) {
    BYTE t; if (px && px->m_bOk && px->ReadU8(t)) { v = (short)(t ? 1 : 0); return TRUE; }
    v = (short)(d ? 1 : 0); return TRUE;
}
static inline BOOL PX_Long (CPropExchange* px, LPCSTR, long& v, long d = 0) {
    DWORD t; if (px && px->m_bOk && px->ReadU32(t)) { v = (long)t; return TRUE; }
    v = d; return TRUE;
}
static inline BOOL PX_Short(CPropExchange* px, LPCSTR, short& v, short d = 0) {
    WORD t; if (px && px->m_bOk && px->ReadU16(t)) { v = (short)t; return TRUE; }
    v = d; return TRUE;
}
static inline BOOL PX_String(CPropExchange* px, LPCSTR, CString& v, LPCSTR d = 0) {
    CString t; if (px && px->m_bOk && px->ReadStr(t)) { v = t; return TRUE; }
    v = d ? d : ""; return TRUE;
}
#endif

/* control-class registration — no OLE registry on Linux */
#ifndef AfxOleRegisterControlClass
static inline BOOL AfxOleRegisterControlClass(HINSTANCE, REFCLSID, LPCSTR, UINT, UINT, BOOL, DWORD, REFGUID, UINT, UINT) { return TRUE; }
static inline BOOL AfxOleUnregisterClass(REFCLSID, LPCSTR) { return TRUE; }
static inline BOOL AfxVerifyLicFile(HINSTANCE, LPCSTR, LPCOLESTR, UINT = 0) { return TRUE; }
#endif

/* dispatch/event/proppage map ENTRY macros (the BEGIN/END forms live in afxwin.h).
   Entries expand to nothing — routing is done in ma_olecontrol.cpp. */
#ifndef DISP_PROPERTY_EX
#define DISP_PROPERTY_EX(theClass, name, getfn, setfn, vt)
#endif
#ifndef DISP_PROPERTY_NOTIFY
#define DISP_PROPERTY_NOTIFY(theClass, name, member, notifyfn, vt)
#endif
#ifndef DISP_STOCKPROP_CAPTION
#define DISP_STOCKPROP_CAPTION()
#endif
#ifndef DISP_STOCKPROP_TEXT
#define DISP_STOCKPROP_TEXT()
#endif
#ifndef DISP_PROPERTY_PARAM
#define DISP_PROPERTY_PARAM(theClass, name, getfn, setfn, vt, vtparams)
#endif
#ifndef DISP_STOCKPROP_FORECOLOR
#define DISP_STOCKPROP_FORECOLOR()
#endif
#ifndef DISP_STOCKPROP_BACKCOLOR
#define DISP_STOCKPROP_BACKCOLOR()
#endif
#ifndef DISP_STOCKPROP_ENABLED
#define DISP_STOCKPROP_ENABLED()
#endif
#ifndef DISP_STOCKPROP_FONT
#define DISP_STOCKPROP_FONT()
#endif
#ifndef EVENT_CUSTOM
#define EVENT_CUSTOM(name, fn, vtargs)
#endif
#ifndef EVENT_CUSTOM_ID
#define EVENT_CUSTOM_ID(name, dispid, fn, vtargs)
#endif

#endif /* __AFXCTL_H__ */
