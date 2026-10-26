#include "fn/protocol/mysql_packet_codec.h"
#include "fn/sql/parser.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

// 本文件不依赖第三方测试框架：每个 test* 函数是一组断言，main 依次调用。
// 这样 Demo 的依赖更少，也便于直接观察协议字节和 Parser 返回值。
namespace {

// 最小断言工具。失败立即结束进程，使 CTest 将该测试标记为失败。
void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

// 仅供测试构造 Classic Protocol 包：三字节 payload 长度、sequence id、payload。
std::vector<char> mysqlPacket(std::uint8_t sequence_id, const std::string& payload)
{
    const std::size_t size = payload.size();
    std::vector<char> packet{
        static_cast<char>(size & 0xffU),
        static_cast<char>((size >> 8U) & 0xffU),
        static_cast<char>((size >> 16U) & 0xffU),
        static_cast<char>(sequence_id),
    };
    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
}

// 响应构造函数返回的是连续字节；通过生产代码的 Decoder 再拆回包，
// 便于按 sequence id 和 payload 内容做断言。
std::vector<fn::protocol::MysqlPacket> decodePackets(
    const std::vector<std::uint8_t>& bytes)
{
    fn::protocol::MysqlPacketDecoder decoder;
    return decoder.feed(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

// 验证包重编码和三类本地服务端响应：OK、ERR、单列 Result Set。
void testPacketResponseEncoding()
{
    // encode -> decode 应保持序号和 payload 完全不变。
    const fn::protocol::MysqlPacket original{
        7,
        std::vector<std::uint8_t>{0x03U, 'S', 'E', 'L', 'E', 'C', 'T'},
    };
    const auto round_trip = decodePackets(fn::protocol::encodeMysqlPacket(original));
    require(round_trip.size() == 1, "encoded packet should decode once");
    require(round_trip[0].sequence_id == original.sequence_id,
            "encoded packet should preserve sequence id");
    require(round_trip[0].payload == original.payload,
            "encoded packet should preserve payload");

    // Protocol 4.1 OK payload 固定至少七字节，首字节为 0x00。
    const auto ok = decodePackets(fn::protocol::makeMysqlOkResponse(1));
    require(ok.size() == 1 && ok[0].sequence_id == 1,
            "OK response should contain sequence 1");
    require(ok[0].payload.size() == 7 && ok[0].payload[0] == 0x00U,
            "OK response should have a protocol-41 OK payload");

    // ERR payload 以 0xff 开头，并在错误码之后使用 '#' 引出 SQLSTATE。
    const auto error = decodePackets(fn::protocol::makeMysqlErrorResponse(
        1, 1064, "42000", "bad NF command"));
    require(error.size() == 1 && error[0].payload[0] == 0xffU,
            "error response should contain an ERR packet");
    require(error[0].payload[3] == '#' && error[0].payload[4] == '4',
            "error response should contain a SQLSTATE marker");

    // 旧客户端的结果集：列数、列定义、元数据 EOF、数据行、行 EOF，共五包。
    const auto legacy_result = decodePackets(
        fn::protocol::makeMysqlSingleColumnResultSet(
            1, "NF_MODE", "BCNF", false, false));
    require(legacy_result.size() == 5,
            "legacy result set should contain metadata and row EOF packets");
    require(legacy_result[0].payload == std::vector<std::uint8_t>({0x01U}),
            "result set should advertise one column");
    require(legacy_result[2].payload.size() == 5 &&
                legacy_result[2].payload[0] == 0xfeU,
            "legacy metadata should end with EOF");
    require(legacy_result[3].payload ==
                std::vector<std::uint8_t>({0x04U, 'B', 'C', 'N', 'F'}),
            "result set row should contain the current mode");

    // 新客户端协商 DEPRECATE_EOF 后省略元数据 EOF，并用 OK-as-EOF 收尾。
    // OPTIONAL_RESULTSET_METADATA 还会在列数之前增加 metadata_follows=1。
    const auto modern_result = decodePackets(
        fn::protocol::makeMysqlSingleColumnResultSet(
            1, "NF_MODE", "2NF", true, true));
    require(modern_result.size() == 4,
            "CLIENT_DEPRECATE_EOF result set should omit metadata EOF");
    require(modern_result[0].payload ==
                std::vector<std::uint8_t>({0x01U, 0x01U}),
            "optional metadata result set should advertise full metadata and one column");
    require(modern_result[2].payload ==
                std::vector<std::uint8_t>({0x03U, '2', 'N', 'F'}),
            "modern result set row should contain the current mode");
    require(modern_result[3].payload.size() == 7 &&
                modern_result[3].payload[0] == 0xfeU,
            "modern result set should end with an OK-as-EOF packet");
}

// TCP 可能把一个 MySQL 包拆成任意片段；逐字节 feed 是最极端的合法情况。
void testPacketDecoderHandlesArbitraryFragments()
{
    fn::protocol::MysqlPacketDecoder decoder;
    const auto encoded = mysqlPacket(7, "\x03SET NF_MODE = 2NF");
    std::vector<fn::protocol::MysqlPacket> decoded;

    for (const char byte : encoded) {
        // make_move_iterator 将临时结果中的 vector payload 移入 decoded，避免复制。
        auto packets = decoder.feed(&byte, 1);
        decoded.insert(decoded.end(),
                       std::make_move_iterator(packets.begin()),
                       std::make_move_iterator(packets.end()));
    }

    require(decoded.size() == 1, "fragmented packet should be emitted exactly once");
    require(decoded[0].sequence_id == 7, "sequence id should be decoded");
    require(std::string(decoded[0].payload.begin(), decoded[0].payload.end()) ==
                "\x03SET NF_MODE = 2NF",
            "packet payload should be preserved");
}

// 反方向也可能发生：一次 read_some 同时返回多个完整 MySQL 包（TCP 粘包）。
void testPacketDecoderHandlesCoalescedPackets()
{
    fn::protocol::MysqlPacketDecoder decoder;
    auto first = mysqlPacket(0, "first");
    const auto second = mysqlPacket(1, "second");
    first.insert(first.end(), second.begin(), second.end());

    const auto decoded = decoder.feed(first.data(), first.size());
    require(decoded.size() == 2, "coalesced packets should be split");
    require(std::string(decoded[0].payload.begin(), decoded[0].payload.end()) == "first",
            "first coalesced payload should match");
    require(std::string(decoded[1].payload.begin(), decoded[1].payload.end()) == "second",
            "second coalesced payload should match");
}

// SET 语句的公共断言：既要解析成功，也要生成正确的 AST 枚举。
void requireSetMode(std::string_view sql,
                    fn::sql::NFMode expected_mode,
                    const char* message)
{
    const auto result = fn::sql::parse(sql);
    require(result.status == fn::sql::ParseStatus::success, message);
    require(result.statement.has_value(), "successful parse should contain a statement");
    const auto& statement = std::get<fn::sql::SetModeStatement>(*result.statement);
    require(statement.mode == expected_mode, "SET NF_MODE should map to the expected mode");
}

// 覆盖四种模式、大小写、可选分号、注释和 AST 日志格式化。
void testSetModes()
{
    requireSetMode("SET NF_MODE = 2NF;", fn::sql::NFMode::second,
                   "2NF command should parse");
    requireSetMode("set nf_mode = 3nf", fn::sql::NFMode::third,
                   "keywords should be case insensitive and semicolon optional");
    requireSetMode("/* mode */ SET NF_MODE = BCNF;", fn::sql::NFMode::boyce_codd,
                   "BCNF command should parse after a comment");
    requireSetMode("SET NF_MODE = OFF;", fn::sql::NFMode::off,
                   "OFF command should parse");

    const auto second = fn::sql::parse("SET NF_MODE = 2NF;");
    require(fn::sql::describe(*second.statement) == "SET NF_MODE = 2NF",
            "SET NF_MODE description should match");
}

// SHOW 应生成与 SetModeStatement 不同的 variant 分支。
void testShowMode()
{
    const auto result = fn::sql::parse("SHOW NF_MODE;");
    require(result.status == fn::sql::ParseStatus::success,
            "SHOW NF_MODE should parse");
    require(std::holds_alternative<fn::sql::ShowModeStatement>(*result.statement),
            "SHOW NF_MODE should create ShowModeStatement");
    require(fn::sql::describe(*result.statement) == "SHOW NF_MODE",
            "SHOW NF_MODE description should match");
}

// 分流规则非常重要：普通 MySQL SQL 要透传，只有疑似 FN SQL 才严格报错。
void testPassThroughAndErrors()
{
    // SELECT、MySQL 自己的 SET/SHOW 均不属于 FN grammar。
    const auto ordinary = fn::sql::parse("SELECT * FROM users");
    require(ordinary.status == fn::sql::ParseStatus::not_fn_statement,
            "ordinary SQL should remain outside the FN parser");

    const auto mysql_set = fn::sql::parse("SET sql_mode = 'STRICT_ALL_TABLES'");
    require(mysql_set.status == fn::sql::ParseStatus::not_fn_statement,
            "ordinary MySQL SET should remain outside the NF parser");

    const auto mysql_show = fn::sql::parse("SHOW DATABASES;");
    require(mysql_show.status == fn::sql::ParseStatus::not_fn_statement,
            "ordinary MySQL SHOW should remain outside the NF parser");

    const auto old_syntax = fn::sql::parse("NF SET MODE 2NF;");
    require(old_syntax.status == fn::sql::ParseStatus::not_fn_statement,
            "the old demo syntax should no longer be recognized");

    // 以 SET NF_MODE 开头但值非法时，应返回 error 而不是悄悄透传。
    const auto unsupported_mode = fn::sql::parse("SET NF_MODE = 1NF;");
    require(unsupported_mode.status == fn::sql::ParseStatus::error,
            "unsupported 1NF mode should fail");
    require(!unsupported_mode.error_message.empty(),
            "parse error should include a message");
    require(unsupported_mode.error_offset != 0,
            "parse error should include an input offset");

    const auto missing_value = fn::sql::parse("SET NF_MODE =;");
    require(missing_value.status == fn::sql::ParseStatus::error,
            "missing mode should fail");

    const auto trailing_input = fn::sql::parse("SHOW NF_MODE unexpected;");
    // grammar 顶层使用 EOF，因此合法前缀之后的垃圾输入也必须失败。
    require(trailing_input.status == fn::sql::ParseStatus::error,
            "trailing input should fail because the grammar checks EOF");
}

}  // namespace

int main()
{
    // 从底层协议到上层 SQL 依次执行，某一项失败会由 require 立即终止。
    testPacketResponseEncoding();
    testPacketDecoderHandlesArbitraryFragments();
    testPacketDecoderHandlesCoalescedPackets();
    testSetModes();
    testShowMode();
    testPassThroughAndErrors();
    std::cout << "All unit tests passed\n";
    return 0;
}
