#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace fn::protocol
{
    // MySQL Classic Protocol 将一条逻辑消息封装为“4 字节包头 + payload”。
    // ┌─────────────── 4 Bytes Header ────────────────┬──── Payload ────┐
    // │ payload length: 3 Bytes │ sequence id: 1 Byte │     ...         │
    // └───────────────────────────────────────────────┴─────────────────┘
    struct MysqlPacket
    {
        std::uint8_t sequence_id = 0;
        std::vector<std::uint8_t> payload;
    };

    // TCP 只提供连续字节流，不保留 MySQL 的包边界。本类可接收任意大小的碎片，
    // 只有凑齐 4 字节包头及完整 payload 后才返回 MysqlPacket。
    class MysqlPacketDecoder
    {
    public:
        // 追加一段新收到的 TCP 数据，并返回这次已经能够拼出的全部完整包。
        [[nodiscard]] std::vector<MysqlPacket> feed(const char *data, std::size_t size);

        // 丢弃所有尚未组成完整包的缓存数据，通常用于切换到 TLS/压缩模式。
        void reset();

    private:
        // buffer_ 可能同时包含已消费数据、完整新包和一个不完整的尾包。
        std::vector<std::uint8_t> buffer_;
        // offset_ 指向 buffer_ 中下一份尚未解析的数据，减少频繁 erase 的成本。
        std::size_t offset_ = 0;
    };

    // 把结构化包编码回完整的 Classic Protocol 字节串（包含 4 字节包头）。
    [[nodiscard]] std::vector<std::uint8_t> encodeMysqlPacket(
        const MysqlPacket &packet);

    // 为本地处理的 FN 命令生成 MySQL 服务端响应。参数 sequence_id 必须接在
    // 客户端请求包的序号之后，否则客户端会认为协议流损坏。
    [[nodiscard]] std::vector<std::uint8_t> makeMysqlOkResponse(
        std::uint8_t sequence_id);

    // ERR 包包含两字节错误码、五字符 SQLSTATE 和供用户阅读的错误消息。
    [[nodiscard]] std::vector<std::uint8_t> makeMysqlErrorResponse(
        std::uint8_t sequence_id,
        std::uint16_t error_code,
        std::string_view sql_state,
        std::string_view message);

    // 构造 SHOW NF_MODE 使用的一列一行文本结果集。最后两个参数来自握手能力位，
    // 决定是否省略元数据 EOF，以及结果集头是否包含 metadata_follows 字段。
    [[nodiscard]] std::vector<std::uint8_t> makeMysqlSingleColumnResultSet(
        std::uint8_t first_sequence_id,
        std::string_view column_name,
        std::string_view value,
        bool client_deprecates_eof,
        bool client_uses_optional_metadata);

} // namespace fn::protocol
