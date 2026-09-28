
// Generated from FNSql.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  FNSqlParser : public antlr4::Parser {
public:
  enum {
    NF = 1, ADD = 2, FD = 3, ARROW = 4, LPAREN = 5, RPAREN = 6, COMMA = 7, 
    SEMI = 8, IDENTIFIER = 9, WS = 10
  };

  enum {
    RuleStatement = 0, RuleNfStatement = 1, RuleTableName = 2, RuleColumnList = 3, 
    RuleIdentifierList = 4
  };

  explicit FNSqlParser(antlr4::TokenStream *input);

  FNSqlParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~FNSqlParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class StatementContext;
  class NfStatementContext;
  class TableNameContext;
  class ColumnListContext;
  class IdentifierListContext; 

  class  StatementContext : public antlr4::ParserRuleContext {
  public:
    StatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    NfStatementContext *nfStatement();
    antlr4::tree::TerminalNode *EOF();
    antlr4::tree::TerminalNode *SEMI();

   
  };

  StatementContext* statement();

  class  NfStatementContext : public antlr4::ParserRuleContext {
  public:
    NfStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *NF();
    antlr4::tree::TerminalNode *ADD();
    antlr4::tree::TerminalNode *FD();
    TableNameContext *tableName();
    std::vector<ColumnListContext *> columnList();
    ColumnListContext* columnList(size_t i);
    antlr4::tree::TerminalNode *ARROW();

   
  };

  NfStatementContext* nfStatement();

  class  TableNameContext : public antlr4::ParserRuleContext {
  public:
    TableNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENTIFIER();

   
  };

  TableNameContext* tableName();

  class  ColumnListContext : public antlr4::ParserRuleContext {
  public:
    ColumnListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LPAREN();
    IdentifierListContext *identifierList();
    antlr4::tree::TerminalNode *RPAREN();

   
  };

  ColumnListContext* columnList();

  class  IdentifierListContext : public antlr4::ParserRuleContext {
  public:
    IdentifierListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> IDENTIFIER();
    antlr4::tree::TerminalNode* IDENTIFIER(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);

   
  };

  IdentifierListContext* identifierList();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

