// 同一个 .g4 文件同时包含 Parser 规则（小写开头）和 Lexer 规则（大写开头）。 ANTLR Generator 会据此生成 FNCommandLexer 和
// FNCommandParser 两组 C++ 类。
grammar FNCommand;

// MySQL 客户端通常不会把末尾分号放进 COM_QUERY，因此分号可选。 EOF 保证整条输入都被消费，避免只接受一个合法前缀。
statement:
	setModeStatement SEMI? EOF // ? 表示 SEMI 出现0次或1次 EOF 表示已经输入到结尾
	| showModeStatement SEMI? EOF; // 竖线表示两个可选分支：SET 或 SHOW。

// SET 语句必须包含等号以及四种 normalizationMode 之一。
setModeStatement: SET NF_MODE EQUAL normalizationMode;

// SHOW 语句没有额外参数。
showModeStatement: SHOW NF_MODE;

// normalizationMode 把多个 Lexer Token 归纳为一个 Parser 概念。
normalizationMode: TWO_NF | THREE_NF | BCNF | OFF;

// 关键字按 MySQL 的使用习惯做成大小写不敏感。 例如 SET : S E T; 会同时接受 SET、set、Set 等组合。
SET: S E T;
SHOW: S H O W;
NF_MODE: N F '_' M O D E;
TWO_NF: '2' N F;
THREE_NF: '3' N F;
BCNF: B C N F;
OFF: O F F;
EQUAL: '=';
SEMI: ';';

// 兜底 Token 让普通 MySQL SQL 可以安静地判定为“不是 NF 语句”。 注释和空白使用 -> skip，不会进入 Parser 可见的 TokenStream。
IDENTIFIER: [a-zA-Z_] [a-zA-Z0-9_]*;
BLOCK_COMMENT: '/*' .*? '*/' -> skip;
LINE_COMMENT: '--' ~[\r\n]* -> skip;
HASH_COMMENT: '#' ~[\r\n]* -> skip;
WS: [ \t\r\n]+ -> skip;
// OTHER 必须放在具体规则之后，只匹配前面规则均未识别的单个字符。
OTHER: .;

// fragment 只供其他 Lexer 规则复用，本身不会独立产生 Token。
fragment A: [aA];
fragment B: [bB];
fragment C: [cC];
fragment D: [dD];
fragment E: [eE];
fragment F: [fF];
fragment H: [hH];
fragment M: [mM];
fragment N: [nN];
fragment O: [oO];
fragment S: [sS];
fragment T: [tT];
fragment W: [wW];