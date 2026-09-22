#include "fn/proxy_session.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <string>
#include <utility>

namespace fn
{
    namespace
    {
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

    void ProxySession::handleError(const char *operation, const boost::system::error_code &error)
    {
        if (closed_)
        {
            return;
        }
        if (error == boost::asio::error::eof)
        {
            std::cout << "[INFO] " << operation << ": peer closed the connection (" << client_name_
                      << ")\n";
        }
        else
        {
            std::cerr << "[ERROR] " << operation << " failed for " << client_name_ << ": "
                      << error.message() << '\n';
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
