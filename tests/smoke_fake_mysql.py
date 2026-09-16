#!/usr/bin/env python3
import socket
import struct
import subprocess
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROXY = ROOT / "build" / "fn_proxy"
BACKEND_PORT = 13306
PROXY_PORT = 13307


def pkt(seq, payload):
    n = len(payload)
    return bytes((n & 0xff, (n >> 8) & 0xff, (n >> 16) & 0xff, seq)) + payload


def recv_exact(s, n):
    out = b""
    while len(out) < n:
        c = s.recv(n - len(out))
        if not c:
            raise EOFError
        out += c
    return out


def recv_pkt(s):
    h = recv_exact(s, 4)
    n = h[0] | (h[1] << 8) | (h[2] << 16)
    return h[3], recv_exact(s, n)


def fake_handshake():
    caps = 0x0000FFFF | (0xFFFF << 16)
    lower = caps & 0xffff
    upper = (caps >> 16) & 0xffff
    payload = bytearray()
    payload += b"\x0a"
    payload += b"8.0.36-fake\x00"
    payload += struct.pack("<I", 123)
    payload += b"abcdefgh"
    payload += b"\x00"
    payload += struct.pack("<H", lower)
    payload += b"\x2d"
    payload += struct.pack("<H", 0x0002)
    payload += struct.pack("<H", upper)
    payload += b"\x15"
    payload += b"\x00" * 10
    payload += b"ijklmnopqrst\x00"
    payload += b"mysql_native_password\x00"
    return bytes(payload)


def ok_packet(seq):
    return pkt(seq, b"\x00\x00\x00\x02\x00\x00\x00")


seen_queries = []
backend_ready = threading.Event()
backend_done = threading.Event()


def backend_thread():
    with socket.socket() as ls:
        ls.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        ls.bind(("127.0.0.1", BACKEND_PORT))
        ls.listen(1)
        backend_ready.set()
        c, _ = ls.accept()
        with c:
            c.sendall(pkt(0, fake_handshake()))
            recv_pkt(c)  # auth response
            c.sendall(ok_packet(2))
            while True:
                try:
                    seq, payload = recv_pkt(c)
                except EOFError:
                    break
                if payload and payload[0] == 0x03:
                    sql = payload[1:].decode(errors="replace")
                    seen_queries.append(sql)
                    c.sendall(ok_packet((seq + 1) & 0xff))
                elif payload and payload[0] == 0x01:
                    break
                else:
                    c.sendall(ok_packet((seq + 1) & 0xff))
    backend_done.set()


t = threading.Thread(target=backend_thread, daemon=True)
t.start()
assert backend_ready.wait(2)

p = subprocess.Popen(
    [str(PROXY), str(PROXY_PORT), "127.0.0.1", str(BACKEND_PORT)],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
)
try:
    deadline = time.time() + 3
    client = None
    while time.time() < deadline:
        try:
            client = socket.create_connection(("127.0.0.1", PROXY_PORT), timeout=0.3)
            break
        except OSError:
            time.sleep(0.05)
    assert client is not None, "proxy did not start"

    with client:
        seq, hs = recv_pkt(client)
        assert seq == 0 and hs[0] == 0x0a

        # Find low capability flags and assert CLIENT_SSL bit was cleared by proxy.
        pos = hs.index(b"\x00", 1) + 1 + 4 + 8 + 1
        low_caps = hs[pos] | (hs[pos + 1] << 8)
        assert (low_caps & 0x0800) == 0, hex(low_caps)

        client.sendall(pkt(1, b"dummy-auth-response"))
        seq, auth_ok = recv_pkt(client)
        assert auth_ok[0] == 0x00

        client.sendall(pkt(0, b"\x03SELECT 1"))
        seq, select_ok = recv_pkt(client)
        assert select_ok[0] == 0x00

        client.sendall(pkt(0, b"\x03NF SET MODE 2NF"))
        seq, err = recv_pkt(client)
        assert err[0] == 0xff
        assert b"intercepted" in err.lower()

        client.sendall(pkt(0, b"\x01"))

    time.sleep(0.2)
    assert seen_queries == ["SELECT 1"], seen_queries
    print("SMOKE TEST PASSED")
    print("backend saw:", seen_queries)
finally:
    p.terminate()
    try:
        out, _ = p.communicate(timeout=2)
    except subprocess.TimeoutExpired:
        p.kill()
        out, _ = p.communicate()
    print("--- proxy log ---")
    print(out)
