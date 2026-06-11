//==============================================================================
// ole2.h -- stub for the Linux port.  The vendored dplay.h #includes <ole2.h>
// for COM scaffolding (IUnknown, GUID).  DirectPlay multiplayer is stubbed in
// this port, so we provide just the minimal COM declarations the headers need.
//==============================================================================
#ifndef MA_PORT_OLE2_H
#define MA_PORT_OLE2_H

#include <windows.h>

#ifndef _OBJBASE_H_
#define _OBJBASE_H_

typedef HRESULT (*LPFNUNKNOWN)(void);

#ifdef __cplusplus
struct IUnknown {
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppv) = 0;
    virtual ULONG   __stdcall AddRef(void) = 0;
    virtual ULONG   __stdcall Release(void) = 0;
};
typedef IUnknown *LPUNKNOWN;
#else
typedef struct IUnknown IUnknown;
typedef struct IUnknown *LPUNKNOWN;
#endif

#define interface struct
#define STDMETHOD(m)        virtual HRESULT __stdcall m
#define STDMETHOD_(t,m)     virtual t __stdcall m
#define STDMETHODIMP        HRESULT __stdcall
#define STDMETHODIMP_(t)    t __stdcall
#define PURE                = 0
#define THIS_
#define THIS                void
#define DECLARE_INTERFACE(i)            struct i
#define DECLARE_INTERFACE_(i,b)         struct i : public b

#endif // _OBJBASE_H_
#endif // MA_PORT_OLE2_H
