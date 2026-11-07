/* NET-WAN-1 (C12 c): opt-in UPnP port mapping on the host's router (PO 11-06: manual forwarding first, documented;
   UPnP opt-in, OFF by default). MA_UPNP=1 turns it on.

   When the host opens a session, a background thread:
     1. finds the router: an SSDP M-SEARCH for an Internet Gateway Device (multicast 239.255.255.250:1900, or
        $MA_UPNP_SSDP=host:port for a test router), up to 3 s;
     2. reads the router's description (HTTP GET of the LOCATION it answered with) and takes the control URL of its
        WANIPConnection or WANPPPConnection service;
     3. asks it (SOAP AddPortMapping) to send UDP <port> to this PC, lease 1 h, renewed every 30 min; logs the
        router's public address if it gives one (GetExternalIPAddress).
   When the session closes, the mapping is removed (DeletePortMapping). Nothing here blocks the game: discovery and
   mapping run on their own thread, and a router that does not answer only costs a log line.
   Plain POSIX sockets, no library. */
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string>

namespace {

pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
int g_port = 0, g_run = 0, g_mapped = 0, g_waiting = 0;   /* g_waiting: the thread sits in the renewal wait */
pthread_t g_thr;
std::string g_ctlHost, g_ctlPath, g_svc, g_local; int g_ctlPort = 0;

void say(const char* fmt, const char* a = "", const char* b = "", int n = 0) {
    char m[512]; snprintf(m, sizeof m, fmt, a, b, n);
    fprintf(stderr, "[upnp] %s\n", m); fflush(stderr);
}

void set_timeout(int fd, int ms) {
    struct timeval tv; tv.tv_sec = ms / 1000; tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv); setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
}

/* "http://host:port/path" -> parts; false if not plain http */
bool split_url(const std::string& u, std::string& host, int& port, std::string& path) {
    if (strncasecmp(u.c_str(), "http://", 7)) return false;
    std::string r = u.substr(7); size_t sl = r.find('/');
    std::string hp = sl == std::string::npos ? r : r.substr(0, sl);
    path = sl == std::string::npos ? "/" : r.substr(sl);
    size_t c = hp.find(':'); host = c == std::string::npos ? hp : hp.substr(0, c);
    port = c == std::string::npos ? 80 : atoi(hp.c_str() + c + 1);
    return !host.empty() && port > 0;
}

/* one HTTP/1.0 exchange; returns the status code (0 on failure), body in *body; *local = our address on that link */
int http(const std::string& host, int port, const std::string& req, std::string* body, std::string* local = 0) {
    struct sockaddr_in a; memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &a.sin_addr) != 1) {
        struct hostent* he = gethostbyname(host.c_str()); if (!he) return 0; memcpy(&a.sin_addr, he->h_addr, 4); }
    int fd = socket(AF_INET, SOCK_STREAM, 0); if (fd < 0) return 0;
    set_timeout(fd, 3000);
    if (connect(fd, (struct sockaddr*)&a, sizeof a) != 0) { close(fd); return 0; }
    if (local) { struct sockaddr_in l; socklen_t ll = sizeof l; getsockname(fd, (struct sockaddr*)&l, &ll); *local = inet_ntoa(l.sin_addr); }
    if (send(fd, req.data(), req.size(), 0) != (ssize_t)req.size()) { close(fd); return 0; }
    std::string all; char b[4096]; ssize_t n;
    while ((n = recv(fd, b, sizeof b, 0)) > 0 && all.size() < (1u << 20)) all.append(b, n);
    close(fd);
    int code = 0; sscanf(all.c_str(), "HTTP/%*s %d", &code);
    size_t h = all.find("\r\n\r\n"); if (body) *body = h == std::string::npos ? "" : all.substr(h + 4);
    return code;
}

std::string tag(const std::string& x, const char* name, size_t from = 0, size_t* at = 0) {
    std::string o = std::string("<") + name + ">", c = std::string("</") + name + ">";
    size_t i = x.find(o, from); if (i == std::string::npos) return "";
    size_t j = x.find(c, i); if (j == std::string::npos) return "";
    if (at) *at = j;
    return x.substr(i + o.size(), j - i - o.size());
}

bool discover() {
    const char* st[] = { "urn:schemas-upnp-org:device:InternetGatewayDevice:1", "urn:schemas-upnp-org:device:InternetGatewayDevice:2",
                         "urn:schemas-upnp-org:service:WANIPConnection:1" };
    struct sockaddr_in to; memset(&to, 0, sizeof to); to.sin_family = AF_INET;
    const char* e = getenv("MA_UPNP_SSDP");
    char h[64] = "239.255.255.250"; int p = 1900;
    if (e && *e) { sscanf(e, "%63[^:]:%d", h, &p); }
    to.sin_port = htons(p); inet_pton(AF_INET, h, &to.sin_addr);
    int fd = socket(AF_INET, SOCK_DGRAM, 0); if (fd < 0) return false;
    set_timeout(fd, 500);
    for (int k = 0; k < 3; k++) {
        char m[512]; int n = snprintf(m, sizeof m, "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\n"
                                      "MX: 2\r\nST: %s\r\n\r\n", st[k]);
        sendto(fd, m, n, 0, (struct sockaddr*)&to, sizeof to);
    }
    std::string loc;
    for (time_t t0 = time(0); loc.empty() && time(0) - t0 < 3; ) {
        char b[2048]; ssize_t n = recv(fd, b, sizeof b - 1, 0); if (n <= 0) continue; b[n] = 0;
        for (char* l = strtok(b, "\r\n"); l; l = strtok(0, "\r\n"))
            if (!strncasecmp(l, "LOCATION:", 9)) { const char* v = l + 9; while (*v == ' ') v++; loc = v; break; }
    }
    close(fd);
    if (loc.empty()) { say("no router answered (UPnP off on the router, or none on this network); forward the port by hand"); return false; }
    std::string dh, dp; int dport;
    if (!split_url(loc, dh, dport, dp)) { say("router description at %s is not plain http; skipped", loc.c_str()); return false; }
    std::string desc;
    std::string req = "GET " + dp + " HTTP/1.0\r\nHost: " + dh + ":" + std::to_string(dport) + "\r\n\r\n";
    if (http(dh, dport, req, &desc, &g_local) != 200) { say("cannot read the router description at %s", loc.c_str()); return false; }
    const char* svcs[] = { "urn:schemas-upnp-org:service:WANIPConnection:2", "urn:schemas-upnp-org:service:WANIPConnection:1",
                           "urn:schemas-upnp-org:service:WANPPPConnection:1" };
    for (const char* s : svcs) {
        size_t i = desc.find(std::string("<serviceType>") + s + "</serviceType>"); if (i == std::string::npos) continue;
        size_t svcEnd = desc.find("</service>", i);
        std::string ctl = tag(desc, "controlURL", i);
        if (ctl.empty() || (svcEnd != std::string::npos && desc.find("<controlURL>", i) > svcEnd)) continue;
        g_svc = s;
        if (!strncasecmp(ctl.c_str(), "http://", 7)) { split_url(ctl, g_ctlHost, g_ctlPort, g_ctlPath); }
        else { std::string base = tag(desc, "URLBase"); std::string bh, bp; int bport;
               if (!base.empty() && split_url(base, bh, bport, bp)) { g_ctlHost = bh; g_ctlPort = bport; }
               else { g_ctlHost = dh; g_ctlPort = dport; }
               g_ctlPath = ctl[0] == '/' ? ctl : "/" + ctl; }
        return true;
    }
    say("the router (%s) has no WAN IP/PPP connection service; forward the port by hand", loc.c_str());
    return false;
}

int soap(const char* action, const std::string& args, std::string* body) {
    std::string env = "<?xml version=\"1.0\"?>\r\n<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                      "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:" + std::string(action) +
                      " xmlns:u=\"" + g_svc + "\">" + args + "</u:" + action + "></s:Body></s:Envelope>\r\n";
    std::string req = "POST " + g_ctlPath + " HTTP/1.0\r\nHost: " + g_ctlHost + ":" + std::to_string(g_ctlPort) +
                      "\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nSOAPAction: \"" + g_svc + "#" + action +
                      "\"\r\nContent-Length: " + std::to_string(env.size()) + "\r\n\r\n" + env;
    return http(g_ctlHost, g_ctlPort, req, body);
}

bool add_mapping(int port) {
    std::string p = std::to_string(port), body;
    std::string args = "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + p + "</NewExternalPort><NewProtocol>UDP</NewProtocol>"
                       "<NewInternalPort>" + p + "</NewInternalPort><NewInternalClient>" + g_local + "</NewInternalClient>"
                       "<NewEnabled>1</NewEnabled><NewPortMappingDescription>MiG Alley</NewPortMappingDescription>"
                       "<NewLeaseDuration>3600</NewLeaseDuration>";
    int code = soap("AddPortMapping", args, &body);
    if (code != 200) {   /* some routers refuse a lease: retry as a permanent mapping, which close() removes */
        args.replace(args.find("<NewLeaseDuration>3600"), 22, "<NewLeaseDuration>0");
        code = soap("AddPortMapping", args, &body);
    }
    return code == 200;
}

void* thread_main(void*) {
    if (!discover()) { pthread_mutex_lock(&g_mu); g_run = 0; pthread_mutex_unlock(&g_mu); return 0; }
    int port; pthread_mutex_lock(&g_mu); port = g_port; pthread_mutex_unlock(&g_mu);
    if (!add_mapping(port)) { say("the router refused to forward UDP %s%s (UPnP disabled, or the port is taken); forward it by hand", std::to_string(port).c_str());
        pthread_mutex_lock(&g_mu); g_run = 0; pthread_mutex_unlock(&g_mu); return 0; }
    std::string body, ext; if (soap("GetExternalIPAddress", "", &body) == 200) ext = tag(body, "NewExternalIPAddress");
    say("router forwards UDP %s to this PC (%s)", std::to_string(port).c_str(), (g_local + (ext.empty() ? "" : "; public address " + ext)).c_str());
    pthread_mutex_lock(&g_mu);
    if (!g_run) { pthread_mutex_unlock(&g_mu); soap("DeletePortMapping", "<NewRemoteHost></NewRemoteHost><NewExternalPort>" +
        std::to_string(port) + "</NewExternalPort><NewProtocol>UDP</NewProtocol>", &body);
        say("session closed meanwhile: router mapping for UDP %s removed", std::to_string(port).c_str()); return 0; }
    g_mapped = 1; g_waiting = 1;
    while (g_run) {                       /* renew every 30 min until close() */
        struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts); ts.tv_sec += 1800;
        if (pthread_cond_timedwait(&g_cv, &g_mu, &ts) != 0 && g_run) { pthread_mutex_unlock(&g_mu); add_mapping(port); pthread_mutex_lock(&g_mu); }
    }
    g_waiting = 0; pthread_mutex_unlock(&g_mu);
    return 0;
}

}  // namespace

extern "C" void ma_upnp_open(int udpPort) {
    const char* e = getenv("MA_UPNP"); if (!e || !*e || *e == '0') return;   /* opt-in */
    pthread_mutex_lock(&g_mu);
    if (g_run) { pthread_mutex_unlock(&g_mu); return; }
    g_port = udpPort; g_run = 1; g_mapped = 0;
    pthread_mutex_unlock(&g_mu);
    say("asking the router to forward UDP %s (MA_UPNP=1)", std::to_string(udpPort).c_str());
    if (pthread_create(&g_thr, 0, thread_main, 0) != 0) { g_run = 0; return; }
    pthread_detach(g_thr);   /* close() never waits for discovery: a thread still at work sees g_run = 0 and cleans up */
}

extern "C" void ma_upnp_close(void) {
    pthread_mutex_lock(&g_mu);
    const int mapped = g_mapped && g_waiting;
    g_run = 0; pthread_cond_signal(&g_cv);
    pthread_mutex_unlock(&g_mu);
    if (mapped) {
        std::string p = std::to_string(g_port), body;
        int code = soap("DeletePortMapping", "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + p +
                        "</NewExternalPort><NewProtocol>UDP</NewProtocol>", &body);
        say(code == 200 ? "router mapping for UDP %s removed" : "could not remove the router mapping for UDP %s (it expires within an hour)", p.c_str());
        g_mapped = 0;
    }
}

#ifdef MA_UPNP_TEST
int main(int argc, char** argv) {   /* g++ -DMA_UPNP_TEST SRC/compat/ma_upnp.cpp -lpthread; MA_UPNP=1 MA_UPNP_SSDP=... ./a.out 47624 */
    ma_upnp_open(argc > 1 ? atoi(argv[1]) : 47624);
    sleep(argc > 2 ? atoi(argv[2]) : 5);
    ma_upnp_close();
    return 0;
}
#endif
