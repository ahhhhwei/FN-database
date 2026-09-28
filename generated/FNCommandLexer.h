
// Generated from FNCommand.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  FNCommandLexer : public antlr4::Lexer {
public:
  enum {
    SET = 1, SHOW = 2, NF_MODE = 3, TWO_NF = 4, THREE_NF = 5, BCNF = 6, 
    OFF = 7, EQUAL = 8, SEMI = 9, IDENTIFIER = 10, BLOCK_COMMENT = 11, LINE_COMMENT = 12, 
    HASH_COMMENT = 13, WS = 14, OTHER = 15
  };

  explicit FNCommandLexer(antlr4::CharStream *input);

  ~FNCommandLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

