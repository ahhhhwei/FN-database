# 最小可运行的 ANTLR4 + C++ SQL Demo

这个独立 Demo 只识别一种自定义语句：

```sql
NF ADD FD table_name (lhs_column, ...) -> (rhs_column, ...);
```

它刻意不接入仓库中的 Proxy、MySQL、Boost 或其他 SQL Parser。目标是把下面的数据流完整展示出来：

```text
SQL 字符串
  ↓
ANTLRInputStream（字符流）
  ↓
FNSqlLexer（字符 → Token）
  ↓
CommonTokenStream（缓存 Token）
  ↓
FNSqlParser（Token → 语法结构）
  ↓
StatementContext / Parse Tree
  ↓
AddFDStatement（业务数据）
```

项目使用 C++17。ANTLR Generator 和 C++ Runtime 都固定为 **4.13.2**。Demo 与正式项目共用仓库根目录的 `third_party/`，因此不会意外使用系统中的其他版本，也不需要保存两份 Runtime。

## 目录结构

```text
docs/antlr-demo/
├── CMakeLists.txt
├── README.md
├── grammar/
│   └── FNSql.g4
├── generated/
│   ├── FNSqlLexer.cpp
│   ├── FNSqlLexer.h
│   ├── FNSqlParser.cpp
│   ├── FNSqlParser.h
│   └── ...
└── src/
    └── main.cpp

../../third_party/
├── antlr-4.13.2-complete.jar
└── antlr4-runtime/
    ├── LICENSE.txt
    └── runtime/src/...
```

`generated/` 已经包含可编译文件。平时改 `main.cpp` 后不必运行 Java；只有修改 `FNSql.g4` 后才需要重新生成。

## 从零生成、编译和运行

下面假设当前目录是仓库中的 `docs/antlr-demo/`。

### 1. 准备工具

Ubuntu / Debian：

```bash
sudo apt update
sudo apt install -y build-essential cmake default-jre-headless

java -version
cmake --version
c++ --version
```

ANTLR 4.13.2 的 jar 和 C++ Runtime 源码已经放在仓库根目录的 `third_party/`，无需执行系统级安装。这里的 Java 只用于下一步运行 Generator。

若要核对或重新取得官方文件，下载地址是：

```text
https://www.antlr.org/download/antlr-4.13.2-complete.jar
https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip
```

### 2. 从 grammar 生成 C++ Lexer / Parser

在 `docs/antlr-demo/` 目录执行下面的完整命令。`cd grammar` 很重要：它让 `-o ../generated` 真正把文件直接生成到 `generated/`，而不是额外创建一层 `grammar/`。

```bash
cd grammar

java -jar ../../../third_party/antlr-4.13.2-complete.jar \
  -Dlanguage=Cpp \
  -no-listener \
  -o ../generated \
  FNSql.g4

cd ..
```

这里没有加 `-visitor`：第一版直接学习 Context API，不生成暂时用不到的 Visitor；`-no-listener` 同理。如果以后学习 Visitor，可把 `-no-listener` 保留并增加 `-visitor`。

生成结果包括：

```text
FNSqlLexer.cpp / FNSqlLexer.h       词法分析器
FNSqlParser.cpp / FNSqlParser.h     语法分析器和 Context 类型
FNSql.tokens / FNSqlLexer.tokens    Token 编号表
FNSql.interp / FNSqlLexer.interp    ANTLR 生成的解释数据
```

### 3. 配置项目

```bash
cmake -S . -B build
```

CMake 会从仓库根目录的 `third_party/antlr4-runtime/runtime/src/` 构建 4.13.2 静态 Runtime，再编译 `main.cpp`、`FNSqlLexer.cpp` 和 `FNSqlParser.cpp`。

### 4. 编译

```bash
cmake --build build -j
```

### 5. 运行

```bash
./build/fn_demo
```

输出的主要内容如下（Token 行还会包含数字类型）：

```text
===== Example 1 =====
NF ADD FD enrollments (student_id) -> (student_name, dept_id);

===== Tokens =====
text="NF", type=1 (NF)
text="ADD", type=2 (ADD)
text="FD", type=3 (FD)
text="enrollments", type=9 (IDENTIFIER)
...
text="<EOF>", type=-1 (EOF)

===== Parse Tree =====
(statement (nfStatement NF ADD FD (tableName enrollments) ... ) ; <EOF>)

===== Parsed Result =====
Table: enrollments

LHS:
student_id

RHS:
student_name
dept_id

===== Example 2 =====
...
===== Parsed Result =====
Table: score

LHS:
student_id
course_id

RHS:
score
grade
```

## Grammar 是怎样工作的

核心 Parser 规则是：

```antlr
statement
    : nfStatement SEMI? EOF
    ;

nfStatement
    : NF ADD FD tableName columnList ARROW columnList
    ;

columnList
    : LPAREN identifierList RPAREN
    ;

identifierList
    : IDENTIFIER (COMMA IDENTIFIER)*
    ;
```

`identifierList` 先要求一个 `IDENTIFIER`，再允许零到多个 `, IDENTIFIER`，所以左右两侧都至少有一个属性。`SEMI?` 表示分号可有可无。最外层的 `EOF` 表示分号之后必须结束；如果没有它，Parser 可能只接受输入开头的合法部分，而忽略尾部垃圾。

词法规则中：

```antlr
IDENTIFIER : [a-zA-Z_] [a-zA-Z0-9_]*;
WS         : [ \t\r\n]+ -> skip;
```

因此 `student`、`student_id`、`course123`、`abc_def` 都是合法标识符。`WS` 会忽略空格、Tab、换行和 `\r`，所以第二个跨行示例与单行语句没有语法差别。关键字当前只接受大写 `NF ADD FD`。

## ANTLR 中各个概念是什么

### `FNSql.g4`

`.g4` 是 grammar（文法）源文件，同时描述两类规则：

- 大写开头的 `NF`、`ARROW`、`IDENTIFIER`、`WS` 等是 Lexer 规则。
- 小写开头的 `statement`、`nfStatement`、`columnList` 等是 Parser 规则。

它不是 C++ 源码，C++ 编译器不能直接编译它。

### Lexer

Lexer 做词法分析：

```text
字符流 → Token
```

例如字符 `student_id` 变成一个类型为 `IDENTIFIER`、文本为 `student_id` 的 Token；字符 `->` 变成 `ARROW` Token。Lexer 只负责识别词，不负责判断左右属性列表的位置是否正确。

### Token

Token 是 Lexer 的一次识别结果，主要包含：

- text：原始文本，如 `student_id`；
- type：Token 类型编号，如 `IDENTIFIER` 对应本 grammar 中的 9；
- 行号、列号以及在输入中的位置。

`main.cpp` 调用 `tokens.fill()`，然后打印每个 Token 的 text、数字 type 和符号名称。

### TokenStream

`CommonTokenStream` 位于 Lexer 和 Parser 之间。它按需从 Lexer 取 Token，并把它们缓存起来，使 Parser 能向前查看、消费或回看 Token。

`fill()` 主动让它一直读取到 `<EOF>`。打印完后调用 `seek(0)`，再让 Parser 从第一个 Token 开始读取。

### Parser

Parser 做语法分析：

```text
Token → 符合 grammar rule 的语法结构
```

它会检查 `NF ADD FD` 的顺序、括号、逗号、箭头、可选分号以及最终 `EOF`。`parser.statement()` 表示从 grammar 的 `statement` 规则开始分析。

### Parse Tree

Parse Tree 是 Parser 按规则匹配后得到的树。规则是非叶子节点，实际 Token 通常是叶子节点。`tree->toStringTree(&parser)` 用一行括号形式把这棵树打印出来。

Parse Tree 很适合表示语法细节，但它仍是 ANTLR 数据结构，不应直接充当业务模型。因此 Demo 随后把它转换成简单的 `AddFDStatement`。

### Context

ANTLR 为每条 Parser rule 生成对应的 Context 类：

| Grammar rule | 生成的 C++ 类型 / 访问方式 |
|---|---|
| `statement` | `FNSqlParser::StatementContext` |
| `nfStatement` | `FNSqlParser::NfStatementContext` |
| `tableName` | `FNSqlParser::TableNameContext` |
| `columnList` | `FNSqlParser::ColumnListContext` |
| `identifierList` | `FNSqlParser::IdentifierListContext` |

所以代码可以沿着树读取：

```cpp
auto* nf = tree->nfStatement();
auto table = nf->tableName()->IDENTIFIER()->getText();
auto lists = nf->columnList();
auto lhsIdentifiers = lists.at(0)->identifierList()->IDENTIFIER();
```

这里 `nf->columnList()` 返回两个 `ColumnListContext*`：第 0 个是箭头左侧，第 1 个是箭头右侧。Context 就是“某条 grammar rule 在这一次解析中匹配到的那一段树”。

## `.g4`、Generator、生成的 `.cpp` 与 Runtime

关系如下：

```text
grammar/FNSql.g4
       ↓  Java 运行 ANTLR Generator，目标参数为 -Dlanguage=Cpp
generated/FNSqlLexer.cpp
generated/FNSqlParser.cpp
       ↓  C++ 编译器
fn_demo
       ↕
ANTLR4 C++ Runtime
```

Generator 读取通用 grammar，并根据 `-Dlanguage=Cpp` 写出 C++ 源码。生成后的 Lexer / Parser 不是完全独立的：字符流、Token、错误恢复、Parse Tree 等通用能力由 C++ Runtime 提供，所以编译时需要 Runtime 的头文件和库，运行时也需要它的代码。

本 Demo 把 Runtime 构建成静态库并链接进 `fn_demo`。因此最终执行 `fn_demo` 时：

- **不需要 Java**；
- **需要的 C++ Runtime 已静态链接进程序**；
- Java 只在修改 `.g4` 后重新运行 Generator 时出现。

生成代码会调用 Runtime 的版本检查；二者在本项目中都来自 4.13.2。

## 为什么错误输入会失败

把示例改成：

```sql
NF ADD FD score (student_id) ??? (score);
```

`?` 没有对应 Lexer 规则，因此 Lexer 会报告 `token recognition error`。跳过这些无法识别的字符后，Parser 在 grammar 要求 `ARROW` 的位置看到了 `LPAREN`，还会报告类似 `mismatched input '(' expecting '->'` 的语法错误。

由于 `main.cpp` 同时检查：

```cpp
lexer.getNumberOfSyntaxErrors()
parser.getNumberOfSyntaxErrors()
```

只要词法或语法阶段记录到错误，程序就打印 `SQL syntax error` 并返回 1。Parser 检查是题目要求的核心；额外的 Lexer 检查避免无法识别的字符被跳过后误判成功。`statement` 末尾的 `EOF` 还保证多余的、可被 Lexer 识别的尾部输入也不会被静默忽略。

## 六个关键对象的关系

1. `ANTLRInputStream` 持有 SQL 字符序列。
2. `FNSqlLexer` 读取这个字符流，根据大写 Lexer rule 产生 Token。
3. `CommonTokenStream` 从 Lexer 获取并缓存 Token。
4. `FNSqlParser` 消费 TokenStream，根据小写 Parser rule 建树。
5. `StatementContext` 是根规则 `statement` 对应的 Parse Tree 根节点。
6. `NfStatementContext` 是根节点下面 `nfStatement` 规则对应的子节点，可继续访问表名和两个属性列表。

最后，Demo 从这些 Context 中复制文本到 `AddFDStatement`。复制完成后，输出业务结果的代码只依赖普通的 `std::string` 和 `std::vector<std::string>`，不再依赖 Parse Tree。
