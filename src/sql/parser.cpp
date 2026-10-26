#include "fn/sql/parser.h"

#include <antlr4-runtime.h>

#include "FNCommandLexer.h"
#include "FNCommandParser.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace fn::sql
{
    namespace
    {
        // ANTLR 默认把语法错误打印到 stderr，不方便代理构造 MySQL ERR 包。
        // 自定义 listener 将第一条错误收集为“字节偏移 + 消息”，交给调用者决定如何展示。
        class SyntaxErrorListener final : public antlr4::BaseErrorListener
        {
        public:
            explicit SyntaxErrorListener(std::string_view input) : input_(input) {}

            // ANTLR 在 Lexer 或 Parser 发现错误时回调此函数。
            // override 保证签名确实覆盖基类虚函数；final 禁止再次继承修改行为。
            void syntaxError(antlr4::Recognizer*,
                             antlr4::Token* offending_symbol,
                             std::size_t line,
                             std::size_t char_position_in_line,
                             const std::string& message,
                             std::exception_ptr) override
            {
                // 一条输入可能触发级联错误，只保留最早一条通常最容易理解。
                if (has_error_)
                {
                    return;
                }

                has_error_ = true;
                error_message_ = message;

                // Parser 错误通常带 offending token，可直接获得它在输入中的起点。
                if (offending_symbol != nullptr &&
                    offending_symbol->getStartIndex() !=
                        std::numeric_limits<std::size_t>::max())
                {
                    error_offset_ = offending_symbol->getStartIndex();
                    return;
                }

                // Lexer 某些错误没有 token，只提供行列号，需要换算为绝对偏移。
                error_offset_ = offsetForLineAndColumn(line, char_position_in_line);
            }

            [[nodiscard]] bool hasError() const
            {
                return has_error_;
            }

            [[nodiscard]] std::size_t errorOffset() const
            {
                return error_offset_;
            }

            [[nodiscard]] const std::string& errorMessage() const
            {
                return error_message_;
            }

        private:
            // ANTLR 行号从 1 开始、列号从 0 开始。逐字符扫描可以正确处理多行 SQL。
            [[nodiscard]] std::size_t offsetForLineAndColumn(
                std::size_t target_line,
                std::size_t target_column) const
            {
                std::size_t offset = 0;
                std::size_t current_line = 1;

                while (current_line < target_line && offset < input_.size())
                {
                    // 后置 ++ 让换行字符本身也计入绝对偏移。
                    if (input_[offset++] == '\n')
                    {
                        ++current_line;
                    }
                }

                // 防止异常行列信息让偏移超过原始输入长度。
                return std::min(offset + target_column, input_.size());
            }

            // string_view 不拥有内存；listener 只在 parse() 调用期间存在，因此安全。
            std::string_view input_;
            bool has_error_ = false;
            std::size_t error_offset_ = 0;
            std::string error_message_;
        };

        // 在真正调用 FN Parser 前做轻量分流。
        // 普通 SELECT/INSERT/SHOW DATABASES 等不属于本项目语法，应直接交给 MySQL，
        // 不能因为 FN grammar 不认识它们就误报语法错误。
        [[nodiscard]] bool isNFCommand(const std::vector<antlr4::Token*>& tokens)
        {
            if (tokens.size() < 2)
            {
                return false;
            }

            const std::size_t first_type = tokens[0]->getType();
            const std::size_t second_type = tokens[1]->getType();
            // 只有 SET NF_MODE 或 SHOW NF_MODE 前缀才进入严格语法检查。
            return (first_type == FNCommandLexer::SET ||
                    first_type == FNCommandLexer::SHOW) &&
                   second_type == FNCommandLexer::NF_MODE;
        }

        [[nodiscard]] ParseResult errorResult(const SyntaxErrorListener& listener)
        {
            // 把 ANTLR 专用的错误收集对象转换为稳定的业务返回类型。
            ParseResult result;
            result.status = ParseStatus::error;
            result.error_offset = listener.errorOffset();
            result.error_message = listener.errorMessage().empty()
                                       ? "invalid NF command"
                                       : listener.errorMessage();
            return result;
        }

        [[nodiscard]] NFMode parseMode(
            FNCommandParser::NormalizationModeContext* mode)
        {
            // Context 中哪个终结符节点非空，就表示 grammar 匹配到了哪个分支。
            if (mode->TWO_NF() != nullptr)
            {
                return NFMode::second;
            }
            if (mode->THREE_NF() != nullptr)
            {
                return NFMode::third;
            }
            if (mode->BCNF() != nullptr)
            {
                return NFMode::boyce_codd;
            }
            // grammar 剩余的唯一合法分支是 OFF。
            return NFMode::off;
        }
    } // namespace

    ParseResult parse(std::string_view sql)
    {
        // ANTLRInputStream 的构造函数接收 std::string；这里显式复制一份并保证
        // 在 Lexer/Parser 的整个生命周期内有效。
        std::string input_text(sql);
        antlr4::ANTLRInputStream input(input_text);

        // Lexer 完成“字符 -> Token”，例如 SET、NF_MODE、EQUAL、TWO_NF。
        FNCommandLexer lexer(&input);
        SyntaxErrorListener error_listener(sql);

        // 移除默认 ConsoleErrorListener，避免库直接向 stderr 打印不可控信息。
        lexer.removeErrorListeners();
        lexer.addErrorListener(&error_listener);

        // CommonTokenStream 会缓存 Lexer 产生的 Token，Parser 随后从这里消费。
        antlr4::CommonTokenStream tokens(&lexer);
        // fill() 主动读取到 EOF，使前缀判断可以查看完整 Token 列表。
        tokens.fill();

        if (!isNFCommand(tokens.getTokens()))
        {
            // 默认构造的 ParseResult 状态就是 not_fn_statement。
            return {};
        }

        // 例如非法字符导致 Lexer 报错时，不再继续进入 Parser。
        if (lexer.getNumberOfSyntaxErrors() != 0)
        {
            return errorResult(error_listener);
        }

        // fill() 已把 TokenStream 读到末尾，Parser 开始前必须移回第一个 Token。
        tokens.seek(0);
        FNCommandParser parser(&tokens);
        parser.removeErrorListeners();
        parser.addErrorListener(&error_listener);

        // statement 是 FNCommand.g4 定义的顶层规则；返回值是 Parse Tree 根节点。
        FNCommandParser::StatementContext* tree = parser.statement();
        if (parser.getNumberOfSyntaxErrors() != 0 || error_listener.hasError())
        {
            return errorResult(error_listener);
        }

        ParseResult result;
        result.status = ParseStatus::success;

        // 将 ANTLR Parse Tree 转换为项目自己的小型 AST。
        // 后续执行层只依赖 Statement，不需要了解 ANTLR 生成类。
        if (FNCommandParser::SetModeStatementContext* set_mode =
                tree->setModeStatement())
        {
            result.statement =
                SetModeStatement{parseMode(set_mode->normalizationMode())};
        }
        else
        {
            result.statement = ShowModeStatement{};
        }

        return result;
    }

    std::string_view nfModeName(NFMode mode)
    {
        // 返回字符串字面量的 view，不产生堆分配，也没有悬空引用问题。
        switch (mode)
        {
        case NFMode::second:
            return "2NF";
        case NFMode::third:
            return "3NF";
        case NFMode::boyce_codd:
            return "BCNF";
        case NFMode::off:
            return "OFF";
        }
        // 防御未来新增枚举值却忘记更新 switch 的情况。
        return "unknown";
    }

    std::string describe(const Statement& statement)
    {
        // std::visit 根据 variant 当前保存的具体类型调用 lambda。
        return std::visit(
            [](const auto& value) -> std::string
            {
                // decay_t 去掉 const/引用，便于和 AST 节点类型做编译期比较。
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, SetModeStatement>)
                {
                    return "SET NF_MODE = " + std::string(nfModeName(value.mode));
                }
                else
                {
                    return "SHOW NF_MODE";
                }
            },
            statement);
    }
} // namespace fn::sql
