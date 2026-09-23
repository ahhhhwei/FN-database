#include "fn/sql/parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace fn::sql {
namespace {

enum class TokenKind {
    word,
    left_parenthesis,
    right_parenthesis,
    comma,
    dot,
    arrow,
    semicolon,
    end,
    invalid,
};

struct Token {
    TokenKind kind = TokenKind::end;
    std::string text;
    std::size_t offset = 0;
};

bool isDelimiter(char character)
{
    return std::isspace(static_cast<unsigned char>(character)) != 0 ||
           character == '(' || character == ')' || character == ',' ||
           character == '.' || character == ';';
}

std::string upper(std::string_view text)
{
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return result;
}

class Lexer {
public:
    explicit Lexer(std::string_view input) : input_(input) {}

    Token next()
    {
        skipWhitespaceAndComments();
        if (position_ == input_.size()) {
            return {TokenKind::end, {}, position_};
        }

        const std::size_t start = position_;
        const char character = input_[position_];
        switch (character) {
        case '(':
            ++position_;
            return {TokenKind::left_parenthesis, "(", start};
        case ')':
            ++position_;
            return {TokenKind::right_parenthesis, ")", start};
        case ',':
            ++position_;
            return {TokenKind::comma, ",", start};
        case '.':
            ++position_;
            return {TokenKind::dot, ".", start};
        case ';':
            ++position_;
            return {TokenKind::semicolon, ";", start};
        case '-':
            if (position_ + 1 < input_.size() && input_[position_ + 1] == '>') {
                position_ += 2;
                return {TokenKind::arrow, "->", start};
            }
            break;
        case '`':
            return quotedIdentifier();
        default:
            break;
        }

        while (position_ < input_.size() && !isDelimiter(input_[position_])) {
            if (input_[position_] == '-' && position_ + 1 < input_.size() &&
                input_[position_ + 1] == '>') {
                break;
            }
            ++position_;
        }
        if (position_ == start) {
            ++position_;
            return {TokenKind::invalid, std::string(1, character), start};
        }
        return {TokenKind::word, std::string(input_.substr(start, position_ - start)), start};
    }

private:
    void skipWhitespaceAndComments()
    {
        for (;;) {
            while (position_ < input_.size() &&
                   std::isspace(static_cast<unsigned char>(input_[position_])) != 0) {
                ++position_;
            }

            if (position_ + 1 < input_.size() && input_[position_] == '/' &&
                input_[position_ + 1] == '*') {
                const std::size_t close = input_.find("*/", position_ + 2);
                position_ = close == std::string_view::npos ? input_.size() : close + 2;
                continue;
            }
            if (position_ + 2 < input_.size() && input_[position_] == '-' &&
                input_[position_ + 1] == '-' &&
                std::isspace(static_cast<unsigned char>(input_[position_ + 2])) != 0) {
                const std::size_t newline = input_.find('\n', position_ + 3);
                position_ = newline == std::string_view::npos ? input_.size() : newline + 1;
                continue;
            }
            if (position_ < input_.size() && input_[position_] == '#') {
                const std::size_t newline = input_.find('\n', position_ + 1);
                position_ = newline == std::string_view::npos ? input_.size() : newline + 1;
                continue;
            }
            return;
        }
    }

    Token quotedIdentifier()
    {
        const std::size_t start = position_++;
        std::string value;
        while (position_ < input_.size()) {
            if (input_[position_] != '`') {
                value.push_back(input_[position_++]);
                continue;
            }
            if (position_ + 1 < input_.size() && input_[position_ + 1] == '`') {
                value.push_back('`');
                position_ += 2;
                continue;
            }
            ++position_;
            return {TokenKind::word, std::move(value), start};
        }
        return {TokenKind::invalid, "unterminated quoted identifier", start};
    }

    std::string_view input_;
    std::size_t position_ = 0;
};

class ParseFailure : public std::runtime_error {
public:
    ParseFailure(std::size_t offset, std::string message)
        : std::runtime_error(std::move(message)), offset(offset)
    {
    }

    std::size_t offset;
};

class Parser {
public:
    explicit Parser(std::string_view input) : lexer_(input), current_(lexer_.next()) {}

    bool hasFnPrefix() const
    {
        return current_.kind == TokenKind::word &&
               (upper(current_.text) == "FN" || upper(current_.text) == "NF");
    }

    Statement parseStatement()
    {
        consume();  // FN or NF
        if (matchesWord("SET")) {
            expectWord("MODE");
            SetModeStatement statement{parseNormalForm()};
            expectEnd();
            return statement;
        }
        if (matchesWord("DEPENDENCY")) {
            matchesWord("ON");
            DependencyStatement statement;
            statement.table = parseQualifiedIdentifier("table name");
            statement.determinants = parseColumnList();
            expect(TokenKind::arrow, "expected '->' after determinant columns");
            statement.dependents = parseColumnList();
            expectEnd();
            return statement;
        }
        if (matchesWord("ANALYZE")) {
            AnalyzeStatement statement{parseQualifiedIdentifier("table name")};
            expectEnd();
            return statement;
        }
        if (matchesWord("DECOMPOSE")) {
            DecomposeStatement statement;
            statement.table = parseQualifiedIdentifier("table name");
            matchesWord("TO");
            statement.target = parseNormalForm();
            if (statement.target != NormalForm::third &&
                statement.target != NormalForm::boyce_codd) {
                fail(current_.offset, "decomposition target must be 3NF or BCNF");
            }
            expectEnd();
            return statement;
        }
        fail(current_.offset, "expected SET, DEPENDENCY, ANALYZE, or DECOMPOSE after FN");
    }

private:
    void consume()
    {
        current_ = lexer_.next();
    }

    bool matchesWord(std::string_view expected)
    {
        if (current_.kind == TokenKind::word && upper(current_.text) == expected) {
            consume();
            return true;
        }
        return false;
    }

    void expectWord(std::string_view expected)
    {
        if (!matchesWord(expected)) {
            fail(current_.offset, "expected '" + std::string(expected) + "'");
        }
    }

    void expect(TokenKind kind, std::string message)
    {
        if (current_.kind != kind) {
            fail(current_.offset, std::move(message));
        }
        consume();
    }

    std::string parseQualifiedIdentifier(std::string_view description)
    {
        if (current_.kind != TokenKind::word || current_.text.empty()) {
            fail(current_.offset, "expected " + std::string(description));
        }
        std::string identifier = current_.text;
        consume();
        while (current_.kind == TokenKind::dot) {
            consume();
            if (current_.kind != TokenKind::word || current_.text.empty()) {
                fail(current_.offset, "expected identifier after '.'");
            }
            identifier += '.';
            identifier += current_.text;
            consume();
        }
        return identifier;
    }

    std::vector<std::string> parseColumnList()
    {
        expect(TokenKind::left_parenthesis, "expected '('");
        std::vector<std::string> columns;
        columns.push_back(parseQualifiedIdentifier("column name"));
        while (current_.kind == TokenKind::comma) {
            consume();
            columns.push_back(parseQualifiedIdentifier("column name"));
        }
        expect(TokenKind::right_parenthesis, "expected ')' after column list");
        return columns;
    }

    NormalForm parseNormalForm()
    {
        if (current_.kind != TokenKind::word) {
            fail(current_.offset, "expected normal form (1NF, 2NF, 3NF, or BCNF)");
        }
        const std::size_t offset = current_.offset;
        const std::string value = upper(current_.text);
        consume();
        if (value == "1NF") {
            return NormalForm::first;
        }
        if (value == "2NF") {
            return NormalForm::second;
        }
        if (value == "3NF") {
            return NormalForm::third;
        }
        if (value == "BCNF") {
            return NormalForm::boyce_codd;
        }
        fail(offset, "unknown normal form '" + value + "'");
    }

    void expectEnd()
    {
        if (current_.kind == TokenKind::semicolon) {
            consume();
        }
        if (current_.kind != TokenKind::end) {
            fail(current_.offset, "unexpected token '" + current_.text + "'");
        }
    }

    [[noreturn]] static void fail(std::size_t offset, std::string message)
    {
        throw ParseFailure(offset, std::move(message));
    }

    Lexer lexer_;
    Token current_;
};

std::string join(const std::vector<std::string>& values)
{
    std::ostringstream output;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            output << ", ";
        }
        output << values[index];
    }
    return output.str();
}

}  // namespace

ParseResult parse(std::string_view sql)
{
    Parser parser(sql);
    if (!parser.hasFnPrefix()) {
        return {};
    }

    try {
        ParseResult result;
        result.status = ParseStatus::success;
        result.statement = parser.parseStatement();
        return result;
    }
    catch (const ParseFailure& error) {
        ParseResult result;
        result.status = ParseStatus::error;
        result.error_offset = error.offset;
        result.error_message = error.what();
        return result;
    }
}

std::string_view normalFormName(NormalForm normal_form)
{
    switch (normal_form) {
    case NormalForm::first:
        return "1NF";
    case NormalForm::second:
        return "2NF";
    case NormalForm::third:
        return "3NF";
    case NormalForm::boyce_codd:
        return "BCNF";
    }
    return "unknown";
}

std::string describe(const Statement& statement)
{
    return std::visit(
        [](const auto& value) -> std::string {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, SetModeStatement>) {
                return "SET MODE " + std::string(normalFormName(value.mode));
            }
            else if constexpr (std::is_same_v<Value, DependencyStatement>) {
                return "DEPENDENCY " + value.table + " (" + join(value.determinants) +
                       ") -> (" + join(value.dependents) + ")";
            }
            else if constexpr (std::is_same_v<Value, AnalyzeStatement>) {
                return "ANALYZE " + value.table;
            }
            else {
                return "DECOMPOSE " + value.table + " TO " +
                       std::string(normalFormName(value.target));
            }
        },
        statement);
}

}  // namespace fn::sql
