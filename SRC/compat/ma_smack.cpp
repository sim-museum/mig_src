/* E1 (MA): the game's Smacker player -- OpenSmack / DoSmack / CloseSmack, which CSmackerDialog
   (SMKDLG.CPP) drives from a 1 ms timer for the title intro, campaign intros and debrief clips.
   Video: decoded by ma_smack_core (libavcodec), blitted every paint pass at the dialog's rect via
   the GDI shim's StretchDIBits. Audio: the file's track streamed to an OpenAL source on the
   context ma_openal.cpp made current. MA_NO_SMACK=1 makes every clip "finish" immediately. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "afxwin.h"              /* CWnd: m_maX/m_maY/m_maW/m_maH/m_maVisible (compat geometry) */
#include "ma_smack_core.h"

extern "C" void ma_gdi_stretch_dibits(void* hdc,int dx,int dy,int dw,int dh,int sx,int sy,int sw,int sh,const void* bits,const void* bmi);
extern "C" int  bob_resolve_path(const char* in, char* out, unsigned long outsz);   /* Win32 path -> drive_c path */

struct SmackTag; typedef struct SmackTag Smack;
typedef unsigned short UWord;
void CloseSmack();

namespace {
struct Player {
    MaSmk*   smk = 0;
    CWnd*    wnd = 0;
    double   t0 = 0.0;          /* wall clock (ms) when frame 0 was due */
    bool     haveFrame = false;
    bool     done = false;
    int      X = -1, Y = -1, W = 0, H = 0;   /* caller placement: X,Y >= 0 = offset in the window, W/H 0 = native */
    /* BITMAPINFOHEADER + 256 RGBQUAD, top-down */
    unsigned char bmi[40 + 256*4];
    /* audio */
    ALuint   src = 0;
    ALuint   bufs[8];
    int      nbufs = 0;
    bool     audio = false;
    bool     playing = false;
    unsigned char pcm[65536];
};
Player* g_p = 0;
double now_ms() { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6; }
int trace() { static int t = -1; if (t < 0) t = getenv("MA_TRACE_SMACK") ? 1 : 0; return t; }

void audio_open(Player* p) {
    if (!ma_smk_audio_rate(p->smk) || getenv("MA_NO_AUDIO")) return;
    if (!alcGetCurrentContext()) { if (trace()) fprintf(stderr, "[smk] no OpenAL context -> silent video\n"); return; }
    alGetError();
    alGenSources(1, &p->src);
    alGenBuffers(8, p->bufs);
    if (alGetError() != AL_NO_ERROR) { p->src = 0; return; }
    alSourcef(p->src, AL_GAIN, 1.0f);
    p->nbufs = 0; p->audio = true;
}
void audio_pump(Player* p) {
    if (!p->audio) return;
    ALint processed = 0;
    alGetSourcei(p->src, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0) { ALuint b; alSourceUnqueueBuffers(p->src, 1, &b); p->bufs[p->nbufs++] = b; }
    const int ch = ma_smk_audio_channels(p->smk);
    while (p->nbufs > 0 && ma_smk_audio_pending(p->smk) >= 4096) {
        int n = ma_smk_take_audio(p->smk, p->pcm, sizeof p->pcm);
        ALuint b = p->bufs[--p->nbufs];
        alBufferData(b, ch == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, p->pcm, n, ma_smk_audio_rate(p->smk));
        alSourceQueueBuffers(p->src, 1, &b);
    }
    ALint st = 0; alGetSourcei(p->src, AL_SOURCE_STATE, &st);
    ALint queued = 0; alGetSourcei(p->src, AL_BUFFERS_QUEUED, &queued);
    if (st != AL_PLAYING && queued >= 2) { alSourcePlay(p->src); p->playing = true; }
}
void audio_close(Player* p) {
    if (!p->audio) return;
    alSourceStop(p->src);
    ALint queued = 0; alGetSourcei(p->src, AL_BUFFERS_QUEUED, &queued);
    while (queued-- > 0) { ALuint b; alSourceUnqueueBuffers(p->src, 1, &b); p->bufs[p->nbufs++] = b; }
    alDeleteSources(1, &p->src);
    alDeleteBuffers(8, p->bufs);
    p->audio = false;
}
void bmi_fill(Player* p) {
    memset(p->bmi, 0, sizeof p->bmi);
    int w = ma_smk_width(p->smk), h = ma_smk_height(p->smk);
    *(int*)(p->bmi + 0) = 40; *(int*)(p->bmi + 4) = w; *(int*)(p->bmi + 8) = -h;    /* top-down */
    *(unsigned short*)(p->bmi + 12) = 1; *(unsigned short*)(p->bmi + 14) = 8;
    *(int*)(p->bmi + 16) = 0;                                                        /* BI_RGB */
    *(unsigned*)(p->bmi + 20) = (unsigned)(ma_smk_pitch(p->smk) * h);
    *(unsigned*)(p->bmi + 32) = 256;
}
}

Smack* OpenSmack(const char* path, void* wnd, int X, int Y, int w, int h)
{
    CloseSmack();
    if (getenv("MA_NO_SMACK")) return 0;
    Player* p = new Player;
    p->wnd = (CWnd*)wnd; p->X = X; p->Y = Y; p->W = w; p->H = h;
    /* File_Man hands back the game's Win32 path (C:\rowan\mig\smacker\intro.smk); libavformat
       read "C:" as a protocol. Resolve it to the install's drive_c path like every other file open. */
    char upath[1024]; upath[0] = 0;
    if (path && !bob_resolve_path(path, upath, sizeof upath)) upath[0] = 0;
    const char* open_path = (upath[0] ? upath : path);
    p->smk = open_path ? ma_smk_open(open_path) : 0;
    if (!p->smk) { fprintf(stderr, "[smk] cannot open '%s' (resolved '%s') -> clip skipped\n", path ? path : "(null)", upath); delete p; return 0; }
    bmi_fill(p);
    audio_open(p);
    p->t0 = now_ms();
    g_p = p;
    if (trace()) fprintf(stderr, "[smk] open '%s' %dx%d %.1f ms/frame audio=%d Hz x%d wnd=%p rect(%d,%d %dx%d)\n",
                         path, ma_smk_width(p->smk), ma_smk_height(p->smk), ma_smk_frame_ms(p->smk),
                         ma_smk_audio_rate(p->smk), ma_smk_audio_channels(p->smk), wnd,
                         p->wnd ? p->wnd->m_maX : -1, p->wnd ? p->wnd->m_maY : -1, p->wnd ? p->wnd->m_maW : -1, p->wnd ? p->wnd->m_maH : -1);
    return (Smack*)p;
}

UWord DoSmack(void* /*wnd*/)
{
    Player* p = g_p;
    if (!p || p->done) return 0;
    /* which frame is due now? decode forward to it (dropping late frames keeps sound and picture together) */
    const double due = (now_ms() - p->t0) / (ma_smk_frame_ms(p->smk) > 1.0 ? ma_smk_frame_ms(p->smk) : 71.0);
    int target = (int)due;
    int guard = 0;
    while (ma_smk_frame_index(p->smk) < target && guard++ < 64) {
        int rc = ma_smk_next_frame(p->smk);
        if (rc == 1) { p->haveFrame = true; continue; }
        if (rc == 0) {                                   /* end of clip: let the audio tail finish */
            audio_pump(p);
            ALint st = AL_STOPPED; if (p->audio) alGetSourcei(p->src, AL_SOURCE_STATE, &st);
            if (!p->audio || st != AL_PLAYING) { p->done = true; if (trace()) fprintf(stderr, "[smk] finished after %d frames\n", ma_smk_frame_index(p->smk) + 1); return 0; }
            return 1;
        }
        p->done = true; return 0;                       /* decode error: end the clip */
    }
    audio_pump(p);
    return 1;
}

void CloseSmack()
{
    Player* p = g_p; g_p = 0;
    if (!p) return;
    audio_close(p);
    if (p->smk) ma_smk_close(p->smk);
    delete p;
}

/* called at the end of every ma_ole_draw_all pass: paint the current frame at the dialog's rect */
extern "C" void ma_smack_paint(void* screenHdc)
{
    Player* p = g_p;
    if (!p || !p->haveFrame || !p->wnd || !screenHdc) return;
    int dx = p->wnd->m_maX, dy = p->wnd->m_maY, dw = p->wnd->m_maW, dh = p->wnd->m_maH;
    int w = ma_smk_width(p->smk), h = ma_smk_height(p->smk);
    if (dw <= 0 || dh <= 0) { dw = w; dh = h; }
    int fx, fy, fw, fh;
    if (p->X >= 0 && p->Y >= 0) {              /* the caller placed it (title intro): native size at that offset */
        fx = dx + p->X; fy = dy + p->Y; fw = p->W > 0 ? p->W : w; fh = p->H > 0 ? p->H : h;
    } else {                                    /* centred, aspect kept, inside the window rect */
        fw = dw; fh = (int)((long)dw * h / (w > 0 ? w : 1));
        if (fh > dh) { fh = dh; fw = (int)((long)dh * w / (h > 0 ? h : 1)); }
        fx = dx + (dw - fw) / 2; fy = dy + (dh - fh) / 2;
    }
    ma_gdi_stretch_dibits(screenHdc, fx, fy, fw, fh, 0, 0, w, h, ma_smk_frame_bits(p->smk), p->bmi);
    if (trace()) { static int n = 0; if (n++ % 120 == 0) fprintf(stderr, "[smk] paint frame %d at (%d,%d %dx%d)\n", ma_smk_frame_index(p->smk), fx, fy, fw, fh); }
}
