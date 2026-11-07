#!/usr/bin/env python3
"""NET-RECORD-1: list or replay an MA_NET_CAPTURE file (see ma_dplay.cpp ma_capture for the record layout).

  ma_net_replay.py list CAP                      counts by direction and kind, duration, peers
  ma_net_replay.py play CAP --to HOST:PORT       send CAP's received side to an instance (stand in for its peer)
  ma_net_replay.py play CAP --as-host PORT       bind PORT, wait for the instance's first packet, then reply to it
       [--speed X]  [--skip-handshake]

Replay stands in for the recorded peer: the instance under test hears exactly what the recorded side heard, at the
recorded times (scaled by --speed), so a two-PC failure can be re-run on one PC. The datagrams go out verbatim
(sequence numbers and CRCs included).
"""
import socket
import struct
import sys
import time

KINDS = {1: "PROBE", 2: "OFFER", 3: "JOIN", 4: "DATA", 5: "ASSIGN", 6: "RDATA", 7: "ACK", 8: "REFUSE",
         9: "KEEP", 10: "GONE", 11: "BYE"}


def records(path):
    with open(path, "rb") as f:
        b = f.read()
    i = 0
    while i + 20 <= len(b):
        magic, d, t, ip = struct.unpack_from("<III4s", b, i)
        port, n = struct.unpack_from("<HH", b, i + 16)
        if magic != 0x5043414D:
            raise SystemExit("bad record at byte %d" % i)
        data = b[i + 20:i + 20 + n]
        i += 20 + n
        yield d, t, socket.inet_ntoa(ip), socket.ntohs(port), data


def kind(data):
    return struct.unpack_from("<I", data, 4)[0] if len(data) >= 8 else 0


def do_list(path):
    cnt, peers, first, last = {}, set(), None, 0
    for d, t, ip, port, data in records(path):
        k = (("sent", "recv")[d], KINDS.get(kind(data), "?%d" % kind(data)))
        cnt[k] = cnt.get(k, 0) + 1
        peers.add("%s:%d" % (ip, port))
        first = t if first is None else first
        last = t
    print("%s: %.1f s, peers %s" % (path, (last - (first or 0)) / 1000.0, ", ".join(sorted(peers))))
    for k in sorted(cnt):
        print("  %-4s %-7s %6d" % (k[0], k[1], cnt[k]))


def do_play(path, to=None, as_host=None, speed=1.0, skip_handshake=False):
    allr = list(records(path))
    recv = [(t, data) for d, t, ip, port, data in allr if d == 1]
    sent0 = min([t for d, t, ip, port, data in allr if d == 0] or [recv[0][0] if recv else 0])
    if skip_handshake:
        recv = [(t, x) for t, x in recv if kind(x) not in (1, 2, 3, 8)]
    if not recv:
        raise SystemExit("no received packets in %s" % path)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if as_host:
        s.bind(("0.0.0.0", as_host))
        print("waiting on port %d for the instance's first packet ..." % as_host, flush=True)
        _, to_addr = s.recvfrom(4096)
        print("instance at %s:%d; playing %d packets" % (to_addr[0], to_addr[1], len(recv)), flush=True)
    else:
        h, p = to.rsplit(":", 1)
        to_addr = (h, int(p))
    s.setblocking(False)
    # the recording's clock is anchored at its first SENT packet, which matches the live instance's first packet.
    # ASSIGN is the answer to JOIN and the instance waits only about a second for it, so it is a REPLY, not a
    # scheduled packet: the moment the live JOIN arrives the recorded ASSIGN goes out, the clock re-anchors there,
    # and the answers to earlier probes still in the schedule are dropped.
    assign = next(((t, x) for t, x in recv if kind(x) == 5), None)
    state = {"t0": sent0, "w0": time.monotonic(), "joined": False, "to": to_addr}

    def drain():
        try:
            while True:
                d, frm = s.recvfrom(4096)
                if as_host:
                    state["to"] = frm                     # the instance opens a NEW socket to join: follow it
                if kind(d) == 3 and assign:
                    s.sendto(assign[1], state["to"])          # every JOIN (the instance repeats it) gets the ASSIGN
                    if not state["joined"]:
                        state["joined"] = True; state["t0"] = assign[0]; state["w0"] = time.monotonic()
                        print("JOIN seen; ASSIGN sent; clock re-anchored", flush=True)
        except BlockingIOError:
            pass

    sent = 0
    for t, data in recv:
        if kind(data) == 5:
            continue                                      # sent by drain() as the reply to JOIN
        if assign and state["joined"] and t < assign[0]:
            continue                                      # pre-join answers that the live instance no longer needs
        while True:
            due = state["w0"] + (t - state["t0"]) / 1000.0 / speed
            now = time.monotonic()
            if now >= due:
                break
            drain()
            time.sleep(min(0.002, max(0.0, due - now)))
        if assign and not state["joined"] and t > assign[0]:
            while not state["joined"]:                    # post-join traffic waits for the live join
                drain(); time.sleep(0.005)
            continue
        s.sendto(data, state["to"])
        sent += 1
    w0 = state["w0"]
    print("sent %d packets over %.1f s" % (sent, time.monotonic() - w0))


def main(a):
    if len(a) < 2 or a[0] not in ("list", "play"):
        raise SystemExit(__doc__)
    if a[0] == "list":
        return do_list(a[1])
    to = as_host = None
    speed, skip = 1.0, False
    i = 2
    while i < len(a):
        if a[i] == "--to":
            to = a[i + 1]; i += 2
        elif a[i] == "--as-host":
            as_host = int(a[i + 1]); i += 2
        elif a[i] == "--speed":
            speed = float(a[i + 1]); i += 2
        elif a[i] == "--skip-handshake":
            skip = True; i += 1
        else:
            raise SystemExit("unknown option %s" % a[i])
    if not (to or as_host):
        raise SystemExit("play needs --to HOST:PORT or --as-host PORT")
    do_play(a[1], to, as_host, speed, skip)


if __name__ == "__main__":
    main(sys.argv[1:])
