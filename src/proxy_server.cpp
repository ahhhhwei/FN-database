#include "fn/proxy_server.h"

#include "fn/proxy_session.h"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <memory>
#include <utility>

namespace fn
{
    ProxyServer::ProxyServer(boost::asio::io_context &io_context, const Config &config)
        : io_context_(io_context), acceptor_(io_context), config_(config)
    {
        // 字符串 IP 转地址
        const auto listen_address = boost::asio::ip::make_address(config_.listen_host);
        // endpoint = ip + port
        const boost::asio::ip::tcp::endpoint endpoint(listen_address, config_.listen_port);

        // 创建 TCP socket
        acceptor_.open(endpoint.protocol());
        // 设置端口复用
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        // 将 endpoint 绑定到 socket
        acceptor_.bind(endpoint);
        // 监听 socket
        acceptor_.listen(boost::asio::socket_base::max_listen_connections);
    }

    void ProxyServer::start()
    {
        std::cout << "[INFO] Proxy listening on " << config_.listen_host << ':'
                  << config_.listen_port << '\n';
        std::cout << "[INFO] MySQL backend is " << config_.mysql_host << ':' << config_.mysql_port
                  << '\n';
        doAccept();
    }

    void ProxyServer::doAccept()
    {
        // 异步等待客户端连接
        acceptor_.async_accept([this](const boost::system::error_code &error, boost::asio::ip::tcp::socket client_socket)
                               {
        if (!error) {
            std::make_shared<ProxySession>(std::move(client_socket), io_context_, config_)->start();
        }
        else if (error != boost::asio::error::operation_aborted) {
            std::cerr << "[ERROR] Accept failed: " << error.message() << '\n';
        }

        if (acceptor_.is_open()) {
            doAccept();
        } });
    }

} // namespace fn
