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
        constexpr std::uint8_t com_query = 0x03;
        constexpr std::uint32_t client_compress = 0x00000020U;
        constexpr std::uint32_t client_ssl = 0x00000800U;
        constexpr std::uint32_t client_deprecate_eof = 0x01000000U;
        constexpr std::uint32_t client_optional_resultset_metadata = 0x02000000U;
        constexpr std::uint32_t client_query_attributes = 0x08000000U;
        constexpr std::uint16_t parse_error_code = 1064U;

        std::string endpointName(const boost::asio::ip::tcp::endpoint &endpoint)
        {
            return endpoint.address().to_string() + ':' + std::to_string(endpoint.port());
        }

        bool readLengthEncodedInteger(const std::vector<std::uint8_t> &payload,
                                      std::size_t &offset,
                                      std::uint64_t &value)
        {
            if (offset >= payload.size())
            {
                return false;
            }

            const std::uint8_t first = payload[offset++];
            if (first < 0xfbU)
            {
                value = first;
                return true;
            }

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
                value |= static_cast<std::uint64_t>(payload[offset++]) <<
                         (index * 8U);
            }
            return true;
        }

        std::optional<std::string_view> queryText(
            const protocol::MysqlPacket &packet,
            std::uint32_t client_capabilities)
        {
            if (packet.payload.empty() || packet.payload.front() != com_query)
            {
                return std::nullopt;
            }

            std::size_t query_offset = 1;
            if ((client_capabilities & client_query_attributes) != 0)
            {
                std::uint64_t parameter_count = 0;
                std::uint64_t parameter_set_count = 0;
                if (!readLengthEncodedInteger(
                        packet.payload, query_offset, parameter_count) ||
                    !readLengthEncodedInteger(
                        packet.payload, query_offset, parameter_set_count))
                {
                    return std::nullopt;
                }

                // The mysql CLI sends this two-field prefix even when no query
                // attributes are present. Non-empty attribute sets contain typed
                // binary values; leave those packets untouched for MySQL.
                if (parameter_count != 0 || parameter_set_count != 1)
                {
                    return std::nullopt;
                }
            }

            const auto *sql_data = reinterpret_cast<const char *>(
                packet.payload.data() + query_offset);
            return std::string_view(sql_data, packet.payload.size() - query_offset);
        }

    } // namespace

    ProxySession::ProxySession(boost::asio::ip::tcp::socket client_socket, boost::asio::io_context &io_context, const Config &config)
        : client_socket_(std::move(client_socket)), backend_socket_(io_context), resolver_(io_context), config_(config)
    {
        boost::system::error_code error;
        // 获取客户端 ip + port
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
        auto self = shared_from_this();
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
                // 连接 MySQL
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
                        // MySQL sends its handshake first, so both directions must start now.
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
        auto self = shared_from_this();
        // 从客户端 socket 读取一些数据，放进 client_buffer_
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
        // 读取 MySQL 返回的数据
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
                // 发送给客户端
                self->writeToClient(length);
            });
    }

    void ProxySession::writeToBackend(std::size_t length)
    {
        auto self = shared_from_this();
        // buffer 数据写入 client
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
                // 等待客户端数据
                self->readFromClient();
            });
    }

    void ProxySession::writePacketToBackend(const protocol::MysqlPacket &packet)
    {
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
                self->processNextClientPacket();
            });
    }

    void ProxySession::writeLocalResponse()
    {
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
                // 等待 MySQL
                self->readFromBackend();
            });
    }

    void ProxySession::routeClientBytes(std::size_t length)
    {
        if (!command_phase_ || inspection_disabled_)
        {
            inspectClientBytes(length);
            writeToBackend(length);
            return;
        }

        for (protocol::MysqlPacket &packet :
             client_packet_decoder_.feed(client_buffer_.data(), length))
        {
            pending_client_packets_.push_back(std::move(packet));
        }
        processNextClientPacket();
    }

    void ProxySession::inspectClientBytes(std::size_t length)
    {
        if (inspection_disabled_)
        {
            return;
        }

        for (const auto &packet : client_packet_decoder_.feed(client_buffer_.data(), length))
        {
            if (!command_phase_)
            {
                // An SSLRequest is a 32-byte handshake response whose capability
                // flags include CLIENT_SSL. Everything after it is encrypted, so
                // this demo inspector must stop looking at the byte stream.
                if (backend_handshake_seen_ && !client_capabilities_seen_ &&
                    packet.payload.size() >= 4)
                {
                    client_capabilities_ =
                        static_cast<std::uint32_t>(packet.payload[0]) |
                        (static_cast<std::uint32_t>(packet.payload[1]) << 8U) |
                        (static_cast<std::uint32_t>(packet.payload[2]) << 16U) |
                        (static_cast<std::uint32_t>(packet.payload[3]) << 24U);
                    client_capabilities_seen_ = true;

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
        if (inspection_disabled_ || command_phase_)
        {
            return;
        }

        for (const auto &packet : backend_packet_decoder_.feed(backend_buffer_.data(), length))
        {
            if (!backend_handshake_seen_)
            {
                backend_handshake_seen_ = true;
                continue;
            }

            // A server OK packet marks the end of the authentication exchange.
            // Auth switch and auth-more-data packets deliberately keep inspection
            // in the authentication phase until the final OK arrives.
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
            readFromClient();
            return;
        }

        protocol::MysqlPacket packet = std::move(pending_client_packets_.front());
        pending_client_packets_.pop_front();

        if (handleQueryLocally(packet))
        {
            return;
        }

        writePacketToBackend(packet);
    }

    bool ProxySession::handleQueryLocally(const protocol::MysqlPacket &packet)
    {
        const std::optional<std::string_view> sql_text =
            queryText(packet, client_capabilities_);
        if (!sql_text.has_value())
        {
            return false;
        }

        const sql::ParseResult result = sql::parse(*sql_text);
        const std::uint8_t response_sequence_id =
            static_cast<std::uint8_t>(packet.sequence_id + 1U);

        if (result.status == sql::ParseStatus::success)
        {
            std::cout << "[INFO] Parsed FN SQL from " << client_name_ << ": "
                      << sql::describe(*result.statement) << '\n';

            if (const auto *set_mode =
                    std::get_if<sql::SetModeStatement>(&*result.statement))
            {
                nf_mode_ = set_mode->mode;
                local_response_buffer_ =
                    protocol::makeMysqlOkResponse(response_sequence_id);
            }
            else
            {
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

        return false;
    }

    void ProxySession::handleError(const char *operation, const boost::system::error_code &error)
    {
        if (closed_)
        {
            return;
        }
        if (error == boost::asio::error::eof)
        {
            std::cout << "[INFO] " << operation << ": peer closed the connection (" << client_name_ << ")\n";
        }
        else
        {
            std::cerr << "[ERROR] " << operation << " failed for " << client_name_ << ": " << error.message() << '\n';
        }
        close();
    }

    // 关闭整个 Session
    void ProxySession::close()
    {
        if (closed_)
        {
            return;
        }
        closed_ = true;

        // 取消还没完成的 DNS 解析
        resolver_.cancel();

        boost::system::error_code ignored_error;
        // 关闭客户端 TCP 读写
        client_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
        // 关闭客户端 socket
        client_socket_.close(ignored_error);
        // 关闭 MySQL TCP 读写
        backend_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
        // 关闭 MySQL socket
        backend_socket_.close(ignored_error);

        std::cout << "[INFO] Client disconnected: " << client_name_ << '\n';
    }

} // namespace fn
