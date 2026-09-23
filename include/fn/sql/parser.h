#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace fn::sql {

enum class NormalForm {
    first,
    second,
    third,
    boyce_codd,
};

struct SetModeStatement {
    NormalForm mode;
};

struct DependencyStatement {
    std::string table;
    std::vector<std::string> determinants;
    std::vector<std::string> dependents;
};

struct AnalyzeStatement {
    std::string table;
};

struct DecomposeStatement {
    std::string table;
    NormalForm target;
};

using Statement = std::variant<SetModeStatement,
                               DependencyStatement,
                               AnalyzeStatement,
                               DecomposeStatement>;

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

// Demo grammar (keywords are case-insensitive, FN and NF are aliases):
//   FN SET MODE <1NF|2NF|3NF|BCNF>
//   FN DEPENDENCY [ON] <table> (<column>, ...) -> (<column>, ...)
//   FN ANALYZE <table>
//   FN DECOMPOSE <table> [TO] <3NF|BCNF>
[[nodiscard]] ParseResult parse(std::string_view sql);

[[nodiscard]] std::string_view normalFormName(NormalForm normal_form);
[[nodiscard]] std::string describe(const Statement& statement);

}  // namespace fn::sql
