/* FreeFalcon Linux Port - afxmt.h (MFC multithreading sync objects, minimal).
 * Single-threaded UI bring-up: the lock/event ops are no-op stubs that report
 * success, so the game's threading scaffolding compiles and runs serially. */
#ifndef FF_STUB_afxmt
#define FF_STUB_afxmt
#define __AFXMT_H__	/* bob headers gate threading members on this (e.g. stub3d.h) */

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

class CSyncObject {
public:
    CSyncObject() {}
    virtual ~CSyncObject() {}
    virtual BOOL Lock(DWORD = INFINITE) { return TRUE; }
    virtual BOOL Unlock() { return TRUE; }
    virtual BOOL Unlock(LONG, LPLONG = NULL) { return TRUE; }
};

class CEvent : public CSyncObject {
public:
    CEvent(BOOL = FALSE, BOOL = FALSE, LPCSTR = NULL, void* = NULL) {}
    BOOL SetEvent()   { return TRUE; }
    BOOL ResetEvent() { return TRUE; }
    BOOL PulseEvent() { return TRUE; }
};

class CMutex : public CSyncObject {
public:
    CMutex(BOOL = FALSE, LPCSTR = NULL, void* = NULL) {}
};

class CCriticalSection : public CSyncObject {
public:
    CCriticalSection() {}
};

class CSemaphore : public CSyncObject {
public:
    CSemaphore(LONG = 1, LONG = 1, LPCSTR = NULL, void* = NULL) {}
};

class CSingleLock {
public:
    CSingleLock(CSyncObject*, BOOL = FALSE) {}
    BOOL Lock(DWORD = INFINITE) { return TRUE; }
    BOOL Unlock() { return TRUE; }
    BOOL IsLocked() { return TRUE; }
};

class CMultiLock {
public:
    CMultiLock(CSyncObject**, DWORD, BOOL = TRUE) {}
    DWORD Lock(DWORD = INFINITE, BOOL = TRUE, DWORD = 0) { return 0; }
    BOOL Unlock() { return TRUE; }
};

#endif
