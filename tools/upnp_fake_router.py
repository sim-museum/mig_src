#!/usr/bin/env python3
"""A fake UPnP Internet Gateway Device for testing ma_upnp.cpp without touching a real router.

  upnp_fake_router.py SSDP_PORT HTTP_PORT LOGFILE [--refuse-lease]

Answers M-SEARCH on 127.0.0.1:SSDP_PORT with a LOCATION on 127.0.0.1:HTTP_PORT, serves a device description with
one WANIPConnection:1 service, and logs every SOAP action with its arguments to LOGFILE (one line each).
--refuse-lease answers AddPortMapping with a non-zero lease with error 725 (OnlyPermanentLeasesSupported).
"""
import re
import socket
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

SSDP, HTTP, LOG = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
REFUSE_LEASE = "--refuse-lease" in sys.argv
DESC = """<?xml version="1.0"?><root xmlns="urn:schemas-upnp-org:device-1-0"><device>
<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType><deviceList><device>
<deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType><deviceList><device>
<deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType><serviceList>
<service><serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>
<controlURL>/ctl/IPConn</controlURL></service></serviceList></device></deviceList></device></deviceList>
</device></root>"""


def log(line):
    with open(LOG, "a") as f:
        f.write(line + "\n")


def ssdp():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", SSDP))
    while True:
        d, a = s.recvfrom(2048)
        if d.startswith(b"M-SEARCH"):
            s.sendto(("HTTP/1.1 200 OK\r\nST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
                      "LOCATION: http://127.0.0.1:%d/desc.xml\r\n\r\n" % HTTP).encode(), a)


class H(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def do_GET(self):
        self.send_response(200); self.send_header("Content-Type", "text/xml"); self.end_headers()
        self.wfile.write(DESC.encode())

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length") or 0)).decode()
        action = (self.headers.get("SOAPAction") or "").strip('"').split("#")[-1]
        args = dict(re.findall(r"<(New\w+)>([^<]*)</New\w+>", body))
        log("%s %s" % (action, " ".join("%s=%s" % kv for kv in sorted(args.items()))))
        if action == "AddPortMapping" and REFUSE_LEASE and args.get("NewLeaseDuration") not in ("0", None):
            self.send_response(500); self.end_headers(); self.wfile.write(b"<errorCode>725</errorCode>"); return
        self.send_response(200); self.send_header("Content-Type", "text/xml"); self.end_headers()
        if action == "GetExternalIPAddress":
            self.wfile.write(b"<s:Envelope><s:Body><u:GetExternalIPAddressResponse>"
                             b"<NewExternalIPAddress>203.0.113.7</NewExternalIPAddress></u:GetExternalIPAddressResponse>"
                             b"</s:Body></s:Envelope>")


threading.Thread(target=ssdp, daemon=True).start()
ThreadingHTTPServer(("127.0.0.1", HTTP), H).serve_forever()
