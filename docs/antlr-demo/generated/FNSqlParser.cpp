
// Generated from FNSql.g4 by ANTLR 4.13.2



#include "FNSqlParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct FNSqlParserStaticData final {
  FNSqlParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  FNSqlParserStaticData(const FNSqlParserStaticData&) = delete;
  FNSqlParserStaticData(FNSqlParserStaticData&&) = delete;
  FNSqlParserStaticData& operator=(const FNSqlParserStaticData&) = delete;
  FNSqlParserStaticData& operator=(FNSqlParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag fnsqlParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<FNSqlParserStaticData> fnsqlParserStaticData = nullptr;

void fnsqlParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (fnsqlParserStaticData != nullptr) {
    return;
  }
#else
  assert(fnsqlParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<FNSqlParserStaticData>(
    std::vector<std::string>{
      "statement", "nfStatement", "tableName", "columnList", "identifierList"
    },
    std::vector<std::string>{
      "", "'NF'", "'ADD'", "'FD'", "'->'", "'('", "')'", "','", "';'"
    },
    std::vector<std::string>{
      "", "NF", "ADD", "FD", "ARROW", "LPAREN", "RPAREN", "COMMA", "SEMI", 
      "IDENTIFIER", "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,10,39,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,1,0,1,0,3,0,13,8,0,
  	1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,1,3,1,3,1,3,1,3,1,4,1,
  	4,1,4,5,4,34,8,4,10,4,12,4,37,9,4,1,4,0,0,5,0,2,4,6,8,0,0,35,0,10,1,0,
  	0,0,2,16,1,0,0,0,4,24,1,0,0,0,6,26,1,0,0,0,8,30,1,0,0,0,10,12,3,2,1,0,
  	11,13,5,8,0,0,12,11,1,0,0,0,12,13,1,0,0,0,13,14,1,0,0,0,14,15,5,0,0,1,
  	15,1,1,0,0,0,16,17,5,1,0,0,17,18,5,2,0,0,18,19,5,3,0,0,19,20,3,4,2,0,
  	20,21,3,6,3,0,21,22,5,4,0,0,22,23,3,6,3,0,23,3,1,0,0,0,24,25,5,9,0,0,
  	25,5,1,0,0,0,26,27,5,5,0,0,27,28,3,8,4,0,28,29,5,6,0,0,29,7,1,0,0,0,30,
  	35,5,9,0,0,31,32,5,7,0,0,32,34,5,9,0,0,33,31,1,0,0,0,34,37,1,0,0,0,35,
  	33,1,0,0,0,35,36,1,0,0,0,36,9,1,0,0,0,37,35,1,0,0,0,2,12,35
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  fnsqlParserStaticData = std::move(staticData);
}

}

FNSqlParser::FNSqlParser(TokenStream *input) : FNSqlParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

FNSqlParser::FNSqlParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  FNSqlParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *fnsqlParserStaticData->atn, fnsqlParserStaticData->decisionToDFA, fnsqlParserStaticData->sharedContextCache, options);
}

FNSqlParser::~FNSqlParser() {
  delete _interpreter;
}

const atn::ATN& FNSqlParser::getATN() const {
  return *fnsqlParserStaticData->atn;
}

std::string FNSqlParser::getGrammarFileName() const {
  return "FNSql.g4";
}

const std::vector<std::string>& FNSqlParser::getRuleNames() const {
  return fnsqlParserStaticData->ruleNames;
}

const dfa::Vocabulary& FNSqlParser::getVocabulary() const {
  return fnsqlParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView FNSqlParser::getSerializedATN() const {
  return fnsqlParserStaticData->serializedATN;
}


//----------------- StatementContext ------------------------------------------------------------------

FNSqlParser::StatementContext::StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FNSqlParser::NfStatementContext* FNSqlParser::StatementContext::nfStatement() {
  return getRuleContext<FNSqlParser::NfStatementContext>(0);
}

tree::TerminalNode* FNSqlParser::StatementContext::EOF() {
  return getToken(FNSqlParser::EOF, 0);
}

tree::TerminalNode* FNSqlParser::StatementContext::SEMI() {
  return getToken(FNSqlParser::SEMI, 0);
}


size_t FNSqlParser::StatementContext::getRuleIndex() const {
  return FNSqlParser::RuleStatement;
}


FNSqlParser::StatementContext* FNSqlParser::statement() {
  StatementContext *_localctx = _tracker.createInstance<StatementContext>(_ctx, getState());
  enterRule(_localctx, 0, FNSqlParser::RuleStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(10);
    nfStatement();
    setState(12);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FNSqlParser::SEMI) {
      setState(11);
      match(FNSqlParser::SEMI);
    }
    setState(14);
    match(FNSqlParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NfStatementContext ------------------------------------------------------------------

FNSqlParser::NfStatementContext::NfStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FNSqlParser::NfStatementContext::NF() {
  return getToken(FNSqlParser::NF, 0);
}

tree::TerminalNode* FNSqlParser::NfStatementContext::ADD() {
  return getToken(FNSqlParser::ADD, 0);
}

tree::TerminalNode* FNSqlParser::NfStatementContext::FD() {
  return getToken(FNSqlParser::FD, 0);
}

FNSqlParser::TableNameContext* FNSqlParser::NfStatementContext::tableName() {
  return getRuleContext<FNSqlParser::TableNameContext>(0);
}

std::vector<FNSqlParser::ColumnListContext *> FNSqlParser::NfStatementContext::columnList() {
  return getRuleContexts<FNSqlParser::ColumnListContext>();
}

FNSqlParser::ColumnListContext* FNSqlParser::NfStatementContext::columnList(size_t i) {
  return getRuleContext<FNSqlParser::ColumnListContext>(i);
}

tree::TerminalNode* FNSqlParser::NfStatementContext::ARROW() {
  return getToken(FNSqlParser::ARROW, 0);
}


size_t FNSqlParser::NfStatementContext::getRuleIndex() const {
  return FNSqlParser::RuleNfStatement;
}


FNSqlParser::NfStatementContext* FNSqlParser::nfStatement() {
  NfStatementContext *_localctx = _tracker.createInstance<NfStatementContext>(_ctx, getState());
  enterRule(_localctx, 2, FNSqlParser::RuleNfStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(16);
    match(FNSqlParser::NF);
    setState(17);
    match(FNSqlParser::ADD);
    setState(18);
    match(FNSqlParser::FD);
    setState(19);
    tableName();
    setState(20);
    columnList();
    setState(21);
    match(FNSqlParser::ARROW);
    setState(22);
    columnList();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TableNameContext ------------------------------------------------------------------

FNSqlParser::TableNameContext::TableNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FNSqlParser::TableNameContext::IDENTIFIER() {
  return getToken(FNSqlParser::IDENTIFIER, 0);
}


size_t FNSqlParser::TableNameContext::getRuleIndex() const {
  return FNSqlParser::RuleTableName;
}


FNSqlParser::TableNameContext* FNSqlParser::tableName() {
  TableNameContext *_localctx = _tracker.createInstance<TableNameContext>(_ctx, getState());
  enterRule(_localctx, 4, FNSqlParser::RuleTableName);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(24);
    match(FNSqlParser::IDENTIFIER);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ColumnListContext ------------------------------------------------------------------

FNSqlParser::ColumnListContext::ColumnListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FNSqlParser::ColumnListContext::LPAREN() {
  return getToken(FNSqlParser::LPAREN, 0);
}

FNSqlParser::IdentifierListContext* FNSqlParser::ColumnListContext::identifierList() {
  return getRuleContext<FNSqlParser::IdentifierListContext>(0);
}

tree::TerminalNode* FNSqlParser::ColumnListContext::RPAREN() {
  return getToken(FNSqlParser::RPAREN, 0);
}


size_t FNSqlParser::ColumnListContext::getRuleIndex() const {
  return FNSqlParser::RuleColumnList;
}


FNSqlParser::ColumnListContext* FNSqlParser::columnList() {
  ColumnListContext *_localctx = _tracker.createInstance<ColumnListContext>(_ctx, getState());
  enterRule(_localctx, 6, FNSqlParser::RuleColumnList);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(26);
    match(FNSqlParser::LPAREN);
    setState(27);
    identifierList();
    setState(28);
    match(FNSqlParser::RPAREN);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IdentifierListContext ------------------------------------------------------------------

FNSqlParser::IdentifierListContext::IdentifierListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FNSqlParser::IdentifierListContext::IDENTIFIER() {
  return getTokens(FNSqlParser::IDENTIFIER);
}

tree::TerminalNode* FNSqlParser::IdentifierListContext::IDENTIFIER(size_t i) {
  return getToken(FNSqlParser::IDENTIFIER, i);
}

std::vector<tree::TerminalNode *> FNSqlParser::IdentifierListContext::COMMA() {
  return getTokens(FNSqlParser::COMMA);
}

tree::TerminalNode* FNSqlParser::IdentifierListContext::COMMA(size_t i) {
  return getToken(FNSqlParser::COMMA, i);
}


size_t FNSqlParser::IdentifierListContext::getRuleIndex() const {
  return FNSqlParser::RuleIdentifierList;
}


FNSqlParser::IdentifierListContext* FNSqlParser::identifierList() {
  IdentifierListContext *_localctx = _tracker.createInstance<IdentifierListContext>(_ctx, getState());
  enterRule(_localctx, 8, FNSqlParser::RuleIdentifierList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(30);
    match(FNSqlParser::IDENTIFIER);
    setState(35);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FNSqlParser::COMMA) {
      setState(31);
      match(FNSqlParser::COMMA);
      setState(32);
      match(FNSqlParser::IDENTIFIER);
      setState(37);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void FNSqlParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  fnsqlParserInitialize();
#else
  ::antlr4::internal::call_once(fnsqlParserOnceFlag, fnsqlParserInitialize);
#endif
}
