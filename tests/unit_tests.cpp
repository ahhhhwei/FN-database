#include "fn/protocol/mysql_packet_codec.h"
#include "fn/sql/parser.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
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

void testPacketDecoderHandlesArbitraryFragments()
{
    fn::protocol::MysqlPacketDecoder decoder;
    const auto encoded = mysqlPacket(7, "\x03NF SET MODE 2NF");
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
                "\x03NF SET MODE 2NF",
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

void testSetMode()
{
    const auto result = fn::sql::parse("NF SET MODE 2NF;");
    require(result.status == fn::sql::ParseStatus::success, "NF SET MODE should parse");
    const auto& statement = std::get<fn::sql::SetModeStatement>(*result.statement);
    require(statement.mode == fn::sql::NormalForm::second, "2NF should map to second normal form");
    require(fn::sql::describe(*result.statement) == "SET MODE 2NF", "SET MODE description should match");
}

void testDependency()
{
    const auto result = fn::sql::parse(
        "/* demo */ FN DEPENDENCY ON `sales`.`orders` (`tenant_id`, id) -> (amount, status);");
    require(result.status == fn::sql::ParseStatus::success, "dependency should parse");
    const auto& statement = std::get<fn::sql::DependencyStatement>(*result.statement);
    require(statement.table == "sales.orders", "qualified table should parse");
    require(statement.determinants == std::vector<std::string>({"tenant_id", "id"}),
            "determinant columns should parse");
    require(statement.dependents == std::vector<std::string>({"amount", "status"}),
            "dependent columns should parse");
}

void testAnalyzeAndDecompose()
{
    const auto analyze = fn::sql::parse("fn analyze users");
    require(analyze.status == fn::sql::ParseStatus::success, "ANALYZE should be case insensitive");
    require(std::get<fn::sql::AnalyzeStatement>(*analyze.statement).table == "users",
            "ANALYZE table should parse");

    const auto decompose = fn::sql::parse("FN DECOMPOSE app.users TO BCNF");
    require(decompose.status == fn::sql::ParseStatus::success, "DECOMPOSE should parse");
    const auto& statement = std::get<fn::sql::DecomposeStatement>(*decompose.statement);
    require(statement.table == "app.users", "DECOMPOSE table should parse");
    require(statement.target == fn::sql::NormalForm::boyce_codd,
            "BCNF target should parse");
}

void testPassThroughAndErrors()
{
    const auto ordinary = fn::sql::parse("SELECT * FROM users");
    require(ordinary.status == fn::sql::ParseStatus::not_fn_statement,
            "ordinary SQL should remain outside the FN parser");

    const auto bad_target = fn::sql::parse("FN DECOMPOSE users TO 2NF");
    require(bad_target.status == fn::sql::ParseStatus::error,
            "invalid decomposition target should fail");
    require(!bad_target.error_message.empty(), "parse error should include a message");

    const auto bad_columns = fn::sql::parse("FN DEPENDENCY users (id,) -> (name)");
    require(bad_columns.status == fn::sql::ParseStatus::error,
            "malformed column list should fail");
    require(bad_columns.error_offset != 0, "parse error should include an input offset");
}

}  // namespace

int main()
{
    testPacketDecoderHandlesArbitraryFragments();
    testPacketDecoderHandlesCoalescedPackets();
    testSetMode();
    testDependency();
    testAnalyzeAndDecompose();
    testPassThroughAndErrors();
    std::cout << "All unit tests passed\n";
    return 0;
}
