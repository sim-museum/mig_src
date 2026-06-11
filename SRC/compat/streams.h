/* FreeFalcon Linux Port - streams.h: minimal DirectShow stub.
 * BoB's FULLPSYS.CPP plays intro movies via the DirectShow filter graph
 * (IGraphBuilder/IMediaControl/IVideoWindow/IMediaEventEx/IBasicAudio). Video
 * playback is deferred — these interfaces exist only so FULLPSYS compiles; the
 * graph is never created at runtime (CoCreateInstance returns failure), so movies
 * are simply skipped. */
#ifndef FF_COMPAT_STREAMS_H
#define FF_COMPAT_STREAMS_H
#ifdef FF_LINUX

#include "windows.h"
#include "objbase.h"

typedef long OAHWND;
#ifndef OATRUE
#define OATRUE  (-1)
#define OAFALSE 0
#endif
/* DirectShow EC_* event codes (IMediaEventEx) */
#ifndef EC_COMPLETE
#define EC_COMPLETE        0x01
#define EC_USERABORT       0x02
#define EC_ERRORABORT      0x03
#define EC_TIME            0x04
#define EC_REPAINT         0x05
#define EC_PALETTE_CHANGED 0x09
#endif
typedef long OAEVENT;
typedef double REFTIME;

struct IGraphBuilder {
#ifdef __cplusplus
    HRESULT QueryInterface(REFIID, void**) { return E_NOINTERFACE; }
    ULONG   AddRef()  { return 1; }
    ULONG   Release() { return 0; }
    HRESULT RenderFile(LPCWSTR, LPCWSTR) { return E_FAIL; }
#endif
};

struct IMediaControl {
#ifdef __cplusplus
    ULONG   Release() { return 0; }
    HRESULT Run()  { return E_FAIL; }
    HRESULT Stop() { return E_FAIL; }
    HRESULT Pause(){ return E_FAIL; }
#endif
};

struct IVideoWindow {
#ifdef __cplusplus
    ULONG   Release() { return 0; }
    HRESULT put_Owner(OAHWND) { return E_FAIL; }
    HRESULT put_WindowStyle(long) { return E_FAIL; }
    HRESULT put_Left(long) { return E_FAIL; }
    HRESULT put_Top(long)  { return E_FAIL; }
    HRESULT put_Width(long) { return E_FAIL; }
    HRESULT put_Height(long) { return E_FAIL; }
    HRESULT put_Visible(long) { return E_FAIL; }
    HRESULT SetWindowPosition(long, long, long, long) { return E_FAIL; }
#endif
};

struct IMediaEventEx {
#ifdef __cplusplus
    ULONG   Release() { return 0; }
    HRESULT SetNotifyWindow(OAHWND, long, LONG_PTR) { return E_FAIL; }
    HRESULT GetEvent(long*, LONG_PTR*, LONG_PTR*, long) { return E_FAIL; }
    HRESULT FreeEventParams(long, LONG_PTR, LONG_PTR) { return E_FAIL; }
    HRESULT WaitForCompletion(long, long*) { return E_FAIL; }
#endif
};

struct IBasicAudio {
#ifdef __cplusplus
    ULONG   Release() { return 0; }
    HRESULT put_Volume(long) { return E_FAIL; }
    HRESULT put_Balance(long) { return E_FAIL; }
#endif
};

static const GUID CLSID_FilterGraph  = { 0xe436ebb3, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID IID_IGraphBuilder  = { 0x56a868a9, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID IID_IMediaControl  = { 0x56a868b1, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID IID_IMediaEventEx  = { 0x56a868c0, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID IID_IVideoWindow   = { 0x56a868b4, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID IID_IBasicAudio    = { 0x56a868b3, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };

#endif /* FF_LINUX */
#endif
