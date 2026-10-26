// Generated from /root/Desktop/FN-database/grammar/FNCommand.g4 by ANTLR 4.13.1
import org.antlr.v4.runtime.atn.*;
import org.antlr.v4.runtime.dfa.DFA;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.*;
import org.antlr.v4.runtime.tree.*;
import java.util.List;
import java.util.Iterator;
import java.util.ArrayList;

@SuppressWarnings({"all", "warnings", "unchecked", "unused", "cast", "CheckReturnValue"})
public class FNCommandParser extends Parser {
	static { RuntimeMetaData.checkVersion("4.13.1", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		SET=1, SHOW=2, NF_MODE=3, TWO_NF=4, THREE_NF=5, BCNF=6, OFF=7, EQUAL=8, 
		SEMI=9, IDENTIFIER=10, BLOCK_COMMENT=11, LINE_COMMENT=12, HASH_COMMENT=13, 
		WS=14, OTHER=15;
	public static final int
		RULE_statement = 0, RULE_setModeStatement = 1, RULE_showModeStatement = 2, 
		RULE_normalizationMode = 3;
	private static String[] makeRuleNames() {
		return new String[] {
			"statement", "setModeStatement", "showModeStatement", "normalizationMode"
		};
	}
	public static final String[] ruleNames = makeRuleNames();

	private static String[] makeLiteralNames() {
		return new String[] {
			null, null, null, null, null, null, null, null, "'='", "';'"
		};
	}
	private static final String[] _LITERAL_NAMES = makeLiteralNames();
	private static String[] makeSymbolicNames() {
		return new String[] {
			null, "SET", "SHOW", "NF_MODE", "TWO_NF", "THREE_NF", "BCNF", "OFF", 
			"EQUAL", "SEMI", "IDENTIFIER", "BLOCK_COMMENT", "LINE_COMMENT", "HASH_COMMENT", 
			"WS", "OTHER"
		};
	}
	private static final String[] _SYMBOLIC_NAMES = makeSymbolicNames();
	public static final Vocabulary VOCABULARY = new VocabularyImpl(_LITERAL_NAMES, _SYMBOLIC_NAMES);

	/**
	 * @deprecated Use {@link #VOCABULARY} instead.
	 */
	@Deprecated
	public static final String[] tokenNames;
	static {
		tokenNames = new String[_SYMBOLIC_NAMES.length];
		for (int i = 0; i < tokenNames.length; i++) {
			tokenNames[i] = VOCABULARY.getLiteralName(i);
			if (tokenNames[i] == null) {
				tokenNames[i] = VOCABULARY.getSymbolicName(i);
			}

			if (tokenNames[i] == null) {
				tokenNames[i] = "<INVALID>";
			}
		}
	}

	@Override
	@Deprecated
	public String[] getTokenNames() {
		return tokenNames;
	}

	@Override

	public Vocabulary getVocabulary() {
		return VOCABULARY;
	}

	@Override
	public String getGrammarFileName() { return "FNCommand.g4"; }

	@Override
	public String[] getRuleNames() { return ruleNames; }

	@Override
	public String getSerializedATN() { return _serializedATN; }

	@Override
	public ATN getATN() { return _ATN; }

	public FNCommandParser(TokenStream input) {
		super(input);
		_interp = new ParserATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@SuppressWarnings("CheckReturnValue")
	public static class StatementContext extends ParserRuleContext {
		public SetModeStatementContext setModeStatement() {
			return getRuleContext(SetModeStatementContext.class,0);
		}
		public TerminalNode EOF() { return getToken(FNCommandParser.EOF, 0); }
		public TerminalNode SEMI() { return getToken(FNCommandParser.SEMI, 0); }
		public ShowModeStatementContext showModeStatement() {
			return getRuleContext(ShowModeStatementContext.class,0);
		}
		public StatementContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_statement; }
	}

	public final StatementContext statement() throws RecognitionException {
		StatementContext _localctx = new StatementContext(_ctx, getState());
		enterRule(_localctx, 0, RULE_statement);
		int _la;
		try {
			setState(20);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case SET:
				enterOuterAlt(_localctx, 1);
				{
				setState(8);
				setModeStatement();
				setState(10);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==SEMI) {
					{
					setState(9);
					match(SEMI);
					}
				}

				setState(12);
				match(EOF);
				}
				break;
			case SHOW:
				enterOuterAlt(_localctx, 2);
				{
				setState(14);
				showModeStatement();
				setState(16);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==SEMI) {
					{
					setState(15);
					match(SEMI);
					}
				}

				setState(18);
				match(EOF);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class SetModeStatementContext extends ParserRuleContext {
		public TerminalNode SET() { return getToken(FNCommandParser.SET, 0); }
		public TerminalNode NF_MODE() { return getToken(FNCommandParser.NF_MODE, 0); }
		public TerminalNode EQUAL() { return getToken(FNCommandParser.EQUAL, 0); }
		public NormalizationModeContext normalizationMode() {
			return getRuleContext(NormalizationModeContext.class,0);
		}
		public SetModeStatementContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_setModeStatement; }
	}

	public final SetModeStatementContext setModeStatement() throws RecognitionException {
		SetModeStatementContext _localctx = new SetModeStatementContext(_ctx, getState());
		enterRule(_localctx, 2, RULE_setModeStatement);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(22);
			match(SET);
			setState(23);
			match(NF_MODE);
			setState(24);
			match(EQUAL);
			setState(25);
			normalizationMode();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ShowModeStatementContext extends ParserRuleContext {
		public TerminalNode SHOW() { return getToken(FNCommandParser.SHOW, 0); }
		public TerminalNode NF_MODE() { return getToken(FNCommandParser.NF_MODE, 0); }
		public ShowModeStatementContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_showModeStatement; }
	}

	public final ShowModeStatementContext showModeStatement() throws RecognitionException {
		ShowModeStatementContext _localctx = new ShowModeStatementContext(_ctx, getState());
		enterRule(_localctx, 4, RULE_showModeStatement);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(27);
			match(SHOW);
			setState(28);
			match(NF_MODE);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class NormalizationModeContext extends ParserRuleContext {
		public TerminalNode TWO_NF() { return getToken(FNCommandParser.TWO_NF, 0); }
		public TerminalNode THREE_NF() { return getToken(FNCommandParser.THREE_NF, 0); }
		public TerminalNode BCNF() { return getToken(FNCommandParser.BCNF, 0); }
		public TerminalNode OFF() { return getToken(FNCommandParser.OFF, 0); }
		public NormalizationModeContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_normalizationMode; }
	}

	public final NormalizationModeContext normalizationMode() throws RecognitionException {
		NormalizationModeContext _localctx = new NormalizationModeContext(_ctx, getState());
		enterRule(_localctx, 6, RULE_normalizationMode);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(30);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 240L) != 0)) ) {
			_errHandler.recoverInline(this);
			}
			else {
				if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
				_errHandler.reportMatch(this);
				consume();
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	public static final String _serializedATN =
		"\u0004\u0001\u000f!\u0002\u0000\u0007\u0000\u0002\u0001\u0007\u0001\u0002"+
		"\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0001\u0000\u0001\u0000\u0003"+
		"\u0000\u000b\b\u0000\u0001\u0000\u0001\u0000\u0001\u0000\u0001\u0000\u0003"+
		"\u0000\u0011\b\u0000\u0001\u0000\u0001\u0000\u0003\u0000\u0015\b\u0000"+
		"\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0002"+
		"\u0001\u0002\u0001\u0002\u0001\u0003\u0001\u0003\u0001\u0003\u0000\u0000"+
		"\u0004\u0000\u0002\u0004\u0006\u0000\u0001\u0001\u0000\u0004\u0007\u001f"+
		"\u0000\u0014\u0001\u0000\u0000\u0000\u0002\u0016\u0001\u0000\u0000\u0000"+
		"\u0004\u001b\u0001\u0000\u0000\u0000\u0006\u001e\u0001\u0000\u0000\u0000"+
		"\b\n\u0003\u0002\u0001\u0000\t\u000b\u0005\t\u0000\u0000\n\t\u0001\u0000"+
		"\u0000\u0000\n\u000b\u0001\u0000\u0000\u0000\u000b\f\u0001\u0000\u0000"+
		"\u0000\f\r\u0005\u0000\u0000\u0001\r\u0015\u0001\u0000\u0000\u0000\u000e"+
		"\u0010\u0003\u0004\u0002\u0000\u000f\u0011\u0005\t\u0000\u0000\u0010\u000f"+
		"\u0001\u0000\u0000\u0000\u0010\u0011\u0001\u0000\u0000\u0000\u0011\u0012"+
		"\u0001\u0000\u0000\u0000\u0012\u0013\u0005\u0000\u0000\u0001\u0013\u0015"+
		"\u0001\u0000\u0000\u0000\u0014\b\u0001\u0000\u0000\u0000\u0014\u000e\u0001"+
		"\u0000\u0000\u0000\u0015\u0001\u0001\u0000\u0000\u0000\u0016\u0017\u0005"+
		"\u0001\u0000\u0000\u0017\u0018\u0005\u0003\u0000\u0000\u0018\u0019\u0005"+
		"\b\u0000\u0000\u0019\u001a\u0003\u0006\u0003\u0000\u001a\u0003\u0001\u0000"+
		"\u0000\u0000\u001b\u001c\u0005\u0002\u0000\u0000\u001c\u001d\u0005\u0003"+
		"\u0000\u0000\u001d\u0005\u0001\u0000\u0000\u0000\u001e\u001f\u0007\u0000"+
		"\u0000\u0000\u001f\u0007\u0001\u0000\u0000\u0000\u0003\n\u0010\u0014";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}