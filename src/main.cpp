#include "fn/config.h"
#include "fn/proxy_server.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <csignal>
#include <exception>
#include <iostream>

int main(int argc, char *argv[])
{
    // 打印帮助信息 --help
    if (fn::helpRequested(argc, argv))
    {
        std::cout << fn::commandLineUsage(argv[0]);
        return 0;
    }

    try
    {
        // 读取启动程序时的输入参数，后续用 gflags 代替
        const fn::Config config = fn::parseCommandLine(argc, argv);

        // 创建一个网络任务管理器
        boost::asio::io_context io_context;
        // 创建代理服务器（中间件）
        fn::ProxyServer server(io_context, config);

        // 监听程序退出信号，比如按下 Ctrl + C，然后把服务器停掉
        // 创建一个 signal 信号，专门监听两个系统信号：SIGINT：Ctrl + C，SIGTERM：kill pid
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const boost::system::error_code &error, int signal)
                           {
            if (!error) {
                std::cout << "[INFO] Received signal " << signal << ", stopping\n";
                io_context.stop();
            } });

        // 开始监听客户端连接
        server.start();
        // 程序持续运行，等待网络事件
        io_context.run();
    }
    catch (const std::exception &error)
    {
        std::cerr << "[ERROR] " << error.what() << '\n';
        std::cerr << fn::commandLineUsage(argv[0]);
        return 1;
    }

    return 0;
}
