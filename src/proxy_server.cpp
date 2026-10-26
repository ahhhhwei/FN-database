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
        // make_address 只接受数字形式 IPv4/IPv6；非法输入会抛出异常并由 main 捕获。
        const auto listen_address = boost::asio::ip::make_address(config_.listen_host);

        // endpoint 是“IP 地址 + TCP 端口”的组合。
        const boost::asio::ip::tcp::endpoint endpoint(listen_address, config_.listen_port);

        // acceptor 的典型初始化顺序：open -> set_option -> bind -> listen。
        acceptor_.open(endpoint.protocol());
        // 允许程序重启后尽快重新绑定仍处于 TIME_WAIT 相关状态的监听端口。
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        // 使用系统允许的最大等待连接队列长度。
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
        // async_accept 立即返回。连接到来后，io_context 才执行这个 lambda。
        // lambda 捕获 this，因此 ProxyServer 必须比所有挂起的 accept 操作活得更久。
        acceptor_.async_accept([this](const boost::system::error_code &error, boost::asio::ip::tcp::socket client_socket)
                               {
        if (!error) {
            // make_shared 创建会话；start 内部的异步回调会继续持有 shared_ptr。
            std::make_shared<ProxySession>(std::move(client_socket), io_context_, config_)->start();
        }
        else if (error != boost::asio::error::operation_aborted) {
            // 服务器正常关闭 acceptor 时会得到 operation_aborted，不需要当成故障打印。
            std::cerr << "[ERROR] Accept failed: " << error.message() << '\n';
        }

        // 每次 async_accept 只接受一个连接，所以回调末尾继续挂起下一次等待。
        if (acceptor_.is_open()) {
            doAccept();
        } });
    }

} // namespace fn
