#include "fn/protocol/mysql_packet_codec.h"

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace fn::protocol
{
    namespace
    {
        // Classic Protocol 包头固定为 4 字节：3 字节 payload 长度 + 1 字节序号。
        constexpr std::size_t mysql_header_size = 4;
        // 三字节无符号整数的最大值，也是单个 Classic Protocol payload 的上限。
        constexpr std::size_t max_mysql_payload_size = 0x00ffffffU;
        // SERVER_STATUS_AUTOCOMMIT，告诉客户端当前会话处于自动提交模式。
        constexpr std::uint16_t server_status_autocommit = 0x0002U;

        // MySQL 多字节整数使用小端序：最低有效字节放在最前面。
        std::size_t payloadLength(const std::uint8_t *header)
        {
            return static_cast<std::size_t>(header[0]) |
                   (static_cast<std::size_t>(header[1]) << 8U) |
                   (static_cast<std::size_t>(header[2]) << 16U);
        }

        // 向字节数组追加一个两字节小端整数。
        void appendLittleEndian2(std::vector<std::uint8_t> &output,
                                 std::uint16_t value)
        {
            output.push_back(static_cast<std::uint8_t>(value & 0xffU));
            output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
        }

        // 向字节数组追加一个四字节小端整数。
        void appendLittleEndian4(std::vector<std::uint8_t> &output,
                                 std::uint32_t value)
        {
            output.push_back(static_cast<std::uint8_t>(value & 0xffU));
            output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
            output.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
            output.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
        }

        // MySQL 的 length-encoded integer 会根据数值大小选择 1、3、4 或 9 字节编码。
        // 例如小于 0xfb 的长度可直接放在一个字节里；0xfc/0xfd/0xfe 是长度标记。
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
            // 0xfe 后固定写入八字节小端整数。
            for (unsigned shift = 0; shift < 64U; shift += 8U)
            {
                output.push_back(static_cast<std::uint8_t>(wide_value >> shift));
            }
        }

        // length-encoded string = length-encoded integer 长度 + 原始字符串字节。
        void appendLengthEncodedString(std::vector<std::uint8_t> &output,
                                       std::string_view value)
        {
            appendLengthEncodedInteger(output, value.size());
            output.insert(output.end(), value.begin(), value.end());
        }

        // 将 payload 加上 Classic Protocol 包头，并追加到 output。
        // output 允许连续保存多个包，SHOW NF_MODE 的结果集正是这样一次构造出来的。
        void appendPacket(std::vector<std::uint8_t> &output,
                          std::uint8_t sequence_id,
                          const std::vector<std::uint8_t> &payload)
        {
            if (payload.size() > max_mysql_payload_size)
            {
                throw std::length_error("MySQL payload exceeds one packet");
            }

            // payload 长度只占三字节，然后写入当前响应包的序号。
            output.push_back(static_cast<std::uint8_t>(payload.size() & 0xffU));
            output.push_back(static_cast<std::uint8_t>((payload.size() >> 8U) & 0xffU));
            output.push_back(static_cast<std::uint8_t>((payload.size() >> 16U) & 0xffU));
            output.push_back(sequence_id);
            output.insert(output.end(), payload.begin(), payload.end());
        }

        // 生成 Protocol 4.1 OK payload。
        // header 通常是 0x00；启用 CLIENT_DEPRECATE_EOF 后也可使用 0xfe 结束结果集。
        std::vector<std::uint8_t> okPayload(std::uint8_t header)
        {
            // affected_rows 和 last_insert_id 都是 length-encoded 0。
            std::vector<std::uint8_t> payload{header, 0x00U, 0x00U};
            appendLittleEndian2(payload, server_status_autocommit);
            // 最后两字节是 warning_count，这里没有警告。
            appendLittleEndian2(payload, 0U);
            return payload;
        }

        // 旧客户端使用的 EOF payload：0xfe + warning_count + status_flags。
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
        // read_some 的输入是 char*；协议按无符号字节解释，因此转换为 uint8_t*。
        if (size != 0)
        {
            const auto *begin = reinterpret_cast<const std::uint8_t *>(data);
            buffer_.insert(buffer_.end(), begin, begin + size);
        }

        std::vector<MysqlPacket> packets;
        // 先确认至少有完整包头，再根据包头判断 payload 是否也已到齐。
        while (buffer_.size() - offset_ >= mysql_header_size)
        {
            const auto *header = buffer_.data() + offset_;
            const std::size_t payload_size = payloadLength(header);
            const std::size_t packet_size = mysql_header_size + payload_size;
            if (buffer_.size() - offset_ < packet_size)
            {
                // TCP 数据尚未收完整，保留尾部，等下一次 feed 继续拼接。
                break;
            }

            MysqlPacket packet;
            // 包头第四字节是 sequence id；payload 不包含四字节包头。
            packet.sequence_id = header[3];
            packet.payload.assign(header + mysql_header_size, header + packet_size);
            packets.push_back(std::move(packet));
            offset_ += packet_size;
        }

        // 全部消费完时直接 clear；否则在已消费前缀足够大时做一次压缩。
        // 这样既避免每解析一个包就搬动内存，也避免缓存无限保留旧数据。
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
        // 切换协议模式后，先前未完成的明文包已经不能继续解释。
        buffer_.clear();
        offset_ = 0;
    }

    std::vector<std::uint8_t> encodeMysqlPacket(const MysqlPacket &packet)
    {
        std::vector<std::uint8_t> encoded;
        // reserve 只预留容量，不改变 size，可减少 appendPacket 期间的重新分配。
        encoded.reserve(mysql_header_size + packet.payload.size());
        appendPacket(encoded, packet.sequence_id, packet.payload);
        return encoded;
    }

    std::vector<std::uint8_t> makeMysqlOkResponse(std::uint8_t sequence_id)
    {
        std::vector<std::uint8_t> response;
        // 普通命令成功响应的 OK header 是 0x00。
        appendPacket(response, sequence_id, okPayload(0x00U));
        return response;
    }

    std::vector<std::uint8_t> makeMysqlErrorResponse(
        std::uint8_t sequence_id,
        std::uint16_t error_code,
        std::string_view sql_state,
        std::string_view message)
    {
        // SQLSTATE 必须恰好五个字符；调用者传错时退回通用错误状态。
        if (sql_state.size() != 5)
        {
            sql_state = "HY000";
        }

        // ERR payload：0xff + 2 字节 error_code + '#' + SQLSTATE + message。
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
        // 一个文本结果集由多个连续包组成，每追加一个包都递增 sequence id。
        std::vector<std::uint8_t> response;
        std::uint8_t sequence_id = first_sequence_id;

        // 第一个包说明列数。本函数固定生成一列。
        std::vector<std::uint8_t> column_count;
        if (client_uses_optional_metadata)
        {
            // 协商 CLIENT_OPTIONAL_RESULTSET_METADATA 后，列数前先写
            // RESULTSET_METADATA_FULL(1)，表示后面仍会发送完整列定义。
            column_count.push_back(0x01U);
        }
        appendLengthEncodedInteger(column_count, 1U);
        appendPacket(response, sequence_id++, column_count);

        // ColumnDefinition41 描述列的 catalog/schema/table/name/类型等元数据。
        std::vector<std::uint8_t> column_definition;
        appendLengthEncodedString(column_definition, "def"); // catalog 固定值
        appendLengthEncodedString(column_definition, ""); // schema（数据库名）
        appendLengthEncodedString(column_definition, ""); // table（表名）
        appendLengthEncodedString(column_definition, ""); // 原始表名
        appendLengthEncodedString(column_definition, column_name);
        appendLengthEncodedString(column_definition, ""); // 原始列名
        column_definition.push_back(0x0cU);                 // 固定字段区长度
        appendLittleEndian2(column_definition, 45U);       // utf8mb4_general_ci 字符集
        // 客户端可根据 column length 决定展示宽度；至少给模式名预留四字符。
        appendLittleEndian4(
            column_definition,
            static_cast<std::uint32_t>(std::max<std::size_t>(4U, value.size())));
        column_definition.push_back(0xfdU); // 列类型：MYSQL_TYPE_VAR_STRING
        // flags=0、decimals=0，最后两字节是协议要求的 filler。
        appendLittleEndian2(column_definition, 0U);
        column_definition.push_back(0x00U); // 小数位数为 0
        appendLittleEndian2(column_definition, 0U);
        appendPacket(response, sequence_id++, column_definition);

        // 旧协议在列元数据之后需要一个 EOF；新协议通过能力位省略它。
        if (!client_deprecates_eof)
        {
            appendPacket(response, sequence_id++, eofPayload());
        }

        // 文本结果集的每个字段都是 length-encoded string；这里只返回一行一列。
        std::vector<std::uint8_t> row;
        appendLengthEncodedString(row, value);
        appendPacket(response, sequence_id++, row);

        // 最后用 EOF 或 header=0xfe 的 OK-as-EOF 结束行数据。
        appendPacket(response,
                     sequence_id,
                     client_deprecates_eof ? okPayload(0xfeU) : eofPayload());
        return response;
    }

} // namespace fn::protocol
