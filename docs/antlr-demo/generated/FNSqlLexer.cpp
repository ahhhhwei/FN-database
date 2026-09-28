
// Generated from FNSql.g4 by ANTLR 4.13.2


#include "FNSqlLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct FNSqlLexerStaticData final {
  FNSqlLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  FNSqlLexerStaticData(const FNSqlLexerStaticData&) = delete;
  FNSqlLexerStaticData(FNSqlLexerStaticData&&) = delete;
  FNSqlLexerStaticData& operator=(const FNSqlLexerStaticData&) = delete;
  FNSqlLexerStaticData& operator=(FNSqlLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag fnsqllexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<FNSqlLexerStaticData> fnsqllexerLexerStaticData = nullptr;

void fnsqllexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (fnsqllexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(fnsqllexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<FNSqlLexerStaticData>(
    std::vector<std::string>{
      "NF", "ADD", "FD", "ARROW", "LPAREN", "RPAREN", "COMMA", "SEMI", "IDENTIFIER", 
      "WS"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
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
  	4,0,10,56,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,2,1,2,1,2,1,3,
  	1,3,1,3,1,4,1,4,1,5,1,5,1,6,1,6,1,7,1,7,1,8,1,8,5,8,45,8,8,10,8,12,8,
  	48,9,8,1,9,4,9,51,8,9,11,9,12,9,52,1,9,1,9,0,0,10,1,1,3,2,5,3,7,4,9,5,
  	11,6,13,7,15,8,17,9,19,10,1,0,3,3,0,65,90,95,95,97,122,4,0,48,57,65,90,
  	95,95,97,122,3,0,9,10,13,13,32,32,57,0,1,1,0,0,0,0,3,1,0,0,0,0,5,1,0,
  	0,0,0,7,1,0,0,0,0,9,1,0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,1,0,0,0,0,
  	17,1,0,0,0,0,19,1,0,0,0,1,21,1,0,0,0,3,24,1,0,0,0,5,28,1,0,0,0,7,31,1,
  	0,0,0,9,34,1,0,0,0,11,36,1,0,0,0,13,38,1,0,0,0,15,40,1,0,0,0,17,42,1,
  	0,0,0,19,50,1,0,0,0,21,22,5,78,0,0,22,23,5,70,0,0,23,2,1,0,0,0,24,25,
  	5,65,0,0,25,26,5,68,0,0,26,27,5,68,0,0,27,4,1,0,0,0,28,29,5,70,0,0,29,
  	30,5,68,0,0,30,6,1,0,0,0,31,32,5,45,0,0,32,33,5,62,0,0,33,8,1,0,0,0,34,
  	35,5,40,0,0,35,10,1,0,0,0,36,37,5,41,0,0,37,12,1,0,0,0,38,39,5,44,0,0,
  	39,14,1,0,0,0,40,41,5,59,0,0,41,16,1,0,0,0,42,46,7,0,0,0,43,45,7,1,0,
  	0,44,43,1,0,0,0,45,48,1,0,0,0,46,44,1,0,0,0,46,47,1,0,0,0,47,18,1,0,0,
  	0,48,46,1,0,0,0,49,51,7,2,0,0,50,49,1,0,0,0,51,52,1,0,0,0,52,50,1,0,0,
  	0,52,53,1,0,0,0,53,54,1,0,0,0,54,55,6,9,0,0,55,20,1,0,0,0,3,0,46,52,1,
  	6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  fnsqllexerLexerStaticData = std::move(staticData);
}

}

FNSqlLexer::FNSqlLexer(CharStream *input) : Lexer(input) {
  FNSqlLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *fnsqllexerLexerStaticData->atn, fnsqllexerLexerStaticData->decisionToDFA, fnsqllexerLexerStaticData->sharedContextCache);
}

FNSqlLexer::~FNSqlLexer() {
  delete _interpreter;
}

std::string FNSqlLexer::getGrammarFileName() const {
  return "FNSql.g4";
}

const std::vector<std::string>& FNSqlLexer::getRuleNames() const {
  return fnsqllexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& FNSqlLexer::getChannelNames() const {
  return fnsqllexerLexerStaticData->channelNames;
}

const std::vector<std::string>& FNSqlLexer::getModeNames() const {
  return fnsqllexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& FNSqlLexer::getVocabulary() const {
  return fnsqllexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView FNSqlLexer::getSerializedATN() const {
  return fnsqllexerLexerStaticData->serializedATN;
}

const atn::ATN& FNSqlLexer::getATN() const {
  return *fnsqllexerLexerStaticData->atn;
}




void FNSqlLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  fnsqllexerLexerInitialize();
#else
  ::antlr4::internal::call_once(fnsqllexerLexerOnceFlag, fnsqllexerLexerInitialize);
#endif
}
