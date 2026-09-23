#include "fn/proxy_session.h"

#include "fn/sql/parser.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace fn
{
    namespace
    {
        constexpr std::uint8_t com_query = 0x03;
        constexpr std::uint32_t client_ssl = 0x00000800U;

        std::string endpointName(const boost::asio::ip::tcp::endpoint &endpoint)
        {
            return endpoint.address().to_string() + ':' + std::to_string(endpoint.port());
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
                self->inspectClientBytes(length);
                // 读到的数据发给 MySQL
                self->writeToBackend(length);
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
                if (backend_handshake_seen_ && packet.payload.size() == 32)
                {
                    const std::uint32_t capabilities =
                        static_cast<std::uint32_t>(packet.payload[0]) |
                        (static_cast<std::uint32_t>(packet.payload[1]) << 8U) |
                        (static_cast<std::uint32_t>(packet.payload[2]) << 16U) |
                        (static_cast<std::uint32_t>(packet.payload[3]) << 24U);
                    if ((capabilities & client_ssl) != 0)
                    {
                        inspection_disabled_ = true;
                        client_packet_decoder_.reset();
                        backend_packet_decoder_.reset();
                        std::cout << "[INFO] TLS requested; SQL inspection disabled for "
                                  << client_name_ << '\n';
                        return;
                    }
                }
                continue;
            }

            inspectQuery(packet.payload);
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
                return;
            }
        }
    }

    void ProxySession::inspectQuery(const std::vector<std::uint8_t> &payload)
    {
        if (payload.empty() || payload.front() != com_query)
        {
            return;
        }

        const auto *sql_data = reinterpret_cast<const char *>(payload.data() + 1);
        const std::string_view sql_text(sql_data, payload.size() - 1);
        const sql::ParseResult result = sql::parse(sql_text);
        if (result.status == sql::ParseStatus::success)
        {
            std::cout << "[INFO] Parsed FN SQL from " << client_name_ << ": "
                      << sql::describe(*result.statement) << '\n';
        }
        else if (result.status == sql::ParseStatus::error)
        {
            std::cerr << "[WARN] Invalid FN SQL from " << client_name_ << " at byte "
                      << result.error_offset << ": " << result.error_message << '\n';
        }
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
