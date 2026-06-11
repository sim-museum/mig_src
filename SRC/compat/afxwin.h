/* FreeFalcon Linux Port - afxwin.h: minimal MFC compatibility layer.
 *
 * BoB uses MFC for its app/UI (CWinApp, CWnd, CDialog, CView, CFrameWnd, CDC...).
 * Neither bob's nor FreeFalcon's original afxwin.h has an implementation, so this
 * is a from-scratch minimal MFC: enough of the class hierarchy, message-map
 * macros and GDI/value types to COMPILE the MFC sources. Behaviour is stubbed
 * (windows never really open); the real UI is to be backed by SDL/GL at runtime.
 *
 * NOTE: CString is bob's OWN class (SRC/H/cstring.h), not provided here.
 */
#ifndef FF_COMPAT_AFXWIN_H
#define FF_COMPAT_AFXWIN_H
#ifdef FF_LINUX

/* bob headers gate on this (e.g. MIG.h: "#ifndef __AFXWIN_H__ #error ...") */
#define __AFXWIN_H__
#define __AFX_H__
#define _AFXWIN_H_

#include "windows.h"
#include "objbase.h"

/* MFC collection cursor */
#ifndef __AFX_POSITION_DEFINED
#define __AFX_POSITION_DEFINED
struct __POSITION {};
typedef __POSITION* POSITION;
#endif
struct CCreateContext;   /* used by CView/CFrameWnd create paths (opaque) */
/* forward decls (classes reference each other before their definitions) */
class CDC; class CFont; class CDocument; class CView; class CWnd; class CArchive;
class CScrollBar; class CBitmap; class CMenu; class CCommandLineInfo;
class CDataExchange; class CPrintInfo; class CCreateContext_;
struct AFX_CMDHANDLERINFO; class CPropExchange; class CFile; class CWinApp;
struct tagHELPINFO; struct COleControlSite;

/* ============================================================
 * Message-map / runtime-class macros — all no-ops. BoB's handlers are wired by
 * these on Windows; on Linux input/events are driven by SDL, so we drop them.
 * ============================================================ */
#define DECLARE_MESSAGE_MAP()
#define BEGIN_MESSAGE_MAP(theClass, baseClass)
#define END_MESSAGE_MAP()
#define DECLARE_DYNAMIC(class_name)   public: virtual class CRuntimeClass* GetRuntimeClass() const { return 0; }
#define IMPLEMENT_DYNAMIC(class_name, base_class)
/* DECLARE_DYNCREATE declares the CreateObject factory bob hand-defines/registers */
#define DECLARE_DYNCREATE(class_name) public: static CObject* CreateObject(); virtual class CRuntimeClass* GetRuntimeClass() const { return 0; }
#define IMPLEMENT_DYNCREATE(class_name, base_class)
#define IMPLEMENT_RUNTIMECLASS(class_name, base_class, wSchema, pfnNew)
#define DECLARE_RUNTIMECLASS(class_name)
#define DECLARE_SERIAL(class_name)
#define IMPLEMENT_SERIAL(class_name, base_class, quan)
#define DECLARE_OLECREATE(class_name)
#define IMPLEMENT_OLECREATE(class_name, ext, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
#define DECLARE_DISPATCH_MAP()
#define BEGIN_DISPATCH_MAP(theClass, baseClass)
#define END_DISPATCH_MAP()
#define DECLARE_EVENTSINK_MAP()
#define BEGIN_EVENTSINK_MAP(theClass, baseClass)
#define END_EVENTSINK_MAP()
#define DECLARE_EVENT_MAP()
#define BEGIN_EVENT_MAP(theClass, baseClass)
#define END_EVENT_MAP()
#define ON_EVENT(theClass, id, dispid, fn, vts)
#define ON_EVENT_RANGE(theClass, idFirst, idLast, dispid, fn, vts)
#ifndef CN_EVENT
#define CN_EVENT  0x0800   /* control-notification: OLE control event */
#endif
#define ON_EVENT_REFLECT(theClass, dispid, fn, vts)
#define ON_PROPNOTIFY(theClass, id, dispid, fn)
#define DISP_FUNCTION(theClass, name, fn, vtret, vtargs)
#define DISP_PROPERTY(theClass, name, memb, vt)
/* VTS_* are string literals (MFC packs them into a BYTE[] param-type list via
   literal concatenation), NOT NULL — using NULL breaks `BYTE p[] = VTS_x VTS_y`. */
#define VTS_NONE      ""
#define VTS_I2        "\x02"
#define VTS_I4        "\x03"
#define VTS_R4        "\x04"
#define VTS_R8        "\x05"
#define VTS_CY        "\x06"
#define VTS_DATE      "\x07"
#define VTS_BSTR      "\x08"
#define VTS_DISPATCH  "\x09"
#define VTS_SCODE     "\x0A"
#define VTS_BOOL      "\x0B"
#define VTS_VARIANT   "\x0C"
#define VTS_UNKNOWN   "\x0D"
#define VTS_UI1       "\x11"
#define VTS_COLOR     "\x03"
#define VTS_XPOS_PIXELS "\x03"
#define VTS_YPOS_PIXELS "\x03"
#define VTS_PI2       "\x42"
#define VTS_PI4       "\x43"
#define VTS_PR4       "\x44"
#define VTS_PR8       "\x45"
#define VTS_PBOOL     "\x4B"
#define VTS_PVARIANT  "\x4C"
#define VTS_PUI1      "\x51"
#define VTS_WBSTR     "\x08"
/* OLE control event firing (COleControl) — no-ops */
#define EVENT_PARAM(...)
#define FireEvent(...)        ((void)0)
/* OLE ActiveX-control factory / property-page / typelib macros — no-ops */
#define BEGIN_OLEFACTORY(class_name)
#define END_OLEFACTORY(class_name)
#define DECLARE_OLECREATE_EX(class_name)
#define IMPLEMENT_OLECREATE_EX(class_name, ext, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
#define DECLARE_OLECTLTYPE(class_name)
#define IMPLEMENT_OLECTLTYPE(class_name, idsUserType, idBmp)
#define DECLARE_OLETYPELIB(class_name)
#define IMPLEMENT_OLETYPELIB(class_name, tlid, wVerMajor, wVerMinor)
#define DECLARE_PROPPAGEIDS(class_name)
#define BEGIN_PROPPAGEIDS(class_name, count)
#define END_PROPPAGEIDS(class_name)
#define PROPPAGEID(clsid)
#define DECLARE_OLEMISC_STATUS(status)
#define BEGIN_CONNECTION_MAP(theClass, theBase)
#define END_CONNECTION_MAP()
#define CONNECTION_IID(iid)
#define CONNECTION_PART(theClass, iid, localClass)
#define BEGIN_PROPERTY_MAP(theClass)
#define END_PROPERTY_MAP()
#define DECLARE_INTERFACE_MAP()
#define BEGIN_INTERFACE_MAP(theClass, baseClass)
#define END_INTERFACE_MAP()
#define afx_msg
#define RUNTIME_CLASS(class_name) (NULL)

/* MFC diagnostic macros — no-ops (NDEBUG-style) */
#ifndef ASSERT
#define ASSERT(f)         ((void)0)
#endif
#define ASSERT_VALID(p)   ((void)0)
#define ASSERT_KINDOF(class_name, p) ((void)0)
#define VERIFY(f)         ((void)(f))
#define TRACE             (void)
#define TRACE0(s)         ((void)0)
#define TRACE1(s,a)       ((void)0)
#define TRACE2(s,a,b)     ((void)0)
#define TRACE3(s,a,b,c)   ((void)0)
#define TRACEN(s)         ((void)0)
#define DEBUG_ONLY(f)     ((void)0)

/* ON_* message-map entries (only valid inside BEGIN/END_MESSAGE_MAP, which are
   empty, but define them so stray expansions vanish too). */
#define ON_COMMAND(id, memberFxn)
#define ON_COMMAND_RANGE(id1, id2, memberFxn)
#define ON_UPDATE_COMMAND_UI(id, memberFxn)
#define ON_CONTROL(code, id, memberFxn)
#define ON_MESSAGE(message, memberFxn)
#define ON_NOTIFY(code, id, memberFxn)
#define ON_BN_CLICKED(id, memberFxn)
#define ON_EN_CHANGE(id, memberFxn)
#define ON_CBN_SELCHANGE(id, memberFxn)
#define ON_WM_CREATE()
#define ON_WM_DESTROY()
#define ON_WM_PAINT()
#define ON_WM_SIZE()
#define ON_WM_TIMER()
#define ON_WM_CLOSE()
#define ON_WM_KEYDOWN()
#define ON_WM_KEYUP()
#define ON_WM_CHAR()
#define ON_WM_CHARTOITEM()
#define ON_WM_CANCELMODE()
#define ON_WM_LBUTTONDOWN()
#define ON_WM_LBUTTONUP()
#define ON_WM_RBUTTONDOWN()
#define ON_WM_RBUTTONUP()
#define ON_WM_MOUSEMOVE()
#define ON_WM_ERASEBKGND()
#define ON_WM_SETFOCUS()
#define ON_WM_KILLFOCUS()
#define ON_WM_ACTIVATE()
#define ON_WM_ACTIVATEAPP()
#define ON_WM_SYSCOMMAND()
#define ON_WM_INITMENUPOPUP()
#define ON_WM_HSCROLL()
#define ON_WM_VSCROLL()
#define ON_WM_MOVE()
#define ON_WM_SETCURSOR()
#define ON_WM_GETMINMAXINFO()
#define ON_WM_SHOWWINDOW()
#define ON_WM_ENABLE()
#define ON_WM_MOUSEWHEEL()
#define ON_WM_CONTEXTMENU()
#define ON_WM_DEVMODECHANGE()
#define ON_WM_INITMENU()
#define ON_WM_NCLBUTTONDOWN()
#define ON_WM_NCMOUSEMOVE()
#define ON_WM_LBUTTONDBLCLK()
#define ON_WM_RBUTTONDBLCLK()
#define ON_WM_SYSKEYDOWN()
#define ON_WM_SYSKEYUP()
#define ON_WM_CAPTURECHANGED()
#define ON_WM_MOUSEACTIVATE()
#define ON_WM_NCHITTEST()
#define ON_WM_QUERYNEWPALETTE()
#define ON_WM_PALETTECHANGED()
#define ON_WM_HELPINFO()
#define ON_REGISTERED_MESSAGE(nMessageVariable, memberFxn)
#define ON_UPDATE_COMMAND_UI_RANGE(id1, id2, memberFxn)
#define ON_LBN_SELCHANGE(id, memberFxn)
#define ON_LBN_DBLCLK(id, memberFxn)
#define ON_BN_DOUBLECLICKED(id, memberFxn)
#define ON_EN_KILLFOCUS(id, memberFxn)
#define ON_EN_SETFOCUS(id, memberFxn)
#define ON_CBN_EDITCHANGE(id, memberFxn)
#define ON_CBN_DROPDOWN(id, memberFxn)

/* ============================================================
 * Value types
 * ============================================================ */
struct CSize : public SIZE {
    CSize() { cx = cy = 0; }
    CSize(int initCX, int initCY) { cx = initCX; cy = initCY; }
    CSize(SIZE s) { cx = s.cx; cy = s.cy; }
};

struct CPoint : public POINT {
    CPoint() { x = y = 0; }
    CPoint(int initX, int initY) { x = initX; y = initY; }
    CPoint(POINT p) { x = p.x; y = p.y; }
    CPoint(SIZE s) { x = s.cx; y = s.cy; }
    CPoint(DWORD dw) { x = (short)LOWORD(dw); y = (short)HIWORD(dw); }
    void Offset(int dx, int dy) { x += dx; y += dy; }
    CPoint& operator=(SIZE s) { x = s.cx; y = s.cy; return *this; }
    CPoint& operator+=(POINT p) { x += p.x; y += p.y; return *this; }
    CPoint& operator-=(POINT p) { x -= p.x; y -= p.y; return *this; }
    CPoint& operator+=(SIZE s) { x += s.cx; y += s.cy; return *this; }
    CPoint& operator-=(SIZE s) { x -= s.cx; y -= s.cy; return *this; }
    CPoint operator+(SIZE s) const { return CPoint(x + s.cx, y + s.cy); }
    CPoint operator-(SIZE s) const { return CPoint(x - s.cx, y - s.cy); }
    CSize  operator-(POINT p) const { return CSize(x - p.x, y - p.y); }
    BOOL operator==(POINT p) const { return x == p.x && y == p.y; }
    BOOL operator!=(POINT p) const { return x != p.x || y != p.y; }
};

struct CRect : public RECT {
    CRect() { left = top = right = bottom = 0; }
    CRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    CRect(RECT r) { left = r.left; top = r.top; right = r.right; bottom = r.bottom; }
    int Width()  const { return right - left; }
    int Height() const { return bottom - top; }
    CPoint TopLeft() const { return CPoint(left, top); }
    CPoint& TopLeft() { return *(CPoint*)this; }
    CPoint BottomRight() const { return CPoint(right, bottom); }
    CSize  Size()    const { return CSize(right - left, bottom - top); }
    void SetRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    void SetRectEmpty() { left = top = right = bottom = 0; }
    bool IsRectEmpty() const { return left == right || top == bottom; }
    bool PtInRect(POINT p) const { return p.x >= left && p.x < right && p.y >= top && p.y < bottom; }
    void OffsetRect(int dx, int dy) { left += dx; right += dx; top += dy; bottom += dy; }
    void InflateRect(int dx, int dy) { left -= dx; right += dx; top -= dy; bottom += dy; }
    CRect& operator+=(POINT p) { OffsetRect(p.x, p.y); return *this; }
    CRect& operator-=(POINT p) { OffsetRect(-p.x, -p.y); return *this; }
    CRect& operator+=(SIZE s) { InflateRect(s.cx, s.cy); return *this; }
    CRect& operator-=(SIZE s) { InflateRect(-s.cx, -s.cy); return *this; }
    CRect& operator=(const RECT& r) { left=r.left; top=r.top; right=r.right; bottom=r.bottom; return *this; }
    BOOL IntersectRect(LPCRECT, LPCRECT) { return FALSE; }
    BOOL UnionRect(LPCRECT, LPCRECT) { return FALSE; }
    void NormalizeRect() {}
    CPoint CenterPoint() const { return CPoint((left+right)/2, (top+bottom)/2); }
    operator LPRECT() { return this; }
    operator LPCRECT() const { return this; }
};

/* CPoint/CSize arithmetic (MFC global operators) */
inline CSize  operator-(POINT a, POINT b) { return CSize(a.x - b.x, a.y - b.y); }
inline CSize  operator+(SIZE a, POINT b)  { return CSize(a.cx + b.x, a.cy + b.y); }
inline CSize  operator+(SIZE a, SIZE b)   { return CSize(a.cx + b.cx, a.cy + b.cy); }
inline CSize  operator-(SIZE a, SIZE b)   { return CSize(a.cx - b.cx, a.cy - b.cy); }
inline CPoint operator+(POINT a, SIZE s)  { return CPoint(a.x + s.cx, a.y + s.cy); }
inline CPoint operator-(POINT a, SIZE s)  { return CPoint(a.x - s.cx, a.y - s.cy); }
inline CPoint operator+(POINT a, POINT b) { return CPoint(a.x + b.x, a.y + b.y); }
inline CRect  operator+(const RECT& r, POINT p) { return CRect(r.left+p.x, r.top+p.y, r.right+p.x, r.bottom+p.y); }
inline CRect  operator-(const RECT& r, POINT p) { return CRect(r.left-p.x, r.top-p.y, r.right-p.x, r.bottom-p.y); }
inline CRect  operator+(const RECT& r, SIZE s)  { return CRect(r.left+s.cx, r.top+s.cy, r.right+s.cx, r.bottom+s.cy); }

/* MFC standard dockbar / prompt resource IDs */
#ifndef AFX_IDW_DOCKBAR_TOP
#define AFX_IDW_DOCKBAR_TOP     0xE81B
#define AFX_IDW_DOCKBAR_BOTTOM  0xE81C
#define AFX_IDW_DOCKBAR_LEFT    0xE81D
#define AFX_IDW_DOCKBAR_RIGHT   0xE81E
#define AFX_IDW_TOOLBAR         0xE81B
#define AFX_IDW_STATUS_BAR      0xE801
#define AFX_IDP_FAILED_TO_LAUNCH_HELP 0xF010
#define AFX_IDP_COMMAND_FAILURE 0xF011
#endif

/* OLE stock-property dispids */
#ifndef DISPID_FORECOLOR
#define DISPID_FORECOLOR  (-501)
#define DISPID_BACKCOLOR  (-501)
#define DISPID_ENABLED    (-514)
#define DISPID_FONT       (-512)
#define DISPID_CAPTION    (-518)
#define DISPID_TEXT       (-517)
#define DISPID_VALUE      0
#endif

/* ============================================================
 * Object / command-target roots
 * ============================================================ */
class CObject {
public:
    CObject() {}
    virtual ~CObject() {}
    virtual void Serialize(class CArchive&) {}
    BOOL IsKindOf(const void*) const { return TRUE; }   /* RUNTIME_CLASS() is (void*)0 here */
};

class CCmdTarget : public CObject {
public:
    CCmdTarget() {}
};

/* ============================================================
 * GDI objects
 * ============================================================ */
class CGdiObject : public CObject {
public:
    HGDIOBJ m_hObject;
    CGdiObject() : m_hObject(NULL) {}
    HGDIOBJ GetSafeHandle() const { return m_hObject; }
    BOOL DeleteObject() { m_hObject = NULL; return TRUE; }
    BOOL Attach(HGDIOBJ h) { m_hObject = h; return TRUE; }
    HGDIOBJ Detach() { HGDIOBJ h = m_hObject; m_hObject = NULL; return h; }
};

class CFont : public CGdiObject {
public:
    CFont() {}
    BOOL CreateFontIndirect(const LOGFONT*) { return TRUE; }
    BOOL CreateFont(int, int, int, int, int, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, LPCSTR) { return TRUE; }
    BOOL CreatePointFont(int, LPCSTR, CDC* = NULL);
    operator HFONT() const { return (HFONT)m_hObject; }
};

class CPen : public CGdiObject {
public:
    CPen() {}
    CPen(int, int, COLORREF) {}
    CPen(int, int, const void*, int = 0) {}   /* ExtCreatePen geometric form (LOGBRUSH*) */
    BOOL CreatePen(int, int, COLORREF) { return TRUE; }
    operator HPEN() const { return (HPEN)m_hObject; }
};

class CBrush : public CGdiObject {
public:
    CBrush() {}
    CBrush(COLORREF) {}
    BOOL CreateSolidBrush(COLORREF) { return TRUE; }
    BOOL CreateStockObject(int) { return TRUE; }
    static CBrush* FromHandle(HBRUSH) { return NULL; }
    operator HBRUSH() const { return (HBRUSH)m_hObject; }
};

class CBitmap : public CGdiObject {
public:
    BOOL CreateCompatibleBitmap(CDC*, int, int) { return TRUE; }
    BOOL CreateBitmap(int, int, UINT, UINT, const void*) { return TRUE; }
    BOOL LoadBitmapA(LPCSTR) { return TRUE; }
    BOOL LoadBitmapA(UINT) { return TRUE; }
    static CBitmap* FromHandle(HBITMAP) { return NULL; }
    int GetBitmap(void*) { return 0; }
    operator HBITMAP() const { return (HBITMAP)m_hObject; }
};

class CDC : public CObject {
public:
    HDC m_hDC;
    CDC() : m_hDC(NULL) {}
    HDC GetSafeHdc() const { return m_hDC; }
    operator HDC() const { return m_hDC; }
    BOOL Attach(HDC h) { m_hDC = h; return TRUE; }
    HDC Detach() { HDC h = m_hDC; m_hDC = NULL; return h; }
    CGdiObject* SelectObject(CGdiObject*) { return NULL; }
    HGDIOBJ SelectStockObject(int) { return NULL; }
    CFont* SelectObject(CFont*) { return NULL; }
    CPen*  SelectObject(CPen*)  { return NULL; }
    CPen*  SelectObject(CPen&)  { return NULL; }
    CBrush* SelectObject(CBrush*) { return NULL; }
    COLORREF SetTextColor(COLORREF c) { return c; }
    COLORREF SetBkColor(COLORREF c) { return c; }
    int SetBkMode(int) { return 0; }
    BOOL TextOutA(int, int, LPCSTR, int) { return TRUE; }
    /* note: callers' `TextOut` is macro-mapped to TextOutA by wingdi; do not add a
       non-A TextOut member here (it would collide with TextOutA). */
    BOOL ExtTextOutA(int, int, UINT, LPCRECT, LPCSTR, UINT, LPINT) { return TRUE; }
    BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, LPCSTR s, UINT n, LPINT d) { return ExtTextOutA(x,y,o,r,s,n,d); }
    /* CString-accepting overloads (template triggers CString::operator LPCTSTR) */
    template<class S> BOOL TextOut(int x, int y, const S& s) { LPCSTR p=(LPCSTR)s; return TextOutA(x,y,p,(int)strlen(p)); }
    template<class S> BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, const S& s, UINT n, LPINT d) { return ExtTextOutA(x,y,o,r,(LPCSTR)s,n,d); }
    template<class S> BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, const S& s, LPINT d) { LPCSTR p=(LPCSTR)s; return ExtTextOutA(x,y,o,r,p,(UINT)strlen(p),d); }
    template<class S> int  DrawText(const S& s, LPRECT r, UINT f) { LPCSTR p=(LPCSTR)s; return DrawText(p,(int)strlen(p),r,f); }
    COLORREF SetPixel(int, int, COLORREF c) { return c; }
    COLORREF GetPixel(int, int) const { return 0; }
    BOOL Polygon(LPPOINT, int) { return TRUE; }
    BOOL Polyline(LPPOINT, int) { return TRUE; }
    BOOL Ellipse(int, int, int, int) { return TRUE; }
    BOOL Ellipse(LPCRECT) { return TRUE; }
    BOOL RoundRect(int, int, int, int, int, int) { return TRUE; }
    int  GetClipBox(LPRECT) const { return 0; }
    BOOL PtVisible(int, int) const { return TRUE; }
    BOOL RectVisible(LPCRECT) const { return TRUE; }
    UINT GetTextAlign() const { return 0; }
    int  GetTextFace(int, LPSTR) const { return 0; }
    BOOL GetTextMetricsA(void*) const { return TRUE; }
    BOOL Rectangle(int, int, int, int) { return TRUE; }
    POINT MoveTo(int, int) { POINT p={0,0}; return p; }
    POINT MoveTo(POINT) { POINT p={0,0}; return p; }
    BOOL LineTo(int, int) { return TRUE; }
    BOOL LineTo(POINT) { return TRUE; }
    BOOL BitBlt(int, int, int, int, CDC*, int, int, DWORD) { return TRUE; }
    BOOL CreateCompatibleDC(CDC*) { return TRUE; }
    int FillRect(LPCRECT, CBrush*) { return 0; }
    void FillSolidRect(LPCRECT, COLORREF) {}
    void FillSolidRect(int, int, int, int, COLORREF) {}
    void Draw3dRect(LPCRECT, COLORREF, COLORREF) {}
    void Draw3dRect(int, int, int, int, COLORREF, COLORREF) {}
    BOOL StretchBlt(int, int, int, int, CDC*, int, int, int, int, DWORD) { return TRUE; }
    int  GetDeviceCaps(int) const { return 0; }
    CSize GetTextExtent(LPCSTR, int) const { return CSize(0, 0); }
    template<class S> CSize GetTextExtent(const S& s) const { (void)s; return CSize(0, 0); }
    CSize GetOutputTextExtent(LPCSTR, int) const { return CSize(0, 0); }
    int  DrawText(LPCSTR, int, LPRECT, UINT) { return 0; }
    UINT SetTextAlign(UINT) { return 0; }
    int  SetMapMode(int) { return 0; }
    int  SetROP2(int) { return 0; }
    int  SetStretchBltMode(int) { return 0; }
    int  GetStretchBltMode() const { return 0; }
    static CDC* FromHandle(HDC) { return NULL; }
    POINT SetViewportOrg(int, int) { POINT p = {0,0}; return p; }
};

inline BOOL CFont::CreatePointFont(int, LPCSTR, CDC*) { return TRUE; }

class CPaintDC : public CDC { public: CPaintDC(CWnd*) {} };
class CClientDC : public CDC { public: CClientDC(CWnd*) {} };
class CWindowDC : public CDC { public: CWindowDC(CWnd*) {} };
class CMetaFileDC : public CDC { public: CMetaFileDC() {} };

/* ============================================================
 * Window / app hierarchy (stubbed — no real windows on Linux)
 * ============================================================ */
class CListBox;
class CWnd : public CCmdTarget {
public:
    enum { adjustBorder = 0, adjustOutside = 1 };
    HWND m_hWnd;
    CWnd() : m_hWnd(NULL) {}
    /* COleControl host-site ptr + dialog help-id, used by CRToolBar/CDialog code */
    void* m_pCtrlSite = NULL;
    UINT  m_nIDHelp = 0;
    BOOL IsZoomed() const { return FALSE; }
    void WinHelp(DWORD, UINT = 0) {}
    /* CWnd virtual handlers the toolbar/dialog fragments forward to via Base:: */
    void OnInitMenu(CMenu*) {}
    void OnInitMenuPopup(CMenu*, UINT, BOOL) {}
    void OnSetFont(CFont*) {}
    void OnCancelMode() {}
    void OnFinalRelease() {}
    void PreSubclassWindow() {}
    BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*) { return FALSE; }
    int  OnCharToItem(UINT, CListBox*, UINT) { return -1; }
    BOOL OnAmbientProperty(void*, DISPID, void*) { return FALSE; }
    static CWnd* WindowFromPoint(CPoint) { return NULL; }
    void CalcWindowRect(LPRECT, UINT = adjustBorder) {}
    HWND GetSafeHwnd() const { return m_hWnd; }
    operator HWND() const { return m_hWnd; }
    BOOL Attach(HWND h) { m_hWnd = h; return TRUE; }
    HWND Detach() { HWND h = m_hWnd; m_hWnd = NULL; return h; }
    CWnd* GetDlgItem(int) const { return NULL; }
    void  GetDlgItem(int, HWND* ph) const { if (ph) *ph = NULL; }
    int GetDlgItemTextA(int, LPSTR, int) { return 0; }
    void SetDlgItemTextA(int, LPCSTR) {}
    BOOL SetWindowTextA(LPCSTR) { return TRUE; }
    int GetWindowTextA(LPSTR, int) { return 0; }
    template<class S> int GetWindowTextA(S& s) { (void)s; return 0; }
    BOOL ShowWindow(int) { return TRUE; }
    BOOL UpdateWindow() { return TRUE; }
    BOOL DestroyWindow() { return TRUE; }
    BOOL MoveWindow(int, int, int, int, BOOL = TRUE) { return TRUE; }
    BOOL MoveWindow(LPCRECT, BOOL = TRUE) { return TRUE; }
    CWnd* GetTopWindow() const { return NULL; }
    static CWnd* GetDesktopWindow() { return NULL; }
    static CWnd* FromHandle(HWND) { return NULL; }
    CWnd* GetLastActivePopup() const { return NULL; }
    void GetClientRect(LPRECT r) const { if (r) { r->left = r->top = 0; r->right = r->bottom = 0; } }
    void GetWindowRect(LPRECT r) const { if (r) { r->left = r->top = r->right = r->bottom = 0; } }
    void ClientToScreen(LPPOINT) const {}
    void ClientToScreen(LPRECT) const {}
    void ScreenToClient(LPPOINT) const {}
    void ScreenToClient(LPRECT) const {}
    BOOL CreateControl(LPCSTR, LPCSTR, DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
    BOOL CreateControl(REFCLSID, LPCSTR, DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
    BOOL CreateControl(REFCLSID, LPCSTR, DWORD, const RECT&, CWnd*, UINT, CFile*, BOOL, BSTR) { return FALSE; }
    /* hosted-ActiveX-control accessors (ClassWizard wrappers call these) */
    void SetProperty(DISPID, VARTYPE, ...) {}
    void GetProperty(DISPID, VARTYPE, void*) const {}
    void InvokeHelper(DISPID, WORD, VARTYPE, void*, const BYTE*, ...) {}
    CWnd* GetNextWindow(UINT = 0) const { return NULL; }
    CWnd* GetWindow(UINT) const { return NULL; }
    int   GetDlgCtrlID() const { return 0; }
    LONG  GetWindowLong(int) const { return 0; }
    LONG  SetWindowLong(int, LONG) { return 0; }
    DWORD GetStyle() const { return 0; }
    DWORD GetExStyle() const { return 0; }
    CScrollBar* GetScrollBarCtrl(int) const { return NULL; }
    void  ModifyStyle(DWORD, DWORD, UINT = 0) {}
#if BOB_LINUX
    /* Linux port: return a shared no-op CDC (not NULL) so callers that deref the
       returned DC -- e.g. IconDescUI::LoadInstances(*pdc) in InitInstance -- have
       a valid object. A real GDI backend replaces this. */
    CDC* GetDC() { static CDC s_stubDC; return &s_stubDC; }
#else
    CDC* GetDC() { return NULL; }
#endif
    int  ReleaseDC(CDC*) { return 1; }
    BOOL EnableWindow(BOOL = TRUE) { return TRUE; }
    CWnd* SetFocus() { return NULL; }
    int MessageBoxA(LPCSTR, LPCSTR = NULL, UINT = 0) { return 0; }
    LRESULT SendMessageA(UINT, WPARAM = 0, LPARAM = 0) { return 0; }
    LRESULT SendMessage(UINT m, WPARAM w = 0, LPARAM l = 0) { return SendMessageA(m, w, l); }
    BOOL PostMessageA(UINT, WPARAM = 0, LPARAM = 0) { return TRUE; }
    /* `PostMessage`/`GetWindowText`/... callers are macro-mapped to the A names */
    void ScrollWindow(int, int, LPCRECT = NULL, LPCRECT = NULL) {}
    class CMenu* GetMenu() const { return NULL; }
    BOOL SetMenu(class CMenu*) { return TRUE; }
    void SendMessageToDescendants(UINT, WPARAM = 0, LPARAM = 0, BOOL = TRUE, BOOL = TRUE) {}
    void DrawMenuBar() {}
    void DragAcceptFiles(BOOL = TRUE) {}
    void RedrawWindow(LPCRECT = NULL, HRGN = NULL, UINT = 0) {}
    LRESULT DefWindowProc(UINT, WPARAM, LPARAM) { return 0; }
    BOOL ModifyStyleEx(DWORD, DWORD, UINT = 0) { return TRUE; }
    CDC* BeginPaint(LPPAINTSTRUCT) { return NULL; }
    void EndPaint(LPPAINTSTRUCT) {}
    void GetWindowPlacement(void*) const {}
    void SetWindowPlacement(const void*) {}
    int  RunModalLoop(DWORD = 0) { return 0; }
    void Invalidate(BOOL = TRUE) {}
    void InvalidateRect(LPCRECT, BOOL = TRUE) {}
    void ClientToScreenRect(LPRECT) const {}
    BOOL SetTimer(UINT, UINT, void* = NULL) { return TRUE; }
    BOOL KillTimer(UINT) { return TRUE; }
    void SetWindowPos(const CWnd*, int, int, int, int, UINT) {}
    void BringWindowToTop() {}
    BOOL IsWindowVisible() const { return FALSE; }
    void SetCapture() {}
    CWnd* GetParent() const { return NULL; }
    CWnd* GetParentFrame() const { return NULL; }
    CWnd* GetParentOwner() const { return NULL; }
    BOOL UpdateData(BOOL = TRUE) { return TRUE; }
    virtual BOOL OnInitDialog() { return TRUE; }
    virtual void DoDataExchange(class CDataExchange*) {}
    virtual LRESULT WindowProc(UINT, WPARAM, LPARAM) { return 0; }
    /* standard message handlers (derived classes call base::OnXxx) */
    afx_msg int  OnCreate(void*) { return 0; }
    afx_msg void OnDestroy() {}
    afx_msg void OnPaint() {}
    afx_msg void OnSize(UINT, int, int) {}
    afx_msg void OnTimer(UINT_PTR) {}
    afx_msg void OnClose() {}
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnLButtonDown(UINT, CPoint) {}
    afx_msg void OnLButtonUp(UINT, CPoint) {}
    afx_msg void OnLButtonDblClk(UINT, CPoint) {}
    afx_msg void OnRButtonDown(UINT, CPoint) {}
    afx_msg void OnRButtonUp(UINT, CPoint) {}
    afx_msg void OnMouseMove(UINT, CPoint) {}
    afx_msg BOOL OnMouseWheel(UINT, short, CPoint) { return FALSE; }
    afx_msg BOOL OnSetCursor(CWnd*, UINT, UINT) { return TRUE; }
    afx_msg void OnKeyDown(UINT, UINT, UINT) {}
    afx_msg void OnKeyUp(UINT, UINT, UINT) {}
    afx_msg void OnChar(UINT, UINT, UINT) {}
    afx_msg void OnHScroll(UINT, UINT, CScrollBar*) {}
    afx_msg void OnVScroll(UINT, UINT, CScrollBar*) {}
    afx_msg void OnSetFocus(CWnd*) {}
    afx_msg void OnKillFocus(CWnd*) {}
    afx_msg void OnActivate(UINT, CWnd*, BOOL) {}
    afx_msg void OnMove(int, int) {}
    afx_msg void OnShowWindow(BOOL, UINT) {}
    afx_msg void OnEnable(BOOL) {}
    afx_msg void OnWindowPosChanging(void*) {}
    afx_msg void OnWindowPosChanged(void*) {}
    afx_msg void OnCaptureChanged(CWnd*) {}
    afx_msg LRESULT OnNotify(WPARAM, LPARAM, LRESULT*) { return 0; }
    afx_msg BOOL OnCommand(WPARAM, LPARAM) { return TRUE; }
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*) {}
    afx_msg void OnDevModeChange(LPSTR) {}
    afx_msg void OnActivateApp(BOOL, DWORD) {}
    afx_msg void OnHelp() {}
    afx_msg void OnHelpFinder() {}
    afx_msg void OnHelpIndex() {}
    afx_msg void OnHelpUsing() {}
    afx_msg void OnContextHelp() {}
    afx_msg LRESULT OnHelpInfo(struct tagHELPINFO*) { return 0; }
    afx_msg LRESULT OnCommandHelp(WPARAM = 0, LPARAM = 0) { return 0; }
    afx_msg int  OnMouseActivate(CWnd*, UINT, UINT) { return 1; }
    virtual BOOL PreCreateWindow(struct tagCREATESTRUCTA&) { return TRUE; }
    virtual BOOL PreTranslateMessage(void*) { return FALSE; }
    virtual void PostNcDestroy() {}
    virtual BOOL OnCmdMsg(UINT, int, void*, AFX_CMDHANDLERINFO*) { return FALSE; }
    afx_msg void OnNcMouseMove(UINT, CPoint) {}
    afx_msg void OnNcLButtonDown(UINT, CPoint) {}
    afx_msg LRESULT OnNcHitTest(CPoint) { return 0; }
    afx_msg void OnNcPaint() {}
    afx_msg BOOL OnNcCreate(void*) { return TRUE; }
    void MapDialogRect(LPRECT) const {}
    BOOL IsFrameWnd() const { return FALSE; }
    CWnd* GetTopLevelParent() const { return NULL; }
    CWnd* GetTopLevelFrame() const { return NULL; }
    CWnd* GetTopLevelOwner() const { return NULL; }
    int   GetSystemMetrics(int) const { return 0; }
};

/* Common control wrappers (all CWnd-derived stubs) */
class CStatic : public CWnd {
public:
    BOOL Create(LPCSTR, DWORD, const RECT&, CWnd*, UINT = 0) { return TRUE; }
    void SetBitmap(HBITMAP) {}
};
class CButton : public CWnd {
public:
    BOOL Create(LPCSTR, DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    UINT GetState() const { return 0; }
    void SetState(BOOL) {}
    int  GetCheck() const { return 0; }
    void SetCheck(int) {}
};
class CEdit : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    void SetSel(int, int, BOOL = FALSE) {}
    void GetSel(int&, int&) const {}
    int  LineLength(int = -1) const { return 0; }
};
class CListBox : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  AddStringA(LPCSTR) { return 0; }
    int  GetCurSel() const { return -1; }
    int  SetCurSel(int) { return -1; }
    int  GetCount() const { return 0; }
    void ResetContent() {}
    DWORD GetItemData(int) const { return 0; }
    int  SetItemData(int, DWORD) { return 0; }
};
class CComboBox : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  AddStringA(LPCSTR) { return 0; }
    int  GetCurSel() const { return -1; }
    int  SetCurSel(int) { return -1; }
    int  GetCount() const { return 0; }
    void ResetContent() {}
    DWORD GetItemData(int) const { return 0; }
};
class CScrollBar : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  GetScrollPos() const { return 0; }
    int  SetScrollPos(int, BOOL = TRUE) { return 0; }
    void SetScrollRange(int, int, BOOL = TRUE) {}
    void GetScrollRange(LPINT, LPINT) const {}
};
class CToolBar : public CWnd {
public:
    BOOL Create(CWnd*, DWORD = 0, UINT = 0) { return TRUE; }
};
class CMenu : public CObject {
public:
    HMENU m_hMenu;
    CMenu() : m_hMenu(NULL) {}
    HMENU GetSafeHmenu() const { return m_hMenu; }
    CMenu* GetSubMenu(int) const { return NULL; }
    UINT GetMenuItemCount() const { return 0; }
    BOOL AppendMenuA(UINT, UINT_PTR = 0, LPCSTR = NULL) { return TRUE; }
    BOOL AppendMenu(UINT f, UINT_PTR id = 0, LPCSTR s = NULL) { return AppendMenuA(f, id, s); }
    BOOL InsertMenu(UINT, UINT, UINT_PTR = 0, LPCSTR = NULL) { return TRUE; }
    BOOL ModifyMenu(UINT, UINT, UINT_PTR = 0, LPCSTR = NULL) { return TRUE; }
    BOOL DeleteMenu(UINT, UINT) { return TRUE; }
    BOOL RemoveMenu(UINT, UINT) { return TRUE; }
    BOOL CreatePopupMenu() { return TRUE; }
    BOOL CreateMenu() { return TRUE; }
    BOOL LoadMenu(UINT) { return TRUE; }
    BOOL LoadMenu(LPCSTR) { return TRUE; }
    BOOL DestroyMenu() { return TRUE; }
    void Attach(HMENU h) { m_hMenu = h; }
    HMENU Detach() { HMENU h = m_hMenu; m_hMenu = NULL; return h; }
    BOOL EnableMenuItem(UINT, UINT) { return TRUE; }
    BOOL CheckMenuItem(UINT, UINT) { return TRUE; }
    UINT GetMenuState(UINT, UINT) const { return 0; }
    int  GetMenuStringA(UINT, LPSTR, int, UINT) const { return 0; }
    BOOL SetMenuItemBitmaps(UINT, UINT, CBitmap*, CBitmap*) { return TRUE; }
    BOOL TrackPopupMenu(UINT, int, int, CWnd*, LPCRECT = NULL) { return TRUE; }
};

/* SetWindowPos z-order sentinels (MFC globals: &wndTop etc.) */
static const CWnd wndTop, wndBottom, wndTopMost, wndNoTopMost;

class CDialog : public CWnd {
public:
    CDialog() {}
    CDialog(UINT, CWnd* = NULL) {}
    CDialog(LPCSTR, CWnd* = NULL) {}
    virtual int DoModal() { return -1; }   /* IDCANCEL-ish */
    BOOL Create(UINT, CWnd* = NULL) { return TRUE; }
    virtual void OnOK() {}
    virtual void OnCancel() {}
    virtual LRESULT OnCommandHelp(WPARAM, LPARAM) { return 0; }
    void EndDialog(int) {}
    void GotoDlgCtrl(CWnd*) {}
    void NextDlgCtrl() const {}
};

class CView : public CWnd {
public:
    CDocument* m_pDocument;
    CView() : m_pDocument(NULL) {}
    virtual void OnDraw(CDC*) {}
    CDocument* GetDocument() const { return m_pDocument; }
    CScrollBar* GetScrollBarCtrl(int) const { return NULL; }
    virtual BOOL OnPreparePrinting(CPrintInfo*) { return TRUE; }
    virtual void OnBeginPrinting(CDC*, CPrintInfo*) {}
    virtual void OnEndPrinting(CDC*, CPrintInfo*) {}
    virtual void OnPrint(CDC*, CPrintInfo*) {}
    BOOL DoPreparePrinting(CPrintInfo*) { return TRUE; }
};

/* COleControl — MFC ActiveX control base (bob's CR* control impls derive from it) */
class COleControl : public CWnd {
public:
    BOOL m_bAutoSize;
    virtual void OnDraw(CDC*, const CRect&, const CRect&) {}
    virtual void DoPropExchange(CPropExchange*) {}
    virtual void OnResetState() {}
    virtual void OnDrawMetafile(CDC*, const CRect&) {}
    void InvalidateControl(LPCRECT = NULL) {}
    void SetModifiedFlag(BOOL = TRUE) {}
    void BoundPropertyChanged(DISPID) {}
    BOOL GetControlSize(int*, int*) { return TRUE; }
    BOOL SetControlSize(int, int) { return TRUE; }
    void SetInitialSize(int, int) {}
    COleControl* GetControlUnknown() { return this; }
    void FireEventV(DISPID, const char*, va_list) {}
    void ThrowError(SCODE, LPCSTR = NULL) {}
    void SetNotPermitted() {}
    void SetNotSupported() {}
    BOOL DoSuperclassPaint(CDC*, const CRect&) { return TRUE; }
    BSTR  GetText() { return NULL; }
    void  SetText(LPCSTR) {}
    OLE_COLOR GetForeColor() { return 0; }
    OLE_COLOR GetBackColor() { return 0; }
    CFont* SelectStockFont(CDC*) { return NULL; }
    BOOL IsModified() const { return FALSE; }
    void Refresh() {}
};

class CFrameWnd : public CWnd {
public:
    BOOL Create(LPCSTR, LPCSTR, DWORD = 0, const RECT& = CRect(), CWnd* = NULL, LPCSTR = NULL) { return TRUE; }
    CView* GetActiveView() const { return NULL; }
    CDocument* GetActiveDocument() const { return NULL; }
    void RecalcLayout(BOOL = TRUE) {}
    BOOL SetActiveView(CView*, BOOL = TRUE) { return TRUE; }
    void ExitHelpMode() {}
};

/* CFile / CArchive / CPrintInfo (afx.h) */
class CFile : public CObject {
public:
    enum { modeRead = 0, modeWrite = 1, modeReadWrite = 2, modeCreate = 0x1000,
           shareDenyNone = 0x40, shareDenyWrite = 0x20, typeBinary = 0x4000, begin = 0, current = 1, end = 2 };
    HANDLE m_hFile;
    CFile() : m_hFile(NULL) {}
    virtual BOOL Open(LPCSTR, UINT, void* = NULL) { return FALSE; }
    virtual UINT Read(void*, UINT) { return 0; }
    virtual void Write(const void*, UINT) {}
    virtual void Close() {}
    virtual DWORD GetLength() const { return 0; }
    virtual LONG Seek(LONG, UINT) { return 0; }
};

class CArchive {
public:
    enum Mode { load = 0, store = 1 };
    BOOL IsStoring() const { return FALSE; }
    BOOL IsLoading() const { return TRUE; }
};

class CPrintInfo {
public:
    BOOL m_bContinuePrinting;
    UINT m_nCurPage;
    CRect m_rectDraw;
    CPrintInfo() : m_bContinuePrinting(TRUE), m_nCurPage(1) {}
    void SetMaxPage(UINT) {}
    void SetMinPage(UINT) {}
    UINT GetFromPage() const { return 1; }
    UINT GetToPage() const { return 1; }
};

class CDocument : public CCmdTarget {
public:
    virtual BOOL OnNewDocument() { return TRUE; }
    virtual void Serialize(CArchive&) {}
    void SetTitle(LPCSTR) {}
    LPCSTR GetTitle() const { return ""; }
    void SetPathName(LPCSTR, BOOL = TRUE) {}
    LPCSTR GetPathName() const { return ""; }
    void EnableCompoundFile(BOOL = TRUE) {}
    void SetModifiedFlag(BOOL = TRUE) {}
    BOOL IsModified() { return FALSE; }
    void UpdateAllViews(CView*, LPARAM = 0, CObject* = NULL) {}
    CView* GetNextView(POSITION&) const { return NULL; }
    POSITION GetFirstViewPosition() const { return (POSITION)0; }
};

class COleDocument : public CDocument {};

void CDocument_dummy();
/* extend CDocument with the methods bob calls (added here to keep the class above
   minimal); these are just declared inline on a derived-friendly basis */

class CDocTemplate : public CCmdTarget {
public:
    CDocTemplate(UINT, void* = NULL, void* = NULL, void* = NULL) {}
};
class CSingleDocTemplate : public CDocTemplate {
public:
    CSingleDocTemplate(UINT id, void* a = NULL, void* b = NULL, void* c = NULL) : CDocTemplate(id,a,b,c) {}
    void SetContainerInfo(UINT) {}
    void SetServerInfo(UINT, UINT = 0, UINT = 0, void* = NULL, void* = NULL) {}
};
class CMultiDocTemplate : public CDocTemplate {
public:
    CMultiDocTemplate(UINT id, void* a = NULL, void* b = NULL, void* c = NULL) : CDocTemplate(id,a,b,c) {}
};

/* MFC OLE / app-init globals */
static inline BOOL AfxOleInit() { return TRUE; }
static inline void AfxEnableControlContainer(void* = NULL) {}
static inline BOOL AfxOleGetUserCtrl() { return FALSE; }
static inline void AfxPostQuitMessage(int = 0) {}
static inline void AfxOleSetUserCtrl(BOOL) {}
static inline CWinApp* AfxGetAppHelper() { return NULL; }

/* DDX/DDV (dialog data exchange) — no-ops */
static inline void DDX_Control(CDataExchange*, int, CWnd&) {}
static inline void DDX_Text(CDataExchange*, int, int&) {}
static inline void DDX_Check(CDataExchange*, int, int&) {}
static inline void DDX_Radio(CDataExchange*, int, int&) {}
static inline void DDX_LBIndex(CDataExchange*, int, int&) {}
static inline void DDX_CBIndex(CDataExchange*, int, int&) {}

class CWinThread : public CCmdTarget {
public:
    CWnd* m_pMainWnd;
    CWnd* m_pActiveWnd;
    MSG   m_msgCur;
    CWinThread() : m_pMainWnd(NULL), m_pActiveWnd(NULL) {}
    virtual BOOL InitInstance() { return TRUE; }
    virtual int  ExitInstance() { return 0; }
    virtual int  Run() { return 0; }
};

class CWinApp : public CWinThread {
public:
    LPCSTR m_pszAppName;
    LPCSTR m_pszHelpFilePath;
    LPCSTR m_pszProfileName;
    LPCSTR m_pszExeName;
    HINSTANCE m_hInstance;
    LPSTR  m_lpCmdLine;
    int    m_nCmdShow;
    CWinApp(LPCSTR n = NULL) : m_pszAppName(n), m_hInstance(NULL), m_lpCmdLine(NULL), m_nCmdShow(0) {}
    virtual BOOL InitInstance() { return TRUE; }
    BOOL InitApplication() { return TRUE; }
    HCURSOR LoadStandardCursor(LPCSTR) const { return NULL; }
    HCURSOR LoadCursor(LPCSTR) const { return NULL; }
    HCURSOR LoadCursor(UINT) const { return NULL; }
    HICON   LoadIcon(LPCSTR) const { return NULL; }
    HICON   LoadIcon(UINT) const { return NULL; }
    HICON   LoadStandardIcon(LPCSTR) const { return NULL; }
    int     DoMessageBox(LPCSTR, UINT, UINT) { return 0; }
    void    ParseCommandLine(CCommandLineInfo&) {}
    BOOL    ProcessShellCommand(CCommandLineInfo&) { return TRUE; }
    void    EnableShellOpen() {}
    void    LoadStdProfileSettings(UINT = 0) {}
    BOOL    OnIdle(LONG) { return FALSE; }
    void    WinHelp(DWORD, UINT = 0) {}
    void    HtmlHelp(DWORD, UINT = 0) {}
    void    SetRegistryKey(LPCSTR) {}
    void    SetRegistryKey(UINT) {}
    BOOL    PumpMessage() { return TRUE; }
    BOOL    IsIdleMessage(void*) { return TRUE; }
    void    Enable3dControls() {}
    void    Enable3dControlsStatic() {}
    void    AddDocTemplate(void*) {}
    HCURSOR DoWaitCursor(int) { return NULL; }
    void    RestoreWaitCursor() {}
    void    BeginWaitCursor() {}
    void    EndWaitCursor() {}
    UINT    GetProfileIntA(LPCSTR, LPCSTR, int n) { return n; }
    BOOL    WriteProfileIntA(LPCSTR, LPCSTR, int) { return TRUE; }
    CString GetProfileStringA(LPCSTR, LPCSTR, LPCSTR = NULL);
    BOOL    WriteProfileStringA(LPCSTR, LPCSTR, LPCSTR) { return TRUE; }
};

/* ============================================================
 * Container templates (afxtempl) — minimal, std-backed
 * ============================================================ */
template <class TYPE, class ARG_TYPE = const TYPE&>
class CArray : public CObject {
    std::vector<TYPE> v;
public:
    int  GetSize() const { return (int)v.size(); }
    int  GetCount() const { return (int)v.size(); }
    void SetSize(int n, int = -1) { v.resize(n); }
    void RemoveAll() { v.clear(); }
    int  Add(ARG_TYPE x) { v.push_back(x); return (int)v.size() - 1; }
    TYPE& operator[](int i) { return v[i]; }
    const TYPE& operator[](int i) const { return v[i]; }
    TYPE& GetAt(int i) { return v[i]; }
    void SetAt(int i, ARG_TYPE x) { v[i] = x; }
    void RemoveAt(int i, int n = 1) { v.erase(v.begin()+i, v.begin()+i+n); }
};

template <class TYPE, class ARG_TYPE = const TYPE&>
class CList : public CObject {
    std::list<TYPE> l;
public:
    int  GetCount() const { return (int)l.size(); }
    BOOL IsEmpty() const { return l.empty(); }
    void RemoveAll() { l.clear(); }
    void AddTail(ARG_TYPE x) { l.push_back(x); }
    void AddHead(ARG_TYPE x) { l.push_front(x); }
    TYPE& GetHead() { return l.front(); }
    TYPE& GetTail() { return l.back(); }
    /* POSITION iteration — stubbed empty (UI lists aren't driven at runtime yet).
       GetHeadPosition()==NULL makes the usual for(pos; pos; GetNext) loops no-op. */
    POSITION GetHeadPosition() const { return (POSITION)0; }
    POSITION GetTailPosition() const { return (POSITION)0; }
    TYPE& GetNext(POSITION&)  { static TYPE d = TYPE(); return d; }
    TYPE& GetPrev(POSITION&)  { static TYPE d = TYPE(); return d; }
    TYPE& GetAt(POSITION)     { static TYPE d = TYPE(); return d; }
    void  SetAt(POSITION, ARG_TYPE) {}
    POSITION Find(ARG_TYPE) const { return (POSITION)0; }
    POSITION FindIndex(int) const { return (POSITION)0; }
    void RemoveAt(POSITION) {}
    void InsertBefore(POSITION, ARG_TYPE) {}
    void InsertAfter(POSITION, ARG_TYPE) {}
};

class CCommandLineInfo {
public:
    BOOL m_bShowSplash;
    BOOL m_bRunEmbedded;
    BOOL m_bRunAutomated;
    CCommandLineInfo() : m_bShowSplash(TRUE), m_bRunEmbedded(FALSE), m_bRunAutomated(FALSE) {}
    void ParseParam(LPCSTR, BOOL, BOOL) {}
};

class CPropExchange {
public:
    BOOL IsLoading() const { return TRUE; }
    BOOL ExchangeProp(LPCSTR, VARTYPE, void*, const void* = NULL) { return TRUE; }
    BOOL ExchangeVersion(DWORD&, DWORD, BOOL = TRUE) { return TRUE; }
};

/* MFC OLE-control event descriptor (used in CCmdTarget::OnCmdMsg event sinks) */
class AFX_EVENT {
public:
    enum EventType { event = 0, command = 1, propRequestEdit = 2 };
    AFX_EVENT(int = event, DISPID = 0, void* = NULL, void* = NULL, void* = NULL) {}
    int      m_eventKind;
    DISPID   m_dispid;
};

class CDataExchange {
public:
    BOOL m_bSaveAndValidate;
    CWnd* m_pDlgWnd;
    CWnd* PrepareCtrl(int) { return NULL; }
    CWnd* PrepareEditCtrl(int) { return NULL; }
};

class COleDispatchDriver {
public:
    LPDISPATCH m_lpDispatch;
    BOOL m_bAutoRelease;
    COleDispatchDriver() : m_lpDispatch(NULL), m_bAutoRelease(TRUE) {}
    COleDispatchDriver(LPDISPATCH p, BOOL autoRel = TRUE) : m_lpDispatch(p), m_bAutoRelease(autoRel) {}
    COleDispatchDriver(const COleDispatchDriver& s) : m_lpDispatch(s.m_lpDispatch), m_bAutoRelease(FALSE) {}
    void AttachDispatch(LPDISPATCH p, BOOL = TRUE) { m_lpDispatch = p; }
    LPDISPATCH DetachDispatch() { LPDISPATCH p = m_lpDispatch; m_lpDispatch = NULL; return p; }
    void ReleaseDispatch() {}
    BOOL CreateDispatch(REFCLSID, void* = NULL) { return FALSE; }
    BOOL CreateDispatch(LPCSTR, void* = NULL) { return FALSE; }
    void InvokeHelper(DISPID, WORD, VARTYPE, void*, const BYTE*, ...) {}
    void SetProperty(DISPID, VARTYPE, ...) {}
    void GetProperty(DISPID, VARTYPE, void*) const {}
};

struct AFX_CMDHANDLERINFO { CCmdTarget* pTarget; void* pmf; };

/* misc MFC/Win32 control-bar + help + dispatch bits */
#ifndef CBRS_GRIPPER
#define CBRS_TOP            0x0001
#define CBRS_BOTTOM         0x0002
#define CBRS_LEFT           0x0004
#define CBRS_RIGHT          0x0008
#define CBRS_ALIGN_ANY      0x000F
#define CBRS_ALIGN_TOP      0x0001
#define CBRS_ALIGN_BOTTOM   0x0002
#define CBRS_ALIGN_LEFT     0x0004
#define CBRS_ALIGN_RIGHT    0x0008
#define CBRS_BORDER_TOP     0x0100
#define CBRS_BORDER_ANY     0x0F00
#define CBRS_GRIPPER        0x00400000
#define CBRS_TOOLTIPS       0x00010000
#define CBRS_FLYBY          0x00020000
#define CBRS_SIZE_DYNAMIC   0x00040000
#endif
#ifndef DISPATCH_METHOD
#define DISPATCH_METHOD     0x1
#define DISPATCH_PROPERTYGET 0x2
#define DISPATCH_PROPERTYPUT 0x4
#endif
#ifndef HID_BASE_RESOURCE
#define HID_BASE_RESOURCE   0x00020000
#define HID_BASE_COMMAND    0x00010000
#endif
typedef HANDLE HTASK;
typedef struct tagHELPINFO { UINT cbSize; int iContextType; int iCtrlId; HANDLE hItemHandle; DWORD_PTR dwContextId; POINT MousePos; } HELPINFO, *LPHELPINFO;
struct AFX_MSGMAP { const AFX_MSGMAP* (*pfnGetBaseMap)(); const void* lpEntries; };

#ifndef HELP_CONTEXT
#define HELP_CONTEXT      0x0001
#define HELP_QUIT         0x0002
#define HELP_INDEX        0x0003
#define HELP_CONTENTS     0x0003
#define HELP_HELPONHELP   0x0004
#define HELP_SETINDEX     0x0005
#define HELP_KEY          0x0101
#define HELP_COMMAND      0x0102
#define HELP_FINDER       0x000B
#endif

#ifndef ID_SEPARATOR
#define ID_SEPARATOR        0
#define ID_INDICATOR_CAPS   0xE721
#define ID_INDICATOR_NUM    0xE722
#define ID_INDICATOR_SCRL   0xE723
#define ID_INDICATOR_EXT    0xE720
#define AFX_IDS_IDLEMESSAGE 0xE001
#endif

/* MFC RAII wait cursor — no-op */
class CWaitCursor { public: CWaitCursor() {} ~CWaitCursor() {} void Restore() {} };

/* Resource handle + LoadString backed by the PE resource loader (bob_resources.cpp). */
extern "C" void* bob_GetResourceHandle(void);
extern "C" void  bob_SetResourceHandle(void*);
extern "C" int   bob_load_string(void* h, unsigned id, char* buf, int maxlen);
static inline int AfxLoadString(UINT id, LPSTR buf, UINT max = 256) { return bob_load_string(bob_GetResourceHandle(), id, buf, (int)max); }
static inline HINSTANCE AfxGetResourceHandle() { return (HINSTANCE)bob_GetResourceHandle(); }
static inline void AfxSetResourceHandle(HINSTANCE h) { bob_SetResourceHandle((void*)h); }

/* The global application object (defined by IMPLEMENT'd CWinApp subclass in bob) */
extern CWinApp* AfxGetApp();
extern HINSTANCE AfxGetInstanceHandle();
extern CWnd* AfxGetMainWnd();
inline void AfxMessageBox(LPCSTR) {}

/* MFC worker-thread spawn. Single-thread bring-up: stubbed (no thread started);
   the periodic 3D draw loop is driven from the main loop instead. AFX_CDECL is
   the cdecl calling-convention tag (empty on Linux/gcc). */
#ifndef AFX_CDECL
#define AFX_CDECL
#endif
typedef UINT (AFX_CDECL *AFX_THREADPROC)(LPVOID);
/* Linux port: run the thread proc on a real (detached) pthread -- the per-view
   draw loop (View3d::drawloop) is spawned via this. See bob_threads.cpp. */
extern "C" void bob_begin_thread(unsigned int (*fn)(void*), void* arg);
inline CWinThread* AfxBeginThread(AFX_THREADPROC threadFn, LPVOID arg, int = 0, UINT = 0, DWORD = 0, void* = NULL) {
    bob_begin_thread((unsigned int(*)(void*))threadFn, arg);
    static CWinThread s_dummyThread;   /* callers only use the pointer as non-NULL */
    return &s_dummyThread;
}
inline CWinThread* AfxBeginThread(const void*, int = 0, UINT = 0, DWORD = 0, void* = NULL) { return NULL; }

/* ANSI/Unicode-neutral aliases bob calls without the A suffix */
#ifndef GetDlgItemText
#define GetDlgItemText GetDlgItemTextA
#endif

#endif /* FF_LINUX */
#endif /* FF_COMPAT_AFXWIN_H */
