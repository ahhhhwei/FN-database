#include "fn/config.h"
#include "fn/proxy_server.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <csignal>
#include <exception>
#include <iostream>

// 程序入口只负责三件事：解析配置、组装服务器对象、运行 Boost.Asio 事件循环。
// 具体连接和协议逻辑都在 ProxyServer / ProxySession 中，避免 main 过度膨胀。
int main(int argc, char *argv[])
{
    // --help 不需要初始化网络；尽早返回还能避免其他参数不完整时报错。
    if (fn::helpRequested(argc, argv))
    {
        std::cout << fn::commandLineUsage(argv[0]);
        return 0;
    }

    try
    {
        // Config 按值保存字符串和端口，因此离开解析函数后仍然安全。
        const fn::Config config = fn::parseCommandLine(argc, argv);

        // io_context 是 Asio 的事件调度器。async_* 注册任务，run() 实际执行回调。
        boost::asio::io_context io_context;

        // server 保存对 io_context 的引用，所以它必须在 run() 返回前保持存活。
        fn::ProxyServer server(io_context, config);

        // signal_set 把操作系统信号接入同一个异步事件循环：
        // SIGINT 通常来自 Ctrl+C，SIGTERM 通常来自 kill 或服务管理器。
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const boost::system::error_code &error, int signal)
                           {
            if (!error) {
                std::cout << "[INFO] Received signal " << signal << ", stopping\n";
                // stop() 让 run() 尽快返回；随后栈对象析构并关闭其系统资源。
                io_context.stop();
            } });

        // start() 只挂起异步 accept，不会阻塞当前线程。
        server.start();

        // 当前实现只有一个执行 run() 的线程，所以同一 io_context 上的回调串行执行。
        // 没有待处理事件或调用 stop() 后，run() 返回，程序正常结束。
        io_context.run();
    }
    catch (const std::exception &error)
    {
        // 配置解析、地址转换、socket 初始化等同步失败都会到达这里。
        std::cerr << "[ERROR] " << error.what() << '\n';
        std::cerr << fn::commandLineUsage(argv[0]);
        return 1;
    }

    return 0;
}
