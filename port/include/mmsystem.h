//==============================================================================
// mmsystem.h -- minimal Windows multimedia shim.  The game uses timeGetTime()
// and a few joystick/wave declarations; multimedia timing is backed by SDL in
// the runtime layer.  Grown compile-error-driven.
//==============================================================================
#ifndef MA_PORT_MMSYSTEM_H
#define MA_PORT_MMSYSTEM_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef UINT MMRESULT;
#define MMSYSERR_NOERROR 0
#define TIMERR_NOERROR   0
#define JOYERR_NOERROR   0

DWORD WINAPI timeGetTime(void);
MMRESULT WINAPI timeBeginPeriod(UINT uPeriod);
MMRESULT WINAPI timeEndPeriod(UINT uPeriod);

#ifdef __cplusplus
}
#endif

#endif // MA_PORT_MMSYSTEM_H
