#pragma once

#include "fn/config.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace fn {

// ProxyServer 只负责监听端口和接受新 TCP 连接。
// 每个已接受的连接都会交给一个独立的 ProxySession 管理。
class ProxyServer {
public:
    // io_context 是 Boost.Asio 的事件循环；config 描述监听端和后端 MySQL。
    ProxyServer(boost::asio::io_context& io_context, const Config& config);

    // 输出启动信息并挂起第一次异步 accept。
    void start();

private:
    // 发起一次 async_accept；回调结束前会再次调用自己，形成持续监听循环。
    void doAccept();

    // 引用 main 中的事件循环。它的生命周期长于 ProxyServer，因此无需拷贝。
    boost::asio::io_context& io_context_;
    // acceptor 封装“打开、绑定、监听、接受”服务器端 TCP socket 的操作。
    boost::asio::ip::tcp::acceptor acceptor_;
    // 每个新会话都需要后端地址，因此服务器保存一份配置副本。
    Config config_;
};

}  // namespace fn
