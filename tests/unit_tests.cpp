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

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

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

std::vector<fn::protocol::MysqlPacket> decodePackets(
    const std::vector<std::uint8_t>& bytes)
{
    fn::protocol::MysqlPacketDecoder decoder;
    return decoder.feed(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

void testPacketResponseEncoding()
{
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

    const auto ok = decodePackets(fn::protocol::makeMysqlOkResponse(1));
    require(ok.size() == 1 && ok[0].sequence_id == 1,
            "OK response should contain sequence 1");
    require(ok[0].payload.size() == 7 && ok[0].payload[0] == 0x00U,
            "OK response should have a protocol-41 OK payload");

    const auto error = decodePackets(fn::protocol::makeMysqlErrorResponse(
        1, 1064, "42000", "bad NF command"));
    require(error.size() == 1 && error[0].payload[0] == 0xffU,
            "error response should contain an ERR packet");
    require(error[0].payload[3] == '#' && error[0].payload[4] == '4',
            "error response should contain a SQLSTATE marker");

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

void testPacketDecoderHandlesArbitraryFragments()
{
    fn::protocol::MysqlPacketDecoder decoder;
    const auto encoded = mysqlPacket(7, "\x03SET NF_MODE = 2NF");
    std::vector<fn::protocol::MysqlPacket> decoded;

    for (const char byte : encoded) {
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

void testPassThroughAndErrors()
{
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
    require(trailing_input.status == fn::sql::ParseStatus::error,
            "trailing input should fail because the grammar checks EOF");
}

}  // namespace

int main()
{
    testPacketResponseEncoding();
    testPacketDecoderHandlesArbitraryFragments();
    testPacketDecoderHandlesCoalescedPackets();
    testSetModes();
    testShowMode();
    testPassThroughAndErrors();
    std::cout << "All unit tests passed\n";
    return 0;
}
