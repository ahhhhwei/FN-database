
// Generated from FNCommand.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  FNCommandParser : public antlr4::Parser {
public:
  enum {
    SET = 1, SHOW = 2, NF_MODE = 3, TWO_NF = 4, THREE_NF = 5, BCNF = 6, 
    OFF = 7, EQUAL = 8, SEMI = 9, IDENTIFIER = 10, BLOCK_COMMENT = 11, LINE_COMMENT = 12, 
    HASH_COMMENT = 13, WS = 14, OTHER = 15
  };

  enum {
    RuleStatement = 0, RuleSetModeStatement = 1, RuleShowModeStatement = 2, 
    RuleNormalizationMode = 3
  };

  explicit FNCommandParser(antlr4::TokenStream *input);

  FNCommandParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~FNCommandParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class StatementContext;
  class SetModeStatementContext;
  class ShowModeStatementContext;
  class NormalizationModeContext; 

  class  StatementContext : public antlr4::ParserRuleContext {
  public:
    StatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SetModeStatementContext *setModeStatement();
    antlr4::tree::TerminalNode *EOF();
    antlr4::tree::TerminalNode *SEMI();
    ShowModeStatementContext *showModeStatement();

   
  };

  StatementContext* statement();

  class  SetModeStatementContext : public antlr4::ParserRuleContext {
  public:
    SetModeStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SET();
    antlr4::tree::TerminalNode *NF_MODE();
    antlr4::tree::TerminalNode *EQUAL();
    NormalizationModeContext *normalizationMode();

   
  };

  SetModeStatementContext* setModeStatement();

  class  ShowModeStatementContext : public antlr4::ParserRuleContext {
  public:
    ShowModeStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SHOW();
    antlr4::tree::TerminalNode *NF_MODE();

   
  };

  ShowModeStatementContext* showModeStatement();

  class  NormalizationModeContext : public antlr4::ParserRuleContext {
  public:
    NormalizationModeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *TWO_NF();
    antlr4::tree::TerminalNode *THREE_NF();
    antlr4::tree::TerminalNode *BCNF();
    antlr4::tree::TerminalNode *OFF();

   
  };

  NormalizationModeContext* normalizationMode();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

