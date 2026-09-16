#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr std::uint32_t kMaxPacketSize = 64u * 1024u * 1024u;
constexpr std::uint16_t kClientSsl = 0x0800;  // CLIENT_SSL capability flag.

struct MySQLPacket {
    std::uint8_t sequence_id{};
    std::vector<std::uint8_t> payload;
};

bool read_exact(int fd, void* buffer, std::size_t size) {
    auto* p = static_cast<std::uint8_t*>(buffer);
    std::size_t done = 0;
    while (done < size) {
        const ssize_t n = ::recv(fd, p + done, size - done, 0);
        if (n == 0) return false;
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        done += static_cast<std::size_t>(n);
    }
    return true;
}

bool write_exact(int fd, const void* buffer, std::size_t size) {
    const auto* p = static_cast<const std::uint8_t*>(buffer);
    std::size_t done = 0;
    while (done < size) {
        const ssize_t n = ::send(fd, p + done, size - done, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        done += static_cast<std::size_t>(n);
    }
    return true;
}

bool read_packet(int fd, MySQLPacket& packet) {
    std::uint8_t header[4];
    if (!read_exact(fd, header, sizeof(header))) return false;

    const std::uint32_t payload_length =
        static_cast<std::uint32_t>(header[0]) |
        (static_cast<std::uint32_t>(header[1]) << 8u) |
        (static_cast<std::uint32_t>(header[2]) << 16u);

    if (payload_length > kMaxPacketSize) {
        std::cerr << "[proxy] packet too large: " << payload_length << " bytes\n";
        return false;
    }

    packet.sequence_id = header[3];
    packet.payload.resize(payload_length);
    return payload_length == 0 || read_exact(fd, packet.payload.data(), payload_length);
}

bool write_packet(int fd, const MySQLPacket& packet) {
    const std::uint32_t len = static_cast<std::uint32_t>(packet.payload.size());
    std::uint8_t header[4] = {
        static_cast<std::uint8_t>(len & 0xffu),
        static_cast<std::uint8_t>((len >> 8u) & 0xffu),
        static_cast<std::uint8_t>((len >> 16u) & 0xffu),
        packet.sequence_id,
    };
    return write_exact(fd, header, sizeof(header)) &&
           (packet.payload.empty() || write_exact(fd, packet.payload.data(), packet.payload.size()));
}

std::string trim_left(std::string s) {
    auto it = std::find_if_not(s.begin(), s.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    s.erase(s.begin(), it);
    return s;
}

std::string first_keyword_upper(const std::string& sql) {
    const std::string trimmed = trim_left(sql);
    std::string keyword;
    for (unsigned char c : trimmed) {
        if (std::isalnum(c) == 0 && c != '_') break;
        keyword.push_back(static_cast<char>(std::toupper(c)));
    }
    return keyword;
}

bool is_intercepted_fn_query(const MySQLPacket& packet, std::string* sql_out = nullptr) {
    // MySQL COM_QUERY command byte is 0x03.
    if (packet.payload.size() < 2 || packet.payload[0] != 0x03) return false;

    std::string sql(reinterpret_cast<const char*>(packet.payload.data() + 1),
                    packet.payload.size() - 1);
    if (sql_out) *sql_out = sql;

    // The project historically uses both "NF" (normal form DSL) and "FN" naming.
    // Intercept either keyword in this minimal demo.
    const std::string keyword = first_keyword_upper(sql);
    return keyword == "NF" || keyword == "FN";
}

MySQLPacket make_error_packet(std::uint8_t sequence_id, const std::string& message) {
    // MySQL ERR_Packet (Protocol::ERR):
    // 0xff + 2-byte error code + '#' + 5-byte SQLSTATE + message
    // Use ER_UNKNOWN_ERROR = 1105, SQLSTATE HY000.
    constexpr std::uint16_t error_code = 1105;
    const std::string sql_state = "HY000";

    MySQLPacket packet;
    packet.sequence_id = sequence_id;
    packet.payload.reserve(1 + 2 + 1 + 5 + message.size());
    packet.payload.push_back(0xff);
    packet.payload.push_back(static_cast<std::uint8_t>(error_code & 0xffu));
    packet.payload.push_back(static_cast<std::uint8_t>((error_code >> 8u) & 0xffu));
    packet.payload.push_back('#');
    packet.payload.insert(packet.payload.end(), sql_state.begin(), sql_state.end());
    packet.payload.insert(packet.payload.end(), message.begin(), message.end());
    return packet;
}

bool disable_ssl_in_handshake(MySQLPacket& packet) {
    // Protocol::HandshakeV10 layout:
    // protocol(1), server_version(NUL), connection_id(4), auth_data_1(8), filler(1),
    // capability_flags_1(2), ...
    if (packet.payload.size() < 16 || packet.payload[0] != 0x0a) return false;

    std::size_t pos = 1;
    while (pos < packet.payload.size() && packet.payload[pos] != 0) ++pos;
    if (pos >= packet.payload.size()) return false;
    ++pos;  // NUL terminator after server version.

    constexpr std::size_t fixed_before_caps = 4 + 8 + 1;
    if (pos + fixed_before_caps + 2 > packet.payload.size()) return false;
    pos += fixed_before_caps;

    std::uint16_t caps = static_cast<std::uint16_t>(packet.payload[pos]) |
                         (static_cast<std::uint16_t>(packet.payload[pos + 1]) << 8u);
    const bool had_ssl = (caps & kClientSsl) != 0;
    caps = static_cast<std::uint16_t>(caps & ~kClientSsl);
    packet.payload[pos] = static_cast<std::uint8_t>(caps & 0xffu);
    packet.payload[pos + 1] = static_cast<std::uint8_t>((caps >> 8u) & 0xffu);
    return had_ssl;
}

int connect_tcp(const std::string& host, std::uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const std::string port_str = std::to_string(port);
    const int rc = ::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
    if (rc != 0) {
        std::cerr << "[proxy] getaddrinfo(" << host << "): " << gai_strerror(rc) << "\n";
        return -1;
    }

    int fd = -1;
    for (addrinfo* p = result; p != nullptr; p = p->ai_next) {
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
        ::close(fd);
        fd = -1;
    }
    ::freeaddrinfo(result);
    return fd;
}

int create_listener(std::uint16_t port) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int yes = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
        ::listen(fd, 128) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

void shutdown_both(int a, int b) {
    ::shutdown(a, SHUT_RDWR);
    ::shutdown(b, SHUT_RDWR);
}

void handle_connection(int client_fd, std::string backend_host, std::uint16_t backend_port) {
    const int backend_fd = connect_tcp(backend_host, backend_port);
    if (backend_fd < 0) {
        std::cerr << "[proxy] cannot connect backend " << backend_host << ':' << backend_port << "\n";
        ::close(client_fd);
        return;
    }

    std::cout << "[proxy] client connected; backend=" << backend_host << ':' << backend_port << "\n";

    std::mutex client_write_mutex;
    std::atomic<bool> closed{false};

    std::thread server_to_client([&] {
        bool first_packet = true;
        MySQLPacket packet;
        while (read_packet(backend_fd, packet)) {
            if (first_packet) {
                if (disable_ssl_in_handshake(packet)) {
                    std::cout << "[proxy] backend advertised TLS; CLIENT_SSL cleared so SQL stays inspectable\n";
                }
                first_packet = false;
            }

            std::lock_guard<std::mutex> lock(client_write_mutex);
            if (!write_packet(client_fd, packet)) break;
        }
        closed = true;
        shutdown_both(client_fd, backend_fd);
    });

    MySQLPacket packet;
    while (!closed && read_packet(client_fd, packet)) {
        std::string sql;
        if (is_intercepted_fn_query(packet, &sql)) {
            std::cout << "[INTERCEPT] " << sql << "\n";
            MySQLPacket err = make_error_packet(
                static_cast<std::uint8_t>(packet.sequence_id + 1u),
                "[FN Proxy] NF/FN statement intercepted by middleware demo");

            std::lock_guard<std::mutex> lock(client_write_mutex);
            if (!write_packet(client_fd, err)) break;
            continue;
        }

        if (!packet.payload.empty() && packet.payload[0] == 0x03) {
            const std::string sql_text(
                reinterpret_cast<const char*>(packet.payload.data() + 1),
                packet.payload.size() - 1);
            std::cout << "[PASS] " << sql_text << "\n";
        }

        if (!write_packet(backend_fd, packet)) break;
    }

    closed = true;
    shutdown_both(client_fd, backend_fd);
    if (server_to_client.joinable()) server_to_client.join();
    ::close(client_fd);
    ::close(backend_fd);
    std::cout << "[proxy] client disconnected\n";
}

std::uint16_t parse_port(const char* s, std::uint16_t fallback) {
    try {
        const int p = std::stoi(s);
        if (p > 0 && p <= 65535) return static_cast<std::uint16_t>(p);
    } catch (...) {
    }
    return fallback;
}

}  // namespace

int main(int argc, char** argv) {
    std::uint16_t listen_port = 3307;
    std::string backend_host = "127.0.0.1";
    std::uint16_t backend_port = 3306;

    if (argc >= 2) listen_port = parse_port(argv[1], listen_port);
    if (argc >= 3) backend_host = argv[2];
    if (argc >= 4) backend_port = parse_port(argv[3], backend_port);

    const int listen_fd = create_listener(listen_port);
    if (listen_fd < 0) {
        std::cerr << "[proxy] failed to listen on port " << listen_port
                  << ": " << std::strerror(errno) << "\n";
        return 1;
    }

    std::cout << "FN MySQL Proxy demo\n"
              << "  listen : 0.0.0.0:" << listen_port << '\n'
              << "  backend: " << backend_host << ':' << backend_port << '\n'
              << "  rule   : COM_QUERY beginning with NF or FN => reject\n"
              << "           all other MySQL packets => transparent pass-through\n";

    while (true) {
        sockaddr_storage client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = ::accept(
            listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            std::cerr << "[proxy] accept failed: " << std::strerror(errno) << "\n";
            break;
        }

        std::thread(handle_connection, client_fd, backend_host, backend_port).detach();
    }

    ::close(listen_fd);
    return 0;
}
