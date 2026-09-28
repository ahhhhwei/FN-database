grammar FNCommand;

// MySQL 客户端通常不会把末尾分号放进 COM_QUERY，因此分号可选。
// EOF 保证整条输入都被消费，避免只接受一个合法前缀。
statement
    : setModeStatement SEMI? EOF
    | showModeStatement SEMI? EOF
    ;

setModeStatement
    : SET NF_MODE EQUAL normalizationMode
    ;

showModeStatement
    : SHOW NF_MODE
    ;

normalizationMode
    : TWO_NF
    | THREE_NF
    | BCNF
    | OFF
    ;

// 关键字按 MySQL 的使用习惯做成大小写不敏感。
SET      : S E T;
SHOW     : S H O W;
NF_MODE  : N F '_' M O D E;
TWO_NF   : '2' N F;
THREE_NF : '3' N F;
BCNF     : B C N F;
OFF      : O F F;
EQUAL    : '=';
SEMI     : ';';

// 兜底 Token 让普通 MySQL SQL 可以安静地判定为“不是 NF 语句”。
IDENTIFIER    : [a-zA-Z_] [a-zA-Z0-9_]*;
BLOCK_COMMENT : '/*' .*? '*/' -> skip;
LINE_COMMENT  : '--' ~[\r\n]* -> skip;
HASH_COMMENT  : '#' ~[\r\n]* -> skip;
WS            : [ \t\r\n]+ -> skip;
OTHER         : .;

fragment A : [aA];
fragment B : [bB];
fragment C : [cC];
fragment D : [dD];
fragment E : [eE];
fragment F : [fF];
fragment H : [hH];
fragment M : [mM];
fragment N : [nN];
fragment O : [oO];
fragment S : [sS];
fragment T : [tT];
fragment W : [wW];
