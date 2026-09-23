#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fn::protocol {

struct MysqlPacket {
    std::uint8_t sequence_id = 0;
    std::vector<std::uint8_t> payload;
};

// TCP does not preserve MySQL packet boundaries. This decoder accepts arbitrary
// byte fragments and emits packets only after their four-byte header and full
// payload have arrived.
class MysqlPacketDecoder {
public:
    [[nodiscard]] std::vector<MysqlPacket> feed(const char* data, std::size_t size);
    void reset();

private:
    std::vector<std::uint8_t> buffer_;
    std::size_t offset_ = 0;
};

}  // namespace fn::protocol
