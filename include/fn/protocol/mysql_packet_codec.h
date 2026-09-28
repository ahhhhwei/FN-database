#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace fn::protocol
{
    // ┌─────────────── 4 Bytes Header ────────────────┬──── Payload ────┐
    // │ payload length: 3 Bytes │ sequence id: 1 Byte │     ...         │
    // └───────────────────────────────────────────────┴─────────────────┘
    struct MysqlPacket
    {
        std::uint8_t sequence_id = 0;
        std::vector<std::uint8_t> payload;
    };

    // TCP does not preserve MySQL packet boundaries. This decoder accepts arbitrary
    // byte fragments and emits packets only after their four-byte header and full
    // payload have arrived.
    class MysqlPacketDecoder
    {
    public:
        [[nodiscard]] std::vector<MysqlPacket> feed(const char *data, std::size_t size);
        void reset();

    private:
        std::vector<std::uint8_t> buffer_;
        std::size_t offset_ = 0;
    };

    // Encode one classic-protocol packet, including its four-byte header.
    [[nodiscard]] std::vector<std::uint8_t> encodeMysqlPacket(
        const MysqlPacket &packet);

    // Small server responses used by locally handled FN commands.
    [[nodiscard]] std::vector<std::uint8_t> makeMysqlOkResponse(
        std::uint8_t sequence_id);
    [[nodiscard]] std::vector<std::uint8_t> makeMysqlErrorResponse(
        std::uint8_t sequence_id,
        std::uint16_t error_code,
        std::string_view sql_state,
        std::string_view message);
    [[nodiscard]] std::vector<std::uint8_t> makeMysqlSingleColumnResultSet(
        std::uint8_t first_sequence_id,
        std::string_view column_name,
        std::string_view value,
        bool client_deprecates_eof,
        bool client_uses_optional_metadata);

} // namespace fn::protocol
