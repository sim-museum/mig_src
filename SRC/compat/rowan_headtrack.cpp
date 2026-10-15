/* HEADTRACK-1 (PO 2026-10-15): TrackIR / Delanclip head tracking for the Rowan ports (MiG Alley, Battle of Britain),
   after the BDG patches' TrackIR design (BDG MiG Alley 0.85F, BDG BoB 0.95-0.99): the tracker's absolute angles turn
   the pilot's head in the cockpit views; when poses stop, the keyboard / mouse / hat views take over again.

   On Linux the tracker reaches the game through opentrack (it drives TrackIR, Delanclip and webcams): its "UDP over
   network" output sends six little-endian doubles per pose -- x, y, z (cm), yaw, pitch, roll (degrees) -- to
   127.0.0.1:4242. Nothing to bundle, so the AppImages stay stand-alone.

   Unlike BDG, which needed TRACKIR=ON in bdg.txt, head tracking here is on whenever opentrack is sending (the PO's
   bdg.txt says TRACKIR=OFF only because no tracker was ever attached). Env (prefix MA_ or BOB_):
     <P>HEADTRACK=0           off
     <P>HEADTRACK_PORT=n      UDP port (4242)
     <P>HEADTRACK_SIGNS=y,p,r +1/-1 per axis (default 1,1,1); opentrack's own per-axis "invert" does the same
     <P>TRACE_HEADTRACK=1     one pose line per second
   This file is identical in ~/ma and ~/bob. */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(MA_LINUX)
#define HT_ENV(s) "MA_" s
#else
#define HT_ENV(s) "BOB_" s
#endif

static int    s_sock = -2;           /* -2: not opened yet, -1: off */
static double s_pose[6];
static double s_lastMs = -1e9;
static float  s_sign[3] = { 1.f, 1.f, 1.f };
static bool   s_trace = false;

static double ht_now_ms()
{
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void ht_open()
{
    s_sock = -1;
    const char* off = getenv(HT_ENV("HEADTRACK"));
    if (off && off[0] == '0')
        return;
    int port = getenv(HT_ENV("HEADTRACK_PORT")) ? atoi(getenv(HT_ENV("HEADTRACK_PORT"))) : 4242;
    if (const char* s = getenv(HT_ENV("HEADTRACK_SIGNS")))
        sscanf(s, "%f,%f,%f", &s_sign[0], &s_sign[1], &s_sign[2]);
    s_trace = getenv(HT_ENV("TRACE_HEADTRACK")) != NULL;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        return;
    sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons((unsigned short)port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(s, (sockaddr*)&a, sizeof a) < 0) {
        fprintf(stderr, "[headtrack] UDP 127.0.0.1:%d busy -- head tracking off\n", port);
        close(s);
        return;
    }
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
    s_sock = s;
    fprintf(stderr, "[headtrack] listening for opentrack on UDP 127.0.0.1:%d\n", port);
}

/* The newest pose, if one arrived in the last 500 ms: head yaw, pitch, roll in degrees (opentrack's signs times
   <P>HEADTRACK_SIGNS) and the fore/aft offset z in cm. False = no tracker: the caller leaves the view alone. */
bool rowan_headtrack_pose(float* yawDeg, float* pitchDeg, float* rollDeg, float* zCm)
{
    if (s_sock == -2)
        ht_open();
    if (s_sock < 0)
        return false;
    double d[6];
    for (;;) {   /* drain: keep only the newest pose */
        ssize_t n = recv(s_sock, d, sizeof d, 0);
        if (n < 0)
            break;
        if (n == (ssize_t)sizeof d) {
            memcpy(s_pose, d, sizeof d);
            s_lastMs = ht_now_ms();
        }
    }
    double now = ht_now_ms();
    if (now - s_lastMs > 500.0)
        return false;
    *yawDeg = (float)(s_sign[0] * s_pose[3]);
    *pitchDeg = (float)(s_sign[1] * s_pose[4]);
    *rollDeg = (float)(s_sign[2] * s_pose[5]);
    *zCm = (float)s_pose[2];
    if (s_trace) {
        static double lastTrace = 0;
        if (now - lastTrace >= 1000.0) {
            lastTrace = now;
            fprintf(stderr, "[headtrack] pose yaw=%.1f pitch=%.1f roll=%.1f z=%.1fcm\n",
                    *yawDeg, *pitchDeg, *rollDeg, *zCm);
        }
    }
    return true;
}
