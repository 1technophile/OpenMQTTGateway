#!/usr/bin/env python3
"""Exercise the OpenMQTTGateway CONNEX module over MQTT (no external dependencies).

Broker settings come from flags or environment variables:
  MQTT_HOST (required), MQTT_PORT (1883), MQTT_USER, MQTT_PASS,
  CONNEX_BASE (gateway base topic, default home/OpenMQTTGateway)

Examples:
  connex_mqtt.py watch                 # print everything the gateway publishes
  connex_mqtt.py status                # ask for gateway state
  connex_mqtt.py pair 1 20             # heater in sync mode first (hold V 3 s, then -, +, V)
  connex_mqtt.py set 2 19.5
  connex_mqtt.py learn | learn-stop
  connex_mqtt.py id A1B2 | burst 200 3 10 | power max | save | erase
  connex_mqtt.py test                  # scripted checks of validation and a setpoint round trip
"""
import argparse
import json
import os
import socket
import struct
import sys
import time


class MiniMQTT:
    """Minimal MQTT 3.1.1 client: CONNECT, PUBLISH/SUBSCRIBE at QoS 0, PINGREQ keepalive."""

    KEEPALIVE = 60  # seconds; a PINGREQ goes out after KEEPALIVE/2 without sending anything

    def __init__(self, host, port=1883, user=None, password=None, client_id=None):
        self.sock = socket.create_connection((host, port), timeout=10)
        self.buf = b""
        self.last_tx = time.time()
        cid = (client_id or "connex-harness-%d" % os.getpid()).encode()
        flags = 0x02  # clean session
        payload = self._str(cid)
        if user:
            flags |= 0x80
            payload += self._str(user.encode())
            if password:
                flags |= 0x40
                payload += self._str(password.encode())
        var = self._str(b"MQTT") + bytes([4, flags]) + struct.pack("!H", self.KEEPALIVE)
        self._send(0x10, var + payload)
        ptype, body = self._read_packet(10)
        if ptype != 0x20 or len(body) < 2 or body[1] != 0:
            raise RuntimeError("MQTT connect refused (code %s)" % (body[1] if len(body) > 1 else "?"))
        self.pid = 1

    @staticmethod
    def _str(b):
        return struct.pack("!H", len(b)) + b

    def _send(self, header, body):
        n, rem = len(body), b""
        while True:
            d, n = n % 128, n // 128
            rem += bytes([d | (0x80 if n else 0)])
            if not n:
                break
        self.sock.sendall(bytes([header]) + rem + body)
        self.last_tx = time.time()

    def _read_packet(self, timeout):
        self.sock.settimeout(timeout)
        def need(k):
            while len(self.buf) < k:
                chunk = self.sock.recv(4096)
                if not chunk:
                    raise ConnectionError("broker closed connection")
                self.buf += chunk
        need(2)
        mult, length, i = 1, 0, 1
        while True:
            need(i + 1)
            b = self.buf[i]
            length += (b & 0x7F) * mult
            mult *= 128
            i += 1
            if not b & 0x80:
                break
        need(i + length)
        header, body = self.buf[0], self.buf[i:i + length]
        self.buf = self.buf[i + length:]
        return header & 0xF0, body

    def publish(self, topic, payload, retain=False):
        self._send(0x30 | (1 if retain else 0), self._str(topic.encode()) + payload.encode())

    def subscribe(self, topic):
        self._send(0x82, struct.pack("!H", self.pid) + self._str(topic.encode()) + b"\x00")
        self.pid += 1

    def messages(self, seconds):
        """Yield (topic, payload) for `seconds`, pinging the broker so long waits stay connected."""
        end = time.time() + seconds
        while True:
            now = time.time()
            left = end - now
            if left <= 0:
                return
            ping_in = self.last_tx + self.KEEPALIVE / 2 - now
            if ping_in <= 0:
                self._send(0xC0, b"")  # PINGREQ; the PINGRESP (0xD0) is skipped below
                continue
            try:
                ptype, body = self._read_packet(min(left, ping_in))
            except socket.timeout:
                continue
            if ptype == 0x30:
                tlen = struct.unpack("!H", body[:2])[0]
                yield body[2:2 + tlen].decode(errors="replace"), body[2 + tlen:].decode(errors="replace")

    def close(self):
        try:
            self._send(0xE0, b"")
            self.sock.close()
        except OSError:
            pass


def show(topic, payload, base):
    short = topic[len(base):] if topic.startswith(base) else topic
    print("%s  %-28s %s" % (time.strftime("%H:%M:%S"), short, payload))


class Harness:
    def __init__(self, args):
        self.base = args.base.rstrip("/")
        self.cmd = self.base + "/commands/MQTTtoCONNEX"
        self.cmd_set = self.base + "/commands/MQTTtoCONNEXset"
        self.mqtt = MiniMQTT(args.host, args.port, args.user, args.password)
        for t in ("/CONNEXtoMQTT/#", "/LWT", "/BH1750toMQTT", "/CLIMAtoMQTT/#"):
            self.mqtt.subscribe(self.base + t)
        # swallow retained messages so later output only shows fresh traffic
        self.retained = list(self.mqtt.messages(1.0))

    def send(self, topic, obj, wait):
        payload = json.dumps(obj)
        print(">> %s %s" % (topic[len(self.base):], payload))
        self.mqtt.publish(topic, payload)
        got = []
        for t, p in self.mqtt.messages(wait):
            show(t, p, self.base)
            got.append((t, p))
        return got

    def zone_msgs(self, got, zone):
        """Zone entries from gateway status messages in `got`."""
        out = []
        for t, p in got:
            if t.endswith("/CONNEXtoMQTT/gateway"):
                try:
                    z = json.loads(p).get("zones", {}).get(str(zone))
                except ValueError:
                    continue
                if z:
                    out.append(z)
        return out

    @staticmethod
    def gateway_msgs(got):
        return [p for t, p in got if t.endswith("/CONNEXtoMQTT/gateway")]


def run_test(h, wait):
    ok = True
    def check(name, cond):
        nonlocal ok
        ok &= bool(cond)
        print("%s  %s" % ("PASS" if cond else "FAIL", name))

    before = None
    got = h.send(h.cmd_set, {}, 3)
    state = [json.loads(p) for t, p in got if t.endswith("/CONNEXtoMQTT/gateway")]
    check("gateway state published", state)
    if state:
        check("radio ok", state[-1].get("radio") == "ok")
        before = state[-1].get("zones", {}).get("1", {}).get("setpoint")
        if not state[-1].get("controllerid"):
            print("NOTE  no controller ID: run 'pair' or 'id' first; skipping setpoint checks")
            return ok

    # Rejected commands publish a non-retained state with "error" and send nothing
    for name, cmd in (("zone 0", {"zone": 0, "temperature": 20}),
                      ("30.5 C", {"zone": 1, "temperature": 30.5}),
                      ("-1.0 C", {"zone": 1, "temperature": -1}),
                      ("null temperature", {"zone": 1, "temperature": None}),
                      ("non-numeric temperature", {"zone": 1, "temperature": "abc"})):
        got = h.send(h.cmd, cmd, 3)
        msgs = [json.loads(p) for p in h.gateway_msgs(got)]
        check("%s rejected with error" % name, msgs and any(m.get("error") for m in msgs))

    got = h.send(h.cmd, {"zone": 1, "temperature": 20.5}, wait)
    z = h.zone_msgs(got, 1)
    # an unacked burst keeps the old setpoint and reports failed_setpoint instead
    check("zone 1 result published", z and 20.5 in (z[-1].get("setpoint"), z[-1].get("failed_setpoint")))
    if z:
        r = z[-1]
        check("result from gateway", r.get("source") == "gateway")
        check("frames sent", r.get("sent", 0) > 0)
        print("INFO  status=%s acked=%s/%s setpoint=%s" % (r.get("status"), r.get("acked"), r.get("sent"), r.get("setpoint")))
        check("acked by a heater", r.get("acked", 0) > 0)

    # Above 25.5 C the setpoint needs frame byte 5 as a high byte (26.0 C = 01 04).
    # The gateway can only prove the frame went out; check the heater display.
    for t in (26.0, 30.0):
        got = h.send(h.cmd, {"zone": 1, "temperature": t}, wait)
        z = h.zone_msgs(got, 1)
        check("%.1f C sent (two-byte setpoint)" % t, z and z[-1].get("setpoint") == t)
        print("CHECK heater display shows %.1f" % t)
        if sys.stdin.isatty():
            input("      press Enter to continue... ")

    got = h.send(h.cmd, {"zone": 1, "temperature": 17.0}, wait)
    z = h.zone_msgs(got, 1)
    check("17.0 C accepted (0xAA byte)", z and z[-1].get("setpoint") == 17.0)

    if before is not None:  # put zone 1 back where the test found it
        h.send(h.cmd, {"zone": 1, "temperature": before}, wait)
        print("INFO  zone 1 restored to %s" % before)
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default=os.environ.get("MQTT_HOST"))
    ap.add_argument("--port", type=int, default=int(os.environ.get("MQTT_PORT", "1883")))
    ap.add_argument("--user", default=os.environ.get("MQTT_USER"))
    ap.add_argument("--password", default=os.environ.get("MQTT_PASS"))
    ap.add_argument("--base", default=os.environ.get("CONNEX_BASE", "home/OpenMQTTGateway"))
    ap.add_argument("--wait", type=float, default=10.0, help="seconds to listen after a setpoint/pair (burst takes ~1-5 s)")
    ap.add_argument("command", choices=["watch", "status", "learn", "learn-stop", "pair", "set", "id", "burst", "power", "save", "erase", "test"])
    ap.add_argument("params", nargs="*")
    a = ap.parse_args()
    if not a.host:
        ap.error("set --host or MQTT_HOST")

    h = Harness(a)
    p = a.params
    try:
        if a.command == "watch":
            for t, pl in h.retained:
                show(t, pl, h.base)
            secs = float(p[0]) if p else 1e9
            for t, pl in h.mqtt.messages(secs):
                show(t, pl, h.base)
        elif a.command == "status":
            h.send(h.cmd_set, {}, 3)
        elif a.command == "learn":
            h.send(h.cmd_set, {"learn": True}, float(p[0]) if p else 125)
        elif a.command == "learn-stop":
            h.send(h.cmd_set, {"learn": False}, 3)
        elif a.command == "pair":
            obj = {"pair": int(p[0])}
            if len(p) > 1:
                obj["temperature"] = float(p[1])
            h.send(h.cmd_set, obj, a.wait)
        elif a.command == "set":
            h.send(h.cmd, {"zone": int(p[0]), "temperature": float(p[1])}, a.wait)
        elif a.command == "id":
            h.send(h.cmd_set, {"controllerid": p[0], "save": True}, 3)
        elif a.command == "burst":
            obj = {"burst": int(p[0]), "save": True}
            if len(p) > 1:
                obj["interval"] = int(p[1])
            if len(p) > 2:
                obj["acks"] = int(p[2])
            h.send(h.cmd_set, obj, 3)
        elif a.command == "power":
            h.send(h.cmd_set, {"power": p[0], "save": True}, 3)
        elif a.command == "save":
            h.send(h.cmd_set, {"save": True}, 3)
        elif a.command == "erase":
            h.send(h.cmd_set, {"erase": True}, 3)
        elif a.command == "test":
            sys.exit(0 if run_test(h, a.wait) else 1)
    except (IndexError, ValueError):
        ap.error("missing or bad parameters for '%s'" % a.command)
    except KeyboardInterrupt:
        pass
    finally:
        h.mqtt.close()


if __name__ == "__main__":
    main()
