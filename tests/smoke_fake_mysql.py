#!/usr/bin/env python3
"""代理端到端冒烟测试。

脚本只使用 Python 标准库，启动三个参与者：

    本脚本模拟的客户端 -> fn_proxy 子进程 -> 后台线程模拟的 MySQL

测试既覆盖真实 TCP 读写，也验证普通 SQL 被原样转发，而 FN SQL 只在代理本地执行。
这里不是完整 MySQL Server 实现，只构造当前测试需要的握手包和 OK 包。
"""

import os
import socket
import struct
import subprocess
import threading
import time
from pathlib import Path

# 仓库根目录由当前脚本路径推导，避免依赖运行命令时的工作目录。
ROOT = Path(__file__).resolve().parents[1]
# CTest 会通过环境变量传入准确的构建产物；手工运行时默认使用 build/fn_proxy。
PROXY = Path(os.environ.get("FN_PROXY_TEST_BINARY", ROOT / "build" / "fn_proxy"))

# HandshakeResponse41 中会用到的 capability flags。
# 测试故意开启新旧协议差异相关的位，确保代理生成正确的现代结果集。
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
    """让内核分配一个当前空闲的本地端口，然后返回端口号。"""

    # bind 到端口 0 表示让操作系统自动挑选 ephemeral port。
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


# 假后端和待测代理不能监听同一个端口，因此分别申请。
BACKEND_PORT = unused_port()
PROXY_PORT = unused_port()


def packet(sequence_id, payload):
    """给 payload 添加三字节小端长度和一字节 sequence id。"""

    length = len(payload)
    header = bytes(
        (length & 0xFF, (length >> 8) & 0xFF, (length >> 16) & 0xFF, sequence_id)
    )
    return header + payload


def recv_exact(sock, length):
    """循环 recv，直到获得指定字节数；不能假设一次 recv 就能收完。"""

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
    """从 TCP 字节流读取一个完整的 MySQL Classic Protocol 包。"""

    # 先读固定四字节包头，再按包头中的 payload 长度继续读取。
    header = recv_exact(sock, 4)
    length = header[0] | (header[1] << 8) | (header[2] << 16)
    return header[3], recv_exact(sock, length)


def send_fragmented(sock, data, fragment_sizes=(1, 2, 5, 13, 4096)):
    """故意把数据切成不规则片段，验证代理不能依赖 TCP 消息边界。"""

    offset = 0
    size_index = 0
    while offset < len(data):
        size = fragment_sizes[size_index % len(fragment_sizes)]
        chunk = data[offset : offset + size]
        sock.sendall(chunk)
        offset += len(chunk)
        size_index += 1


def fake_handshake():
    """构造足以被代理和测试客户端识别的 Initial Handshake。"""

    # 这里声明所有能力，包括 CLIENT_SSL；代理应透明转发，不能擅自删除安全能力。
    capabilities = 0xFFFFFFFF  # 包含 CLIENT_SSL，代理不得清除该位。
    payload = bytearray()
    # protocol version 10 + NUL 结尾的服务端版本字符串。
    payload += b"\x0a"
    payload += b"8.0.36-fake\x00"
    # connection id 和认证随机串的前半部分。
    payload += struct.pack("<I", 123)
    payload += b"abcdefgh"
    payload += b"\x00"
    # capability flags 被协议拆成低 16 位和高 16 位，中间还有字符集与状态位。
    payload += struct.pack("<H", capabilities & 0xFFFF)
    payload += b"\x2d"
    payload += struct.pack("<H", 0x0002)
    payload += struct.pack("<H", capabilities >> 16)
    payload += b"\x15"
    payload += b"\x00" * 10
    # 认证随机串后半部分和认证插件名。
    payload += b"ijklmnopqrst\x00"
    payload += b"mysql_native_password\x00"
    return bytes(payload)


def ok_packet(sequence_id):
    """构造最小 Protocol 4.1 OK 包：无影响行、无 insert id、autocommit。"""

    return packet(sequence_id, b"\x00\x00\x00\x02\x00\x00\x00")


def fake_auth_response():
    """构造测试客户端发送的简化 HandshakeResponse41。"""

    payload = bytearray()
    # 前四字节是代理真正需要读取的 capability flags。
    payload += struct.pack("<I", CLIENT_CAPABILITIES)
    # max packet size、字符集和 23 字节保留区。
    payload += struct.pack("<I", 0x00FFFFFF)
    payload += b"\x2d"
    payload += b"\x00" * 23
    # 用户名以 NUL 结束；最后的 0 表示空认证数据。
    payload += b"smoke-test\x00"
    payload += b"\x00"
    return bytes(payload)


def query_packet(query):
    """构造启用 CLIENT_QUERY_ATTRIBUTES 后的 COM_QUERY 包。"""

    # 0x03 是 COM_QUERY。能力协商后 SQL 前有两个 length-encoded 计数：
    # parameter_count=0、parameter_set_count=1，这正是 mysql CLI 的空属性形式。
    return packet(0, b"\x03\x00\x01" + query)


# 后台线程把收到的普通查询记录下来，主线程最后验证转发边界。
seen_queries = []
# Event 用于在线程之间通知“开始监听”和“处理结束”，避免依赖固定 sleep。
backend_ready = threading.Event()
backend_done = threading.Event()
# daemon 线程中的异常不会自动让主线程失败，因此显式收集后再断言。
backend_errors = []


def backend_thread():
    """运行一个只支持握手、OK、COM_QUERY、COM_QUIT 的最小假 MySQL。"""

    try:
        with socket.socket() as listener:
            # 测试频繁运行时允许快速复用地址。
            listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listener.bind(("127.0.0.1", BACKEND_PORT))
            listener.listen(1)
            backend_ready.set()

            connection, _ = listener.accept()
            with connection:
                # MySQL 协议由服务端先发送 Initial Handshake。
                send_fragmented(connection, packet(0, fake_handshake()))

                # 客户端用 sequence 1 回应；假后端不校验密码，只校验测试数据结构。
                sequence_id, auth_payload = recv_packet(connection)
                assert sequence_id == 1
                assert auth_payload == fake_auth_response()
                send_fragmented(connection, ok_packet(2))

                # 登录后循环处理命令。FN 命令若实现正确，永远不会到达这里。
                while True:
                    sequence_id, payload = recv_packet(connection)
                    if payload == b"\x01":  # COM_QUIT
                        break
                    if payload and payload[0] == 0x03:  # COM_QUERY
                        # 去掉 COM_QUERY 命令字节，保留其余字节用于“原样转发”断言。
                        seen_queries.append(payload[1:])
                    # 测试只关心路由，不实现真正的 SELECT 结果，所以统一回复 OK。
                    send_fragmented(connection, ok_packet((sequence_id + 1) & 0xFF))
    except Exception as error:
        # 将后台异常带回主线程，否则 daemon 线程失败可能被悄悄忽略。
        backend_errors.append(error)
    finally:
        # 即使启动阶段失败也释放等待者，主线程会通过 backend_errors 得到真实原因。
        backend_ready.set()
        backend_done.set()


# 先启动假后端，确保代理连接它时监听端口已经就绪。
thread = threading.Thread(target=backend_thread, daemon=True)
thread.start()
assert backend_ready.wait(2), "fake backend did not start"
assert not backend_errors, backend_errors

# 把代理作为真实子进程启动，并合并 stdout/stderr 以便失败时输出完整日志。
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
    # 代理启动是异步过程；在三秒期限内重试连接，避免机器快慢导致偶发失败。
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
        # 握手必须逐字节一致，证明代理没有破坏服务端能力位或认证数据。
        sequence_id, handshake = recv_packet(client)
        assert sequence_id == 0
        assert handshake == fake_handshake(), "proxy changed the MySQL handshake"

        # 从 Handshake 的变长服务端版本字段之后定位低 16 位 capability flags。
        capability_offset = handshake.index(b"\x00", 1) + 1 + 4 + 8 + 1
        low_capabilities = handshake[capability_offset] | (
            handshake[capability_offset + 1] << 8
        )
        assert low_capabilities & 0x0800, "CLIENT_SSL was unexpectedly removed"

        # 完成简化登录交换，后续包才属于 Command Phase。
        send_fragmented(client, packet(1, fake_auth_response()))
        _, auth_ok = recv_packet(client)
        assert auth_ok[0] == 0x00

        ordinary_queries = [
            b"SELECT 1",
            b"SELECT '" + (b"x" * 20000) + b"'",
        ]

        # 普通 SELECT 应到达假后端，并把后端 OK 原样传回客户端。
        send_fragmented(client, query_packet(ordinary_queries[0]))
        _, response = recv_packet(client)
        assert response[0] == 0x00

        # SET NF_MODE 应在代理本地修改会话状态并返回 OK，不得发给假后端。
        send_fragmented(client, query_packet(b"SET NF_MODE = 2NF;"))
        sequence_id, response = recv_packet(client)
        assert sequence_id == 1
        assert response[0] == 0x00

        # SHOW NF_MODE 应返回四个现代文本结果集包：头、列定义、行、OK-as-EOF。
        send_fragmented(client, query_packet(b"SHOW NF_MODE;"))
        sequence_id, result_header = recv_packet(client)
        assert sequence_id == 1
        assert result_header == b"\x01\x01"  # 完整元数据，列数为 1。
        sequence_id, column_definition = recv_packet(client)
        assert sequence_id == 2
        assert b"NF_MODE" in column_definition
        sequence_id, row = recv_packet(client)
        assert sequence_id == 3
        assert row == b"\x032NF"
        sequence_id, result_end = recv_packet(client)
        assert sequence_id == 4
        assert result_end == b"\xfe\x00\x00\x02\x00\x00\x00"

        # 疑似 FN SQL 但模式非法时，代理应返回 ERR(0xff) 和 SQLSTATE 42000。
        send_fragmented(client, query_packet(b"SET NF_MODE = 1NF;"))
        sequence_id, response = recv_packet(client)
        assert sequence_id == 1
        assert response[0] == 0xFF
        assert response[4:9] == b"42000"

        # 20 KiB 查询远大于 ProxySession 的 8 KiB read buffer，可覆盖跨读取拼包。
        send_fragmented(client, query_packet(ordinary_queries[1]))
        _, response = recv_packet(client)
        assert response[0] == 0x00

        # COM_QUIT 没有服务端响应；它通知假后端结束命令循环。
        send_fragmented(client, packet(0, b"\x01"))

    # 确认后台线程正常退出，且没有隐藏异常。
    assert backend_done.wait(2), "fake backend did not finish"
    assert not backend_errors, backend_errors
    expected_forwarded_queries = [b"\x00\x01" + query for query in ordinary_queries]
    # 查询属性前缀和 SQL 都必须原样保留；三个 FN 命令不能出现在列表里。
    assert seen_queries == expected_forwarded_queries, (
        "proxy should forward ordinary SQL unchanged and keep NF commands local"
    )
    print("SMOKE TEST PASSED")
    print("forwarded query sizes:", [len(query) for query in seen_queries])
finally:
    # 无论前面哪个断言失败都终止代理，避免测试留下监听端口的孤儿进程。
    process.terminate()
    try:
        output, _ = process.communicate(timeout=2)
    except subprocess.TimeoutExpired:
        process.kill()
        output, _ = process.communicate()
    print("--- proxy log ---")
    print(output)

# 除协议结果外，再确认代理确实走过 ANTLR/FN SQL 的本地处理分支。
assert "Parsed FN SQL" in output, "proxy did not parse the FN COM_QUERY packet"
