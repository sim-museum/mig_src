/* tools/dplay_probe.cpp -- R6.2 packet gate: prove the DirectPlay TRANSPORT, without the game.
 *
 * WHY NOT DRIVE THE UI. The two-process test cannot go through the front end yet: gl-lock
 * serialises the display so host and client cannot both run, and the lobby's "Create Game" click
 * does not fire (BOB_AUTOCLICK advances per screen PAINT and the lobby idles after one). Those are
 * harness problems, not transport problems -- so this exercises the object directly through the
 * same COM entry point the game uses. If this passes and the game still cannot host, the fault is
 * in the UI path, and the two are no longer confounded.
 *
 *   dplay_probe host    -- Open(CREATE), then Receive in a loop, print what arrives
 *   dplay_probe join    -- EnumSessions, Open(JOIN), CreatePlayer, Send one packet
 *   dplay_probe solo    -- EnumSessions with NO host: must report 0 sessions and DP_OK
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "DPLAY.H"

extern "C" HRESULT ma_dplay_create(void** ppv);

static int g_found = 0;
static BOOL FAR PASCAL sess_cb(LPCDPSESSIONDESC2 sd, LPDWORD, DWORD, LPVOID) {
    g_found++;
    printf("  [probe] session found: \"%s\"\n", sd && sd->lpszSessionNameA ? sd->lpszSessionNameA : "(unnamed)");
    return TRUE;
}

int main(int argc, char** argv) {
    const char* mode = argc > 1 ? argv[1] : "solo";
    IDirectPlay4* dp = 0;
    if (ma_dplay_create((void**)&dp) != DP_OK || !dp) { printf("FAIL: no object\n"); return 2; }
    dp->InitializeConnection(0, 0);

    if (!strcmp(mode, "host")) {
        DPSESSIONDESC2 d; memset(&d, 0, sizeof(d));
        d.dwSize = sizeof(d); d.lpszSessionNameA = (char*)"BoB probe session"; d.dwMaxPlayers = 8;
        if (dp->Open(&d, DPOPEN_CREATE) != DP_OK) { printf("FAIL: Open(CREATE)\n"); return 2; }
        DPID pid = 0; DPNAME nm; memset(&nm, 0, sizeof(nm));
        nm.dwSize = sizeof(nm); nm.lpszShortNameA = (char*)"host";
        dp->CreatePlayer(&pid, &nm, 0, 0, 0, 0);
        printf("  [probe] hosting as pid %u; waiting for a packet\n", (unsigned)pid);
        if (pid != DPID_SERVERPLAYER) { printf("FAIL: host pid is %u, expected %u\n",
                                               (unsigned)pid, (unsigned)DPID_SERVERPLAYER); }
        for (int i = 0; i < 200; i++) {          /* ~20 s */
            DPID f = 0, t = 0; char buf[512]; DWORD n = sizeof(buf);
            if (dp->Receive(&f, &t, 0, buf, &n) == DP_OK) {
                buf[n < sizeof(buf) ? n : sizeof(buf) - 1] = 0;
                printf("  [probe] RECEIVED %u bytes from pid %u: \"%s\"\n", (unsigned)n, (unsigned)f, buf);
                printf("PASS host\n"); dp->Close(); dp->Release(); return 0;
            }
            usleep(100000);
        }
        printf("FAIL: no packet arrived\n"); dp->Close(); dp->Release(); return 1;
    }

    /* join / solo both start by looking for a host */
    dp->EnumSessions(0, 800, sess_cb, 0, 0);
    if (!strcmp(mode, "solo")) {
        printf("  [probe] solo: %d session(s) found\n", g_found);
        if (g_found == 0) { printf("PASS solo (no host -> 0 sessions, no error)\n"); dp->Release(); return 0; }
        printf("FAIL solo: found a session with no host running\n"); dp->Release(); return 1;
    }
    if (g_found == 0) { printf("FAIL join: no session found\n"); dp->Release(); return 1; }
    DPSESSIONDESC2 d; memset(&d, 0, sizeof(d)); d.dwSize = sizeof(d);
    if (dp->Open(&d, DPOPEN_JOIN) != DP_OK) { printf("FAIL: Open(JOIN)\n"); dp->Release(); return 2; }
    DPID pid = 0; DPNAME nm; memset(&nm, 0, sizeof(nm));
    nm.dwSize = sizeof(nm); nm.lpszShortNameA = (char*)"client";
    dp->CreatePlayer(&pid, &nm, 0, 0, 0, 0);
    /* R6.3: the host owns the id space -- a client must NOT mint its own. */
    printf("  [probe] client pid = %u\n", (unsigned)pid);
    if (pid == DPID_SERVERPLAYER) {
        printf("FAIL: client got pid %u, the same as the host -- ids are not unique\n", (unsigned)pid);
        dp->Close(); dp->Release(); return 1;
    }
    const char* msg = "hello from the client";
    HRESULT r = dp->Send(pid, DPID_SERVERPLAYER, 0, (LPVOID)msg, (DWORD)strlen(msg) + 1);
    printf("  [probe] Send -> %s\n", r == DP_OK ? "DP_OK" : "FAILED");
    dp->Close(); dp->Release();
    return r == DP_OK ? 0 : 1;
}
