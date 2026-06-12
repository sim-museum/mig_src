/* Mig Alley Linux port - afxole.h: minimal MFC OLE container compat.
 * Mig Alley's main window is an MFC OLE-container Doc/View app (MIGDOC/MIGVIEW/
 * CNTRITEM). FreeFalcon/BoB never used OLE, so this was a 4-line stub. Provide
 * just enough COleDocument / COleClientItem surface for CntrItem.h + the doc/view
 * to compile. OLE embedding is not exercised at runtime (the 3D view renders
 * directly), so the methods are no-op stubs. */
#ifndef FF_STUB_afxole_h
#define FF_STUB_afxole_h

#if defined(FF_LINUX) || defined(MA_LINUX)
#include "afxwin.h"

#ifndef OLE_NOTIFICATION_DEFINED
#define OLE_NOTIFICATION_DEFINED
typedef unsigned long OLE_NOTIFICATION;
typedef unsigned long OLE_VERB;
#endif

/* COleDocument itself is the empty stub already in afxwin.h. */

/* Document-item base (afxwin's COleDocument owns these). */
class CDocItem : public CCmdTarget
{
public:
    CDocItem() {}
    virtual ~CDocItem() {}
    virtual BOOL IsBlank() const { return TRUE; }
    virtual void Serialize(CArchive&) {}
};

/* MFC OLE container item base. */
class COleClientItem : public CDocItem
{
public:
    COleClientItem(COleDocument* pContainer = 0) { (void)pContainer; }
    virtual ~COleClientItem() {}

    CDocument* GetDocument() const   { return 0; }
    CView*     GetActiveView() const { return 0; }
    UINT       GetItemState() const  { return 0; }
    void       Activate(LONG, CView*, LPMSG = 0) {}
    BOOL       Draw(CDC*, LPCRECT, LPCRECT = 0) { return TRUE; }
    void       Delete() {}
    void       Close() {}
    void       Run() {}

    /* overridable notifications */
    virtual void OnChange(OLE_NOTIFICATION, DWORD) {}
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}
    virtual void OnGetItemPosition(CRect&) {}
    virtual void OnDeactivateUI(BOOL) {}
    virtual BOOL OnChangeItemPosition(const CRect&) { return TRUE; }
    virtual void Serialize(CArchive&) {}
};

#endif /* FF_LINUX || MA_LINUX */
#endif
