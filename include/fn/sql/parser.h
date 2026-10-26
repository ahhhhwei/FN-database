#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace fn::sql {

// 业务层使用的范式模式。它与 ANTLR 生成类解耦，执行层无需依赖 Parse Tree。
enum class NFMode {
    second,
    third,
    boyce_codd,
    off,
};

// “SET NF_MODE = ...”成功解析后的抽象语法树（AST）节点。
struct SetModeStatement {
    NFMode mode;
};

// “SHOW NF_MODE”没有额外参数，因此节点本身为空即可表达语义。
struct ShowModeStatement {};

// 当前支持两种语句。std::variant 保证一个 Statement 同一时刻只属于其中一种。
using Statement = std::variant<SetModeStatement,
                               ShowModeStatement>;

// 解析结果必须区分三种情况：普通 MySQL SQL、合法 FN SQL、非法 FN SQL。
enum class ParseStatus {
    // 输入不是以 SET/SHOW NF_MODE 开头，应原样交给后端 MySQL。
    not_fn_statement,
    // 输入是合法 FN SQL，statement 中保存对应 AST。
    success,
    // 输入看起来是 FN SQL，但不符合 grammar，应向客户端返回语法错误。
    error,
};

struct ParseResult {
    ParseStatus status = ParseStatus::not_fn_statement;
    // 只有 status == success 时才有值。
    std::optional<Statement> statement;
    // error_offset 是从输入开头起算的字节位置，便于日志定位。
    std::size_t error_offset = 0;
    std::string error_message;
};

// 支持的自定义 SQL（关键字大小写不敏感，分号可选）：
//   SET NF_MODE = <2NF|3NF|BCNF|OFF>;
//   SHOW NF_MODE;
[[nodiscard]] ParseResult parse(std::string_view sql);

// 将枚举转换为 SQL 中使用的稳定名称，例如 NFMode::second -> "2NF"。
[[nodiscard]] std::string_view nfModeName(NFMode mode);

// 将 AST 格式化为规范化 SQL，主要用于日志输出。
[[nodiscard]] std::string describe(const Statement& statement);

}  // namespace fn::sql
