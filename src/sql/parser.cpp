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
        class SyntaxErrorListener final : public antlr4::BaseErrorListener
        {
        public:
            explicit SyntaxErrorListener(std::string_view input) : input_(input) {}

            void syntaxError(antlr4::Recognizer*,
                             antlr4::Token* offending_symbol,
                             std::size_t line,
                             std::size_t char_position_in_line,
                             const std::string& message,
                             std::exception_ptr) override
            {
                if (has_error_)
                {
                    return;
                }

                has_error_ = true;
                error_message_ = message;

                if (offending_symbol != nullptr &&
                    offending_symbol->getStartIndex() !=
                        std::numeric_limits<std::size_t>::max())
                {
                    error_offset_ = offending_symbol->getStartIndex();
                    return;
                }

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
            [[nodiscard]] std::size_t offsetForLineAndColumn(
                std::size_t target_line,
                std::size_t target_column) const
            {
                std::size_t offset = 0;
                std::size_t current_line = 1;

                while (current_line < target_line && offset < input_.size())
                {
                    if (input_[offset++] == '\n')
                    {
                        ++current_line;
                    }
                }

                return std::min(offset + target_column, input_.size());
            }

            std::string_view input_;
            bool has_error_ = false;
            std::size_t error_offset_ = 0;
            std::string error_message_;
        };

        [[nodiscard]] bool isNFCommand(const std::vector<antlr4::Token*>& tokens)
        {
            if (tokens.size() < 2)
            {
                return false;
            }

            const std::size_t first_type = tokens[0]->getType();
            const std::size_t second_type = tokens[1]->getType();
            return (first_type == FNCommandLexer::SET ||
                    first_type == FNCommandLexer::SHOW) &&
                   second_type == FNCommandLexer::NF_MODE;
        }

        [[nodiscard]] ParseResult errorResult(const SyntaxErrorListener& listener)
        {
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
            return NFMode::off;
        }
    } // namespace

    ParseResult parse(std::string_view sql)
    {
        std::string input_text(sql);
        antlr4::ANTLRInputStream input(input_text);
        FNCommandLexer lexer(&input);
        SyntaxErrorListener error_listener(sql);

        lexer.removeErrorListeners();
        lexer.addErrorListener(&error_listener);

        antlr4::CommonTokenStream tokens(&lexer);
        tokens.fill();

        if (!isNFCommand(tokens.getTokens()))
        {
            return {};
        }

        if (lexer.getNumberOfSyntaxErrors() != 0)
        {
            return errorResult(error_listener);
        }

        tokens.seek(0);
        FNCommandParser parser(&tokens);
        parser.removeErrorListeners();
        parser.addErrorListener(&error_listener);

        FNCommandParser::StatementContext* tree = parser.statement();
        if (parser.getNumberOfSyntaxErrors() != 0 || error_listener.hasError())
        {
            return errorResult(error_listener);
        }

        ParseResult result;
        result.status = ParseStatus::success;

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
        return "unknown";
    }

    std::string describe(const Statement& statement)
    {
        return std::visit(
            [](const auto& value) -> std::string
            {
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
