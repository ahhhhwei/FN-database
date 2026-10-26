#pragma once

#include "fn/config.h"
#include "fn/protocol/mysql_packet_codec.h"
#include "fn/sql/parser.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace fn {

// 一个 ProxySession 对应一条“客户端 <-> 代理 <-> MySQL”连接链路。
// enable_shared_from_this 让异步回调可以持有 shared_ptr，避免回调执行前对象被销毁。
class ProxySession : public std::enable_shared_from_this<ProxySession> {
public:
    // client_socket 是 ProxyServer 已接受的客户端连接；构造时所有权移入会话。
    ProxySession(boost::asio::ip::tcp::socket client_socket,
                 boost::asio::io_context& io_context,
                 const Config& config);

    // 启动会话：先连接后端 MySQL，成功后同时开始双向读取。
    void start();

private:
    // 异步解析 mysql_host，并连接解析得到的某个 TCP endpoint。
    void connectBackend();

    // 分别挂起客户端方向和后端方向的一次异步读取。
    void readFromClient();
    void readFromBackend();

    // 登录、TLS 等无需解析的原始字节转发函数。
    void writeToBackend(std::size_t length);
    void writeToClient(std::size_t length);

    // 命令阶段使用的包级写入：普通包发后端，本地结果直接发客户端。
    void writePacketToBackend(const protocol::MysqlPacket& packet);
    void writeLocalResponse();

    // 根据当前协议阶段决定“透明转发”还是“拆包并检查 COM_QUERY”。
    void routeClientBytes(std::size_t length);

    // 登录阶段只观察握手能力位，不修改实际转发的数据。
    void inspectClientBytes(std::size_t length);
    void inspectBackendBytes(std::size_t length);

    // 串行消费已解出的客户端包，避免复用异步写缓冲区时覆盖尚未发送的数据。
    void processNextClientPacket();

    // 若 packet 是本项目支持的 FN SQL，则本地生成响应并返回 true。
    [[nodiscard]] bool handleQueryLocally(const protocol::MysqlPacket& packet);

    // 所有异步错误统一进入这里；close 可被多次调用但只实际关闭一次。
    void handleError(const char* operation, const boost::system::error_code& error);
    void close();

    // read_some 每次最多读取 8 KiB。TCP 拆包由 MysqlPacketDecoder 负责处理。
    static constexpr std::size_t buffer_size = 8192;

    // 客户端 socket 和后端 MySQL socket 构成代理的两个网络端点。
    boost::asio::ip::tcp::socket client_socket_;
    boost::asio::ip::tcp::socket backend_socket_;
    // resolver 将域名和端口解析为一个或多个可连接的 endpoint。
    boost::asio::ip::tcp::resolver resolver_;

    // 两个方向独立使用缓冲区，避免并发读写相互覆盖。
    std::array<char, buffer_size> client_buffer_{};
    std::array<char, buffer_size> backend_buffer_{};

    Config config_;
    // 日志中使用的“IP:端口”客户端标识。
    std::string client_name_;

    // TCP 没有消息边界，因此两个方向分别维护一个 MySQL 包解码器。
    protocol::MysqlPacketDecoder client_packet_decoder_;
    protocol::MysqlPacketDecoder backend_packet_decoder_;

    // 一次 TCP 读取可能包含多个 MySQL 包，先进入队列再逐个处理。
    std::deque<protocol::MysqlPacket> pending_client_packets_;

    // async_write 只保存内存视图，操作完成前底层 vector 必须一直存活。
    std::vector<std::uint8_t> backend_write_buffer_;
    std::vector<std::uint8_t> local_response_buffer_;

    // NF_MODE 是会话级状态：每条客户端连接初始为 OFF，互不共享。
    sql::NFMode nf_mode_ = sql::NFMode::off;

    // 客户端在 HandshakeResponse41 中声明的 32 位能力集合。
    std::uint32_t client_capabilities_ = 0;

    // 以下布尔值描述 MySQL 连接状态机，而不是业务状态。
    bool backend_handshake_seen_ = false;
    bool client_capabilities_seen_ = false;
    bool command_phase_ = false;
    // TLS/压缩启用后字节不再是可直接检查的 Classic Protocol 包。
    bool inspection_disabled_ = false;
    bool closed_ = false;
};

}  // namespace fn
