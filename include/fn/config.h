#pragma once

#include <cstdint>
#include <string>

namespace fn
{
    struct Config
    {
        std::string listen_host = "0.0.0.0";
        // 端口号是 0~65535，uint16_t 是16位无符号整数
        std::uint16_t listen_port = 13306;

        std::string mysql_host = "127.0.0.1";
        std::uint16_t mysql_port = 3306;
    };

    // [[nodiscard]] 这个函数的返回值不应该被随便丢掉
    [[nodiscard]] bool helpRequested(int argc, char *argv[]);
    [[nodiscard]] Config parseCommandLine(int argc, char *argv[]);
    [[nodiscard]] std::string commandLineUsage(const char *program_name);

} // namespace fn
