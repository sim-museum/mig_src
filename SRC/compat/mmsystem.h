/* FreeFalcon Linux Port - mmsystem.h compatibility */
#ifndef FF_COMPAT_MMSYSTEM_H
#define FF_COMPAT_MMSYSTEM_H
#ifdef FF_LINUX

#include "compat_types.h"
#include "compat_winbase.h"  /* timeGetTime / timeBeginPeriod / timeEndPeriod */

typedef UINT MMRESULT;
#define MMSYSERR_NOERROR  0
#define MMSYSERR_ERROR    1
#define TIMERR_NOERROR    0

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
#pragma pack(push, 1)
typedef struct tWAVEFORMATEX {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX *LPCWAVEFORMATEX;
#pragma pack(pop)
#endif

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

#ifndef _PCMWAVEFORMAT_
#define _PCMWAVEFORMAT_
#pragma pack(push, 1)
typedef struct waveformat_tag {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
} WAVEFORMAT, *PWAVEFORMAT, *LPWAVEFORMAT;

typedef struct pcmwaveformat_tag {
    WAVEFORMAT wf;
    WORD       wBitsPerSample;
} PCMWAVEFORMAT, *PPCMWAVEFORMAT, *LPPCMWAVEFORMAT;
#pragma pack(pop)
#endif /* _PCMWAVEFORMAT_ */

typedef struct timecaps_tag {
    UINT wPeriodMin;
    UINT wPeriodMax;
} TIMECAPS, *PTIMECAPS, *LPTIMECAPS;

static inline MMRESULT timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc) {
    (void)cbtc;
    if (ptc) { ptc->wPeriodMin = 1; ptc->wPeriodMax = 1000000; }
    return MMSYSERR_NOERROR;
}

/* PlaySound stubs */
#define SND_SYNC      0x0000
#define SND_ASYNC     0x0001
#define SND_NODEFAULT 0x0002
#define SND_LOOP      0x0008
#define SND_PURGE     0x0040
#define SND_FILENAME  0x00020000
static inline BOOL PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound) {
    (void)pszSound; (void)hmod; (void)fdwSound;
    return TRUE;
}
#define PlaySound PlaySoundA
static inline BOOL sndPlaySoundA(LPCSTR pszSound, UINT fuSound) { (void)pszSound; (void)fuSound; return TRUE; }
#define sndPlaySound sndPlaySoundA

/* Joystick API (mmsystem.h) */
#ifndef JOY_RETURNX
#define JOY_RETURNX        0x00000001
#define JOY_RETURNY        0x00000002
#define JOY_RETURNZ        0x00000004
#define JOY_RETURNR        0x00000008
#define JOY_RETURNU        0x00000010
#define JOY_RETURNV        0x00000020
#define JOY_RETURNPOV      0x00000040
#define JOY_RETURNBUTTONS  0x00000080
#define JOY_RETURNALL      (JOY_RETURNX|JOY_RETURNY|JOY_RETURNZ|JOY_RETURNR|JOY_RETURNU|JOY_RETURNV|JOY_RETURNPOV|JOY_RETURNBUTTONS)
#define JOYERR_NOERROR     0
#define JOYERR_PARMS       165
#define JOYERR_NOCANDO     166
#define JOYERR_UNPLUGGED   167
#endif

typedef struct joyinfo_tag {
    UINT wXpos;
    UINT wYpos;
    UINT wZpos;
    UINT wButtons;
} JOYINFO, *PJOYINFO, *LPJOYINFO;

typedef struct joyinfoex_tag {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwXpos;
    DWORD dwYpos;
    DWORD dwZpos;
    DWORD dwRpos;
    DWORD dwUpos;
    DWORD dwVpos;
    DWORD dwButtons;
    DWORD dwButtonNumber;
    DWORD dwPOV;
    DWORD dwReserved1;
    DWORD dwReserved2;
} JOYINFOEX, *PJOYINFOEX, *LPJOYINFOEX;

static inline MMRESULT joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji) { (void)uJoyID; (void)pji; return JOYERR_UNPLUGGED; }
static inline MMRESULT joyGetPos(UINT uJoyID, LPJOYINFO pji)     { (void)uJoyID; (void)pji; return JOYERR_UNPLUGGED; }
static inline UINT     joyGetNumDevs(void) { return 0; }

/* MIDI output API (mmsystem.h) — stubbed; MIDI music playback not implemented. */
DECLARE_HANDLE(HMIDIOUT);
typedef HMIDIOUT *LPHMIDIOUT;

#ifndef MIDI_MAPPER
#define MIDI_MAPPER     ((UINT)-1)
#define MOD_MIDIPORT    1
#define MOD_SYNTH       2
#define MOD_SQSYNTH     3
#define MOD_FMSYNTH     4
#define MOD_MAPPER      5
#define MOD_WAVETABLE   6
#define MOD_SWSYNTH     7
#define MHDR_DONE       0x00000001
#define MHDR_PREPARED   0x00000002
#define CALLBACK_NULL   0x00000000
#define CALLBACK_FUNCTION 0x00030000
#endif

typedef struct midihdr_tag {
    LPSTR              lpData;
    DWORD              dwBufferLength;
    DWORD              dwBytesRecorded;
    DWORD_PTR          dwUser;
    DWORD              dwFlags;
    struct midihdr_tag *lpNext;
    DWORD_PTR          reserved;
    DWORD              dwOffset;
    DWORD_PTR          dwReserved[8];
} MIDIHDR, *LPMIDIHDR;

typedef struct tagMIDIOUTCAPSA {
    WORD  wMid;
    WORD  wPid;
    UINT  vDriverVersion;
    CHAR  szPname[32];
    WORD  wTechnology;
    WORD  wVoices;
    WORD  wNotes;
    WORD  wChannelMask;
    DWORD dwSupport;
} MIDIOUTCAPSA, *LPMIDIOUTCAPSA;
typedef MIDIOUTCAPSA MIDIOUTCAPS, *LPMIDIOUTCAPS;

static inline MMRESULT midiOutOpen(LPHMIDIOUT ph, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
{ (void)uDeviceID;(void)dwCallback;(void)dwInstance;(void)fdwOpen; if(ph)*ph=0; return MMSYSERR_ERROR; }
static inline MMRESULT midiOutClose(HMIDIOUT h)                              { (void)h; return MMSYSERR_NOERROR; }
static inline MMRESULT midiOutShortMsg(HMIDIOUT h, DWORD dwMsg)             { (void)h;(void)dwMsg; return MMSYSERR_NOERROR; }
static inline MMRESULT midiOutLongMsg(HMIDIOUT h, LPMIDIHDR p, UINT cb)     { (void)h;(void)p;(void)cb; return MMSYSERR_NOERROR; }
static inline MMRESULT midiOutPrepareHeader(HMIDIOUT h, LPMIDIHDR p, UINT cb)   { (void)h;(void)p;(void)cb; return MMSYSERR_NOERROR; }
static inline MMRESULT midiOutUnprepareHeader(HMIDIOUT h, LPMIDIHDR p, UINT cb) { (void)h;(void)p;(void)cb; return MMSYSERR_NOERROR; }
static inline MMRESULT midiOutReset(HMIDIOUT h)                              { (void)h; return MMSYSERR_NOERROR; }
static inline UINT     midiOutGetNumDevs(void)                              { return 0; }
static inline MMRESULT midiOutGetDevCapsA(UINT_PTR id, LPMIDIOUTCAPSA p, UINT cb) { (void)id;(void)p;(void)cb; return MMSYSERR_ERROR; }
#define midiOutGetDevCaps midiOutGetDevCapsA

/* ---- multimedia timer (timeSetEvent callback) -------------------------------
   Single-thread bring-up: timeSetEvent is a no-op returning a fake non-zero id;
   the periodic 3D-frame callback is driven by the main loop instead. */
#ifndef TIME_ONESHOT
#define TIME_ONESHOT 0x0000
#define TIME_PERIODIC 0x0001
#define TIME_CALLBACK_FUNCTION 0x0000
#define TIME_KILL_SYNCHRONOUS  0x0100
#endif
typedef void (CALLBACK *LPTIMECALLBACK)(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
/* Linux port: real periodic/one-shot timer over pthreads (bob_threads.cpp) --
   drives Mast3d::StaticTimeProc -> the game "move cycle". */
#ifdef __cplusplus
extern "C" unsigned int bob_time_set_event(unsigned ms, unsigned res,
    void (*cb)(unsigned,unsigned,unsigned long,unsigned long,unsigned long), unsigned long user, unsigned fdw);
extern "C" unsigned int bob_time_kill_event(unsigned id);
#else
extern unsigned int bob_time_set_event(unsigned, unsigned, void(*)(unsigned,unsigned,unsigned long,unsigned long,unsigned long), unsigned long, unsigned);
extern unsigned int bob_time_kill_event(unsigned);
#endif
static inline MMRESULT timeSetEvent(UINT delay, UINT res, LPTIMECALLBACK cb, DWORD_PTR user, UINT fdw) {
    return (MMRESULT)bob_time_set_event(delay, res,
        (void(*)(unsigned,unsigned,unsigned long,unsigned long,unsigned long))cb, (unsigned long)user, fdw);
}
static inline MMRESULT timeKillEvent(UINT id) { return (MMRESULT)bob_time_kill_event(id); }
/* timeBeginPeriod/timeEndPeriod already provided by compat_winbase.h */

#endif /* FF_LINUX */
#endif
