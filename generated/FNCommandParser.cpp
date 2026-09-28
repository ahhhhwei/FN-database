
// Generated from FNCommand.g4 by ANTLR 4.13.2



#include "FNCommandParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct FNCommandParserStaticData final {
  FNCommandParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  FNCommandParserStaticData(const FNCommandParserStaticData&) = delete;
  FNCommandParserStaticData(FNCommandParserStaticData&&) = delete;
  FNCommandParserStaticData& operator=(const FNCommandParserStaticData&) = delete;
  FNCommandParserStaticData& operator=(FNCommandParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag fncommandParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<FNCommandParserStaticData> fncommandParserStaticData = nullptr;

void fncommandParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (fncommandParserStaticData != nullptr) {
    return;
  }
#else
  assert(fncommandParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<FNCommandParserStaticData>(
    std::vector<std::string>{
      "statement", "setModeStatement", "showModeStatement", "normalizationMode"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "'='", "';'"
    },
    std::vector<std::string>{
      "", "SET", "SHOW", "NF_MODE", "TWO_NF", "THREE_NF", "BCNF", "OFF", 
      "EQUAL", "SEMI", "IDENTIFIER", "BLOCK_COMMENT", "LINE_COMMENT", "HASH_COMMENT", 
      "WS", "OTHER"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,15,33,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,1,0,1,0,3,0,11,8,0,1,0,1,0,
  	1,0,1,0,3,0,17,8,0,1,0,1,0,3,0,21,8,0,1,1,1,1,1,1,1,1,1,1,1,2,1,2,1,2,
  	1,3,1,3,1,3,0,0,4,0,2,4,6,0,1,1,0,4,7,31,0,20,1,0,0,0,2,22,1,0,0,0,4,
  	27,1,0,0,0,6,30,1,0,0,0,8,10,3,2,1,0,9,11,5,9,0,0,10,9,1,0,0,0,10,11,
  	1,0,0,0,11,12,1,0,0,0,12,13,5,0,0,1,13,21,1,0,0,0,14,16,3,4,2,0,15,17,
  	5,9,0,0,16,15,1,0,0,0,16,17,1,0,0,0,17,18,1,0,0,0,18,19,5,0,0,1,19,21,
  	1,0,0,0,20,8,1,0,0,0,20,14,1,0,0,0,21,1,1,0,0,0,22,23,5,1,0,0,23,24,5,
  	3,0,0,24,25,5,8,0,0,25,26,3,6,3,0,26,3,1,0,0,0,27,28,5,2,0,0,28,29,5,
  	3,0,0,29,5,1,0,0,0,30,31,7,0,0,0,31,7,1,0,0,0,3,10,16,20
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  fncommandParserStaticData = std::move(staticData);
}

}

FNCommandParser::FNCommandParser(TokenStream *input) : FNCommandParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

FNCommandParser::FNCommandParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  FNCommandParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *fncommandParserStaticData->atn, fncommandParserStaticData->decisionToDFA, fncommandParserStaticData->sharedContextCache, options);
}

FNCommandParser::~FNCommandParser() {
  delete _interpreter;
}

const atn::ATN& FNCommandParser::getATN() const {
  return *fncommandParserStaticData->atn;
}

std::string FNCommandParser::getGrammarFileName() const {
  return "FNCommand.g4";
}

const std::vector<std::string>& FNCommandParser::getRuleNames() const {
  return fncommandParserStaticData->ruleNames;
}

const dfa::Vocabulary& FNCommandParser::getVocabulary() const {
  return fncommandParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView FNCommandParser::getSerializedATN() const {
  return fncommandParserStaticData->serializedATN;
}


//----------------- StatementContext ------------------------------------------------------------------

FNCommandParser::StatementContext::StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FNCommandParser::SetModeStatementContext* FNCommandParser::StatementContext::setModeStatement() {
  return getRuleContext<FNCommandParser::SetModeStatementContext>(0);
}

tree::TerminalNode* FNCommandParser::StatementContext::EOF() {
  return getToken(FNCommandParser::EOF, 0);
}

tree::TerminalNode* FNCommandParser::StatementContext::SEMI() {
  return getToken(FNCommandParser::SEMI, 0);
}

FNCommandParser::ShowModeStatementContext* FNCommandParser::StatementContext::showModeStatement() {
  return getRuleContext<FNCommandParser::ShowModeStatementContext>(0);
}


size_t FNCommandParser::StatementContext::getRuleIndex() const {
  return FNCommandParser::RuleStatement;
}


FNCommandParser::StatementContext* FNCommandParser::statement() {
  StatementContext *_localctx = _tracker.createInstance<StatementContext>(_ctx, getState());
  enterRule(_localctx, 0, FNCommandParser::RuleStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(20);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FNCommandParser::SET: {
        enterOuterAlt(_localctx, 1);
        setState(8);
        setModeStatement();
        setState(10);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FNCommandParser::SEMI) {
          setState(9);
          match(FNCommandParser::SEMI);
        }
        setState(12);
        match(FNCommandParser::EOF);
        break;
      }

      case FNCommandParser::SHOW: {
        enterOuterAlt(_localctx, 2);
        setState(14);
        showModeStatement();
        setState(16);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FNCommandParser::SEMI) {
          setState(15);
          match(FNCommandParser::SEMI);
        }
        setState(18);
        match(FNCommandParser::EOF);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SetModeStatementContext ------------------------------------------------------------------

FNCommandParser::SetModeStatementContext::SetModeStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FNCommandParser::SetModeStatementContext::SET() {
  return getToken(FNCommandParser::SET, 0);
}

tree::TerminalNode* FNCommandParser::SetModeStatementContext::NF_MODE() {
  return getToken(FNCommandParser::NF_MODE, 0);
}

tree::TerminalNode* FNCommandParser::SetModeStatementContext::EQUAL() {
  return getToken(FNCommandParser::EQUAL, 0);
}

FNCommandParser::NormalizationModeContext* FNCommandParser::SetModeStatementContext::normalizationMode() {
  return getRuleContext<FNCommandParser::NormalizationModeContext>(0);
}


size_t FNCommandParser::SetModeStatementContext::getRuleIndex() const {
  return FNCommandParser::RuleSetModeStatement;
}


FNCommandParser::SetModeStatementContext* FNCommandParser::setModeStatement() {
  SetModeStatementContext *_localctx = _tracker.createInstance<SetModeStatementContext>(_ctx, getState());
  enterRule(_localctx, 2, FNCommandParser::RuleSetModeStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(22);
    match(FNCommandParser::SET);
    setState(23);
    match(FNCommandParser::NF_MODE);
    setState(24);
    match(FNCommandParser::EQUAL);
    setState(25);
    normalizationMode();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ShowModeStatementContext ------------------------------------------------------------------

FNCommandParser::ShowModeStatementContext::ShowModeStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FNCommandParser::ShowModeStatementContext::SHOW() {
  return getToken(FNCommandParser::SHOW, 0);
}

tree::TerminalNode* FNCommandParser::ShowModeStatementContext::NF_MODE() {
  return getToken(FNCommandParser::NF_MODE, 0);
}


size_t FNCommandParser::ShowModeStatementContext::getRuleIndex() const {
  return FNCommandParser::RuleShowModeStatement;
}


FNCommandParser::ShowModeStatementContext* FNCommandParser::showModeStatement() {
  ShowModeStatementContext *_localctx = _tracker.createInstance<ShowModeStatementContext>(_ctx, getState());
  enterRule(_localctx, 4, FNCommandParser::RuleShowModeStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(27);
    match(FNCommandParser::SHOW);
    setState(28);
    match(FNCommandParser::NF_MODE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NormalizationModeContext ------------------------------------------------------------------

FNCommandParser::NormalizationModeContext::NormalizationModeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FNCommandParser::NormalizationModeContext::TWO_NF() {
  return getToken(FNCommandParser::TWO_NF, 0);
}

tree::TerminalNode* FNCommandParser::NormalizationModeContext::THREE_NF() {
  return getToken(FNCommandParser::THREE_NF, 0);
}

tree::TerminalNode* FNCommandParser::NormalizationModeContext::BCNF() {
  return getToken(FNCommandParser::BCNF, 0);
}

tree::TerminalNode* FNCommandParser::NormalizationModeContext::OFF() {
  return getToken(FNCommandParser::OFF, 0);
}


size_t FNCommandParser::NormalizationModeContext::getRuleIndex() const {
  return FNCommandParser::RuleNormalizationMode;
}


FNCommandParser::NormalizationModeContext* FNCommandParser::normalizationMode() {
  NormalizationModeContext *_localctx = _tracker.createInstance<NormalizationModeContext>(_ctx, getState());
  enterRule(_localctx, 6, FNCommandParser::RuleNormalizationMode);
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
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 240) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void FNCommandParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  fncommandParserInitialize();
#else
  ::antlr4::internal::call_once(fncommandParserOnceFlag, fncommandParserInitialize);
#endif
}
