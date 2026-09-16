# FN MySQL Proxy - minimal runnable demo

这是一个最小的 MySQL Wire Protocol 透明代理 Demo。

目标：

- 用户仍然使用标准 `mysql` 客户端登录；
- 代理监听 `3307`，后端真实 MySQL 默认为 `127.0.0.1:3306`；
- 握手、认证、普通 SQL、结果集全部透明转发；
- 只检查 `COM_QUERY (0x03)`；
- SQL 第一个关键字是 `NF` 或 `FN` 时，不转发给 MySQL，而是直接返回 MySQL `ERR_Packet`；
- 普通 SQL 原样放行。

## 架构

```text
mysql client
    |
    | MySQL Wire Protocol
    v
FN Proxy :3307
    |
    |-- NF/FN ...  --> ERR_Packet（拦截）
    |
    `-- other SQL  --> transparent forwarding
                         |
                         v
                    MySQL :3306
```

## 为什么这个 Demo 不需要 libmysqlclient

因为它不是“程序调用 MySQL API”，而是一个真正的 TCP/MySQL 协议代理。

代理直接透传真实 MySQL 的：

- Handshake
- Authentication
- COM_QUERY
- ResultSet
- COM_INIT_DB
- COM_PING
- COM_QUIT
- 其他未解析的数据包

这使得用户侧体验接近 ShardingSphere-Proxy：客户端连接的是代理端口，而不是修改应用代码去调用某个 SDK。

## 编译

Linux / WSL Ubuntu：

```bash
sudo apt update
sudo apt install -y build-essential cmake

mkdir build
cd build
cmake ..
cmake --build . -j
```

也可以直接：

```bash
g++ -std=c++17 -O2 -pthread src/main.cpp -o fn_proxy
```

## 运行

假设真实 MySQL：

```text
127.0.0.1:3306
```

启动代理：

```bash
./build/fn_proxy
```

等价于：

```bash
./build/fn_proxy 3307 127.0.0.1 3306
```

参数顺序：

```text
fn_proxy <listen_port> <backend_host> <backend_port>
```

## 使用标准 mysql 客户端登录代理

```bash
mysql -h127.0.0.1 -P3307 -uroot -p --protocol=TCP
```

代理会转发真实 MySQL 的握手和认证，所以用户名/密码仍然是 MySQL 自己验证。

Demo 会从服务端握手 capability 中清除 `CLIENT_SSL`，防止客户端与 MySQL 建立 TLS 后导致代理看不到明文 SQL。因此它仅适合作为本地研究 Demo，不适合生产环境。

## 测试普通 SQL：放行

```sql
SELECT VERSION();
SHOW DATABASES;
USE test;
CREATE TABLE t_demo(id INT PRIMARY KEY);
SELECT * FROM t_demo;
```

代理日志会出现类似：

```text
[PASS] SELECT VERSION()
[PASS] SHOW DATABASES
```

MySQL 正常返回结果。

## 测试 NF/FN SQL：拦截

```sql
NF SET MODE 2NF;
```

或者：

```sql
FN TEST;
```

客户端应该收到类似：

```text
ERROR 1105 (HY000): [FN Proxy] NF/FN statement intercepted by middleware demo
```

代理日志：

```text
[INTERCEPT] NF SET MODE 2NF
```

该 SQL 不会发送给真实 MySQL。

## 关键代码路径

客户端 -> Proxy：

```text
read MySQL packet
    |
    v
payload[0] == 0x03 ?       # COM_QUERY
    |
    +-- no  --> backend
    |
    `-- yes
         |
         v
      read SQL text
         |
         v
      first keyword
       /       \
   NF / FN     other
      |          |
      v          v
 ERR_Packet    backend
```

## 当前故意没有实现的内容

这是 v0.0 wire-proxy demo，不是完整 MySQL Proxy。暂不处理：

- 完整 MySQL Protocol 状态机
- TLS MITM
- 多包超大 SQL 的逻辑拼接
- Prepared Statement (`COM_STMT_PREPARE`) 中的 NF 检测
- SQL Parser / AST
- NF DSL Parser
- FD Engine
- 2NF / 3NF / BCNF
- Connection Pool
- Transaction Pinning
- Backend failover

下一步最合理的演进是：

```text
现在：
NF/FN prefix -> reject

下一版：
NF/FN prefix
    -> NF Lexer
    -> NF Parser
    -> NFCommand AST
    -> MetadataManager
    -> FD/NF Engine
```

普通 MySQL SQL 仍走 Fast Path 透明转发。
