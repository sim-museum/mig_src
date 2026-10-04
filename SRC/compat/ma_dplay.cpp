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
#include <time.h>
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
#include <vector>
#include <map>
#include <deque>
#include "DPLAY.H"
#include <pthread.h>
/* MPJOIN-1 (2026-09-26): the shim had NO lock, and two threads use it at once in every flight --
   the aggregator thread (AGGRGTOR ReceiveMessage/SendEx, 50/s) and the game thread
   (ProcessInfoPackets, the send path). Both walk and rewrite the one message queue, so a message
   could be taken twice, lost, or handed to the wrong reader (measured: a joiner's broadcast copy
   tagged for the host's game half was drained under the aggregator's call). Real DirectPlay is
   thread-safe; so is this now: one recursive lock around every method that touches the queue or
   the socket. MA_MP_NOLOCK=1 reverts. */
struct MaDpGuard {
    static pthread_mutex_t* mu() {
        static pthread_mutex_t m; static int init = 0;
        if (!init) { pthread_mutexattr_t a; pthread_mutexattr_init(&a);
                     pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE); pthread_mutex_init(&m, &a); init = 1; }
        return &m;
    }
    bool on;
    MaDpGuard() { static int off = -1; if (off < 0) off = getenv("MA_MP_NOLOCK") ? 1 : 0; on = !off; if (on) pthread_mutex_lock(mu()); }
    ~MaDpGuard() { if (on) pthread_mutex_unlock(mu()); }
};

/* MP-6 S2: the drain site that is about to call Receive(); see ma_dplay.cpp notes. */
const char* ma_recv_caller = 0;

static int dp_trace(void) { static int t = -1; if (t < 0) t = getenv("MA_TRACE_DPLAY") ? 1 : 0; return t; }
#define DPT(...) do { if (dp_trace()) { fprintf(stderr, "[dplay] " __VA_ARGS__); } } while (0)
/* R6.4: log each unimplemented method ONCE. The first host run produced a 24.7-MILLION-line log
   because the game calls SendEx every frame once a session is live -- a per-call trace turns a
   useful "the next blocker names itself" signal into an unreadable flood, and fills the disk. */
#define UNIMPL(n) do { if (dp_trace()) { static int _once = 0; \
        if (!_once) { _once = 1; fprintf(stderr, "[dplay] %s: not implemented yet\n", (n)); } } } while (0)

/* EPIC-MATCHMAKER (sgw_link.cpp): the Serious Games Week matchmaker, when the player has configured one */
extern "C" int  sgw_configured(void);
extern "C" void sgw_announce_start(const char* game, int port, const char* title);
extern "C" void sgw_announce_stop(void);
extern "C" int  sgw_hosts(const char* game, struct sockaddr_in* out, int max);
#ifndef SGW_GAME_ID
#define SGW_GAME_ID "ma"
#endif
/* which address offered which session (from the LAN probe or a matchmaker-listed host), so a join reaches the
   session the player picked */
struct SgwOffer { char name[128]; struct sockaddr_in addr; };
static SgwOffer g_offers[32]; static int g_noffers = 0; static int g_lastoffer = -1;
static void sgw_remember_offer(const char* name, const struct sockaddr_in& a)
{
	for (int i = 0; i < g_noffers; i++) if (!strcmp(g_offers[i].name, name)) { g_offers[i].addr = a; g_lastoffer = i; return; }
	g_lastoffer = g_noffers;
	if (g_noffers < 32) { snprintf(g_offers[g_noffers].name, sizeof g_offers[0].name, "%s", name); g_offers[g_noffers].addr = a; g_noffers++; }
}
/* The game keeps a COPY of the DPSESSIONDESC2 its enum callback saw and passes it back to Open(JOIN); the name pointer
   in that copy points into a receive buffer long gone, but the GUID travels by value. So each offer's GUID carries its
   index ('SGW\0' + index), and Open(JOIN) finds the host by it. */
static const unsigned long SGW_GUID_TAG = 0x53475700ul;
static void sgw_tag_guid(DPSESSIONDESC2* sd)
{
	if (!sd->lpszSessionNameA) return;
	for (int i = 0; i < g_noffers; i++)
		if (!strcmp(g_offers[i].name, sd->lpszSessionNameA)) { sd->guidInstance.Data1 = SGW_GUID_TAG; sd->guidInstance.Data2 = (unsigned short)i; return; }
}
static const struct sockaddr_in* sgw_offer_for(const DPSESSIONDESC2* d)
{
	if (!d) return 0;
	if (d->guidInstance.Data1 == SGW_GUID_TAG && d->guidInstance.Data2 < g_noffers) return &g_offers[d->guidInstance.Data2].addr;
	/* an untagged desc (the game rebuilt it, or its search found nothing this time): the session last offered */
	return g_lastoffer >= 0 ? &g_offers[g_lastoffer].addr : 0;
}
static int dp_port(void) { const char* e = getenv("MA_DPLAY_PORT"); int p = e ? atoi(e) : 0; return p > 0 ? p : 47624; }
static const char* dp_host(void) { const char* e = getenv("MA_DPLAY_HOST"); return (e && *e) ? e : "127.0.0.1"; }

/* Wire framing. Deliberately tiny and self-describing: the whole point of the GPL-era design is
 * that a peer update is a few dozen bytes. */
enum { DPMAGIC = 0x424f4250 };            /* 'BOBP' */
enum { MSG_PROBE = 1, MSG_OFFER = 2, MSG_JOIN = 3, MSG_DATA = 4, MSG_ASSIGN = 5,
       MSG_RDATA = 6, MSG_ACK = 7 };   /* E2-6: reliable data ([WireHdr][u32 seq][payload]) and its acknowledgement (to = seq) */
struct WireHdr { unsigned int magic, kind, from, to; };

/* MPJOIN-1: 64 slots is under three seconds of a flying host's group stream (~25 packets/s) for
   a peer that is not reading -- a joiner in the locker room. The reply it then waits for was the
   packet dropped. 1024 slots, and a full queue now evicts the oldest GROUP-addressed packet
   (stream traffic, superseded every frame) before it refuses a new one. */
static const int MAXQ = 1024;
struct QMsg { unsigned int from, to, len; unsigned int only; long t; char data[2048]; };   /* MPJOIN-1: 1024 truncated the 1292-byte CS struct */   /* only: MPJOIN-1 */

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
    /* EPIC M / MP S5 (2026-09-14): this process can own MORE THAN ONE player. The MA host
       creates the AGGREGATOR as a player (measured: pid 1) and then its own game player
       (pid 3), and the two talk to each other inside the one process. A shim that remembers
       only the last-created id mis-handles both directions: traffic addressed to the
       aggregator is not recognised as a local player, so the receive filter's catch-all hands
       it to the game half instead, and a send from one local player to the other leaves on
       the wire and is never delivered at home. Remember them all. */
    DPID localPids[8]; int nlocal;
    DPID assignedPid;      /* R6.3: what the host gave us (client side); 0 until it answers */
    DPID groups[8]; int gmembers[8]; DPID gplayers[8][32]; int ngroups;   /* R6.4 (E2-3: 32 members -- 16 players need more than 8) */
    /* MP-6 S3 (2026-09-13, cross-port from bob fb4ea17): pids this HOST has handed to joining
       clients. The game only ever calls AddPlayerToGroup for its OWN player, so without this a
       group holds one member and the receive filter cannot tell a group id from a stranger's
       player id. */
    DPID joined[32]; int njoined;
    struct sockaddr_in peer; /* host: last client seen. client: the host. */
    int  havePeer;
    /* EPIC-MP-CAMPAIGN E2-3: the HOST is the hub of a star. Every client's address, by the player ids it uses, so
       a send reaches the right client (or all of them), and a client's packet for another client or for everyone
       is forwarded. Before this the host knew one address ("last client seen") and a third player broke it.
       MA_DPLAY_SINGLEPEER=1 reverts to the old single-peer behaviour. */
    enum { MAXCLIENTPIDS = 64 };
    struct ClientPid { DPID pid; struct sockaddr_in addr; } cpid[MAXCLIENTPIDS]; int ncpid;
    void learnClient(DPID pid, const struct sockaddr_in& a) {
        if (!pid || getenv("MA_DPLAY_SINGLEPEER")) return;
        for (int i = 0; i < ncpid; i++) if (cpid[i].pid == pid) { cpid[i].addr = a; return; }
        if (ncpid < MAXCLIENTPIDS) { cpid[ncpid].pid = pid; cpid[ncpid].addr = a; ncpid++;
            DPT("star: client pid %u at %s:%d (%d known)\n", (unsigned)pid, inet_ntoa(a.sin_addr), (int)ntohs(a.sin_port), ncpid); }
    }
    const struct sockaddr_in* clientAddr(DPID pid) const {
        for (int i = 0; i < ncpid; i++) if (cpid[i].pid == pid) return &cpid[i].addr;
        return 0;
    }
    static bool sameAddr(const struct sockaddr_in& a, const struct sockaddr_in& b) {
        return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
    }
    /* send one wire packet to every distinct client address except `skip` (may be null) */
    int fanOut(const void* pkt, size_t n, const struct sockaddr_in* skip, bool reliable = false) {
        int sent = 0;
        for (int i = 0; i < ncpid; i++) {
            bool dup = false;
            for (int j = 0; j < i; j++) if (sameAddr(cpid[j].addr, cpid[i].addr)) { dup = true; break; }
            if (dup || (skip && sameAddr(cpid[i].addr, *skip))) continue;
            if (xmit((const char*)pkt, n, cpid[i].addr, reliable) > 0) sent++;   /* E2-6 */
        }
        return sent;
    }
    int  nclients() const {
        int n = 0;
        for (int i = 0; i < ncpid; i++) { bool dup = false; for (int j = 0; j < i; j++) if (sameAddr(cpid[j].addr, cpid[i].addr)) dup = true; if (!dup) n++; }
        return n;
    }
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

    void qpush(unsigned f, unsigned t, const char* d, unsigned n, unsigned only = 0) {
        int nx = (qt + 1) % MAXQ;
        if (nx == qh) {
            int ev = -1;
            for (int i = qh; i != qt; i = (i + 1) % MAXQ)
                if (q[i].to != 0 && !isLocalPlayer(q[i].to) && q[i].to != (unsigned)myPid) { ev = i; break; }
            if (ev < 0) { DPT("queue full, dropping a packet\n"); return; }
            const unsigned evto = q[ev].to;
            removeAt(ev);
            static long nev = 0; if ((nev++ % 500) == 0) DPT("queue full: evicted oldest group packet (to=%u) x%ld\n", evto, nev);
            nx = (qt + 1) % MAXQ;
        }
        q[qt].from = f; q[qt].to = t; q[qt].only = only; q[qt].t = nowMs(); q[qt].len = n > sizeof(q[qt].data) ? sizeof(q[qt].data) : n;
        memcpy(q[qt].data, d, q[qt].len); qt = nx;
        noteAnnounce("QUEUED", f, t, d, n);
    }
    int qcount() const { return (qt - qh + MAXQ) % MAXQ; }
    /* MPJOIN-1 (2026-09-26): a send to DPID_ALLPLAYERS (0) is delivered by DirectPlay to EVERY player
       in the session except the sender -- one copy each, including every LOCAL player. The host has
       two (its aggregator, pid 1, and its game half, pid 3), and the shim queued ONE copy, which the
       first caller to poll took. In the 3-D that is the aggregator thread, so a joiner's PID_PASSWORD
       (AttemptToJoin: to=DPID_ALLPLAYERS) was eaten by AGGRGTOR and never reached CheckPassword;
       the joiner timed out and showed "Incorrect password" -- joining a game in progress was
       impossible. Fan a broadcast out to one tagged copy per local player; Receive hands a tagged
       copy only to the player it is for. MA_MP_NOBCASTFAN=1 reverts. */
    void qpushFan(unsigned f, unsigned t, const char* d, unsigned n) {
        static int off = -1;
        if (off < 0) off = getenv("MA_MP_NOBCASTFAN") ? 1 : 0;
        if (t != 0 || nlocal < 2 || off) { qpush(f, t, d, n); return; }
        int k = 0;
        for (int i = 0; i < nlocal; i++)
            if ((unsigned)localPids[i] != f) { qpush(f, t, d, n, (unsigned)localPids[i]); k++; }
        if (getenv("MA_TRACE_DPLAY")) DPT("broadcast from pid %u fanned out to %d local players\n", f, k);
    }
    /* A tagged copy whose player is not reading (the host's aggregator polls only while a flight
       runs) would sit in the queue for ever; drop tagged copies older than 10 s. The other local
       player's copy is separate, so nothing that anyone reads is lost. */
    void purgeStale() {
        const long now = nowMs();
        int w = qh;
        for (int i = qh; i != qt; i = (i + 1) % MAXQ) {
            if (q[i].only && now - q[i].t > 10000) {      /* drop */
                if (getenv("MA_TRACE_DPLAY")) { unsigned id = 0; memcpy(&id, q[i].data, 4);
                    DPT("broadcast copy for pid %u UNREAD for 10 s, dropped (id=0x%x len=%u)\n", q[i].only, id, q[i].len); }
                continue;
            }
            if (w != i) q[w] = q[i];
            w = (w + 1) % MAXQ;
        }
        qt = w;
    }
    /* remove q[idx], preserving the order of everything still queued */
    void removeAt(int idx) {
        for (int i = idx; i != qh; i = (i - 1 + MAXQ) % MAXQ)
            q[i] = q[(i - 1 + MAXQ) % MAXQ];
        qh = (qh + 1) % MAXQ;
    }
    static long nowMs() { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L; }
    /* a tagged broadcast copy belongs to one local player; an untagged message to whoever the
       routing below allows */
    bool eligible(int i, unsigned toIn) const { return q[i].only == 0 || toIn == 0 || q[i].only == toIn; }

    /* Drain the socket: answer discovery probes, absorb joins, queue data. Called from every path
     * the game pumps (Receive / GetMessageCount / EnumSessions) so a host answers probes while it
     * is simply sitting in its own message loop. */
    /* PO-76 (S418): COUNT the pumps. A host answers discovery ONLY from here, and here runs only
       when the game calls Receive / GetMessageCount / EnumSessions. So "nobody can find my session"
       and "the game is not pumping" are the same symptom from outside, and a client that finds
       nothing cannot tell them apart. Report periodically -- never at exit, since MA_SHOT-style
       runs leave via _exit() (S328b/S330b/S416). */
    long pumps = 0;
    /* ===================== E2-6: RELIABLE DELIVERY (DPSEND_GUARANTEED) =====================
       DirectPlay's guaranteed sends were delivered, once, in order. This shim sent everything as bare UDP
       datagrams, so on the internet a lost packet in a savegame or battlefield burst was lost for good and the
       receiving player hung in a "Timed out" wait. Guaranteed sends now travel as MSG_RDATA with a per-link
       sequence number; the receiver ACKs each one, drops duplicates and delivers in order; the sender
       retransmits with backoff until acked or a deadline. Per LINK (remote address), so the star hub's
       forwarding is reliable on each leg. Unguaranteed traffic (the in-flight aggregator stream) stays plain:
       a late position is worse than a lost one. MA_NO_RELIABLE=1 reverts; MA_NET_LOSS=<percent> drops that
       share of every outgoing datagram (a loss simulator for testing). */
    struct RelOut  { unsigned seq; unsigned long first, last; int tries; std::vector<char> pkt; };
    struct RelLink { struct sockaddr_in a; unsigned nextSeq, expect; std::deque<RelOut> out;
                     std::map<unsigned, std::vector<char> > held; };
    std::vector<RelLink> rlinks;
    unsigned long relStats[4] = {0,0,0,0};   /* sent, retransmitted, delivered, given up */
    static unsigned long relNow() { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
                                    return ts.tv_sec * 1000ul + ts.tv_nsec / 1000000; }
    static bool relOff() { static int o = -1; if (o < 0) o = getenv("MA_NO_RELIABLE") ? 1 : 0; return o != 0; }
    RelLink& rlink(const struct sockaddr_in& a) {
        for (size_t i = 0; i < rlinks.size(); i++) if (sameAddr(rlinks[i].a, a)) return rlinks[i];
        RelLink L; L.a = a; L.nextSeq = 1; L.expect = 1; rlinks.push_back(L); return rlinks.back();
    }
    ssize_t rawsend(const void* p, size_t n, const struct sockaddr_in& a) {
        static int loss = -1; if (loss < 0) { const char* e = getenv("MA_NET_LOSS"); loss = e ? atoi(e) : 0; }
        if (loss > 0 && (rand() % 100) < loss) return (ssize_t)n;     /* simulated loss: "sent", never arrives */
        return sendto(fd, p, n, 0, (const struct sockaddr*)&a, sizeof(a));
    }
    /* send one data packet ([WireHdr MSG_DATA][payload]) to a, reliably if asked */
    ssize_t xmit(const char* pkt, size_t n, const struct sockaddr_in& a, bool reliable) {
        if (!reliable || relOff() || n < sizeof(WireHdr)) return rawsend(pkt, n, a);
        RelLink& L = rlink(a);
        RelOut o; o.seq = L.nextSeq++; o.first = o.last = relNow(); o.tries = 0;
        o.pkt.resize(n + 4);
        memcpy(&o.pkt[0], pkt, sizeof(WireHdr));
        ((WireHdr*)&o.pkt[0])->kind = MSG_RDATA;
        memcpy(&o.pkt[sizeof(WireHdr)], &o.seq, 4);
        memcpy(&o.pkt[sizeof(WireHdr) + 4], pkt + sizeof(WireHdr), n - sizeof(WireHdr));
        L.out.push_back(o); relStats[0]++;
        rawsend(&L.out.back().pkt[0], L.out.back().pkt.size(), a);
        return (ssize_t)n;
    }
    void relAck(const struct sockaddr_in& a, unsigned seq) {
        for (size_t i = 0; i < rlinks.size(); i++) if (sameAddr(rlinks[i].a, a)) {
            std::deque<RelOut>& q = rlinks[i].out;
            for (size_t k = 0; k < q.size(); k++) if (q[k].seq == seq) { q.erase(q.begin() + k); break; }
            return;
        }
    }
    void relService() {
        static unsigned long lastSvc = 0, lastLog = 0; unsigned long now = relNow();
        if (now - lastSvc < 20) return; lastSvc = now;
        static int giveupMs = -1; if (giveupMs < 0) { const char* e = getenv("MA_REL_GIVEUP_MS"); giveupMs = e ? atoi(e) : 20000; }
        for (size_t i = 0; i < rlinks.size(); i++) {
            std::deque<RelOut>& q = rlinks[i].out;
            for (size_t k = 0; k < q.size(); ) {
                RelOut& o = q[k];
                unsigned long rto = 150ul << (o.tries < 3 ? o.tries : 3);          /* 150, 300, 600, 1200 ms */
                if (now - o.first > (unsigned long)giveupMs) {
                    relStats[3]++;
                    DPT("[rel] GAVE UP seq %u to %s:%d after %d tries\n", o.seq, inet_ntoa(rlinks[i].a.sin_addr), (int)ntohs(rlinks[i].a.sin_port), o.tries);
                    q.erase(q.begin() + k); continue;
                }
                if (now - o.last >= rto) { rawsend(&o.pkt[0], o.pkt.size(), rlinks[i].a); o.last = now; o.tries++; relStats[1]++; }
                k++;
            }
        }
        if (now - lastLog > 10000 && (relStats[0] || relStats[1])) { lastLog = now;
            size_t pend = 0; for (size_t i = 0; i < rlinks.size(); i++) pend += rlinks[i].out.size();
            DPT("[rel] sent=%lu resent=%lu delivered=%lu gaveup=%lu pending=%zu links=%zu\n",
                relStats[0], relStats[1], relStats[2], relStats[3], pend, rlinks.size()); }
    }
    void relRecv(const char* buf, ssize_t n, const struct sockaddr_in& from) {
        if (n < (ssize_t)(sizeof(WireHdr) + 4)) return;
        unsigned seq; memcpy(&seq, buf + sizeof(WireHdr), 4);
        WireHdr ack; ack.magic = DPMAGIC; ack.kind = MSG_ACK; ack.from = 0; ack.to = seq;
        rawsend(&ack, sizeof(ack), from);
        RelLink& L = rlink(from);
        if (seq < L.expect || L.held.count(seq)) return;                    /* duplicate (our ACK was lost) */
        std::vector<char> d((size_t)n - 4);
        memcpy(&d[0], buf, sizeof(WireHdr)); ((WireHdr*)&d[0])->kind = MSG_DATA;
        memcpy(&d[sizeof(WireHdr)], buf + sizeof(WireHdr) + 4, (size_t)n - sizeof(WireHdr) - 4);
        if (seq != L.expect) { if (L.held.size() < 8192) L.held[seq] = d; return; }   /* early: hold for order */
        std::vector<std::vector<char> > ready; ready.push_back(d); L.expect++;
        for (;;) { std::map<unsigned, std::vector<char> >::iterator it = L.held.find(L.expect);
                   if (it == L.held.end()) break; ready.push_back(it->second); L.held.erase(it); L.expect++; }
        for (size_t r = 0; r < ready.size(); r++) { relStats[2]++; handleData(&ready[r][0], (ssize_t)ready[r].size(), from, true); }
    }
    /* a data packet that has arrived (plain, or reliable and now in order): the hub forwards it, then it is queued */
    void handleData(char* buf, ssize_t n, const struct sockaddr_in& from, bool reliable) {
        WireHdr* h = (WireHdr*)buf;
                if (isHost && !getenv("MA_DPLAY_SINGLEPEER")) {
                    /* E2-3: a client's packet for another client, or for everyone / a group, crosses the hub */
                    learnClient((DPID)h->from, from);
                    const struct sockaddr_in* dst = clientAddr((DPID)h->to);
                    if (dst) {
                        if (!sameAddr(*dst, from)) xmit(buf, (size_t)n, *dst, reliable);
                    } else if (!isLocalPlayer((unsigned)h->to)) {
                        int f = fanOut(buf, (size_t)n, &from, reliable);
                        if (f) DPT("star: forwarded %d bytes pid %u -> %u to %d other client(s)\n", (int)(n - sizeof(WireHdr)), h->from, h->to, f);
                    }
                    if (dst && !isLocalPlayer((unsigned)h->to)) return;	/* for another client only: not ours */
                }
                qpushFan(h->from, h->to, buf + sizeof(WireHdr), (unsigned)(n - sizeof(WireHdr)));
                DPT("received %d data bytes from pid %u\n", (int)(n - sizeof(WireHdr)), h->from);
    }
    void pump() { MaDpGuard _g;
        relService();   /* E2-6 */
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
                /* E2-6: a repeat JOIN from a known address gets the SAME pid back */
                { bool known = false;
                  for (int ci = 0; ci < ncpid; ci++) if (sameAddr(cpid[ci].addr, from)) {
                      WireHdr ah; ah.magic = DPMAGIC; ah.kind = MSG_ASSIGN; ah.from = (unsigned)DPID_SERVERPLAYER; ah.to = (unsigned)cpid[ci].pid;
                      rawsend(&ah, sizeof(ah), from); known = true;
                      DPT("repeat JOIN from %s:%d -> re-sent pid %u\n", inet_ntoa(from.sin_addr), (int)ntohs(from.sin_port), (unsigned)cpid[ci].pid);
                      break; }
                  if (known) continue; }
                DPID given = nextPid++;
                if (njoined < 32) joined[njoined++] = given;   /* MP-6 S3 (E2-3: 32) */
                learnClient(given, from);                      /* E2-3: the star's address book */
                WireHdr ah; ah.magic = DPMAGIC; ah.kind = MSG_ASSIGN;
                ah.from = (unsigned)DPID_SERVERPLAYER; ah.to = (unsigned)given;
                sendto(fd, &ah, sizeof(ah), 0, (struct sockaddr*)&from, fl);
                DPT("client joined from %s:%d -> assigned pid %u\n",
                    inet_ntoa(from.sin_addr), (int)ntohs(from.sin_port), (unsigned)given);
            } else if (h->kind == MSG_ASSIGN && !isHost) {
                assignedPid = (DPID)h->to;
                DPT("host assigned us pid %u\n", (unsigned)assignedPid);
            } else if (h->kind == MSG_DATA) {
                handleData(buf, n, from, false);
            } else if (h->kind == MSG_RDATA) {
                relRecv(buf, n, from);
            } else if (h->kind == MSG_ACK) {
                relAck(from, h->to);
            }
        }
    }
    int mksock(int bindIt) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) return 0;
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
        /* E2-5 (measured, BoB five players): the host sends a ~250 KB savegame to every guest at once (~500 packets
           each); with the 208 KB default receive buffer a slower guest's kernel dropped part of it, the load FAILED and
           that guest never launched. 4 MB is this system's rmem_max/wmem_max. MA_UDP_BUF=<bytes> overrides. */
        { int b = 4 * 1024 * 1024; const char* e = getenv("MA_UDP_BUF"); if (e && atoi(e) > 0) b = atoi(e);
          setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &b, sizeof(b)); setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &b, sizeof(b)); }
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
    BobDPlay4() : ref(1), fd(-1), isHost(0), nextPid(DPID_SERVERPLAYER), myPid(0), nlocal(0), assignedPid(0), njoined(0),
                  havePeer(0), ncpid(0), ngroups(0), qh(0), qt(0) {
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

    HRESULT STDMETHODCALLTYPE Open(LPDPSESSIONDESC2 d, DWORD flags) override { MaDpGuard _g;
        if (flags & DPOPEN_CREATE) {
            isHost = 1;
            if (d && d->lpszSessionNameA) { strncpy(sessName, d->lpszSessionNameA, sizeof(sessName)-1); }
            else strncpy(sessName, "Battle of Britain", sizeof(sessName)-1);
            if (d) sessGuid = d->guidInstance;
            if (!mksock(1)) return DPERR_CANTCREATEPLAYER;
            DPT("Open(CREATE) session \"%s\"\n", sessName);
            sgw_announce_start(SGW_GAME_ID, dp_port(), sessName);	/* EPIC-MATCHMAKER: list it while it runs */
            return DP_OK;
        }
        if (flags & DPOPEN_JOIN) {
            isHost = 0;
            if (!mksock(0)) return DPERR_NOCONNECTION;
            memset(&peer, 0, sizeof(peer));
            peer.sin_family = AF_INET; peer.sin_port = htons(dp_port());
            peer.sin_addr.s_addr = inet_addr(dp_host());
            if (const struct sockaddr_in* oa = sgw_offer_for(d)) peer = *oa;	/* EPIC-MATCHMAKER: the host that offered it */
            DPT("Open(JOIN): session desc guid %08lx/%u, %d offer(s) known\n", d ? (unsigned long)d->guidInstance.Data1 : 0ul,
                d ? (unsigned)d->guidInstance.Data2 : 0u, g_noffers);
            havePeer = 1;
            WireHdr h; h.magic = DPMAGIC; h.kind = MSG_JOIN; h.from = 0; h.to = 0;
            sendto(fd, &h, sizeof(h), 0, (struct sockaddr*)&peer, sizeof(peer));
            DPT("Open(JOIN) -> host %s:%d\n", inet_ntoa(peer.sin_addr), (int)ntohs(peer.sin_port));	/* the address actually joined */
            for (int i = 0; i < 40 && assignedPid == 0; i++) { pump(); usleep(25000);  /* R6.3 */
                if (i % 8 == 7 && assignedPid == 0) rawsend(&h, sizeof(h), peer);   /* E2-6: a lost JOIN or ASSIGN must not end the join */
            }
            if (assignedPid == 0) DPT("host did not assign a pid (joining anyway)\n");
            return DP_OK;
        }
        UNIMPL("Open(other flags)");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Close() override { MaDpGuard _g;
        DPT("Close\n");
        if (isHost) sgw_announce_stop();	/* EPIC-MATCHMAKER */
        if (fd >= 0) { close(fd); fd = -1; }
        isHost = 0; havePeer = 0; ncpid = 0; qh = qt = 0; rlinks.clear();   /* E2-6 */
        return DP_OK;
    }

    /* Probe for a host and report what answers. With no host running, no callback fires and this
     * returns DP_OK with an empty list -- which is the honest answer, not an error. */
    HRESULT STDMETHODCALLTYPE EnumSessions(LPDPSESSIONDESC2 d, DWORD timeout,
                                           LPDPENUMSESSIONSCALLBACK2 cb, LPVOID ctx, DWORD flags) override {
        (void)d;
        if (fd < 0 && !mksock(0)) return DPERR_NOCONNECTION;
        struct sockaddr_in to; memset(&to, 0, sizeof(to));
        to.sin_family = AF_INET; to.sin_port = htons(dp_port()); to.sin_addr.s_addr = inet_addr(dp_host());
        WireHdr h; h.magic = DPMAGIC; h.kind = MSG_PROBE; h.from = 0; h.to = 0;
        sendto(fd, &h, sizeof(h), 0, (struct sockaddr*)&to, sizeof(to));
        DPT("EnumSessions: probing %s:%d\n", dp_host(), dp_port());
        {	/* EPIC-MATCHMAKER: also ask every host the matchmaker lists for this game */
            struct sockaddr_in mh[16]; int nm = sgw_hosts(SGW_GAME_ID, mh, 16);
            for (int i = 0; i < nm; i++) sendto(fd, &h, sizeof(h), 0, (struct sockaddr*)&mh[i], sizeof(mh[i]));
            if (nm) DPT("EnumSessions: probed %d matchmaker host(s)\n", nm);
        }

        int found = 0;
        unsigned waitms = timeout ? (timeout > 2000 ? 2000 : timeout) : 400;
        /* FUNC-SWEEP-MA (cross-port of BoB 5ed3d06): the Select-Session timer (SetTimer 0 ms) calls this with
           DPENUMSESSIONS_ASYNC on every pass; a fixed 400 ms wait throttled the front end to ~3 Hz on that
           screen. ASYNC: short wait + a 2 s cache of seen sessions (a host stays listed between replies, as
           in real async DirectPlay). MA_DPLAY_ENUM_WAIT_MS=<ms> overrides; =400 restores the old timing. */
        const bool async = (flags & DPENUMSESSIONS_ASYNC) != 0;
        struct SessSeen { char name[128]; unsigned long long ms; };
        static SessSeen seen[8]; static int nseen = 0;
        auto nowms = []() { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
                            return (unsigned long long)ts.tv_sec * 1000ull + ts.tv_nsec / 1000000; };
        auto asyncRemember = [&](const char* nm) {
            for (int k = 0; k < nseen; k++) if (!strcmp(seen[k].name, nm)) { seen[k].ms = nowms(); return; }
            if (nseen < 8) { snprintf(seen[nseen].name, sizeof(seen[nseen].name), "%s", nm); seen[nseen].ms = nowms(); nseen++; }
        };
        if (async) {
            static int envw = -2;
            if (envw == -2) { const char* e = getenv("MA_DPLAY_ENUM_WAIT_MS"); envw = e ? atoi(e) : -1; }
            waitms = envw >= 0 ? (unsigned)envw : 30;
        }
        for (unsigned t = 0; t < waitms; t += 20) {
            char buf[2048]; struct sockaddr_in from; socklen_t fl = sizeof(from);
            ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fl);
            if (n >= (ssize_t)sizeof(WireHdr)) {
                WireHdr* rh = (WireHdr*)buf;
                if (rh->magic == DPMAGIC && (rh->kind == MSG_RDATA || rh->kind == MSG_ACK)) {
                    /* E2-6: this loop reads the shared socket too -- handle reliable packets exactly as pump() does */
                    MaDpGuard _g2;
                    if (rh->kind == MSG_RDATA) relRecv(buf, n, from); else relAck(from, rh->to);
                }
                if (rh->magic == DPMAGIC && rh->kind == MSG_OFFER) {
                    sgw_remember_offer(buf + sizeof(WireHdr), from);	/* EPIC-MATCHMAKER */
                    DPSESSIONDESC2 sd; memset(&sd, 0, sizeof(sd));
                    sd.dwSize = sizeof(sd);
                    sd.lpszSessionNameA = buf + sizeof(WireHdr); sgw_tag_guid(&sd);
                    sd.dwMaxPlayers = 16; sd.dwCurrentPlayers = 1;
                    sd.dwFlags = 0;
                    DWORD tmo = waitms;
                    found++;
                    DPT("EnumSessions: found \"%s\"\n", sd.lpszSessionNameA);
                    if (async) { asyncRemember(sd.lpszSessionNameA); continue; }
                    if (cb && !cb(&sd, &tmo, 0, ctx)) break;
                }
            }
            usleep(20000);
        }
        if (async) {
            int listed = 0; unsigned long long t = nowms();
            for (int k = 0; k < nseen; k++) {
                if (t - seen[k].ms > 2000) continue;
                DPSESSIONDESC2 sd; memset(&sd, 0, sizeof(sd)); sd.dwSize = sizeof(sd);
                sd.lpszSessionNameA = seen[k].name; sgw_tag_guid(&sd); sd.dwMaxPlayers = 16; sd.dwCurrentPlayers = 1;
                DWORD tmo = waitms; listed++;
                if (cb && !cb(&sd, &tmo, 0, ctx)) break;
            }
            DPT("EnumSessions -> %d session(s) (async: %d new reply(ies), %d listed from cache)\n", listed, found, listed);
            return DP_OK;
        }
        DPT("EnumSessions -> %d session(s)\n", found);
        return DP_OK;
    }

    HRESULT STDMETHODCALLTYPE CreatePlayer(LPDPID pid, LPDPNAME nm, HANDLE, LPVOID, DWORD, DWORD) override { MaDpGuard _g;
        /* R6.3: a client uses the id the HOST gave it; only the host mints ids. */
        myPid = (!isHost && assignedPid != 0) ? assignedPid : nextPid++;
        if (nlocal < 8) localPids[nlocal++] = myPid;   /* MP S5: every local player, not just the last */
        if (pid) *pid = myPid;
        DPT("CreatePlayer \"%s\" -> pid %u\n",
            (nm && nm->lpszShortNameA) ? nm->lpszShortNameA : "(unnamed)", (unsigned)myPid);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE DestroyPlayer(DPID id) override { DPT("DestroyPlayer %u\n", (unsigned)id); return DP_OK; }

    HRESULT STDMETHODCALLTYPE Send(DPID from, DPID to, DWORD sendflags, LPVOID data, DWORD len) override { MaDpGuard _g;
        const bool rel = (sendflags & DPSEND_GUARANTEED) != 0;   /* E2-6: the game asks for guaranteed delivery */
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
        /* EPIC M / MP S5 (2026-09-14): LOCAL LOOPBACK for a group send. Measured the same day:
           the host's aggregator sends the aggregate packet from pid 1 to group 2 twelve times a
           second and the shim puts it on the wire and nothing else -- so the HOST'S OWN player
           (pid 3, a member of group 2) never receives it, and InitSyncPhase, which waits for
           exactly that packet, never succeeds on the host. The host therefore never sends its
           dummy back, the aggregate is built with players=0, and the CLIENT (which does receive
           the packet, over the wire) fails the `num != CurrPlayers` gate that follows. One missing
           delivery stalls both sides.

           MA was written against a DirectPlay that expands a group send to ALL its members,
           including local ones -- the game's own structure is the evidence: the host's game half
           has no other route to a packet its own aggregator produces. Deliver a copy into the
           local queue when the destination is a group this side knows and our player belongs to,
           or when it is addressed to our own player id. The receive filter then routes it by
           membership exactly as it routes the wire copy, so the aggregator (to=aggID=1, not a
           member of group 2) still cannot steal it. MA_NO_LOOPBACK=1 reverts. */
        if (!getenv("MA_NO_LOOPBACK") && !isLocalPlayerOnly((unsigned)to, (unsigned)from) &&
            (isLocalPlayer((unsigned)to) || isGroupWithLocalMember((unsigned)to, (unsigned)from)))
        {
            qpush((unsigned)from, (unsigned)to, (const char*)data, (unsigned)len);
            if (getenv("MA_TRACE_AGG")) {
                static long n = 0; static time_t last = 0; time_t now = time(0); n++;
                if (now != last) { last = now;
                    fprintf(stderr, "[agg] loopback %ld/s  from=%u to=%u (local player or group)\n",
                            n, (unsigned)from, (unsigned)to);
                    fflush(stderr); n = 0; }
            }
        }
        /* MPJOIN-1 (cross-port of BoB MP S9, bob_dplay.cpp): a send to a LOCAL player is delivered
           above and must not ALSO cross the wire. The host's game half sends its move packet to its
           own aggregator (pid 3 -> pid 1) twelve times a second; on the wire that reached every
           joiner, where no player 1 exists and the receive filter's catch-all handed it to the
           game. A joiner sitting in the locker room while the host flies was flooded until its
           queue overflowed and dropped the host's PID_PASSWORDVALID -- "Incorrect password".
           MA_MP_WIRE_LOCAL=1 reverts. */
        if (isLocalPlayer((unsigned)to) && !getenv("MA_MP_WIRE_LOCAL")) return DP_OK;
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
        if (isHost && ncpid && !getenv("MA_DPLAY_SINGLEPEER")) {	/* E2-3: the hub sends to one client or to all */
            const struct sockaddr_in* dst = clientAddr(to);
            ssize_t s1 = 0; int f = 0;
            if (dst) s1 = xmit(out, sizeof(WireHdr) + len, *dst, rel);
            else f = fanOut(out, sizeof(WireHdr) + len, 0, rel);
            DPT("Send %u bytes pid %u -> %u (star: %s)\n", (unsigned)len, (unsigned)from, (unsigned)to,
                dst ? (s1 > 0 ? "one client" : strerror(errno)) : (f ? "all clients" : "nobody"));
            return (dst ? s1 > 0 : f > 0) ? DP_OK : DPERR_GENERIC;
        }
        ssize_t s = xmit(out, sizeof(WireHdr) + len, peer, rel);   /* E2-6 */
        DPT("Send %u bytes pid %u -> %u (%s)\n", (unsigned)len, (unsigned)from, (unsigned)to,
            s > 0 ? "ok" : strerror(errno));
        return s > 0 ? DP_OK : DPERR_GENERIC;
    }
    /* MP-6 S3: every player id this side has seen -- our own, and any joiner the host handed a pid. */
    bool isLocalPlayer(unsigned pid) const {
        for (int i = 0; i < nlocal; i++) if ((unsigned)localPids[i] == pid) return true;
        return false;
    }
    /* true when `to` is a local player and it IS the sender -- nothing to loop back. */
    bool isLocalPlayerOnly(unsigned to, unsigned from) const { return to == from; }
    /* EPIC M / MP S6 (2026-09-14): a group send must reach local members OTHER than the sender.
       S5's version asked only "does this group have a local member", which is true of the sender
       itself, so a player's own broadcast came back to it. Measured consequence: the game announces
       its entry with SendMessageToPlayers(playergroupID), each side received its OWN PID_IAMIN and
       called AddPlayerToGame for its own slot, and the bare CurrPlayers++ there counted it a second
       time. MA_LOOPBACK_SELF=1 restores S5's behaviour as the negative control. */
    bool isGroupWithLocalMember(unsigned gid, unsigned exceptPid) const {
        const bool self = getenv("MA_LOOPBACK_SELF") != 0;
        for (int gi = 0; gi < ngroups; gi++) {
            if ((unsigned)groups[gi] != gid) continue;
            for (int k = 0; k < gmembers[gi]; k++) {
                const unsigned m = (unsigned)gplayers[gi][k];
                if (!isLocalPlayer(m)) continue;
                if (!self && m == exceptPid) continue;
                return true;
            }
        }
        return false;
    }
    bool isKnownPlayer(unsigned pid) const {
        if (isLocalPlayer(pid)) return true;
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
    HRESULT STDMETHODCALLTYPE Receive(LPDPID from, LPDPID to, DWORD rflags, LPVOID data, LPDWORD size) override { MaDpGuard _g;
        const unsigned toIn   = to   ? (unsigned)*to   : 0u;   /* BEFORE the shim overwrites *to */
        const unsigned fromIn = from ? (unsigned)*from : 0u;
        pump();
        purgeStale();
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
        /* MPJOIN-1: the head may be another local player's copy of a broadcast */
        while (idx != qt && !eligible(idx, toIn)) idx = (idx + 1) % MAXQ;
        if (idx == qt) return DPERR_NOMESSAGES;
        if (!nofilter && (rflags & DPRECEIVE_FROMPLAYER) && from
            && !getenv("MA_MP_NOFROMFILTER")) {
            int found = -1;
            for (int i = qh; i != qt; i = (i + 1) % MAXQ)
                if (q[i].from == fromIn && eligible(i, toIn)) { found = i; break; }
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
                const bool ok = (dst == toIn) || (dst == 0 && eligible(i, toIn)) ||
                                ((strict && isKnownGroup(dst)) ? inGroup(dst, toIn)
                                                               : (inGroup(dst, toIn) || !isKnownPlayer(dst)));
                if (ok && eligible(i, toIn)) { found = i; break; }   /* MPJOIN-1: the catch-all below also matches dst 0 */
            }
            if (found < 0) return DPERR_NOMESSAGES;
            idx = found;
        }
        QMsg& m = q[idx];
        /* EPIC M / MP S9 (2026-09-19): WHO drains the queue, per second, by caller tag and by (from,to,len).
           S9's sender/aggregator traces showed the client's InitSyncPhase "got" only 6 aggregate packets a
           second while the wire carried 46 -- so 40/s are being taken by some OTHER caller's filter and
           discarded there. Print the census under MA_TRACE_AGG so the thief is named, not guessed. */
        {
            static int a_on = -1;
            if (a_on < 0) a_on = getenv("MA_TRACE_AGG") ? 1 : 0;
            if (a_on) {
                struct Row { const char* tag; unsigned from, to, len; long n; };
                static Row rows[32]; static int nrows = 0; static time_t last = 0;
                const char* tag = ma_recv_caller ? ma_recv_caller : "(untagged)";
                int r = -1;
                for (int i = 0; i < nrows; i++)
                    if (rows[i].from == m.from && rows[i].to == m.to && rows[i].len == m.len && rows[i].tag == tag) { r = i; break; }
                if (r < 0 && nrows < 32) { r = nrows++; rows[r].tag = tag; rows[r].from = m.from; rows[r].to = m.to; rows[r].len = m.len; rows[r].n = 0; }
                if (r >= 0) rows[r].n++;
                time_t now = 0; ::time(&now);
                if (now != last) { last = now;
                    fprintf(stderr, "[agg] drained/s:");
                    for (int i = 0; i < nrows; i++) if (rows[i].n) { fprintf(stderr, "  %s from=%u to=%u len=%u x%ld", rows[i].tag, rows[i].from, rows[i].to, rows[i].len, rows[i].n); rows[i].n = 0; }
                    fprintf(stderr, "\n"); fflush(stderr); }
            }
        }
        if (size && *size < m.len) { *size = m.len; return DPERR_BUFFERTOOSMALL; }
        if (m.only && getenv("MA_TRACE_DPLAY")) { unsigned id = 0; memcpy(&id, m.data, 4);
            DPT("broadcast copy for pid %u delivered to reader to=%u flags=0x%lx (id=0x%x len=%u)\n", m.only, toIn, (unsigned long)rflags, id, m.len); }
        if (from) *from = (DPID)m.from;
        /* MPJOIN-1: lpidTo is IN/OUT, and DirectPlay writes back the LOCAL PLAYER the message was
           delivered to -- never a group id or 0. Writing m.to back made the game's receive loops
           (`to=myDPlayID; while (ReceiveNextMessage(...,to,...))`) filter their NEXT call on the
           previous message's group/broadcast id, so a copy fanned out to this player could not be
           asked for by its own id. MA_MP_OLDTOOUT=1 restores writing m.to. */
        if (to) {
            if (getenv("MA_MP_OLDTOOUT"))                                   *to = (DPID)m.to;
            else if (m.only)                                                 *to = (DPID)m.only;
            else if ((rflags & DPRECEIVE_TOPLAYER) && isLocalPlayer(toIn))   *to = (DPID)toIn;
            else                                                             *to = (DPID)m.to;
        }
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
    HRESULT STDMETHODCALLTYPE GetMessageCount(DPID, LPDWORD n) override { MaDpGuard _g;
        pump(); if (n) *n = (DWORD)qcount(); return DP_OK;
    }
    /* R6.4: GROUPS. The game creates a group immediately after Open(CREATE) -- the trace named
       CreateGroup as the next stop, which is what the per-method logging is for. For a session
       this size a group is just an id plus a membership list; the Aggrgtor addresses traffic by
       PLAYER id, so group routing is not on the packet path yet. Implemented as real bookkeeping
       rather than DP_OK-and-forget, so EnumGroups/EnumGroupPlayers can answer truthfully. */
    HRESULT STDMETHODCALLTYPE CreateGroup(LPDPID pid, LPDPNAME nm, LPVOID, DWORD, DWORD) override { MaDpGuard _g;
        DPID g = nextPid++;
        if (pid) *pid = g;
        if (ngroups < 8) {
            groups[ngroups] = g; gmembers[ngroups] = 0;
            /* MP-6 S3: a group created after clients joined must contain them too */
            for (int j = 0; j < njoined && gmembers[ngroups] < 32; j++) {
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
    HRESULT STDMETHODCALLTYPE AddPlayerToGroup(DPID g, DPID p) override { MaDpGuard _g;
        bool matched = false;
        for (int i = 0; i < ngroups; i++)
            if (groups[i] == g && gmembers[i] < 32) { gplayers[i][gmembers[i]++] = p; matched = true; break; }
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
                 c->dwMaxBufferSize = 1024; c->dwMaxPlayers = 16; }
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
        d->dwMaxPlayers = 16; d->dwCurrentPlayers = isHost ? 1 + nclients() : (havePeer ? 2 : 1);
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
