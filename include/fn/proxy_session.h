#pragma once

#include "fn/config.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <string>

namespace fn {

class ProxySession : public std::enable_shared_from_this<ProxySession> {
public:
    ProxySession(boost::asio::ip::tcp::socket client_socket,
                 boost::asio::io_context& io_context,
                 const Config& config);

    void start();

private:
    void connectBackend();

    void readFromClient();
    void readFromBackend();

    void writeToBackend(std::size_t length);
    void writeToClient(std::size_t length);

    void handleError(const char* operation, const boost::system::error_code& error);
    void close();

    static constexpr std::size_t buffer_size = 8192;

    boost::asio::ip::tcp::socket client_socket_;
    boost::asio::ip::tcp::socket backend_socket_;
    boost::asio::ip::tcp::resolver resolver_;

    std::array<char, buffer_size> client_buffer_{};
    std::array<char, buffer_size> backend_buffer_{};

    Config config_;
    std::string client_name_;
    bool closed_ = false;
};

}  // namespace fn
