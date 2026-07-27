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
#define _AFXWIN_H_

#include <stdarg.h>     // Linux/GCC port: va_list for hosted-OCX dispatch forwarding
#include <typeinfo>     // Linux/GCC port: typeid() for eventsink class matching
#include "windows.h"
#include "objbase.h"
#include "cstring.h"   // Linux/GCC port: define CString (game's own, guarded by #ifndef __AFX_H__) BEFORE we claim __AFX_H__
#define __AFX_H__

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
#define ON_EN_UPDATE(id, fn)        // Linux/GCC port
#define ON_MESSAGE_CLASS(...)		// Linux/GCC port: variadic — callers use 2- and 3-arg forms
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
/* Linux/GCC port: real OCX-event routing (ma_eventsink.cpp). The eventsink map becomes a
   per-class member MaRegEvents() (so it can take the addresses of the PROTECTED afx_msg
   handlers); a file-scope registrar auto-calls it at startup, registering {dialog-CLASS,
   control-id, event-dispid} -> thunk. ma_evt_fire matches by the dialog's RUNTIME type
   (typeid -- the many dialogs reuse the same IDC_ ids). ma_evt_call adapts to each handler
   signature (overload resolution on the member-fn-ptr type). */
extern "C" void ma_evt_register(const void* tinfo, int id, int dispid, void (*thunk)(void*));
extern "C" int  ma_evt_fire(void* dlg, const void* tinfo, int id, int dispid);
extern "C" { extern long ma_evtA0, ma_evtA1; extern void* ma_evtP; }
template<class C> inline void ma_evt_call(C* c, void (C::*f)())          { (c->*f)(); }
template<class C> inline void ma_evt_call(C* c, void (C::*f)(int))       { (c->*f)((int)ma_evtA0); }
template<class C> inline void ma_evt_call(C* c, void (C::*f)(long))      { (c->*f)((long)ma_evtA0); }
template<class C> inline void ma_evt_call(C* c, void (C::*f)(short))     { (c->*f)((short)ma_evtA0); }
template<class C> inline void ma_evt_call(C* c, void (C::*f)(int,int))   { (c->*f)((int)ma_evtA0,(int)ma_evtA1); }
template<class C> inline void ma_evt_call(C* c, void (C::*f)(long,long)) { (c->*f)((long)ma_evtA0,(long)ma_evtA1); }
template<class C> inline void ma_evt_call(C* c, void (C::*f)(LPCSTR))    { (c->*f)((LPCSTR)ma_evtP); }
template<class C, class M> inline void ma_evt_call(C*, M) {}   /* fallback: uncovered signature */
#define MA_EVT_CAT2(a,b) a##b
#define MA_EVT_CAT(a,b) MA_EVT_CAT2(a,b)
#define DECLARE_EVENTSINK_MAP() public: static void MaRegEvents();
#define BEGIN_EVENTSINK_MAP(theClass, baseClass) \
    static struct MA_EVT_CAT(MaEvtAuto_,__LINE__) { MA_EVT_CAT(MaEvtAuto_,__LINE__)(); } MA_EVT_CAT(g_maEvtAuto_,__LINE__); \
    MA_EVT_CAT(MaEvtAuto_,__LINE__)::MA_EVT_CAT(MaEvtAuto_,__LINE__)() { theClass::MaRegEvents(); } \
    void theClass::MaRegEvents() {
#define END_EVENTSINK_MAP() }
#define DECLARE_EVENT_MAP()
#define BEGIN_EVENT_MAP(theClass, baseClass)
#define END_EVENT_MAP()
#define ON_EVENT(theClass, id, dispid, fn, vts) \
    { struct MA_EVT_CAT(MaT_,__LINE__) { static void thunk(void* d){ ma_evt_call((theClass*)d, &theClass::fn); } }; \
      ma_evt_register(&typeid(theClass), (int)(id), (int)(dispid), &MA_EVT_CAT(MaT_,__LINE__)::thunk); }
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
#define ON_WM_GETDLGCODE()
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
#define DISPID_BACKCOLOR  (-503)
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
/* GDI software-canvas backend (ma_gdi.cpp). HDC/HGDIOBJ are opaque handles into it. */
extern "C" {
    void* ma_gdi_screen_dc(void);
    void* ma_gdi_create_dc(void);
    void  ma_gdi_delete_dc(void*);
    void* ma_gdi_create_bitmap(int, int);
    void  ma_gdi_delete_bitmap(void*);
    void  ma_gdi_bitmap_size(void*, int*, int*);
    void* ma_gdi_select_bitmap(void*, void*);
    void  ma_gdi_set_pen(void*, int, unsigned, int);
    void  ma_gdi_set_brush(void*, unsigned, int);
    void  ma_gdi_set_text_color(void*, unsigned);
    void  ma_gdi_set_bk_color(void*, unsigned);
    void  ma_gdi_set_bk_mode(void*, int);
    void  ma_gdi_set_font(void*, void*);
    void* ma_gdi_get_font(void*);
    void  ma_gdi_set_viewport_org(void*, int, int, int*, int*);
    void  ma_gdi_fill_solid(void*, int, int, int, int, unsigned);
    void  ma_gdi_fill_rect(void*, int, int, int, int, unsigned);
    void  ma_gdi_rectangle(void*, int, int, int, int);
    void  ma_gdi_move_to(void*, int, int);
    void  ma_gdi_line_to(void*, int, int);
    void  ma_gdi_set_pixel(void*, int, int, unsigned);
    unsigned ma_gdi_get_pixel(void*, int, int);
    void  ma_gdi_bitblt(void*, int, int, int, int, void*, int, int, unsigned long);
    void  ma_gdi_stretchblt(void*, int, int, int, int, void*, int, int, int, int, unsigned long);
    void  ma_gdi_text_out(void*, int, int, const char*, int);
    void  ma_gdi_get_text_metrics(void*, void*);
    void  ma_gdi_get_text_extent(void*, const char*, int, int*, int*);
    /* font subsystem */
    void* ma_gdi_font_create(int height, int weight, int italic, const char* face);
    void  ma_gdi_font_delete(void*);
}

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
    ~CFont() { if (m_hObject) { ma_gdi_font_delete(m_hObject); m_hObject = NULL; } }
    BOOL CreateFontIndirect(const LOGFONT* lf);
    BOOL CreateFont(int h, int, int, int, int weight, BYTE italic, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, LPCSTR face) {
        if (m_hObject) ma_gdi_font_delete(m_hObject);
        m_hObject = (HGDIOBJ)ma_gdi_font_create(h, weight, italic, face); return TRUE; }
    int  GetLogFont(LOGFONT* p) { if(p){LOGFONT z={0}; *p=z;} return sizeof(LOGFONT); }  // Linux/GCC port
    BOOL CreatePointFont(int, LPCSTR, CDC* = NULL);
    operator HFONT() const { return (HFONT)m_hObject; }
};

class CPen : public CGdiObject {
public:
    COLORREF ma_color; int ma_width; int ma_null;
    CPen() : ma_color(0), ma_width(1), ma_null(0) {}
    CPen(int s, int w, COLORREF c) : ma_color(c), ma_width(w), ma_null(s==5/*PS_NULL*/) {}
    CPen(int, int, const void*, int = 0) : ma_color(0), ma_width(1), ma_null(0) {}   /* ExtCreatePen geometric form */
    BOOL CreatePen(int s, int w, COLORREF c) { ma_color=c; ma_width=w?w:1; ma_null=(s==5); return TRUE; }
    operator HPEN() const { return (HPEN)m_hObject; }
};

class CBrush : public CGdiObject {
public:
    COLORREF ma_color; int ma_null;
    CBrush() : ma_color(0), ma_null(0) {}
    CBrush(COLORREF c) : ma_color(c), ma_null(0) {}
    BOOL CreateSolidBrush(COLORREF c) { ma_color=c; ma_null=0; return TRUE; }
    BOOL CreateStockObject(int i) { ma_color = (i==0/*WHITE_BRUSH*/) ? 0x00FFFFFF : 0; ma_null=(i==5/*NULL_BRUSH*/); return TRUE; }
    static CBrush* FromHandle(HBRUSH h);
    operator HBRUSH() const { return (HBRUSH)m_hObject; }
};

class CBitmap : public CGdiObject {
public:
    ~CBitmap() { if (m_hObject) { ma_gdi_delete_bitmap(m_hObject); m_hObject = NULL; } }
    BOOL CreateCompatibleBitmap(CDC*, int w, int h) { if (m_hObject) ma_gdi_delete_bitmap(m_hObject); m_hObject = (HGDIOBJ)ma_gdi_create_bitmap(w,h); return m_hObject!=NULL; }
    BOOL CreateBitmap(int w, int h, UINT, UINT, const void*) { if (m_hObject) ma_gdi_delete_bitmap(m_hObject); m_hObject = (HGDIOBJ)ma_gdi_create_bitmap(w,h); return m_hObject!=NULL; }
    BOOL LoadBitmapA(LPCSTR) { return TRUE; }
    BOOL LoadBitmapA(UINT) { return TRUE; }
    static CBitmap* FromHandle(HBITMAP) { return NULL; }
    int GetBitmap(void* p) { if(p){ int w=0,h=0; ma_gdi_bitmap_size(m_hObject,&w,&h); /* BITMAP{type,w,h,wbytes,planes,bpp,bits} */ int* bm=(int*)p; bm[0]=0; bm[1]=w; bm[2]=h; } return sizeof(int)*7; }
    operator HBITMAP() const { return (HBITMAP)m_hObject; }
};

/* stock brush handles: encode color in the handle so FromHandle can recover it */
inline CBrush* CBrush::FromHandle(HBRUSH h) {
    static CBrush s_black, s_white, s_null;
    long v = (long)(size_t)h;
    if (v == 1) { s_white.ma_color=0x00FFFFFF; s_white.ma_null=0; return &s_white; }
    if (v == 2) { s_null.ma_null=1; return &s_null; }
    s_black.ma_color=0; s_black.ma_null=0; return &s_black;   /* default/black */
}

class CDC : public CObject {
public:
    HDC m_hDC;
    CDC() : m_hDC(NULL) {}
    HDC GetSafeHdc() const { return m_hDC; }
    operator HDC() const { return m_hDC; }
    CFont* GetCurrentFont() { return 0; }          // Linux/GCC port
    UINT   GetBoundsRect(LPRECT, UINT) { return 0; }
    BOOL Attach(HDC h) { m_hDC = h; return TRUE; }
    HDC Detach() { HDC h = m_hDC; m_hDC = NULL; return h; }
    CGdiObject* SelectObject(CGdiObject*) { return NULL; }
    HGDIOBJ SelectStockObject(int) { return NULL; }
    CFont* SelectObject(CFont* f) { if (f) ma_gdi_set_font((void*)m_hDC, (void*)f->m_hObject); return NULL; }
    CPen*  SelectObject(CPen* p)  { if (p) ma_gdi_set_pen((void*)m_hDC, p->ma_width, (unsigned)p->ma_color, p->ma_null); return NULL; }
    CPen*  SelectObject(CPen& p)  { ma_gdi_set_pen((void*)m_hDC, p.ma_width, (unsigned)p.ma_color, p.ma_null); return NULL; }
    CBrush* SelectObject(CBrush* b) { if (b) ma_gdi_set_brush((void*)m_hDC, (unsigned)b->ma_color, b->ma_null); return NULL; }
    CBitmap* SelectObject(CBitmap* b) { if (b) ma_gdi_select_bitmap((void*)m_hDC, (void*)b->m_hObject); return NULL; }
    COLORREF SetTextColor(COLORREF c) { ma_gdi_set_text_color((void*)m_hDC, (unsigned)c); return c; }
    COLORREF SetBkColor(COLORREF c) { ma_gdi_set_bk_color((void*)m_hDC, (unsigned)c); return c; }
    int SetBkMode(int m) { ma_gdi_set_bk_mode((void*)m_hDC, m); return 0; }
    BOOL TextOutA(int x, int y, LPCSTR s, int n) { if (s) ma_gdi_text_out((void*)m_hDC, x, y, s, n); return TRUE; }
    /* note: callers' `TextOut` is macro-mapped to TextOutA by wingdi; do not add a
       non-A TextOut member here (it would collide with TextOutA). */
    BOOL ExtTextOutA(int x, int y, UINT opt, LPCRECT r, LPCSTR s, UINT n, LPINT) {
        if ((opt & 2/*ETO_OPAQUE*/) && r) ma_gdi_fill_rect((void*)m_hDC, r->left, r->top, r->right, r->bottom, /*bk*/0);
        if (s) ma_gdi_text_out((void*)m_hDC, x, y, s, (int)n); return TRUE; }
    BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, LPCSTR s, UINT n, LPINT d) { return ExtTextOutA(x,y,o,r,s,n,d); }
    /* CString-accepting overloads (template triggers CString::operator LPCTSTR) */
    template<class S> BOOL TextOut(int x, int y, const S& s) { LPCSTR p=(LPCSTR)s; return TextOutA(x,y,p,(int)strlen(p)); }
    template<class S> BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, const S& s, UINT n, LPINT d) { return ExtTextOutA(x,y,o,r,(LPCSTR)s,n,d); }
    template<class S> BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, const S& s, LPINT d) { LPCSTR p=(LPCSTR)s; return ExtTextOutA(x,y,o,r,p,(UINT)strlen(p),d); }
    template<class S> int  DrawText(const S& s, LPRECT r, UINT f) { LPCSTR p=(LPCSTR)s; return DrawText(p,(int)strlen(p),r,f); }
    COLORREF SetPixel(int x, int y, COLORREF c) { ma_gdi_set_pixel((void*)m_hDC, x, y, (unsigned)c); return c; }
    COLORREF GetPixel(int x, int y) const { return (COLORREF)ma_gdi_get_pixel((void*)m_hDC, x, y); }
    COLORREF GetNearestColor(COLORREF c) const { return c; }   /* 32-bit canvas: identity */
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
    BOOL GetTextMetricsA(void* tm) const { ma_gdi_get_text_metrics((void*)m_hDC, tm); return TRUE; }
    BOOL Rectangle(int l, int t, int r, int b) { ma_gdi_rectangle((void*)m_hDC, l, t, r, b); return TRUE; }
    BOOL DrawIcon(int, int, HICON) { return TRUE; }       /* icons not yet rasterised */
    BOOL DrawIcon(POINT, HICON) { return TRUE; }
    POINT MoveTo(int x, int y) { ma_gdi_move_to((void*)m_hDC, x, y); POINT p={(LONG)x,(LONG)y}; return p; }
    POINT MoveTo(POINT pt) { ma_gdi_move_to((void*)m_hDC, pt.x, pt.y); return pt; }
    BOOL LineTo(int x, int y) { ma_gdi_line_to((void*)m_hDC, x, y); return TRUE; }
    BOOL LineTo(POINT pt) { ma_gdi_line_to((void*)m_hDC, pt.x, pt.y); return TRUE; }
    BOOL BitBlt(int x, int y, int w, int h, CDC* s, int sx, int sy, DWORD rop) { ma_gdi_bitblt((void*)m_hDC, x, y, w, h, s?(void*)s->m_hDC:0, sx, sy, (unsigned long)rop); return TRUE; }
    BOOL CreateCompatibleDC(CDC*) { m_hDC = (HDC)ma_gdi_create_dc(); return m_hDC!=NULL; }
    int FillRect(LPCRECT r, CBrush* b) { if (r) ma_gdi_fill_rect((void*)m_hDC, r->left, r->top, r->right, r->bottom, (unsigned)(b?b->ma_color:0)); return 1; }
    void FillSolidRect(LPCRECT r, COLORREF c) { if (r) ma_gdi_fill_solid((void*)m_hDC, r->left, r->top, r->right, r->bottom, (unsigned)c); }
    void FillSolidRect(int l, int t, int w, int h, COLORREF c) { ma_gdi_fill_solid((void*)m_hDC, l, t, l+w, t+h, (unsigned)c); }
    void Draw3dRect(LPCRECT, COLORREF, COLORREF) {}
    void Draw3dRect(int, int, int, int, COLORREF, COLORREF) {}
    BOOL StretchBlt(int x, int y, int w, int h, CDC* s, int sx, int sy, int sw, int sh, DWORD rop) { ma_gdi_stretchblt((void*)m_hDC, x, y, w, h, s?(void*)s->m_hDC:0, sx, sy, sw, sh, (unsigned long)rop); return TRUE; }
    int  GetDeviceCaps(int index) const { switch(index){case 12/*BITSPIXEL*/:return 32;case 14/*PLANES*/:return 1;case 8/*HORZRES*/:return 1024;case 10/*VERTRES*/:return 768;case 88:case 90:return 96;default:return 0;} }
    CSize GetTextExtent(LPCSTR s, int n) const { int cx=0,cy=0; ma_gdi_get_text_extent((void*)m_hDC, s, n, &cx, &cy); return CSize(cx, cy); }
    template<class S> CSize GetTextExtent(const S& s) const { LPCSTR p=(LPCSTR)s; int cx=0,cy=0; ma_gdi_get_text_extent((void*)m_hDC, p, (int)strlen(p), &cx, &cy); return CSize(cx, cy); }
    CSize GetOutputTextExtent(LPCSTR s, int n) const { return GetTextExtent(s, n); }
    /* S59: real multi-line DrawText (parity #9: the Quick Mission text ran off the
       panel edge — CRStaticCtrl::OnDraw asks for DT_LEFT|DT_WORDBREAK and the old
       stub drew one unwrapped line). Splits on '\n', word-wraps to the rect width
       under DT_WORDBREAK (measuring with the current font via ma_gdi_get_text_extent),
       treats tabs as spaces (DT_TABSTOP not expanded), supports DT_CALCRECT, and
       returns the text height like Windows. Text that fits stays one line, so
       existing single-line callers are unchanged. */
    int  DrawText(LPCSTR s, int n, LPRECT r, UINT f) {
        if (!s || !r) return 0;
        if (n < 0) n = (int)strlen(s);
        const int calc  = (f & 0x400u /*DT_CALCRECT*/) != 0;
        const int wrap  = (f & 0x10u /*DT_WORDBREAK*/) && !(f & 0x20u /*DT_SINGLELINE*/);
        const int width = r->right - r->left;
        int lineh = 13; { int cx=0, cy=0; ma_gdi_get_text_extent((void*)m_hDC, "Ay", 2, &cx, &cy); if (cy > 0) lineh = cy; }
        int y = r->top, maxw = 0, i = 0;
        while (i < n) {
            int nl = i; while (nl < n && s[nl] != '\n') nl++;      /* hard break */
            int end = nl, next = (nl < n) ? nl + 1 : n;
            if (wrap && width > 0) {
                int fitEnd = -1, p = i;
                while (p < end) {
                    int we = p;                                     /* extend by one word */
                    while (we < end && (s[we]==' '||s[we]=='\t')) we++;
                    while (we < end && s[we]!=' ' && s[we]!='\t') we++;
                    if (we == p) break;                             /* all-space tail */
                    int cx=0, cy=0; ma_gdi_get_text_extent((void*)m_hDC, s+i, we-i, &cx, &cy);
                    if (cx > width) {
                        end = (fitEnd > i) ? fitEnd : we;           /* lone over-wide word stays whole */
                        next = end; while (next < n && (s[next]==' '||s[next]=='\t')) next++;
                        break;
                    }
                    fitEnd = we; p = we;
                }
            }
            int dlen = end - i;
            while (dlen > 0 && (s[i+dlen-1]==' '||s[i+dlen-1]=='\t')) dlen--;   /* trim */
            if (dlen > 0) {
                if (calc) { int cx=0, cy=0; ma_gdi_get_text_extent((void*)m_hDC, s+i, dlen, &cx, &cy); if (cx > maxw) maxw = cx; }
                else ma_gdi_text_out((void*)m_hDC, r->left, y, s+i, dlen);
            }
            y += lineh;
            i = (next > i) ? next : i + 1;                          /* guaranteed progress */
        }
        int h = y - r->top;
        if (calc) { r->bottom = r->top + h; if (maxw > width) r->right = r->left + maxw; }
        return h;
    }
    UINT SetTextAlign(UINT) { return 0; }
    int  SetMapMode(int) { return 0; }
    int  SetROP2(int) { return 0; }
    int  SetStretchBltMode(int) { return 0; }
    int  GetStretchBltMode() const { return 0; }
    static CDC* FromHandle(HDC) { return NULL; }
    POINT SetViewportOrg(int x, int y) { int ox=0, oy=0; ma_gdi_set_viewport_org((void*)m_hDC, x, y, &ox, &oy); POINT p = {(LONG)ox,(LONG)oy}; return p; }
    POINT SetViewportOrg(POINT pt) { return SetViewportOrg(pt.x, pt.y); }
    POINT GetViewportOrg() const { POINT p={0,0}; return p; }
};

inline BOOL CFont::CreatePointFont(int pt, LPCSTR face, CDC*) {
    if (m_hObject) ma_gdi_font_delete(m_hObject);
    m_hObject = (HGDIOBJ)ma_gdi_font_create((pt * 96) / 720, 0, 0, face); return TRUE; }
inline BOOL CFont::CreateFontIndirect(const LOGFONT* lf) {
    if (m_hObject) ma_gdi_font_delete(m_hObject);
    int h = lf ? (int)lf->lfHeight : 12;
    int w = lf ? (int)lf->lfWeight : 0;
    int it = lf ? (int)lf->lfItalic : 0;
    m_hObject = (HGDIOBJ)ma_gdi_font_create(h, w, it, lf ? lf->lfFaceName : 0); return TRUE; }

class CPaintDC : public CDC { public: CPaintDC(CWnd*) {} };
class CClientDC : public CDC { public: CClientDC(CWnd*) {} };
class CWindowDC : public CDC { public: CWindowDC(CWnd*) {} };
class CMetaFileDC : public CDC { public: CMetaFileDC() {} };

/* ============================================================
 * Window / app hierarchy (stubbed — no real windows on Linux)
 * ============================================================ */
class CListBox;
/* DDX control registry (ma_dlgitem.cpp): (dialog,id) -> CWnd* wrapper */
extern "C" void  ma_ddx_register(void* dlg, int id, void* ctrl);
extern "C" void* ma_ddx_lookup(void* dlg, int id);
/* hosted OCX dispatch (ma_olecontrol.cpp): route client InvokeHelper/Get/SetProperty */
extern "C" void ma_ole_invoke(void* client, DISPID, WORD, VARTYPE, void* pvRet, const BYTE* params, va_list);
extern "C" void ma_ole_setprop(void* client, DISPID, VARTYPE, va_list);
extern "C" void ma_ole_getprop(void* client, DISPID, VARTYPE, void* pvRet);
extern "C" void ma_ole_draw(void* client, void* parentWnd, void* screenHdc);
extern "C" void ma_ole_draw_all(void* screenHdc);                 /* draw every hosted control */
extern "C" void ma_ole_draw_toolbar(void* dialog, void* screenHdc, int ox, int oy); /* parent-scoped (CRToolBar) */
extern "C" int  ma_ole_toolbar_click(void* dialog, int ox, int oy, int sx, int sy);  /* fire a toolbar button's handler */
extern "C" void ma_ole_remove_by_parent(void* parent);           /* drop a destroyed panel's controls */
extern "C" void ma_ole_create(void* client, const void* clsid, void* parent);  /* register type by CLSID */
extern "C" void ma_ole_set_relative(void* client);   /* control is template-positioned (client-relative) */
extern "C" void ma_ole_set_id(void* client, int id); /* record control's dialog id (for click->event) */
extern "C" int  ma_ole_listbox_click(int sx, int sy); /* route a click on a child-dialog listbox (e.g. CLoad file list) to its Select handler */
extern "C" void ma_ole_set_label(void* client, const char* text); /* apply RT_DLGINIT label (statics) */
extern "C" int  ma_ole_click(int sx, int sy);        /* hit-test buttons, fire Clicked to dialog */
/* RT_DIALOG template (ma_dlgtmpl.cpp): control client-relative rects by (dialog,id) */
extern "C" void ma_dlg_load_template(unsigned idd, void* dlg);
extern "C" int  ma_dlg_rect(void* dlg, int id, int* x, int* y, int* w, int* h);
extern "C" int  ma_dlg_label(void* dlg, int id, char* out, int outsz);
/* S57 (BoB S124 §8f) — installed-template layer: membership, unbound-static hosting, art */
extern "C" int  ma_dlg_in_template(void* dlg, int id);
extern "C" int  ma_dlg_enum_statics(void* dlg, int* ids, int maxn);
extern "C" int  ma_dlg_template_visible(void* dlg, int id);  /* S59: template WS_VISIBLE bit (initial show state) */
extern "C" int  ma_dlg_never_visible(void* dlg, int id);     /* S59: parked outside the dialog rect (Windows-clipped) */
extern "C" int  ma_dlg_artnum(void* dlg, int id, long* outFn);
extern "C" int  ma_pe_layer_on(void);
extern "C" void ma_ole_set_artnum(void* client, long fn);
inline void ma_host_template_statics(void* dlgp);   /* defined after CWnd, below */
extern "C" int  ma_ole_mouse(void* client, void* parentWnd, int sx, int sy, int clicked, long* outRow, long* outCol);
/* mouse state from the SDL pump (bob_video.cpp), in canvas coordinates */
extern "C" void ma_mouse_pos(int* x, int* y, int* lbtn);
extern "C" int  ma_mouse_take_click(int* x, int* y);
class CWnd : public CCmdTarget {
public:
    enum { adjustBorder = 0, adjustOutside = 1 };
    HWND m_hWnd;
    CWnd() : m_hWnd(NULL), m_maX(0), m_maY(0), m_maW(0), m_maH(0), m_maParent(0), m_maVisible(1) {}
    /* window rect tracking (hosted OCX controls position via MoveWindow; OnDraw
       draws within GetClientRect). x,y = screen origin; w,h = size. */
    int m_maX, m_maY, m_maW, m_maH;
    CWnd* m_maParent;                /* set for hosted controls so GetParent() works */
    int m_maVisible;                 /* ShowWindow state: 1=shown (default), 0=SW_HIDE */
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
    CWnd* GetDlgItem(int id) const { return (CWnd*)ma_ddx_lookup((void*)this, id); }
    void  GetDlgItem(int, HWND* ph) const { if (ph) *ph = (HWND)1; /* sentinel: control exists */ }
    int GetDlgItemTextA(int, LPSTR, int) { return 0; }
    void SetDlgItemTextA(int, LPCSTR) {}
    BOOL SetWindowTextA(LPCSTR) { return TRUE; }
    int GetWindowTextA(LPSTR, int) { return 0; }
    template<class S> int GetWindowTextA(S& s) { (void)s; return 0; }
    BOOL ShowWindow(int nCmdShow) { m_maVisible = (nCmdShow != 0 /*SW_HIDE*/); return TRUE; }
    BOOL UpdateWindow() { return TRUE; }
    BOOL DestroyWindow() { return TRUE; }
    BOOL MoveWindow(int x, int y, int w, int h, BOOL = TRUE) { m_maX=x; m_maY=y; m_maW=w; m_maH=h; return TRUE; }
    BOOL MoveWindow(LPCRECT r, BOOL = TRUE) { if (r) { m_maX=r->left; m_maY=r->top; m_maW=r->right-r->left; m_maH=r->bottom-r->top; } return TRUE; }
    CWnd* GetTopWindow() const { return NULL; }
    static CWnd* GetDesktopWindow() { return NULL; }
    static CWnd* FromHandle(HWND) { return NULL; }
    CWnd* GetLastActivePopup() const { return NULL; }
    /* NULL-this safe: the toolbar/dialog bring-up calls these on not-yet-created child
       widgets; the old zero-rect stub didn't deref `this`, so guard to keep that. */
    void GetClientRect(LPRECT r) const { if (!r) return; if (!this) { r->left=r->top=r->right=r->bottom=0; return; } r->left = r->top = 0; r->right = m_maW; r->bottom = m_maH; }
    void GetWindowRect(LPRECT r) const { if (!r) return; if (!this) { r->left=r->top=r->right=r->bottom=0; return; } r->left = m_maX; r->top = m_maY; r->right = m_maX + m_maW; r->bottom = m_maY + m_maH; }
    void ClientToScreen(LPPOINT) const {}
    void ClientToScreen(LPRECT) const {}
    void ScreenToClient(LPPOINT) const {}
    void ScreenToClient(LPRECT) const {}
    BOOL CreateControl(LPCSTR, LPCSTR, DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
    BOOL CreateControl(REFCLSID clsid, LPCSTR, DWORD, const RECT&, CWnd* p, UINT) { ma_ole_create((void*)this, &clsid, (void*)p); return TRUE; }
    BOOL CreateControl(REFCLSID clsid, LPCSTR, DWORD, const RECT&, CWnd* p, UINT, CFile*, BOOL, BSTR) { ma_ole_create((void*)this, &clsid, (void*)p); return TRUE; }
    /* real CWnd::Create (the OCX client wrappers override this to call CreateControl) */
    virtual BOOL Create(LPCSTR, LPCSTR, DWORD, const RECT&, CWnd*, UINT, CCreateContext* = NULL) { return FALSE; }
    /* hosted-ActiveX-control accessors (ClassWizard wrappers call these) — routed to
       the hosted OCX (ma_olecontrol.cpp) by dispid. */
    void SetProperty(DISPID dispid, VARTYPE vt, ...) { va_list ap; va_start(ap, vt); ma_ole_setprop((void*)this, dispid, vt, ap); va_end(ap); }
    void GetProperty(DISPID dispid, VARTYPE vt, void* pv) const { ma_ole_getprop((void*)this, dispid, vt, pv); }
    void InvokeHelper(DISPID dispid, WORD wFlags, VARTYPE vtRet, void* pvRet, const BYTE* params, ...) { va_list ap; va_start(ap, params); ma_ole_invoke((void*)this, dispid, wFlags, vtRet, pvRet, params, ap); va_end(ap); }
    CWnd* GetNextWindow(UINT = 0) const { return NULL; }
    CWnd* GetWindow(UINT) const { return NULL; }
    int   GetDlgCtrlID() const { return 0; }
    LONG  GetWindowLong(int) const { return 0; }
    LONG  SetWindowLong(int, LONG) { return 0; }
    DWORD GetStyle() const { return 0; }
    DWORD GetExStyle() const { return 0; }
    CScrollBar* GetScrollBarCtrl(int) const { return NULL; }
    void  ModifyStyle(DWORD, DWORD, UINT = 0) {}
    /* Linux port: return a shared CDC bound to the screen canvas (HDC 1) so callers
       that deref it -- e.g. CRListBoxCtrl::UpdateScrollBar's GetTextMetrics, and
       IconDescUI::LoadInstances(*pdc) -- get a valid, drawable DC. */
    CDC* GetDC() { static CDC s_screenDC; s_screenDC.m_hDC = (HDC)1; return &s_screenDC; }
    int  ReleaseDC(CDC*) { return 1; }
    BOOL EnableWindow(BOOL = TRUE) { return TRUE; }
    CWnd* SetFocus() { return NULL; }
    int MessageBoxA(LPCSTR, LPCSTR = NULL, UINT uType = 0) { return ((uType & 0xF) == 5 /*MB_RETRYCANCEL*/) ? 2 /*IDCANCEL*/ : 0; }
    /* Rowan front-end custom messages (WM_USER+: WM_GETFILE/WM_GETGLOBALFONT/WM_GETARTWORK/
       WM_GETXYOFFSET/WM_GETOFFSCREENDC/...) are routed to the window's handlers. RDialog
       overrides OnRowanMessage to dispatch them; no real Win32 message map on Linux. */
    virtual LRESULT OnRowanMessage(UINT, WPARAM, LPARAM) { return 0; }
    LRESULT SendMessageA(UINT m, WPARAM w = 0, LPARAM l = 0) { if (this && m >= 0x400 /*WM_USER*/) return OnRowanMessage(m, w, l); return 0; }
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
    CDC* BeginPaint(LPPAINTSTRUCT ps) { static CDC s_paintDC; s_paintDC.m_hDC=(HDC)1; if(ps){PAINTSTRUCT z={0}; *ps=z;} return &s_paintDC; }  // Linux/GCC port: valid stub DC (m_hDC opaque; GDI present ignores it)
    void EndPaint(LPPAINTSTRUCT) {}
    void GetWindowPlacement(void*) const {}
    void SetWindowPlacement(const void*) {}
    int  RunModalLoop(DWORD = 0) { return 0; }
    void Invalidate(BOOL = TRUE) {}
    void InvalidateRect(LPCRECT, BOOL = TRUE) {}
    void ClientToScreenRect(LPRECT) const {}
    BOOL SetTimer(UINT, UINT, void* = NULL) { return TRUE; }
    BOOL KillTimer(UINT) { return TRUE; }
    static const CWnd wndTop, wndBottom, wndTopMost, wndNoTopMost;	// Linux/GCC port: SetWindowPos z-order sentinels (callers pass &CWnd::wndTopMost)
    void SetWindowPos(const CWnd*, int, int, int, int, UINT) {}
    void BringWindowToTop() {}
    BOOL IsWindowVisible() const { return FALSE; }
    BOOL IsWindowEnabled() const { return TRUE; }	// Linux/GCC port
    LRESULT OnRegisteredMouseWheel(WPARAM, LPARAM) { return 0; }	// Linux/GCC port: Rowan custom wheel msg
    void SetCapture() {}
    CWnd* GetParent() const { return m_maParent; }
    CWnd* GetParentFrame() const { return NULL; }
    CWnd* GetParentOwner() const { return NULL; }
    BOOL UpdateData(BOOL bSave = TRUE);              /* defined after CDataExchange */
    virtual BOOL OnInitDialog();                     /* drives UpdateData(FALSE) */
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
// Linux/GCC port: storage for the CWnd z-order sentinels (C++17 inline vars — single def, no separate .cpp)
inline const CWnd CWnd::wndTop{};
inline const CWnd CWnd::wndBottom{};
inline const CWnd CWnd::wndTopMost{};
inline const CWnd CWnd::wndNoTopMost{};

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
    void SetLimitText(UINT) {}          // Linux/GCC port
    void LimitText(int = 0) {}
    void SetReadOnly(BOOL = TRUE) {}
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
    BOOL Create(UINT idd, CWnd* pParent = NULL) { if (pParent) m_maParent = pParent; ma_dlg_load_template(idd, this); OnInitDialog(); ma_host_template_statics((void*)this); return TRUE; }
      /* ^ store the parent BEFORE OnInitDialog: real MFC sets the parent window in Create, and
         many dialogs' OnInitDialog do `((RDialog*)GetParent())->SetMaxSize(...)` etc. Without this
         GetParent() returned NULL -> the OOB dialogs (CSquads/…) SEGV'd building their tree.
         S57: after OnInitDialog (i.e. after every DDX_Control has registered), host the
         template's label statics the dialog class never binds — on Windows the dialog manager
         creates EVERY template item; DDX-driven creation silently missed them (BoB §8f; e.g.
         ~6 prefs-Others row labels, prefs-Controls "Dead Zone:"/"Airframe"). */
    virtual void OnOK() {}
    virtual void OnCancel() {}
    virtual LRESULT OnCommandHelp(WPARAM, LPARAM) { return 0; }
    void EndDialog(int) {}
    void GotoDlgCtrl(CWnd*) {}
    void NextDlgCtrl() const {}
};

/* S57 (BoB S124 §8f "template-driven static hosting"): host the installed template's
   RStatic items the dialog class never DDX_Control-binds, so their labels render.
   Runs AFTER OnInitDialog (all DDX registrations done); each unbound static gets a
   synthetic CWnd client (same registry/draw path as DDX-hosted controls; leaks with
   the never-freed dialog, the pre-existing hosted-control pattern) positioned from
   the template rect and captioned from DLGINIT/IDS. Also registered with the DDX
   registry so GetDlgItem(id) dispatch reaches it and re-Create is idempotent. */
inline void ma_host_template_statics(void* dlgp) {
    if (!ma_pe_layer_on()) return;
    int ids[160];
    int n = ma_dlg_enum_statics(dlgp, ids, 160);
    for (int i = 0; i < n; i++) {
        int id = ids[i];
        if (ma_ddx_lookup(dlgp, id)) continue;            /* DDX-bound (or already hosted) */
        int x, y, w, h;
        if (!ma_dlg_rect(dlgp, id, &x, &y, &w, &h) || w <= 0 || h <= 0) continue;
        if (ma_dlg_never_visible(dlgp, id) == 1) continue;   /* S59: Windows-clipped (outside the dialog rect) — can never paint */
        char lbl[128];
        if (!ma_dlg_label(dlgp, id, lbl, sizeof(lbl)) || !lbl[0]) continue;  /* nothing to show */
        CWnd* client = new CWnd();
        client->m_maX = x; client->m_maY = y; client->m_maW = w; client->m_maH = h;
        client->m_maParent = (CWnd*)dlgp;
        /* S59: !WS_VISIBLE template statics exist but start HIDDEN on Windows (a runtime
           ShowWindow via GetDlgItem can still reveal them) */
        if (ma_dlg_template_visible(dlgp, id) == 0) client->m_maVisible = 0;
        /* RStatic coclass — ma_ole_create matches on Data1 only */
        static const struct MaClsid { unsigned long d1; unsigned short d2, d3; unsigned char d4[8]; }
            rstaticClsid = { 0xc42bac3d, 0, 0, {0,0,0,0,0,0,0,0} };
        ma_ole_create((void*)client, (const void*)&rstaticClsid, dlgp);
        ma_ole_set_id((void*)client, id);
        ma_ole_set_relative((void*)client);
        ma_ole_set_label((void*)client, lbl);
        ma_ddx_register(dlgp, id, (void*)client);
    }
}

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
    COleControl() : m_bAutoSize(0), m_foreColor(0), m_backColor(0x00FFFFFF) { m_maText[0]=0; }
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
    char m_maText[512];
    BSTR  GetText() { return NULL; }
    void  SetText(LPCSTR s) { if (s) { strncpy(m_maText, s, sizeof(m_maText)-1); m_maText[sizeof(m_maText)-1]=0; } else m_maText[0]=0; OnTextChanged(); }
    CString InternalGetText() { return CString(m_maText); }  /* CString: callers use .IsEmpty()/LPCTSTR */
    /* CRComboCtrl::SetIndex sets the displayed item via SetWindowText; route it to m_maText so
       InternalGetText() (what OnDraw renders) reflects the selection. CWnd's SetWindowTextA is a
       no-op; this COleControl override shadows it for OCX controls. */
    BOOL SetWindowTextA(LPCSTR s) { SetText(s); return TRUE; }
    BOOL GetEnabled() { return TRUE; }
    void SetEnabled(BOOL) {}
    virtual void OnTextChanged() {}
    OLE_COLOR GetForeColor() { return m_foreColor; }
    OLE_COLOR GetBackColor() { return m_backColor; }
    void SetForeColor(OLE_COLOR c) { m_foreColor = c; }
    void SetBackColor(OLE_COLOR c) { m_backColor = c; }
    OLE_COLOR m_foreColor, m_backColor;
    /* OLE_COLOR is already a 0x00BBGGRR COLORREF in our compat; pass through */
    COLORREF TranslateColor(OLE_COLOR clr, HPALETTE = NULL) { return (COLORREF)(clr & 0x00FFFFFF); }
    OLE_COLOR AmbientForeColor() { return 0; }
    OLE_COLOR AmbientBackColor() { return 0x00FFFFFF; }
    CFont* AmbientFont() { return NULL; }
    BOOL AmbientUserMode() { return TRUE; }
    BOOL AmbientShowHatching() { return FALSE; }
    BOOL AmbientShowGrabHandles() { return FALSE; }
    CFont* SelectStockFont(CDC*) { return NULL; }
    BOOL IsModified() const { return FALSE; }
    void Refresh() {}
    void InitializeIIDs(const void*, const void*) {}
    void ExchangeVersion(CPropExchange*, DWORD) {}
};

class CFrameWnd : public CWnd {
public:
    CView* m_pActiveView;                       // Linux/GCC port: real active-view association
    CFrameWnd() : m_pActiveView(0) {}
    BOOL Create(LPCSTR, LPCSTR, DWORD = 0, const RECT& = CRect(), CWnd* = NULL, LPCSTR = NULL) { return TRUE; }
    CView* GetActiveView() const { return m_pActiveView; }
    CDocument* GetActiveDocument() const { return m_pActiveView ? m_pActiveView->GetDocument() : 0; }
    void RecalcLayout(BOOL = TRUE) {}
    BOOL SetActiveView(CView* v, BOOL = TRUE) { m_pActiveView = v; return TRUE; }
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

class COleClientItem;	// Linux/GCC port
class COleDocument : public CDocument {
public:	// Linux/GCC port: OLE in-place editing stubs (no OLE on Linux)
	COleClientItem* GetInPlaceActiveItem(CWnd* = 0) const { return 0; }
};

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

/* DDX/DDV (dialog data exchange) */
static void DDX_Control(CDataExchange*, int, CWnd&);   /* defined after CDataExchange */
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
    BOOL SetThreadPriority(int) { return TRUE; }   // Linux/GCC port
    int  GetThreadPriority() { return 0; }
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
    void    RegisterShellFileTypes(BOOL = FALSE) {}	// Linux/GCC port: no shell integration
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

/* Real doubly-linked list with MFC POSITION semantics (POSITION == node*). The
   front-end (e.g. CRListBoxCtrl) stores/draws its rows by POSITION iteration, so
   this must actually work — a stub broke all runtime list traversal. */
template <class TYPE, class ARG_TYPE = const TYPE&>
class CList : public CObject {
    struct Node { Node* next; Node* prev; TYPE data; };
    Node* m_head; Node* m_tail; int m_count;
public:
    CList() : m_head(0), m_tail(0), m_count(0) {}
    ~CList() { RemoveAll(); }
    int  GetCount() const { return m_count; }
    int  GetSize() const { return m_count; }
    BOOL IsEmpty() const { return m_count == 0; }
    void RemoveAll() { Node* n = m_head; while (n) { Node* nx = n->next; delete n; n = nx; } m_head = m_tail = 0; m_count = 0; }
    POSITION AddTail(ARG_TYPE x) { Node* n = new Node; n->data = x; n->next = 0; n->prev = m_tail; if (m_tail) m_tail->next = n; else m_head = n; m_tail = n; m_count++; return (POSITION)n; }
    POSITION AddHead(ARG_TYPE x) { Node* n = new Node; n->data = x; n->prev = 0; n->next = m_head; if (m_head) m_head->prev = n; else m_tail = n; m_head = n; m_count++; return (POSITION)n; }
    TYPE& GetHead() { return m_head->data; }
    TYPE& GetTail() { return m_tail->data; }
    const TYPE& GetHead() const { return m_head->data; }
    const TYPE& GetTail() const { return m_tail->data; }
    POSITION GetHeadPosition() const { return (POSITION)m_head; }
    POSITION GetTailPosition() const { return (POSITION)m_tail; }
    TYPE& GetNext(POSITION& p) { Node* n = (Node*)p; p = (POSITION)(n ? n->next : 0); return n->data; }
    TYPE& GetPrev(POSITION& p) { Node* n = (Node*)p; p = (POSITION)(n ? n->prev : 0); return n->data; }
    TYPE& GetAt(POSITION p) { return ((Node*)p)->data; }
    const TYPE& GetAt(POSITION p) const { return ((Node*)p)->data; }
    void  SetAt(POSITION p, ARG_TYPE x) { ((Node*)p)->data = x; }
    void  RemoveAt(POSITION p) { Node* n = (Node*)p; if (!n) return; if (n->prev) n->prev->next = n->next; else m_head = n->next; if (n->next) n->next->prev = n->prev; else m_tail = n->prev; delete n; m_count--; }
    TYPE  RemoveHead() { TYPE d = m_head->data; RemoveAt((POSITION)m_head); return d; }
    TYPE  RemoveTail() { TYPE d = m_tail->data; RemoveAt((POSITION)m_tail); return d; }
    POSITION Find(ARG_TYPE x) const { Node* n = m_head; while (n) { if (n->data == x) return (POSITION)n; n = n->next; } return (POSITION)0; }
    POSITION FindIndex(int idx) const { Node* n = m_head; while (n && idx-- > 0) n = n->next; return (POSITION)n; }
    POSITION InsertBefore(POSITION pp, ARG_TYPE x) { Node* at = (Node*)pp; if (!at) return AddHead(x); Node* n = new Node; n->data = x; n->prev = at->prev; n->next = at; if (at->prev) at->prev->next = n; else m_head = n; at->prev = n; m_count++; return (POSITION)n; }
    POSITION InsertAfter(POSITION pp, ARG_TYPE x) { Node* at = (Node*)pp; if (!at) return AddTail(x); Node* n = new Node; n->data = x; n->next = at->next; n->prev = at; if (at->next) at->next->prev = n; else m_tail = n; at->next = n; m_count++; return (POSITION)n; }
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
    DWORD GetVersion() { return 0; }
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

/* --- DDX framework wiring (needs complete CDataExchange) --- */
static void DDX_Control(CDataExchange* pDX, int id, CWnd& ctrl) {
    if (!pDX) return;
    ma_ddx_register((void*)pDX->m_pDlgWnd, id, (void*)&ctrl);
    /* No dialog template auto-creates the OCX on Linux: drive the client wrapper's virtual
       Create -> CreateControl(GetClsid()) so the host learns the control type. */
    CRect r0;
    ctrl.Create((LPCSTR)0, (LPCSTR)0, 0, r0, pDX->m_pDlgWnd, (UINT)id, (CCreateContext*)0);
    ma_ole_set_id((void*)&ctrl, id);           /* so a click can fire this control's event by id */
    /* position it from the parsed RT_DIALOG template (client-relative pixels) */
    int dx, dy, dw, dh;
    if (ma_dlg_rect((void*)pDX->m_pDlgWnd, id, &dx, &dy, &dw, &dh)) {
        ctrl.MoveWindow(dx, dy, dw, dh);
        ma_ole_set_relative((void*)&ctrl);     /* client-relative -> add parent origin when drawn */
        /* S59 (parity #9): Windows creates !WS_VISIBLE template controls HIDDEN; only a
           later runtime ShowWindow(SW_SHOW) reveals them (e.g. IDD 287 id=2023 "I.D."
           label — template-hidden, never shown). Route the bit as the INITIAL show
           state; the game's own ShowWindow calls still override, exactly as on Windows. */
        if (ma_dlg_template_visible((void*)pDX->m_pDlgWnd, id) == 0) ctrl.m_maVisible = 0;
    }
    /* apply the control's label text parsed from RT_DLGINIT (statics: "Display Driver:" etc.;
       S57 also buttons/edit-buttons — design-time String, e.g. the tickbox glyph) */
    { char lbl[128]; if (ma_dlg_label((void*)pDX->m_pDlgWnd, id, lbl, sizeof(lbl))) ma_ole_set_label((void*)&ctrl, lbl); }
    /* S57: the control's persisted FIL_* art (tickbox box art etc.), resolved via F_GRAFIX.G */
    { long fn; if (ma_dlg_artnum((void*)pDX->m_pDlgWnd, id, &fn)) ma_ole_set_artnum((void*)&ctrl, fn); }
}
inline BOOL CWnd::UpdateData(BOOL bSave) {
    CDataExchange dx;
    dx.m_bSaveAndValidate = bSave;
    dx.m_pDlgWnd = this;
    DoDataExchange(&dx);
    return TRUE;
}
inline BOOL CWnd::OnInitDialog() { UpdateData(FALSE); return TRUE; }

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
inline CWinThread* AfxGetThread() { return (CWinThread*)AfxGetApp(); }	// Linux/GCC port: CWinApp is-a CWinThread
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
