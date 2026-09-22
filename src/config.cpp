#include "fn/config.h"

#include <charconv>
#include <stdexcept>
#include <string_view>

namespace fn
{
    namespace
    {
        // 字符串端口转换成数字
        std::uint16_t parsePort(std::string_view text, std::string_view option_name)
        {
            unsigned int value = 0;
            const char *const begin = text.data();
            const char *const end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (text.empty() || result.ec != std::errc{} || result.ptr != end || value == 0 || value > 65535)
            {
                throw std::invalid_argument("invalid port for " + std::string(option_name) + ": " + std::string(text));
            }
            return static_cast<std::uint16_t>(value);
        }

        std::string_view nextValue(int argc, char *argv[], int &index, std::string_view option_name)
        {
            if (index + 1 >= argc)
            {
                throw std::invalid_argument("missing value for " + std::string(option_name));
            }
            ++index;
            return argv[index];
        }

    } // namespace

    bool helpRequested(int argc, char *argv[])
    {
        for (int index = 1; index < argc; ++index)
        {
            const std::string_view argument(argv[index]);
            if (argument == "--help" || argument == "-h")
            {
                return true;
            }
        }
        return false;
    }

    Config parseCommandLine(int argc, char *argv[])
    {
        Config config;

        for (int index = 1; index < argc; ++index)
        {
            const std::string_view argument(argv[index]);

            if (argument == "--listen-host")
            {
                config.listen_host = nextValue(argc, argv, index, argument);
                if (config.listen_host.empty())
                {
                    throw std::invalid_argument("--listen-host cannot be empty");
                }
            }
            else if (argument == "--listen-port")
            {
                config.listen_port = parsePort(nextValue(argc, argv, index, argument), argument);
            }
            else if (argument == "--mysql-host")
            {
                config.mysql_host = nextValue(argc, argv, index, argument);
                if (config.mysql_host.empty())
                {
                    throw std::invalid_argument("--mysql-host cannot be empty");
                }
            }
            else if (argument == "--mysql-port")
            {
                config.mysql_port = parsePort(nextValue(argc, argv, index, argument), argument);
            }
            else
            {
                throw std::invalid_argument("unknown argument: " + std::string(argument));
            }
        }

        return config;
    }

    std::string commandLineUsage(const char *program_name)
    {
        const std::string executable = program_name == nullptr ? "fn_proxy" : program_name;
        return "Usage: " + executable + " [options]\n"
                                        "  --listen-host <address>  Listen address (default: 0.0.0.0)\n"
                                        "  --listen-port <port>     Listen port (default: 13306)\n"
                                        "  --mysql-host <host>      MySQL server host (default: 127.0.0.1)\n"
                                        "  --mysql-port <port>      MySQL server port (default: 3306)\n"
                                        "  -h, --help               Show this help\n";
    }

} // namespace fn
