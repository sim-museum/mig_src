/* SRC/compat/ma_dplay.cpp -- DirectPlay over UDP sockets.
 *
 * ADOPTED FROM BoB (cross-port note 43, 2026-08-30), not rewritten. BoB implemented this as
 * R6.1 (the object) and R6.2 (the transport) against the SAME vendored DX6 DPLAY.H, in the same
 * Rowan engine, for the same DPlay/Aggrgtor layer -- and it is measured working there: the
 * lobby is reached with a negative control, and a packet crosses two processes.
 *
 * MA S370 had priced writing this from scratch at 53 pure-virtual methods (this port defines
 * PURE as "= 0", so every one must exist before the class can be instantiated), of which the
 * game calls 17. That estimate was right, which is exactly why it should be adopted instead:
 * the cheapest correct implementation is the one already measured somewhere else.
 *
 * Differences from the BoB original are ONLY the env-var prefix (MA_ for BOB_) and the entry
 * point name. Keep it that way: if either port fixes a transport bug, the diff should stay
 * readable so the other can take it. Changes here belong in a cross-port note.
 *
 * WHY THIS FILE EXISTS. Multiplayer was never missing game code: the engine's DPlay class and
 * Aggrgtor packet layer are compiled in, and the lobby screens render and navigate. What was
 * missing was the OBJECT, and the gap was ONE call --
 *
 *     DPlay::CreateDPlayInterface()  (SRC/COMMS/Comms.cpp:807)
 *       -> CoCreateInstance(CLSID_DirectPlay, ..., IID_IDirectPlay4A, &lpDP4)
 *
 * -- against a compat CoCreateInstance that answered E_NOINTERFACE for every CLSID.
 * (Both backlogs first recorded the gap as a missing `DirectPlayCreate`. True, and irrelevant:
 * the game never calls it. Corrected in ma S323 / bob R6-S318.)
 *
 * WHAT IS IMPLEMENTED, AND WHY EXACTLY THIS SET. Not chosen from the header -- OBSERVED. Every
 * unimplemented method logs itself under MA_TRACE_DPLAY=1, so walking the UI made the game name
 * what it needs:
 *
 *     Multi-Player  -> CoCreateInstance, EnumConnections            (R6.1)
 *     Join Game     -> InitializeConnection, EnumSessions           (R6.2)
 *     Back          -> CancelMessage, Close, Release  (ref -> 0, no leak)
 *
 * IDirectPlay4 is declared with DECLARE_INTERFACE_, which this compat layer expands to a C++
 * abstract class, so this SUBCLASSES it and the compiler lays out the 53-entry vtable. The 36
 * still-unimplemented overrides are GENERATED from SRC/H/DPLAY.H, never typed: hand-ordering COM
 * function pointers is a silent-corruption trap.
 *
 * THE TRANSPORT is deliberately plain UDP on one socket, in the spirit of what DirectPlay's
 * TCP/IP provider did: a host binds a port; clients discover it with a broadcast-style probe and
 * then exchange datagrams. The game's own Aggrgtor already handles sequencing, reserve packets and
 * loss -- duplicating that here would be building a second protocol beside the one the game ships.
 *
 * MA_DPLAY_PORT   override the port (default 47624, DirectPlay's classic port)
 * MA_DPLAY_HOST   client: where to look for a host (default 127.0.0.1)
 * MA_TRACE_DPLAY  log every call, including the unimplemented ones
 * MA_NO_DPLAY     restore E_NOINTERFACE -- the negative control for tools/port/mp_connect.sh
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "DPLAY.H"

/* MP-6 S2: the drain site that is about to call Receive(); see ma_dplay.cpp notes. */
const char* ma_recv_caller = 0;

static int dp_trace(void) { static int t = -1; if (t < 0) t = getenv("MA_TRACE_DPLAY") ? 1 : 0; return t; }
#define DPT(...) do { if (dp_trace()) { fprintf(stderr, "[dplay] " __VA_ARGS__); } } while (0)
/* R6.4: log each unimplemented method ONCE. The first host run produced a 24.7-MILLION-line log
   because the game calls SendEx every frame once a session is live -- a per-call trace turns a
   useful "the next blocker names itself" signal into an unreadable flood, and fills the disk. */
#define UNIMPL(n) do { if (dp_trace()) { static int _once = 0; \
        if (!_once) { _once = 1; fprintf(stderr, "[dplay] %s: not implemented yet\n", (n)); } } } while (0)

static int dp_port(void) { const char* e = getenv("MA_DPLAY_PORT"); int p = e ? atoi(e) : 0; return p > 0 ? p : 47624; }
static const char* dp_host(void) { const char* e = getenv("MA_DPLAY_HOST"); return (e && *e) ? e : "127.0.0.1"; }

/* Wire framing. Deliberately tiny and self-describing: the whole point of the GPL-era design is
 * that a peer update is a few dozen bytes. */
enum { DPMAGIC = 0x424f4250 };            /* 'BOBP' */
enum { MSG_PROBE = 1, MSG_OFFER = 2, MSG_JOIN = 3, MSG_DATA = 4, MSG_ASSIGN = 5 };
struct WireHdr { unsigned int magic, kind, from, to; };

static const int MAXQ = 64;
struct QMsg { unsigned int from, to, len; char data[1024]; };

static GUID  g_tcpGuid = { 0x36E95EE0, 0x8577, 0x11cf, { 0x96,0x0c,0x00,0x80,0xc7,0x53,0x4e,0x82 } };
static char  g_tcpName[] = "Internet TCP/IP Connection For DirectPlay";
static DWORD g_tcpBlob[16];

class BobDPlay4 : public IDirectPlay4
{
    int  ref;
    int  fd;                 /* the one UDP socket */
    int  isHost;
    DPID nextPid;
    DPID myPid;
    DPID assignedPid;      /* R6.3: what the host gave us (client side); 0 until it answers */
    DPID groups[8]; int gmembers[8]; DPID gplayers[8][8]; int ngroups;   /* R6.4 */
    /* MP-6 S3 (2026-09-13, cross-port from bob fb4ea17): pids this HOST has handed to joining
       clients. The game only ever calls AddPlayerToGroup for its OWN player, so without this a
       group holds one member and the receive filter cannot tell a group id from a stranger's
       player id. */
    DPID joined[8]; int njoined;
    struct sockaddr_in peer; /* host: last client seen. client: the host. */
    int  havePeer;
    char sessName[128];
    GUID sessGuid;
    QMsg q[MAXQ]; int qh, qt;

    /* MP-6 S2: set by the game immediately before each Receive() so a delivery can be attributed
       to the drain that took it. Two sites poll: AGGRGTOR.CPP:1918 and COMMS.CPP:3870. */
    /* MP-6 S2 (2026-09-13): count the ANNOUNCE packet at all three stages, so "the host never saw
       it" can be told from "the host was never sent it" and from "it was delivered and the game
       dropped it". The game's packet begins with ULong PacketID (struct CommonData), and
       PID_IAMIN is 0xf00000e4, so the shim can recognise one without parsing anything else.
       S1 measured AddPlayerToGame firing on the MA client and never on the MA host, and BoB the
       exact mirror -- so the loss is directional and this says where it happens. MA_TRACE_IAMIN=1. */
    static bool isAnnounce(const char* d, unsigned n) {
        if (n < 4) return false;
        unsigned id; memcpy(&id, d, 4);
        return id == 0xf00000e4u;
    }
    void noteAnnounce(const char* stage, unsigned f, unsigned t, const char* d, unsigned n) {
        if (!isAnnounce(d, n) || !getenv("MA_TRACE_IAMIN")) return;
        fprintf(stderr, "[iamin-wire] %s from=%u to=%u len=%u\n", stage, f, t, n);
        fflush(stderr);
    }

    void qpush(unsigned f, unsigned t, const char* d, unsigned n) {
        int nx = (qt + 1) % MAXQ;
        if (nx == qh) { DPT("queue full, dropping a packet\n"); return; }
        q[qt].from = f; q[qt].to = t; q[qt].len = n > sizeof(q[qt].data) ? sizeof(q[qt].data) : n;
        memcpy(q[qt].data, d, q[qt].len); qt = nx;
        noteAnnounce("QUEUED", f, t, d, n);
    }
    int qcount() const { return (qt - qh + MAXQ) % MAXQ; }

    /* Drain the socket: answer discovery probes, absorb joins, queue data. Called from every path
     * the game pumps (Receive / GetMessageCount / EnumSessions) so a host answers probes while it
     * is simply sitting in its own message loop. */
    /* PO-76 (S418): COUNT the pumps. A host answers discovery ONLY from here, and here runs only
       when the game calls Receive / GetMessageCount / EnumSessions. So "nobody can find my session"
       and "the game is not pumping" are the same symptom from outside, and a client that finds
       nothing cannot tell them apart. Report periodically -- never at exit, since MA_SHOT-style
       runs leave via _exit() (S328b/S330b/S416). */
    long pumps = 0;
    void pump() {
        if (getenv("MA_TRACE_DPLAY") && (pumps == 0 || (pumps % 500) == 0))
            fprintf(stderr, "[dplay] pump #%ld (host=%d) -- discovery is answered only from here\n",
                    pumps, (int)isHost), fflush(stderr);
        pumps++;
        if (fd < 0) return;
        char buf[2048];
        for (;;) {
            struct sockaddr_in from; socklen_t fl = sizeof(from);
            ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fl);
            if (n < (ssize_t)sizeof(WireHdr)) break;
            WireHdr* h = (WireHdr*)buf;
            if (h->magic != DPMAGIC) continue;
            if (h->kind == MSG_PROBE && isHost) {
                char out[sizeof(WireHdr) + sizeof(sessName)];
                WireHdr* oh = (WireHdr*)out;
                oh->magic = DPMAGIC; oh->kind = MSG_OFFER; oh->from = 0; oh->to = 0;
                memcpy(out + sizeof(WireHdr), sessName, sizeof(sessName));
                sendto(fd, out, sizeof(out), 0, (struct sockaddr*)&from, fl);
                DPT("probe from a client -> offered session \"%s\"\n", sessName);
            } else if (h->kind == MSG_JOIN && isHost) {
                peer = from; havePeer = 1;
                /* R6.3: THE HOST OWNS THE ID SPACE. Before this, each object started nextPid at
                   DPID_SERVERPLAYER independently, so host and client both allocated pid 1 -- the
                   packets still crossed (R6.2 passed) but every player was indistinguishable, and
                   the Aggrgtor addresses its packets BY pid. Found by reading the R6.2 trace, not
                   by a failure: a two-node echo cannot expose an id collision. */
                DPID given = nextPid++;
                if (njoined < 8) joined[njoined++] = given;   /* MP-6 S3 */
                WireHdr ah; ah.magic = DPMAGIC; ah.kind = MSG_ASSIGN;
                ah.from = (unsigned)DPID_SERVERPLAYER; ah.to = (unsigned)given;
                sendto(fd, &ah, sizeof(ah), 0, (struct sockaddr*)&from, fl);
                DPT("client joined from %s:%d -> assigned pid %u\n",
                    inet_ntoa(from.sin_addr), (int)ntohs(from.sin_port), (unsigned)given);
            } else if (h->kind == MSG_ASSIGN && !isHost) {
                assignedPid = (DPID)h->to;
                DPT("host assigned us pid %u\n", (unsigned)assignedPid);
            } else if (h->kind == MSG_DATA) {
                qpush(h->from, h->to, buf + sizeof(WireHdr), (unsigned)(n - sizeof(WireHdr)));
                DPT("received %d data bytes from pid %u\n", (int)(n - sizeof(WireHdr)), h->from);
            }
        }
    }
    int mksock(int bindIt) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) return 0;
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        if (bindIt) {
            struct sockaddr_in a; memset(&a, 0, sizeof(a));
            a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = htons(dp_port());
            if (bind(fd, (struct sockaddr*)&a, sizeof(a)) != 0) {
                DPT("bind(%d) failed: %s\n", dp_port(), strerror(errno));
                close(fd); fd = -1; return 0;
            }
            DPT("host bound to UDP %d\n", dp_port());
        }
        return 1;
    }
public:
    BobDPlay4() : ref(1), fd(-1), isHost(0), nextPid(DPID_SERVERPLAYER), myPid(0), assignedPid(0), njoined(0),
                  havePeer(0), ngroups(0), qh(0), qt(0) {
        memset(&peer, 0, sizeof(peer)); memset(sessName, 0, sizeof(sessName));
        memset(&sessGuid, 0, sizeof(sessGuid));
    }
    ~BobDPlay4() { if (fd >= 0) close(fd); }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj) override {
        (void)riid; if (!ppvObj) return E_POINTER;
        *ppvObj = (LPVOID)this; ref++; return DP_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return (ULONG)++ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        int r = --ref; DPT("Release -> ref=%d\n", r);
        if (r <= 0) { delete this; return 0; }
        return (ULONG)r;
    }

    HRESULT STDMETHODCALLTYPE EnumConnections(LPCGUID, LPDPENUMCONNECTIONSCALLBACK cb,
                                              LPVOID ctx, DWORD) override {
        DPT("EnumConnections -> 1 provider\n");
        if (cb) {
            DPNAME nm; memset(&nm, 0, sizeof(nm));
            nm.dwSize = sizeof(nm); nm.lpszShortNameA = g_tcpName; nm.lpszLongNameA = g_tcpName;
            cb(&g_tcpGuid, (LPVOID)g_tcpBlob, sizeof(g_tcpBlob), &nm, 0, ctx);
        }
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE InitializeConnection(LPVOID lpConnection, DWORD) override {
        (void)lpConnection;
        DPT("InitializeConnection (TCP/IP provider selected)\n");
        return DP_OK;
    }

    HRESULT STDMETHODCALLTYPE Open(LPDPSESSIONDESC2 d, DWORD flags) override {
        if (flags & DPOPEN_CREATE) {
            isHost = 1;
            if (d && d->lpszSessionNameA) { strncpy(sessName, d->lpszSessionNameA, sizeof(sessName)-1); }
            else strncpy(sessName, "Battle of Britain", sizeof(sessName)-1);
            if (d) sessGuid = d->guidInstance;
            if (!mksock(1)) return DPERR_CANTCREATEPLAYER;
            DPT("Open(CREATE) session \"%s\"\n", sessName);
            return DP_OK;
        }
        if (flags & DPOPEN_JOIN) {
            isHost = 0;
            if (!mksock(0)) return DPERR_NOCONNECTION;
            memset(&peer, 0, sizeof(peer));
            peer.sin_family = AF_INET; peer.sin_port = htons(dp_port());
            peer.sin_addr.s_addr = inet_addr(dp_host());
            havePeer = 1;
            WireHdr h; h.magic = DPMAGIC; h.kind = MSG_JOIN; h.from = 0; h.to = 0;
            sendto(fd, &h, sizeof(h), 0, (struct sockaddr*)&peer, sizeof(peer));
            DPT("Open(JOIN) -> host %s:%d\n", dp_host(), dp_port());
            for (int i = 0; i < 40 && assignedPid == 0; i++) { pump(); usleep(25000); }  /* R6.3 */
            if (assignedPid == 0) DPT("host did not assign a pid (joining anyway)\n");
            return DP_OK;
        }
        UNIMPL("Open(other flags)");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Close() override {
        DPT("Close\n");
        if (fd >= 0) { close(fd); fd = -1; }
        isHost = 0; havePeer = 0; qh = qt = 0;
        return DP_OK;
    }

    /* Probe for a host and report what answers. With no host running, no callback fires and this
     * returns DP_OK with an empty list -- which is the honest answer, not an error. */
    HRESULT STDMETHODCALLTYPE EnumSessions(LPDPSESSIONDESC2 d, DWORD timeout,
                                           LPDPENUMSESSIONSCALLBACK2 cb, LPVOID ctx, DWORD) override {
        (void)d;
        if (fd < 0 && !mksock(0)) return DPERR_NOCONNECTION;
        struct sockaddr_in to; memset(&to, 0, sizeof(to));
        to.sin_family = AF_INET; to.sin_port = htons(dp_port()); to.sin_addr.s_addr = inet_addr(dp_host());
        WireHdr h; h.magic = DPMAGIC; h.kind = MSG_PROBE; h.from = 0; h.to = 0;
        sendto(fd, &h, sizeof(h), 0, (struct sockaddr*)&to, sizeof(to));
        DPT("EnumSessions: probing %s:%d\n", dp_host(), dp_port());

        int found = 0;
        unsigned waitms = timeout ? (timeout > 2000 ? 2000 : timeout) : 400;
        for (unsigned t = 0; t < waitms; t += 20) {
            char buf[2048]; struct sockaddr_in from; socklen_t fl = sizeof(from);
            ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fl);
            if (n >= (ssize_t)sizeof(WireHdr)) {
                WireHdr* rh = (WireHdr*)buf;
                if (rh->magic == DPMAGIC && rh->kind == MSG_OFFER) {
                    DPSESSIONDESC2 sd; memset(&sd, 0, sizeof(sd));
                    sd.dwSize = sizeof(sd);
                    sd.lpszSessionNameA = buf + sizeof(WireHdr);
                    sd.dwMaxPlayers = 8; sd.dwCurrentPlayers = 1;
                    sd.dwFlags = 0;
                    DWORD tmo = waitms;
                    found++;
                    DPT("EnumSessions: found \"%s\"\n", sd.lpszSessionNameA);
                    if (cb && !cb(&sd, &tmo, 0, ctx)) break;
                }
            }
            usleep(20000);
        }
        DPT("EnumSessions -> %d session(s)\n", found);
        return DP_OK;
    }

    HRESULT STDMETHODCALLTYPE CreatePlayer(LPDPID pid, LPDPNAME nm, HANDLE, LPVOID, DWORD, DWORD) override {
        /* R6.3: a client uses the id the HOST gave it; only the host mints ids. */
        myPid = (!isHost && assignedPid != 0) ? assignedPid : nextPid++;
        if (pid) *pid = myPid;
        DPT("CreatePlayer \"%s\" -> pid %u\n",
            (nm && nm->lpszShortNameA) ? nm->lpszShortNameA : "(unnamed)", (unsigned)myPid);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE DestroyPlayer(DPID id) override { DPT("DestroyPlayer %u\n", (unsigned)id); return DP_OK; }

    HRESULT STDMETHODCALLTYPE Send(DPID from, DPID to, DWORD, LPVOID data, DWORD len) override {
        /* PO-76 (S431, cross-port from BoB R26): NO PEER IS NOT AN ERROR.
         * This returned DPERR_NOCONNECTION whenever nobody had connected, and the game reads that
         * as "comms are broken". Real DirectPlay does not: a Send to a GROUP with no members
         * transmits nothing and returns DP_OK. Only an absent socket is a connection error.
         * S430 traced the consequence end to end: the game broadcasts to a group the moment it
         * starts a flight (UISendFlyNow -> SendMessageToPlayers(playergroupID) -> SendEx), so a
         * HOST WHO IS ALONE could never take off -- UISendFlyNow FALSE, UINetworkSelectFly refuses,
         * CommsSelectFly returns FALSE, and the FLY click silently did nothing.
         * MA_STRICT_SEND=1 restores the old behaviour as the negative control. */
        noteAnnounce("SENT", (unsigned)from, (unsigned)to, (const char*)data, (unsigned)len);
        if (fd < 0) return DPERR_NOCONNECTION;
        if (!havePeer) {
            if (getenv("MA_STRICT_SEND")) return DPERR_NOCONNECTION;
            DPT("Send %u bytes pid %u -> %u (no peer yet -- DP_OK, nothing transmitted)\n",
                (unsigned)len, (unsigned)from, (unsigned)to);
            return DP_OK;
        }
        char out[2048];
        if (len > sizeof(out) - sizeof(WireHdr)) len = sizeof(out) - sizeof(WireHdr);
        WireHdr* h = (WireHdr*)out;
        h->magic = DPMAGIC; h->kind = MSG_DATA; h->from = (unsigned)from; h->to = (unsigned)to;
        memcpy(out + sizeof(WireHdr), data, len);
        ssize_t s = sendto(fd, out, sizeof(WireHdr) + len, 0, (struct sockaddr*)&peer, sizeof(peer));
        DPT("Send %u bytes pid %u -> %u (%s)\n", (unsigned)len, (unsigned)from, (unsigned)to,
            s > 0 ? "ok" : strerror(errno));
        return s > 0 ? DP_OK : DPERR_GENERIC;
    }
    /* MP-6 S3: every player id this side has seen -- our own, and any joiner the host handed a pid. */
    bool isKnownPlayer(unsigned pid) const {
        if (pid == (unsigned)myPid) return true;
        for (int i = 0; i < njoined; i++) if ((unsigned)joined[i] == pid) return true;
        return false;
    }
    bool isKnownGroup(unsigned gid) const {
        for (int gi = 0; gi < ngroups; gi++) if ((unsigned)groups[gi] == gid) return true;
        return false;
    }
    bool inGroup(unsigned gid, unsigned pid) const {
        for (int gi = 0; gi < ngroups; gi++) {
            if ((unsigned)groups[gi] != gid) continue;
            for (int k = 0; k < gmembers[gi]; k++)
                if ((unsigned)gplayers[gi][k] == pid) return true;
        }
        return false;
    }
    HRESULT STDMETHODCALLTYPE Receive(LPDPID from, LPDPID to, DWORD rflags, LPVOID data, LPDWORD size) override {
        const unsigned toIn   = to   ? (unsigned)*to   : 0u;   /* BEFORE the shim overwrites *to */
        const unsigned fromIn = from ? (unsigned)*from : 0u;
        pump();
        if (qcount() == 0) return DPERR_NOMESSAGES;
        /* MP-6 S3, ported from bob fb4ea17. This shim ignored lpidTo entirely and returned the
           queue head to whoever asked first. DPlay::ReceiveNextMessage passes `To` IN and its own
           comment says it depends on the filter -- "receive message to mydplayid in case I am
           aggregator. Dont want to receive packets sent to aggregator here!!!!" -- and the game
           has nine such callers, each a wait loop looking for one specific reply. S2 measured the
           consequence: the host's PID_IAMIN was delivered to a loop asking for player 1 and
           discarded (addplayer 0), while the client's identical packet reached the dispatcher.
           MA_NO_RECV_FILTER=1 restores take-the-head; MA_MP_NOFROMFILTER=1 disables only the
           FROMPLAYER half. */
        static int nofilter = -1;
        if (nofilter < 0) nofilter = getenv("MA_NO_RECV_FILTER") ? 1 : 0;
        int idx = qh;
        if (!nofilter && (rflags & DPRECEIVE_FROMPLAYER) && from
            && !getenv("MA_MP_NOFROMFILTER")) {
            int found = -1;
            for (int i = qh; i != qt; i = (i + 1) % MAXQ)
                if (q[i].from == fromIn) { found = i; break; }
            if (found < 0) return DPERR_NOMESSAGES;
            idx = found;
        }
        else if (!nofilter && (rflags & DPRECEIVE_TOPLAYER) && to) {
            int found = -1;
            for (int i = qh; i != qt; i = (i + 1) % MAXQ) {
                unsigned dst = q[i].to;
                /* deliver what is addressed to this player, to a group it belongs to (real
                   DirectPlay expands a group send to its members), to 0 (the game's broadcast
                   address), or to an id this side does not know as a player -- that last case is a
                   group from the other side's numbering and must not be dropped, or the FlyNow
                   broadcast dies. Traffic addressed to ANOTHER KNOWN PLAYER stays queued for the
                   caller it belongs to, which is the whole point. */
                /* MP-6 S4: a KNOWN group is routed by MEMBERSHIP; only an id this side knows
                   nothing about falls through to the permissive catch-all.
                   Without this, dst=2 (the group) is not a known PLAYER, so !isKnownPlayer(dst)
                   handed the announce to whichever caller polled first -- measured as
                   WINMOVE.CPP:2416, the aggregator drain inside SendInit2Packet, which passes
                   `to = _DPlay.aggID` (1) and whose own comment says it wants only "packets that
                   have been sent to me as a result of sends to ID 0". It is not a member of group
                   2, so it should never have been given group traffic. S3's group adoption is what
                   makes this decidable: before it, a guest knew no groups at all and the catch-all
                   was the only way a broadcast could ever arrive. MA_MP_LOOSEGROUP=1 reverts. */
                const bool strict = !getenv("MA_MP_LOOSEGROUP");
                const bool ok = (dst == toIn) || (dst == 0) ||
                                ((strict && isKnownGroup(dst)) ? inGroup(dst, toIn)
                                                               : (inGroup(dst, toIn) || !isKnownPlayer(dst)));
                if (ok) { found = i; break; }
            }
            if (found < 0) return DPERR_NOMESSAGES;
            idx = found;
        }
        QMsg& m = q[idx];
        if (size && *size < m.len) { *size = m.len; return DPERR_BUFFERTOOSMALL; }
        if (from) *from = (DPID)m.from;
        if (to)   *to   = (DPID)m.to;
        if (data && size) { memcpy(data, m.data, m.len); *size = m.len; }
        /* MP-6 S2: WHICH caller drained it. Two sites poll this queue -- AGGRGTOR.CPP:1918 with
           DPRECEIVE_TOPLAYER (0x1, the aggregator, which runs in the 3-D) and COMMS.CPP:3870 (the
           dispatcher that reaches ProcessPlayerMessage). This shim ignores the flags and returns
           the queue head to whoever asks first, so printing the flags names the thief. */
        if (isAnnounce(m.data, m.len) && getenv("MA_TRACE_IAMIN")) {
            fprintf(stderr, "[iamin-wire] DELIVERED caller=%s from=%u to=%u len=%u flags=0x%lx toarg=%u\n",
                    ma_recv_caller ? ma_recv_caller : "(untagged)",
                    m.from, m.to, m.len, (unsigned long)rflags, toIn);
            fflush(stderr);
        }
        /* remove q[idx], preserving the order of everything still queued */
        for (int i = idx; i != qh; i = (i - 1 + MAXQ) % MAXQ)
            q[i] = q[(i - 1 + MAXQ) % MAXQ];
        qh = (qh + 1) % MAXQ;
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE GetMessageCount(DPID, LPDWORD n) override {
        pump(); if (n) *n = (DWORD)qcount(); return DP_OK;
    }
    /* R6.4: GROUPS. The game creates a group immediately after Open(CREATE) -- the trace named
       CreateGroup as the next stop, which is what the per-method logging is for. For a session
       this size a group is just an id plus a membership list; the Aggrgtor addresses traffic by
       PLAYER id, so group routing is not on the packet path yet. Implemented as real bookkeeping
       rather than DP_OK-and-forget, so EnumGroups/EnumGroupPlayers can answer truthfully. */
    HRESULT STDMETHODCALLTYPE CreateGroup(LPDPID pid, LPDPNAME nm, LPVOID, DWORD, DWORD) override {
        DPID g = nextPid++;
        if (pid) *pid = g;
        if (ngroups < 8) {
            groups[ngroups] = g; gmembers[ngroups] = 0;
            /* MP-6 S3: a group created after clients joined must contain them too */
            for (int j = 0; j < njoined && gmembers[ngroups] < 8; j++) {
                gplayers[ngroups][gmembers[ngroups]++] = joined[j];
                DPT("seeded group %u with already-joined pid %u\n", (unsigned)g, (unsigned)joined[j]);
            }
            ngroups++;
        }
        DPT("CreateGroup \"%s\" -> gid %u\n",
            (nm && nm->lpszShortNameA) ? nm->lpszShortNameA : "(unnamed)", (unsigned)g);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE DestroyGroup(DPID g) override { DPT("DestroyGroup %u\n", (unsigned)g); return DP_OK; }
    HRESULT STDMETHODCALLTYPE AddPlayerToGroup(DPID g, DPID p) override {
        bool matched = false;
        for (int i = 0; i < ngroups; i++)
            if (groups[i] == g && gmembers[i] < 8) { gplayers[i][gmembers[i]++] = p; matched = true; break; }
        /* MP-6 S3: a GUEST calls this for its own player with the group id the HOST created and
           sent over the wire (COMMS.CPP:2095 -> :2187). That id is in the host's numbering and is
           not in this side's groups[], so the loop matches nothing and the guest never records its
           own membership. Adopt it. Measured in BoB before the same fix: "AddPlayerToGroup player 4
           -> group 2 : NO SUCH GROUP on this side (ngroups=0)". MA_MP_NOADOPT=1 reverts. */
        if (!matched && !getenv("MA_MP_NOADOPT") && ngroups < 8) {
            groups[ngroups] = g; gmembers[ngroups] = 0;
            gplayers[ngroups][gmembers[ngroups]++] = p;
            ngroups++;
            matched = true;
            DPT("adopted group %u from the wire and joined player %u to it\n", (unsigned)g, (unsigned)p);
        }
        if (getenv("MA_TRACE_IAMIN")) {
            fprintf(stderr, "[group] AddPlayerToGroup player %u -> group %u : %s (ngroups=%d)\n",
                    (unsigned)p, (unsigned)g, matched ? "recorded" : "NO SUCH GROUP on this side", ngroups);
            fflush(stderr);
        }
        DPT("AddPlayerToGroup player %u -> group %u\n", (unsigned)p, (unsigned)g);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE DeletePlayerFromGroup(DPID g, DPID p) override {
        DPT("DeletePlayerFromGroup %u from %u\n", (unsigned)p, (unsigned)g); return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE SetGroupData(DPID, LPVOID, DWORD, DWORD) override { return DP_OK; }
    HRESULT STDMETHODCALLTYPE GetGroupData(DPID, LPVOID, LPDWORD sz, DWORD) override {
        if (sz) *sz = 0; return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE SetGroupName(DPID, LPDPNAME, DWORD) override { return DP_OK; }
    HRESULT STDMETHODCALLTYPE GetGroupName(DPID, LPVOID, LPDWORD sz) override {
        if (sz) *sz = 0; return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE EnumGroups(LPGUID, LPDPENUMPLAYERSCALLBACK2 cb, LPVOID ctx, DWORD) override {
        DPT("EnumGroups -> %d\n", ngroups);
        if (cb) for (int i = 0; i < ngroups; i++) {
            DPNAME nm; memset(&nm, 0, sizeof(nm)); nm.dwSize = sizeof(nm);
            if (!cb(groups[i], 0, &nm, 0, ctx)) break;
        }
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE EnumGroupPlayers(DPID g, LPGUID, LPDPENUMPLAYERSCALLBACK2 cb,
                                               LPVOID ctx, DWORD) override {
        for (int i = 0; i < ngroups; i++) if (groups[i] == g) {
            DPT("EnumGroupPlayers group %u -> %d\n", (unsigned)g, gmembers[i]);
            if (cb) for (int k = 0; k < gmembers[i]; k++) {
                DPNAME nm; memset(&nm, 0, sizeof(nm)); nm.dwSize = sizeof(nm);
                if (!cb(gplayers[i][k], 0, &nm, 0, ctx)) break;
            }
            break;
        }
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE CancelMessage(DWORD, DWORD) override { return DP_OK; }
    HRESULT STDMETHODCALLTYPE GetCaps(LPDPCAPS c, DWORD) override {
        if (c) { DWORD sz = c->dwSize ? c->dwSize : sizeof(DPCAPS); memset(c, 0, sz); c->dwSize = sz;
                 c->dwMaxBufferSize = 1024; c->dwMaxPlayers = 8; }
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE SetSessionDesc(LPDPSESSIONDESC2 d, DWORD) override {
        if (d && d->lpszSessionNameA) strncpy(sessName, d->lpszSessionNameA, sizeof(sessName)-1);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE GetSessionDesc(LPVOID data, LPDWORD size) override {
        DWORD need = sizeof(DPSESSIONDESC2);
        if (!size) return E_POINTER;
        if (!data || *size < need) { *size = need; return DPERR_BUFFERTOOSMALL; }
        DPSESSIONDESC2* d = (DPSESSIONDESC2*)data;
        memset(d, 0, need); d->dwSize = need;
        d->lpszSessionNameA = sessName; d->guidInstance = sessGuid;
        d->dwMaxPlayers = 8; d->dwCurrentPlayers = havePeer ? 2 : 1;
        *size = need;
        return DP_OK;
    }

    HRESULT STDMETHODCALLTYPE EnumPlayers(LPGUID a0, LPDPENUMPLAYERSCALLBACK2 a1, LPVOID a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("EnumPlayers");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerAddress(DPID a0, LPVOID a1, LPDWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("GetPlayerAddress");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerCaps(DPID a0, LPDPCAPS a1, DWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("GetPlayerCaps");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerData(DPID a0, LPVOID a1, LPDWORD a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("GetPlayerData");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerName(DPID a0, LPVOID a1, LPDWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("GetPlayerName");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Initialize(LPGUID a0) override {
        (void)a0;
        UNIMPL("Initialize");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetPlayerData(DPID a0, LPVOID a1, DWORD a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("SetPlayerData");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetPlayerName(DPID a0, LPDPNAME a1, DWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("SetPlayerName");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE AddGroupToGroup(DPID a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("AddGroupToGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CreateGroupInGroup(DPID a0, LPDPID a1, LPDPNAME a2, LPVOID a3, DWORD a4, DWORD a5) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        (void)a5;
        UNIMPL("CreateGroupInGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DeleteGroupFromGroup(DPID a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("DeleteGroupFromGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EnumGroupsInGroup(DPID a0, LPGUID a1, LPDPENUMPLAYERSCALLBACK2 a2, LPVOID a3, DWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("EnumGroupsInGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupConnectionSettings(DWORD a0, DPID a1, LPVOID a2, LPDWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("GetGroupConnectionSettings");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SecureOpen(LPCDPSESSIONDESC2 a0, DWORD a1, LPCDPSECURITYDESC a2, LPCDPCREDENTIALS a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("SecureOpen");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SendChatMessage(DPID a0, DPID a1, DWORD a2, LPDPCHAT a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("SendChatMessage");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetGroupConnectionSettings(DWORD a0, DPID a1, LPDPLCONNECTION a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("SetGroupConnectionSettings");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE StartSession(DWORD a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("StartSession");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupFlags(DPID a0, LPDWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetGroupFlags");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupParent(DPID a0, LPDPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetGroupParent");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerAccount(DPID a0, DWORD a1, LPVOID a2, LPDWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("GetPlayerAccount");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerFlags(DPID a0, LPDWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetPlayerFlags");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupOwner(DPID a0, LPDPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetGroupOwner");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetGroupOwner(DPID a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("SetGroupOwner");
        return DPERR_UNSUPPORTED;
    }
    /* R6.4: SendEx is Send with async/priority/timeout extras the game does not depend on here.
       The session goes live and the game sends every frame through it, so this is on the hot path:
       forward to the same datagram send rather than duplicating it. */
    HRESULT STDMETHODCALLTYPE SendEx(DPID from, DPID to, DWORD flags, LPVOID data, DWORD len,
                                     DWORD, DWORD, LPVOID, LPDWORD msgid) override {
        if (msgid) *msgid = 0;
        return Send(from, to, flags, data, len);
    }
    HRESULT STDMETHODCALLTYPE GetMessageQueue(DPID a0, DPID a1, DWORD a2, LPDWORD a3, LPDWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("GetMessageQueue");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CancelPriority(DWORD a0, DWORD a1, DWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("CancelPriority");
        return DPERR_UNSUPPORTED;
    }
};

extern "C" HRESULT ma_dplay_create(void** ppv)
{
    if (!ppv) return E_POINTER;
    BobDPlay4* p = new BobDPlay4();
    *ppv = (void*)static_cast<IDirectPlay4*>(p);
    DPT("CoCreateInstance(CLSID_DirectPlay) -> %p\n", (void*)p);
    return DP_OK;
}
