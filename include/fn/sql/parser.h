#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace fn::sql {

enum class NFMode {
    second,
    third,
    boyce_codd,
    off,
};

struct SetModeStatement {
    NFMode mode;
};

struct ShowModeStatement {};

using Statement = std::variant<SetModeStatement,
                               ShowModeStatement>;

enum class ParseStatus {
    not_fn_statement,
    success,
    error,
};

struct ParseResult {
    ParseStatus status = ParseStatus::not_fn_statement;
    std::optional<Statement> statement;
    std::size_t error_offset = 0;
    std::string error_message;
};

// Supported custom SQL (keywords are case-insensitive):
//   SET NF_MODE = <2NF|3NF|BCNF|OFF>;
//   SHOW NF_MODE;
[[nodiscard]] ParseResult parse(std::string_view sql);

[[nodiscard]] std::string_view nfModeName(NFMode mode);
[[nodiscard]] std::string describe(const Statement& statement);

}  // namespace fn::sql
