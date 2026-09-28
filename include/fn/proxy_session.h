#pragma once

#include "fn/config.h"
#include "fn/protocol/mysql_packet_codec.h"
#include "fn/sql/parser.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace fn {

class ProxySession : public std::enable_shared_from_this<ProxySession> {
public:
    ProxySession(boost::asio::ip::tcp::socket client_socket,
                 boost::asio::io_context& io_context,
                 const Config& config);

    void start();

private:
    void connectBackend();

    void readFromClient();
    void readFromBackend();

    void writeToBackend(std::size_t length);
    void writeToClient(std::size_t length);
    void writePacketToBackend(const protocol::MysqlPacket& packet);
    void writeLocalResponse();

    void routeClientBytes(std::size_t length);
    void inspectClientBytes(std::size_t length);
    void inspectBackendBytes(std::size_t length);
    void processNextClientPacket();
    [[nodiscard]] bool handleQueryLocally(const protocol::MysqlPacket& packet);

    void handleError(const char* operation, const boost::system::error_code& error);
    void close();

    static constexpr std::size_t buffer_size = 8192;

    boost::asio::ip::tcp::socket client_socket_;
    boost::asio::ip::tcp::socket backend_socket_;
    boost::asio::ip::tcp::resolver resolver_;

    std::array<char, buffer_size> client_buffer_{};
    std::array<char, buffer_size> backend_buffer_{};

    Config config_;
    std::string client_name_;
    protocol::MysqlPacketDecoder client_packet_decoder_;
    protocol::MysqlPacketDecoder backend_packet_decoder_;
    std::deque<protocol::MysqlPacket> pending_client_packets_;
    std::vector<std::uint8_t> backend_write_buffer_;
    std::vector<std::uint8_t> local_response_buffer_;
    sql::NFMode nf_mode_ = sql::NFMode::off;
    std::uint32_t client_capabilities_ = 0;
    bool backend_handshake_seen_ = false;
    bool client_capabilities_seen_ = false;
    bool command_phase_ = false;
    bool inspection_disabled_ = false;
    bool closed_ = false;
};

}  // namespace fn
