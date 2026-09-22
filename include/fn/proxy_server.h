#pragma once

#include "fn/config.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace fn {

class ProxyServer {
public:
    ProxyServer(boost::asio::io_context& io_context, const Config& config);

    void start();

private:
    void doAccept();

    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    Config config_;
};

}  // namespace fn
