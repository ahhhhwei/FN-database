#include <antlr4-runtime.h>

#include "FNSqlLexer.h"
#include "FNSqlParser.h"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Parse Tree 是 ANTLR 的语法结构；这个结构体是程序真正需要的业务数据。
struct AddFDStatement
{
    std::string table;
    std::vector<std::string> lhs;
    std::vector<std::string> rhs;
};

int main()
{
    const std::vector<std::string> sqlExamples = {
        "NF ADD FD enrollments "
        "(student_id) -> "
        "(student_name, dept_id);",

        "NF ADD FD score\n"
        "(student_id, course_id)\n"
        "->\n"
        "(score, grade);",
    };

    for (std::size_t exampleIndex = 0; exampleIndex < sqlExamples.size(); ++exampleIndex)
    {
        std::cout << "===== Example " << exampleIndex + 1 << " =====\n";
        std::cout << sqlExamples[exampleIndex] << "\n\n";

        // Step 1：ANTLRInputStream 把 SQL 字符串包装成 ANTLR 的字符流。
        std::string sql = sqlExamples[exampleIndex];
        antlr4::ANTLRInputStream input(sql);

        // Step 2：Lexer 读取字符流，并按 FNSql.g4 的词法规则产生 Token。
        FNSqlLexer lexer(&input);

        // Step 3：TokenStream 缓存 Lexer 产生的 Token，供 Parser 消费。
        antlr4::CommonTokenStream tokens(&lexer);
        tokens.fill();

        std::cout << "===== Tokens =====\n";
        for (antlr4::Token* token : tokens.getTokens())
        {
            std::cout << "text=" << std::quoted(token->getText()) << ", type=";

            if (token->getType() == antlr4::Token::EOF)
            {
                std::cout << "-1 (EOF)";
            }
            else
            {
                const std::string typeName(
                    lexer.getVocabulary().getSymbolicName(token->getType()));
                std::cout << token->getType() << " (" << typeName << ')';
            }

            std::cout << '\n';
        }
        std::cout << '\n';

        // 打印 Token 后把读取位置明确移回开头，再交给 Parser。
        tokens.seek(0);

        // Step 4：Parser 按语法规则消费 Token，从 statement 规则开始分析。
        FNSqlParser parser(&tokens);
        FNSqlParser::StatementContext* tree = parser.statement();

        // Step 5：Parse Tree 展示“输入匹配了哪些 grammar rule”。
        std::cout << "===== Parse Tree =====\n";
        std::cout << tree->toStringTree(&parser) << "\n\n";

        // Step 6：ANTLR 默认会向 stderr 输出具体错误；这里决定程序是否失败。
        // Parser 检查满足题目要求；同时检查 Lexer，避免非法字符被跳过后误判成功。
        if (lexer.getNumberOfSyntaxErrors() != 0 ||
            parser.getNumberOfSyntaxErrors() != 0)
        {
            std::cerr << "SQL syntax error\n";
            return 1;
        }

        // Step 7：第一版直接通过生成的 Context API 读取 Parse Tree。
        FNSqlParser::NfStatementContext* nf = tree->nfStatement();
        const std::vector<FNSqlParser::ColumnListContext*> columnLists =
            nf->columnList();

        AddFDStatement stmt;
        stmt.table = nf->tableName()->IDENTIFIER()->getText();

        for (antlr4::tree::TerminalNode* identifier :
             columnLists.at(0)->identifierList()->IDENTIFIER())
        {
            stmt.lhs.push_back(identifier->getText());
        }

        for (antlr4::tree::TerminalNode* identifier :
             columnLists.at(1)->identifierList()->IDENTIFIER())
        {
            stmt.rhs.push_back(identifier->getText());
        }

        // 从这里开始只使用业务结构体，不再依赖任何 Parser Context。
        std::cout << "===== Parsed Result =====\n";
        std::cout << "Table: " << stmt.table << "\n\n";

        std::cout << "LHS:\n";
        for (const std::string& column : stmt.lhs)
        {
            std::cout << column << '\n';
        }

        std::cout << "\nRHS:\n";
        for (const std::string& column : stmt.rhs)
        {
            std::cout << column << '\n';
        }

        std::cout << "\n";
    }

    return 0;
}
