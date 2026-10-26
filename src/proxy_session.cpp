#include "fn/proxy_session.h"

#include "fn/sql/parser.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace fn
{
    namespace
    {
        // COM_QUERY 是 MySQL Classic Protocol 中“执行文本 SQL”的命令字节。
        constexpr std::uint8_t com_query = 0x03;

        // 以下位来自客户端 HandshakeResponse41 的 capability flags。
        // 代理需要根据这些位决定后续字节能否解析，以及应返回哪种结果集格式。
        constexpr std::uint32_t client_compress = 0x00000020U;
        constexpr std::uint32_t client_ssl = 0x00000800U;
        constexpr std::uint32_t client_deprecate_eof = 0x01000000U;
        constexpr std::uint32_t client_optional_resultset_metadata = 0x02000000U;
        constexpr std::uint32_t client_query_attributes = 0x08000000U;

        // 1064/42000 与 MySQL 常见的 SQL 语法错误表现一致。
        constexpr std::uint16_t parse_error_code = 1064U;

        // endpoint 没有直接面向日志的格式化接口，这里统一输出“IP:端口”。
        std::string endpointName(const boost::asio::ip::tcp::endpoint &endpoint)
        {
            return endpoint.address().to_string() + ':' + std::to_string(endpoint.port());
        }

        bool readLengthEncodedInteger(const std::vector<std::uint8_t> &payload,
                                      std::size_t &offset,
                                      std::uint64_t &value)
        {
            // offset 由引用传入：成功后它会移动到整数之后，便于连续读取字段。
            if (offset >= payload.size())
            {
                return false;
            }

            const std::uint8_t first = payload[offset++];
            if (first < 0xfbU)
            {
                // 0x00~0xfa 的值直接由首字节表示。
                value = first;
                return true;
            }

            // 0xfc/0xfd/0xfe 分别表示后面跟 2/3/8 个小端字节。
            // 0xfb 表示 SQL NULL，0xff 保留；二者都不能作为这里的计数值。
            std::size_t byte_count = 0;
            if (first == 0xfcU)
            {
                byte_count = 2;
            }
            else if (first == 0xfdU)
            {
                byte_count = 3;
            }
            else if (first == 0xfeU)
            {
                byte_count = 8;
            }
            else
            {
                return false;
            }

            if (payload.size() - offset < byte_count)
            {
                return false;
            }

            value = 0;
            for (std::size_t index = 0; index < byte_count; ++index)
            {
                // 每个字节移到对应的小端位置，再用按位或组合成整数。
                value |= static_cast<std::uint64_t>(payload[offset++]) <<
                         (index * 8U);
            }
            return true;
        }

        std::optional<std::string_view> queryText(
            const protocol::MysqlPacket &packet,
            std::uint32_t client_capabilities)
        {
            // 非 COM_QUERY 包（例如 COM_QUIT）没有可供 ANTLR 解析的 SQL。
            if (packet.payload.empty() || packet.payload.front() != com_query)
            {
                return std::nullopt;
            }

            std::size_t query_offset = 1;
            if ((client_capabilities & client_query_attributes) != 0)
            {
                // MySQL 8 的 COM_QUERY 可以在 SQL 前携带命名查询属性。
                // 即使没有属性，mysql CLI 也会发送 parameter_count=0、set_count=1。
                std::uint64_t parameter_count = 0;
                std::uint64_t parameter_set_count = 0;
                if (!readLengthEncodedInteger(
                        packet.payload, query_offset, parameter_count) ||
                    !readLengthEncodedInteger(
                        packet.payload, query_offset, parameter_set_count))
                {
                    return std::nullopt;
                }

                // 非空属性还包含 NULL bitmap、类型、名称和二进制值。当前 FN SQL
                // 不消费这些属性，因此保持原包不变并交给真实 MySQL 处理更安全。
                if (parameter_count != 0 || parameter_set_count != 1)
                {
                    return std::nullopt;
                }
            }

            // string_view 不复制 SQL，只引用 packet.payload；调用期间 packet 必须存活。
            const auto *sql_data = reinterpret_cast<const char *>(
                packet.payload.data() + query_offset);
            return std::string_view(sql_data, packet.payload.size() - query_offset);
        }

    } // namespace

    ProxySession::ProxySession(boost::asio::ip::tcp::socket client_socket, boost::asio::io_context &io_context, const Config &config)
        : client_socket_(std::move(client_socket)), backend_socket_(io_context), resolver_(io_context), config_(config)
    {
        boost::system::error_code error;
        // 使用带 error_code 的重载可避免 remote_endpoint 失败时抛异常。
        const auto endpoint = client_socket_.remote_endpoint(error);
        client_name_ = error ? "unknown" : endpointName(endpoint);
    }

    void ProxySession::start()
    {
        std::cout << "[INFO] Client connected: " << client_name_ << '\n';
        connectBackend();
    }

    void ProxySession::connectBackend()
    {
        // 异步操作必须捕获 shared_ptr，而不是裸 this。这样会话至少活到回调结束。
        auto self = shared_from_this();

        // async_resolve 同时支持 IP 和域名，结果可能包含多个 IPv4/IPv6 endpoint。
        resolver_.async_resolve(
            config_.mysql_host,
            std::to_string(config_.mysql_port),
            [self](const boost::system::error_code &error, const boost::asio::ip::tcp::resolver::results_type &endpoints)
            {
                if (error)
                {
                    self->handleError("resolve backend", error);
                    return;
                }
                // async_connect 会依次尝试解析得到的 endpoint，直到成功或全部失败。
                boost::asio::async_connect(
                    self->backend_socket_,
                    endpoints,
                    [self](const boost::system::error_code &connect_error, const boost::asio::ip::tcp::endpoint &endpoint)
                    {
                        if (connect_error)
                        {
                            self->handleError("connect backend", connect_error);
                            return;
                        }
                        std::cout << "[INFO] Connected to MySQL: " << endpointName(endpoint) << " for client " << self->client_name_ << '\n';
                        // MySQL 主动发送初始握手。两个方向都开始读，登录交互才能往返。
                        self->readFromBackend();
                        self->readFromClient();
                    }); });
    }

    void ProxySession::readFromClient()
    {
        if (closed_)
        {
            return;
        }
        // read_some 不保证填满整个数组，也不保证一次正好读到一个 MySQL 包。
        auto self = shared_from_this();
        client_socket_.async_read_some(
            boost::asio::buffer(client_buffer_),
            [self](const boost::system::error_code &error, std::size_t length)
            {
                if (error)
                {
                    self->handleError("read from client", error);
                    return;
                }
                self->routeClientBytes(length);
            });
    }

    void ProxySession::readFromBackend()
    {
        if (closed_)
        {
            return;
        }
        auto self = shared_from_this();
        // 后端方向同样可能发生拆包/粘包；登录阶段 inspectBackendBytes 会观察包边界。
        backend_socket_.async_read_some(
            boost::asio::buffer(backend_buffer_),
            [self](const boost::system::error_code &error, std::size_t length)
            {
                if (error)
                {
                    self->handleError("read from backend", error);
                    return;
                }
                self->inspectBackendBytes(length);
                // 无论是否观察到握手状态变化，后端原始字节都必须完整发给客户端。
                self->writeToClient(length);
            });
    }

    void ProxySession::writeToBackend(std::size_t length)
    {
        // client_buffer_ 是成员变量，能保证 async_write 完成前内存仍然有效。
        auto self = shared_from_this();
        boost::asio::async_write(
            backend_socket_,
            boost::asio::buffer(client_buffer_.data(), length),
            [self](const boost::system::error_code &error, std::size_t bytes_transferred)
            {
                if (error)
                {
                    self->handleError("write to backend", error);
                    return;
                }
                std::cout << "[DEBUG] client -> backend: " << bytes_transferred << " bytes (" << self->client_name_ << ")\n";
                // 同一方向始终保持“一次读 -> 一次写 -> 下一次读”，避免覆盖缓冲区。
                self->readFromClient();
            });
    }

    void ProxySession::writePacketToBackend(const protocol::MysqlPacket &packet)
    {
        // 命令阶段已经拆成 MysqlPacket，因此先恢复包头再发送给后端。
        // 原 packet 是局部变量，不能直接给 async_write；编码结果保存在成员中。
        backend_write_buffer_ = protocol::encodeMysqlPacket(packet);

        auto self = shared_from_this();
        boost::asio::async_write(
            backend_socket_,
            boost::asio::buffer(backend_write_buffer_),
            [self](const boost::system::error_code &error, std::size_t bytes_transferred)
            {
                if (error)
                {
                    self->handleError("write packet to backend", error);
                    return;
                }

                std::cout << "[DEBUG] client packet -> backend: "
                          << bytes_transferred << " bytes (" << self->client_name_ << ")\n";
                // 一个包发送完成后才处理队列下一项，保护 backend_write_buffer_。
                self->processNextClientPacket();
            });
    }

    void ProxySession::writeLocalResponse()
    {
        // local_response_buffer_ 可能包含一个 OK/ERR 包，也可能包含整个结果集的多个包。
        auto self = shared_from_this();
        boost::asio::async_write(
            client_socket_,
            boost::asio::buffer(local_response_buffer_),
            [self](const boost::system::error_code &error, std::size_t bytes_transferred)
            {
                if (error)
                {
                    self->handleError("write local FN response", error);
                    return;
                }

                std::cout << "[DEBUG] local FN response -> client: "
                          << bytes_transferred << " bytes (" << self->client_name_ << ")\n";
                // 本地响应写完后继续处理客户端队列，不需要等待后端 MySQL。
                self->processNextClientPacket();
            });
    }

    void ProxySession::writeToClient(std::size_t length)
    {
        auto self = shared_from_this();
        boost::asio::async_write(
            client_socket_,
            boost::asio::buffer(backend_buffer_.data(), length),
            [self](const boost::system::error_code &error, std::size_t bytes_transferred)
            {
                if (error)
                {
                    self->handleError("write to client", error);
                    return;
                }
                std::cout << "[DEBUG] backend -> client: " << bytes_transferred << " bytes (" << self->client_name_ << ")\n";
                // 当前后端数据已转发完成，可以安全复用 backend_buffer_ 继续读取。
                self->readFromBackend();
            });
    }

    void ProxySession::routeClientBytes(std::size_t length)
    {
        // 登录尚未完成时，字节可能是握手响应或认证交换；TLS/压缩时更无法
        // 查看明文 SQL。这两种情况只观察必要信息，然后原样转发。
        if (!command_phase_ || inspection_disabled_)
        {
            inspectClientBytes(length);
            writeToBackend(length);
            return;
        }

        // 明文命令阶段先从 TCP 字节流中提取完整 MySQL 包。
        for (protocol::MysqlPacket &packet :
             client_packet_decoder_.feed(client_buffer_.data(), length))
        {
            pending_client_packets_.push_back(std::move(packet));
        }
        processNextClientPacket();
    }

    void ProxySession::inspectClientBytes(std::size_t length)
    {
        // 已确认 TLS/压缩后，不再让 Classic Protocol 解码器接触后续字节。
        if (inspection_disabled_)
        {
            return;
        }

        for (const auto &packet : client_packet_decoder_.feed(client_buffer_.data(), length))
        {
            if (!command_phase_)
            {
                // HandshakeResponse41 的前四字节是客户端能力位。SSLRequest 也是
                // 32 字节的特殊握手响应；它之后的 TLS 记录已经不是 MySQL 明文包。
                if (backend_handshake_seen_ && !client_capabilities_seen_ &&
                    packet.payload.size() >= 4)
                {
                    client_capabilities_ =
                        static_cast<std::uint32_t>(packet.payload[0]) |
                        (static_cast<std::uint32_t>(packet.payload[1]) << 8U) |
                        (static_cast<std::uint32_t>(packet.payload[2]) << 16U) |
                        (static_cast<std::uint32_t>(packet.payload[3]) << 24U);
                    client_capabilities_seen_ = true;

                    // 仅声明 CLIENT_SSL 不够；32 字节长度才表示这确实是 SSLRequest。
                    if (packet.payload.size() == 32 &&
                        (client_capabilities_ & client_ssl) != 0)
                    {
                        inspection_disabled_ = true;
                        client_packet_decoder_.reset();
                        backend_packet_decoder_.reset();
                        std::cout << "[INFO] TLS requested; SQL inspection disabled for "
                                  << client_name_ << '\n';
                        return;
                    }

                    // 压缩协议会重新定义包头，当前 MysqlPacketDecoder 无法继续使用。
                    if ((client_capabilities_ & client_compress) != 0)
                    {
                        inspection_disabled_ = true;
                        client_packet_decoder_.reset();
                        backend_packet_decoder_.reset();
                        std::cout << "[INFO] Compression requested; SQL inspection disabled for "
                                  << client_name_ << '\n';
                        return;
                    }
                }
                continue;
            }
        }
    }

    void ProxySession::inspectBackendBytes(std::size_t length)
    {
        // 登录完成后，后端响应无需本地解析；它们直接转发给客户端。
        if (inspection_disabled_ || command_phase_)
        {
            return;
        }

        for (const auto &packet : backend_packet_decoder_.feed(backend_buffer_.data(), length))
        {
            if (!backend_handshake_seen_)
            {
                // 建立 TCP 后服务端发送的第一个完整包就是 Initial Handshake。
                backend_handshake_seen_ = true;
                continue;
            }

            // 登录最终 OK 包以 0x00 开头。AuthSwitch(0xfe) 和
            // AuthMoreData(0x01) 不会误触发，因此多轮认证仍能继续透明转发。
            if (!packet.payload.empty() && packet.payload.front() == 0x00)
            {
                command_phase_ = true;
                backend_packet_decoder_.reset();
                std::cout << "[INFO] MySQL authentication complete; SQL inspection enabled for "
                          << client_name_ << '\n';
                return;
            }
        }
    }

    void ProxySession::processNextClientPacket()
    {
        if (pending_client_packets_.empty())
        {
            // 当前批次已处理完，再挂起下一次读取。
            readFromClient();
            return;
        }

        protocol::MysqlPacket packet = std::move(pending_client_packets_.front());
        pending_client_packets_.pop_front();

        // 本地命令已经向客户端发起异步响应，完成回调会继续消费队列。
        if (handleQueryLocally(packet))
        {
            return;
        }

        // 非 FN 命令保持原 sequence id 和 payload，重新编码包头后交给 MySQL。
        writePacketToBackend(packet);
    }

    bool ProxySession::handleQueryLocally(const protocol::MysqlPacket &packet)
    {
        // 同时处理普通 COM_QUERY 和带空 CLIENT_QUERY_ATTRIBUTES 前缀的形式。
        const std::optional<std::string_view> sql_text =
            queryText(packet, client_capabilities_);
        if (!sql_text.has_value())
        {
            return false;
        }

        const sql::ParseResult result = sql::parse(*sql_text);
        // 服务端响应序号必须接在客户端请求序号之后；uint8_t 转换自然按 256 回绕。
        const std::uint8_t response_sequence_id =
            static_cast<std::uint8_t>(packet.sequence_id + 1U);

        if (result.status == sql::ParseStatus::success)
        {
            std::cout << "[INFO] Parsed FN SQL from " << client_name_ << ": "
                      << sql::describe(*result.statement) << '\n';

            if (const auto *set_mode =
                    std::get_if<sql::SetModeStatement>(&*result.statement))
            {
                // SET 只修改当前 ProxySession 的内存状态，并返回一个 OK 包。
                nf_mode_ = set_mode->mode;
                local_response_buffer_ =
                    protocol::makeMysqlOkResponse(response_sequence_id);
            }
            else
            {
                // SHOW 将当前状态编码为客户端能够直接显示的一列一行结果集。
                local_response_buffer_ = protocol::makeMysqlSingleColumnResultSet(
                    response_sequence_id,
                    "NF_MODE",
                    sql::nfModeName(nf_mode_),
                    (client_capabilities_ & client_deprecate_eof) != 0,
                    (client_capabilities_ & client_optional_resultset_metadata) != 0);
            }

            writeLocalResponse();
            return true;
        }

        if (result.status == sql::ParseStatus::error)
        {
            // 只有“看起来像 FN SQL 但语法非法”的输入到这里；普通 SQL 不会拦截。
            std::cerr << "[WARN] Invalid FN SQL from " << client_name_ << " at byte "
                      << result.error_offset << ": " << result.error_message << '\n';

            local_response_buffer_ = protocol::makeMysqlErrorResponse(
                response_sequence_id,
                parse_error_code,
                "42000",
                result.error_message);
            writeLocalResponse();
            return true;
        }

        // not_fn_statement：由调用者继续转发给真实 MySQL。
        return false;
    }

    void ProxySession::handleError(const char *operation, const boost::system::error_code &error)
    {
        // 客户端和后端可能同时报错；closed_ 防止重复日志和重复关闭。
        if (closed_)
        {
            return;
        }
        if (error == boost::asio::error::eof)
        {
            // EOF 表示对端正常关闭 TCP，不属于协议错误。
            std::cout << "[INFO] " << operation << ": peer closed the connection (" << client_name_ << ")\n";
        }
        else
        {
            std::cerr << "[ERROR] " << operation << " failed for " << client_name_ << ": " << error.message() << '\n';
        }
        close();
    }

    void ProxySession::close()
    {
        if (closed_)
        {
            return;
        }
        closed_ = true;

        // 取消还没完成的 DNS 解析，相关异步回调会收到 operation_aborted。
        resolver_.cancel();

        // shutdown/close 都使用 error_code 重载：清理阶段不再抛异常。
        boost::system::error_code ignored_error;
        client_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
        client_socket_.close(ignored_error);
        backend_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
        backend_socket_.close(ignored_error);

        std::cout << "[INFO] Client disconnected: " << client_name_ << '\n';
    }

} // namespace fn
