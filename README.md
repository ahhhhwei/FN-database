# FN MySQL Transparent Proxy

一个最小可运行的 C++17 MySQL TCP 透明代理：

```text
MySQL Client  <---- TCP byte stream ---->  FN Proxy  <---- TCP byte stream ---->  MySQL Server
                                            :13306                              :3306
```

当前版本不理解 MySQL 协议，也不会修改数据。Handshake、认证、SQL 命令、结果集和可选 TLS 都由客户端与真实 MySQL Server 端到端完成。代理只负责异步、全双工地搬运 TCP 字节。

## 依赖与编译

Ubuntu / Debian：

```bash
sudo apt update
sudo apt install -y build-essential cmake libboost-system-dev

cmake -S . -B build
cmake --build build -j
```

生成的程序是 `build/fn_proxy`。

## 启动

使用默认配置（监听 `0.0.0.0:13306`，连接 `127.0.0.1:3306`）：

```bash
./build/fn_proxy
```

完整参数：

```bash
./build/fn_proxy \
  --listen-host 0.0.0.0 \
  --listen-port 13306 \
  --mysql-host 127.0.0.1 \
  --mysql-port 3306
```

查看帮助：

```bash
./build/fn_proxy --help
```

`--listen-host` 当前应为数字形式的 IPv4/IPv6 地址；`--mysql-host` 可以是 IP 或可解析的主机名。

## 使用真实 MySQL 测试

先确认 MySQL 正在 `127.0.0.1:3306` 监听，再通过代理连接：

```bash
mysql -h 127.0.0.1 -P 13306 -u root -p --protocol=TCP
```

登录后可执行完整的读写测试：

```sql
SHOW DATABASES;
CREATE DATABASE fn_test;
USE fn_test;
CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50));
INSERT INTO users VALUES (1, 'Alice');
SELECT * FROM users;
UPDATE users SET name = 'Bob' WHERE id = 1;
DELETE FROM users WHERE id = 1;
SHOW TABLES;
DROP DATABASE fn_test;
```

所有行为应与直连 `3306` 相同。代理日志会显示连接建立、断开以及每次实际转发的字节数。

## 无 MySQL 环境的冒烟测试

仓库内的 Python 测试会启动一个假的 TCP/MySQL 后端，验证：

- 后端首先发送的数据能到达客户端；
- Handshake 中的 capability 字节没有被改写；
- 客户端数据按原样到达后端，包括 `NF` 开头的数据；
- 超过 8192 字节的数据与刻意拆分的 TCP 写入仍可正确转发。

```bash
python3 tests/smoke_fake_mysql.py
```

## 代码结构与职责

```text
main
  `-- ProxyServer
        `-- one ProxySession per client
              |-- client_socket
              `-- backend_socket
```

- `Config`：保存默认配置并解析少量命令行参数。
- `ProxyServer`：创建监听 socket、持续异步 accept，并为每个客户端创建一个 `ProxySession`。
- `ProxySession`：解析后端地址、连接 MySQL，并维护两条彼此独立的异步链：`client read -> backend write -> client read` 与 `backend read -> client write -> backend read`。
- `main`：组装对象、处理退出信号并运行单线程 `io_context` 事件循环。

每个方向只有前一批数据写完后才会复用对应 buffer，因此 buffer 不会被未完成的异步写覆盖。两个方向使用不同 buffer，可以并行推进。所有异步回调都捕获 `shared_from_this()` 得到的 `self`，保证 Session 存活到回调完成；任一方向失败都会幂等地关闭两端 socket。

## 当前边界与下一阶段

当前代码故意不做 MySQL Packet 重组、SQL 解析、认证处理、连接池或 TLS 解密。`async_read_some()` 返回的内容可能是半个包、一个包或多个包，都会按实际字节数直接转发。

下一阶段需要拦截 MySQL 包时，应在 `ProxySession` 的 read 完成与对应 write 开始之间接入独立的 `MySQLPacketInterceptor`。该组件必须自行维护跨 read 的重组状态，并在完整逻辑包可用后才检查内容；`ProxyServer` 仍只负责接入连接。若连接启用 TLS，则不能直接查看其中的 SQL，除非明确设计 TLS 终止方案。
