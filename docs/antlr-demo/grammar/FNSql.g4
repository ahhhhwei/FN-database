grammar FNSql;

// 最外层规则必须匹配 EOF，避免只接受一段合法前缀。
statement
    : nfStatement SEMI? EOF
    ;

nfStatement
    : NF ADD FD tableName columnList ARROW columnList
    ;

tableName
    : IDENTIFIER
    ;

columnList
    : LPAREN identifierList RPAREN
    ;

// 至少一个标识符；后面可以有任意多个“逗号 + 标识符”。
identifierList
    : IDENTIFIER (COMMA IDENTIFIER)*
    ;

NF         : 'NF';
ADD        : 'ADD';
FD         : 'FD';
ARROW      : '->';
LPAREN     : '(';
RPAREN     : ')';
COMMA      : ',';
SEMI       : ';';
IDENTIFIER : [a-zA-Z_] [a-zA-Z0-9_]*;

// 空格、Tab、CR 和换行只负责分隔 Token，不进入 TokenStream。
WS         : [ \t\r\n]+ -> skip;
