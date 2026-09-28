#include "fn/protocol/mysql_packet_codec.h"

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace fn::protocol
{
    namespace
    {
        constexpr std::size_t mysql_header_size = 4; // 4 字节包头
        constexpr std::size_t max_mysql_payload_size = 0x00ffffffU;
        constexpr std::uint16_t server_status_autocommit = 0x0002U;

        std::size_t payloadLength(const std::uint8_t *header)
        {
            return static_cast<std::size_t>(header[0]) |
                   (static_cast<std::size_t>(header[1]) << 8U) |
                   (static_cast<std::size_t>(header[2]) << 16U);
        }

        void appendLittleEndian2(std::vector<std::uint8_t> &output,
                                 std::uint16_t value)
        {
            output.push_back(static_cast<std::uint8_t>(value & 0xffU));
            output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
        }

        void appendLittleEndian4(std::vector<std::uint8_t> &output,
                                 std::uint32_t value)
        {
            output.push_back(static_cast<std::uint8_t>(value & 0xffU));
            output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
            output.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
            output.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
        }

        void appendLengthEncodedInteger(std::vector<std::uint8_t> &output,
                                        std::size_t value)
        {
            if (value < 0xfbU)
            {
                output.push_back(static_cast<std::uint8_t>(value));
                return;
            }
            if (value <= 0xffffU)
            {
                output.push_back(0xfcU);
                appendLittleEndian2(output, static_cast<std::uint16_t>(value));
                return;
            }
            if (value <= 0xffffffU)
            {
                output.push_back(0xfdU);
                output.push_back(static_cast<std::uint8_t>(value & 0xffU));
                output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
                output.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
                return;
            }

            output.push_back(0xfeU);
            const std::uint64_t wide_value = value;
            for (unsigned shift = 0; shift < 64U; shift += 8U)
            {
                output.push_back(static_cast<std::uint8_t>(wide_value >> shift));
            }
        }

        void appendLengthEncodedString(std::vector<std::uint8_t> &output,
                                       std::string_view value)
        {
            appendLengthEncodedInteger(output, value.size());
            output.insert(output.end(), value.begin(), value.end());
        }

        void appendPacket(std::vector<std::uint8_t> &output,
                          std::uint8_t sequence_id,
                          const std::vector<std::uint8_t> &payload)
        {
            if (payload.size() > max_mysql_payload_size)
            {
                throw std::length_error("MySQL payload exceeds one packet");
            }

            output.push_back(static_cast<std::uint8_t>(payload.size() & 0xffU));
            output.push_back(static_cast<std::uint8_t>((payload.size() >> 8U) & 0xffU));
            output.push_back(static_cast<std::uint8_t>((payload.size() >> 16U) & 0xffU));
            output.push_back(sequence_id);
            output.insert(output.end(), payload.begin(), payload.end());
        }

        std::vector<std::uint8_t> okPayload(std::uint8_t header)
        {
            std::vector<std::uint8_t> payload{header, 0x00U, 0x00U};
            appendLittleEndian2(payload, server_status_autocommit);
            appendLittleEndian2(payload, 0U);
            return payload;
        }

        std::vector<std::uint8_t> eofPayload()
        {
            std::vector<std::uint8_t> payload{0xfeU};
            appendLittleEndian2(payload, 0U);
            appendLittleEndian2(payload, server_status_autocommit);
            return payload;
        }

    } // namespace

    std::vector<MysqlPacket> MysqlPacketDecoder::feed(const char *data, std::size_t size)
    {
        if (size != 0)
        {
            const auto *begin = reinterpret_cast<const std::uint8_t *>(data);
            buffer_.insert(buffer_.end(), begin, begin + size);
        }

        std::vector<MysqlPacket> packets;
        while (buffer_.size() - offset_ >= mysql_header_size)
        {
            const auto *header = buffer_.data() + offset_;
            const std::size_t payload_size = payloadLength(header);
            const std::size_t packet_size = mysql_header_size + payload_size;
            if (buffer_.size() - offset_ < packet_size)
            {
                break;
            }

            MysqlPacket packet;
            packet.sequence_id = header[3];
            packet.payload.assign(header + mysql_header_size, header + packet_size);
            packets.push_back(std::move(packet));
            offset_ += packet_size;
        }

        if (offset_ == buffer_.size())
        {
            buffer_.clear();
            offset_ = 0;
        }
        else if (offset_ != 0 && offset_ >= buffer_.size() / 2)
        {
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

    std::vector<std::uint8_t> encodeMysqlPacket(const MysqlPacket &packet)
    {
        std::vector<std::uint8_t> encoded;
        encoded.reserve(mysql_header_size + packet.payload.size());
        appendPacket(encoded, packet.sequence_id, packet.payload);
        return encoded;
    }

    std::vector<std::uint8_t> makeMysqlOkResponse(std::uint8_t sequence_id)
    {
        std::vector<std::uint8_t> response;
        appendPacket(response, sequence_id, okPayload(0x00U));
        return response;
    }

    std::vector<std::uint8_t> makeMysqlErrorResponse(
        std::uint8_t sequence_id,
        std::uint16_t error_code,
        std::string_view sql_state,
        std::string_view message)
    {
        if (sql_state.size() != 5)
        {
            sql_state = "HY000";
        }

        std::vector<std::uint8_t> payload{0xffU};
        appendLittleEndian2(payload, error_code);
        payload.push_back('#');
        payload.insert(payload.end(), sql_state.begin(), sql_state.end());
        payload.insert(payload.end(), message.begin(), message.end());

        std::vector<std::uint8_t> response;
        appendPacket(response, sequence_id, payload);
        return response;
    }

    std::vector<std::uint8_t> makeMysqlSingleColumnResultSet(
        std::uint8_t first_sequence_id,
        std::string_view column_name,
        std::string_view value,
        bool client_deprecates_eof,
        bool client_uses_optional_metadata)
    {
        std::vector<std::uint8_t> response;
        std::uint8_t sequence_id = first_sequence_id;

        std::vector<std::uint8_t> column_count;
        if (client_uses_optional_metadata)
        {
            // RESULTSET_METADATA_FULL precedes column_count when
            // CLIENT_OPTIONAL_RESULTSET_METADATA was negotiated.
            column_count.push_back(0x01U);
        }
        appendLengthEncodedInteger(column_count, 1U);
        appendPacket(response, sequence_id++, column_count);

        std::vector<std::uint8_t> column_definition;
        appendLengthEncodedString(column_definition, "def");
        appendLengthEncodedString(column_definition, ""); // schema
        appendLengthEncodedString(column_definition, ""); // table
        appendLengthEncodedString(column_definition, ""); // original table
        appendLengthEncodedString(column_definition, column_name);
        appendLengthEncodedString(column_definition, ""); // original name
        column_definition.push_back(0x0cU);                 // fixed fields length
        appendLittleEndian2(column_definition, 45U);       // utf8mb4_general_ci
        appendLittleEndian4(
            column_definition,
            static_cast<std::uint32_t>(std::max<std::size_t>(4U, value.size())));
        column_definition.push_back(0xfdU); // MYSQL_TYPE_VAR_STRING
        appendLittleEndian2(column_definition, 0U);
        column_definition.push_back(0x00U); // decimals
        appendLittleEndian2(column_definition, 0U);
        appendPacket(response, sequence_id++, column_definition);

        if (!client_deprecates_eof)
        {
            appendPacket(response, sequence_id++, eofPayload());
        }

        std::vector<std::uint8_t> row;
        appendLengthEncodedString(row, value);
        appendPacket(response, sequence_id++, row);

        appendPacket(response,
                     sequence_id,
                     client_deprecates_eof ? okPayload(0xfeU) : eofPayload());
        return response;
    }

} // namespace fn::protocol
