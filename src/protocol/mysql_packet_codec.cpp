#include "fn/protocol/mysql_packet_codec.h"

#include <utility>

namespace fn::protocol {
namespace {

constexpr std::size_t mysql_header_size = 4;

std::size_t payloadLength(const std::uint8_t* header)
{
    return static_cast<std::size_t>(header[0]) |
           (static_cast<std::size_t>(header[1]) << 8U) |
           (static_cast<std::size_t>(header[2]) << 16U);
}

}  // namespace

std::vector<MysqlPacket> MysqlPacketDecoder::feed(const char* data, std::size_t size)
{
    if (size != 0) {
        const auto* begin = reinterpret_cast<const std::uint8_t*>(data);
        buffer_.insert(buffer_.end(), begin, begin + size);
    }

    std::vector<MysqlPacket> packets;
    while (buffer_.size() - offset_ >= mysql_header_size) {
        const auto* header = buffer_.data() + offset_;
        const std::size_t payload_size = payloadLength(header);
        const std::size_t packet_size = mysql_header_size + payload_size;
        if (buffer_.size() - offset_ < packet_size) {
            break;
        }

        MysqlPacket packet;
        packet.sequence_id = header[3];
        packet.payload.assign(header + mysql_header_size, header + packet_size);
        packets.push_back(std::move(packet));
        offset_ += packet_size;
    }

    if (offset_ == buffer_.size()) {
        buffer_.clear();
        offset_ = 0;
    }
    else if (offset_ != 0 && offset_ >= buffer_.size() / 2) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(offset_));
        offset_ = 0;
    }

    return packets;
}

void MysqlPacketDecoder::reset()
{
    buffer_.clear();
    offset_ = 0;
}

}  // namespace fn::protocol
