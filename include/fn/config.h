#pragma once

#include <cstdint>
#include <string>

namespace fn
{
    // 程序启动配置。这里保存值而不是引用，确保命令行解析结束后数据仍然有效。
    struct Config
    {
        // 代理对 MySQL 客户端开放的监听地址和端口。
        std::string listen_host = "0.0.0.0";
        // TCP 端口占 16 位；本项目额外禁止保留值 0。
        std::uint16_t listen_port = 13306;

        // 代理需要连接的真实 MySQL 地址和端口。
        std::string mysql_host = "127.0.0.1";
        std::uint16_t mysql_port = 3306;
    };

    // 扫描 argv 中是否包含 -h/--help。[[nodiscard]] 提醒调用者不要忽略结果。
    [[nodiscard]] bool helpRequested(int argc, char *argv[]);

    // 将命令行选项转换为强类型 Config；参数非法时抛出 std::invalid_argument。
    [[nodiscard]] Config parseCommandLine(int argc, char *argv[]);

    // 生成帮助文本；program_name 通常传 argv[0]。
    [[nodiscard]] std::string commandLineUsage(const char *program_name);

} // namespace fn
