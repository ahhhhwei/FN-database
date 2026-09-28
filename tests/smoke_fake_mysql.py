#!/usr/bin/env python3
"""End-to-end smoke test for pass-through SQL and local NF commands."""

import os
import socket
import struct
import subprocess
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROXY = Path(os.environ.get("FN_PROXY_TEST_BINARY", ROOT / "build" / "fn_proxy"))

CLIENT_PROTOCOL_41 = 0x00000200
CLIENT_SECURE_CONNECTION = 0x00008000
CLIENT_DEPRECATE_EOF = 0x01000000
CLIENT_OPTIONAL_RESULTSET_METADATA = 0x02000000
CLIENT_QUERY_ATTRIBUTES = 0x08000000
CLIENT_CAPABILITIES = (
    CLIENT_PROTOCOL_41
    | CLIENT_SECURE_CONNECTION
    | CLIENT_DEPRECATE_EOF
    | CLIENT_OPTIONAL_RESULTSET_METADATA
    | CLIENT_QUERY_ATTRIBUTES
)


def unused_port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


BACKEND_PORT = unused_port()
PROXY_PORT = unused_port()


def packet(sequence_id, payload):
    length = len(payload)
    header = bytes(
        (length & 0xFF, (length >> 8) & 0xFF, (length >> 16) & 0xFF, sequence_id)
    )
    return header + payload


def recv_exact(sock, length):
    chunks = []
    remaining = length
    while remaining:
        chunk = sock.recv(remaining)
        if not chunk:
            raise EOFError("connection closed")
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)


def recv_packet(sock):
    header = recv_exact(sock, 4)
    length = header[0] | (header[1] << 8) | (header[2] << 16)
    return header[3], recv_exact(sock, length)


def send_fragmented(sock, data, fragment_sizes=(1, 2, 5, 13, 4096)):
    offset = 0
    size_index = 0
    while offset < len(data):
        size = fragment_sizes[size_index % len(fragment_sizes)]
        chunk = data[offset : offset + size]
        sock.sendall(chunk)
        offset += len(chunk)
        size_index += 1


def fake_handshake():
    capabilities = 0xFFFFFFFF  # Includes CLIENT_SSL; the proxy must not clear it.
    payload = bytearray()
    payload += b"\x0a"
    payload += b"8.0.36-fake\x00"
    payload += struct.pack("<I", 123)
    payload += b"abcdefgh"
    payload += b"\x00"
    payload += struct.pack("<H", capabilities & 0xFFFF)
    payload += b"\x2d"
    payload += struct.pack("<H", 0x0002)
    payload += struct.pack("<H", capabilities >> 16)
    payload += b"\x15"
    payload += b"\x00" * 10
    payload += b"ijklmnopqrst\x00"
    payload += b"mysql_native_password\x00"
    return bytes(payload)


def ok_packet(sequence_id):
    return packet(sequence_id, b"\x00\x00\x00\x02\x00\x00\x00")


def fake_auth_response():
    payload = bytearray()
    payload += struct.pack("<I", CLIENT_CAPABILITIES)
    payload += struct.pack("<I", 0x00FFFFFF)
    payload += b"\x2d"
    payload += b"\x00" * 23
    payload += b"smoke-test\x00"
    payload += b"\x00"
    return bytes(payload)


def query_packet(query):
    # With CLIENT_QUERY_ATTRIBUTES negotiated, COM_QUERY carries two
    # length-encoded counters before the SQL text. The mysql CLI sends 0
    # parameters and one parameter set when no attributes were supplied.
    return packet(0, b"\x03\x00\x01" + query)


seen_queries = []
backend_ready = threading.Event()
backend_done = threading.Event()
backend_errors = []


def backend_thread():
    try:
        with socket.socket() as listener:
            listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listener.bind(("127.0.0.1", BACKEND_PORT))
            listener.listen(1)
            backend_ready.set()

            connection, _ = listener.accept()
            with connection:
                send_fragmented(connection, packet(0, fake_handshake()))

                sequence_id, auth_payload = recv_packet(connection)
                assert sequence_id == 1
                assert auth_payload == fake_auth_response()
                send_fragmented(connection, ok_packet(2))

                while True:
                    sequence_id, payload = recv_packet(connection)
                    if payload == b"\x01":  # COM_QUIT
                        break
                    if payload and payload[0] == 0x03:  # COM_QUERY
                        seen_queries.append(payload[1:])
                    send_fragmented(connection, ok_packet((sequence_id + 1) & 0xFF))
    except Exception as error:  # Surface thread failures in the main test.
        backend_errors.append(error)
    finally:
        backend_ready.set()
        backend_done.set()


thread = threading.Thread(target=backend_thread, daemon=True)
thread.start()
assert backend_ready.wait(2), "fake backend did not start"
assert not backend_errors, backend_errors

process = subprocess.Popen(
    [
        str(PROXY),
        "--listen-host",
        "127.0.0.1",
        "--listen-port",
        str(PROXY_PORT),
        "--mysql-host",
        "127.0.0.1",
        "--mysql-port",
        str(BACKEND_PORT),
    ],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
)

try:
    deadline = time.time() + 3
    client = None
    while time.time() < deadline:
        try:
            client = socket.create_connection(("127.0.0.1", PROXY_PORT), timeout=0.5)
            break
        except OSError:
            time.sleep(0.05)
    assert client is not None, "proxy did not start"

    with client:
        sequence_id, handshake = recv_packet(client)
        assert sequence_id == 0
        assert handshake == fake_handshake(), "proxy changed the MySQL handshake"

        capability_offset = handshake.index(b"\x00", 1) + 1 + 4 + 8 + 1
        low_capabilities = handshake[capability_offset] | (
            handshake[capability_offset + 1] << 8
        )
        assert low_capabilities & 0x0800, "CLIENT_SSL was unexpectedly removed"

        send_fragmented(client, packet(1, fake_auth_response()))
        _, auth_ok = recv_packet(client)
        assert auth_ok[0] == 0x00

        ordinary_queries = [
            b"SELECT 1",
            b"SELECT '" + (b"x" * 20000) + b"'",
        ]

        send_fragmented(client, query_packet(ordinary_queries[0]))
        _, response = recv_packet(client)
        assert response[0] == 0x00

        send_fragmented(client, query_packet(b"SET NF_MODE = 2NF;"))
        sequence_id, response = recv_packet(client)
        assert sequence_id == 1
        assert response[0] == 0x00

        send_fragmented(client, query_packet(b"SHOW NF_MODE;"))
        sequence_id, result_header = recv_packet(client)
        assert sequence_id == 1
        assert result_header == b"\x01\x01"  # FULL metadata, one column
        sequence_id, column_definition = recv_packet(client)
        assert sequence_id == 2
        assert b"NF_MODE" in column_definition
        sequence_id, row = recv_packet(client)
        assert sequence_id == 3
        assert row == b"\x032NF"
        sequence_id, result_end = recv_packet(client)
        assert sequence_id == 4
        assert result_end == b"\xfe\x00\x00\x02\x00\x00\x00"

        send_fragmented(client, query_packet(b"SET NF_MODE = 1NF;"))
        sequence_id, response = recv_packet(client)
        assert sequence_id == 1
        assert response[0] == 0xFF
        assert response[4:9] == b"42000"

        send_fragmented(client, query_packet(ordinary_queries[1]))
        _, response = recv_packet(client)
        assert response[0] == 0x00

        send_fragmented(client, packet(0, b"\x01"))

    assert backend_done.wait(2), "fake backend did not finish"
    assert not backend_errors, backend_errors
    expected_forwarded_queries = [b"\x00\x01" + query for query in ordinary_queries]
    assert seen_queries == expected_forwarded_queries, (
        "proxy should forward ordinary SQL unchanged and keep NF commands local"
    )
    print("SMOKE TEST PASSED")
    print("forwarded query sizes:", [len(query) for query in seen_queries])
finally:
    process.terminate()
    try:
        output, _ = process.communicate(timeout=2)
    except subprocess.TimeoutExpired:
        process.kill()
        output, _ = process.communicate()
    print("--- proxy log ---")
    print(output)

assert "Parsed FN SQL" in output, "proxy did not parse the FN COM_QUERY packet"
