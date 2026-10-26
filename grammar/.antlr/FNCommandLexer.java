// Generated from /root/Desktop/FN-database/grammar/FNCommand.g4 by ANTLR 4.13.1
import org.antlr.v4.runtime.Lexer;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.TokenStream;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.atn.*;
import org.antlr.v4.runtime.dfa.DFA;
import org.antlr.v4.runtime.misc.*;

@SuppressWarnings({"all", "warnings", "unchecked", "unused", "cast", "CheckReturnValue", "this-escape"})
public class FNCommandLexer extends Lexer {
	static { RuntimeMetaData.checkVersion("4.13.1", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		SET=1, SHOW=2, NF_MODE=3, TWO_NF=4, THREE_NF=5, BCNF=6, OFF=7, EQUAL=8, 
		SEMI=9, IDENTIFIER=10, BLOCK_COMMENT=11, LINE_COMMENT=12, HASH_COMMENT=13, 
		WS=14, OTHER=15;
	public static String[] channelNames = {
		"DEFAULT_TOKEN_CHANNEL", "HIDDEN"
	};

	public static String[] modeNames = {
		"DEFAULT_MODE"
	};

	private static String[] makeRuleNames() {
		return new String[] {
			"SET", "SHOW", "NF_MODE", "TWO_NF", "THREE_NF", "BCNF", "OFF", "EQUAL", 
			"SEMI", "IDENTIFIER", "BLOCK_COMMENT", "LINE_COMMENT", "HASH_COMMENT", 
			"WS", "OTHER", "A", "B", "C", "D", "E", "F", "H", "M", "N", "O", "S", 
			"T", "W"
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


	public FNCommandLexer(CharStream input) {
		super(input);
		_interp = new LexerATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@Override
	public String getGrammarFileName() { return "FNCommand.g4"; }

	@Override
	public String[] getRuleNames() { return ruleNames; }

	@Override
	public String getSerializedATN() { return _serializedATN; }

	@Override
	public String[] getChannelNames() { return channelNames; }

	@Override
	public String[] getModeNames() { return modeNames; }

	@Override
	public ATN getATN() { return _ATN; }

	public static final String _serializedATN =
		"\u0004\u0000\u000f\u00ab\u0006\uffff\uffff\u0002\u0000\u0007\u0000\u0002"+
		"\u0001\u0007\u0001\u0002\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002"+
		"\u0004\u0007\u0004\u0002\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002"+
		"\u0007\u0007\u0007\u0002\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002"+
		"\u000b\u0007\u000b\u0002\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e"+
		"\u0002\u000f\u0007\u000f\u0002\u0010\u0007\u0010\u0002\u0011\u0007\u0011"+
		"\u0002\u0012\u0007\u0012\u0002\u0013\u0007\u0013\u0002\u0014\u0007\u0014"+
		"\u0002\u0015\u0007\u0015\u0002\u0016\u0007\u0016\u0002\u0017\u0007\u0017"+
		"\u0002\u0018\u0007\u0018\u0002\u0019\u0007\u0019\u0002\u001a\u0007\u001a"+
		"\u0002\u001b\u0007\u001b\u0001\u0000\u0001\u0000\u0001\u0000\u0001\u0000"+
		"\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0002"+
		"\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002"+
		"\u0001\u0002\u0001\u0003\u0001\u0003\u0001\u0003\u0001\u0003\u0001\u0004"+
		"\u0001\u0004\u0001\u0004\u0001\u0004\u0001\u0005\u0001\u0005\u0001\u0005"+
		"\u0001\u0005\u0001\u0005\u0001\u0006\u0001\u0006\u0001\u0006\u0001\u0006"+
		"\u0001\u0007\u0001\u0007\u0001\b\u0001\b\u0001\t\u0001\t\u0005\tb\b\t"+
		"\n\t\f\te\t\t\u0001\n\u0001\n\u0001\n\u0001\n\u0005\nk\b\n\n\n\f\nn\t"+
		"\n\u0001\n\u0001\n\u0001\n\u0001\n\u0001\n\u0001\u000b\u0001\u000b\u0001"+
		"\u000b\u0001\u000b\u0005\u000by\b\u000b\n\u000b\f\u000b|\t\u000b\u0001"+
		"\u000b\u0001\u000b\u0001\f\u0001\f\u0005\f\u0082\b\f\n\f\f\f\u0085\t\f"+
		"\u0001\f\u0001\f\u0001\r\u0004\r\u008a\b\r\u000b\r\f\r\u008b\u0001\r\u0001"+
		"\r\u0001\u000e\u0001\u000e\u0001\u000f\u0001\u000f\u0001\u0010\u0001\u0010"+
		"\u0001\u0011\u0001\u0011\u0001\u0012\u0001\u0012\u0001\u0013\u0001\u0013"+
		"\u0001\u0014\u0001\u0014\u0001\u0015\u0001\u0015\u0001\u0016\u0001\u0016"+
		"\u0001\u0017\u0001\u0017\u0001\u0018\u0001\u0018\u0001\u0019\u0001\u0019"+
		"\u0001\u001a\u0001\u001a\u0001\u001b\u0001\u001b\u0001l\u0000\u001c\u0001"+
		"\u0001\u0003\u0002\u0005\u0003\u0007\u0004\t\u0005\u000b\u0006\r\u0007"+
		"\u000f\b\u0011\t\u0013\n\u0015\u000b\u0017\f\u0019\r\u001b\u000e\u001d"+
		"\u000f\u001f\u0000!\u0000#\u0000%\u0000\'\u0000)\u0000+\u0000-\u0000/"+
		"\u00001\u00003\u00005\u00007\u0000\u0001\u0000\u0011\u0003\u0000AZ__a"+
		"z\u0004\u000009AZ__az\u0002\u0000\n\n\r\r\u0003\u0000\t\n\r\r  \u0002"+
		"\u0000AAaa\u0002\u0000BBbb\u0002\u0000CCcc\u0002\u0000DDdd\u0002\u0000"+
		"EEee\u0002\u0000FFff\u0002\u0000HHhh\u0002\u0000MMmm\u0002\u0000NNnn\u0002"+
		"\u0000OOoo\u0002\u0000SSss\u0002\u0000TTtt\u0002\u0000WWww\u00a2\u0000"+
		"\u0001\u0001\u0000\u0000\u0000\u0000\u0003\u0001\u0000\u0000\u0000\u0000"+
		"\u0005\u0001\u0000\u0000\u0000\u0000\u0007\u0001\u0000\u0000\u0000\u0000"+
		"\t\u0001\u0000\u0000\u0000\u0000\u000b\u0001\u0000\u0000\u0000\u0000\r"+
		"\u0001\u0000\u0000\u0000\u0000\u000f\u0001\u0000\u0000\u0000\u0000\u0011"+
		"\u0001\u0000\u0000\u0000\u0000\u0013\u0001\u0000\u0000\u0000\u0000\u0015"+
		"\u0001\u0000\u0000\u0000\u0000\u0017\u0001\u0000\u0000\u0000\u0000\u0019"+
		"\u0001\u0000\u0000\u0000\u0000\u001b\u0001\u0000\u0000\u0000\u0000\u001d"+
		"\u0001\u0000\u0000\u0000\u00019\u0001\u0000\u0000\u0000\u0003=\u0001\u0000"+
		"\u0000\u0000\u0005B\u0001\u0000\u0000\u0000\u0007J\u0001\u0000\u0000\u0000"+
		"\tN\u0001\u0000\u0000\u0000\u000bR\u0001\u0000\u0000\u0000\rW\u0001\u0000"+
		"\u0000\u0000\u000f[\u0001\u0000\u0000\u0000\u0011]\u0001\u0000\u0000\u0000"+
		"\u0013_\u0001\u0000\u0000\u0000\u0015f\u0001\u0000\u0000\u0000\u0017t"+
		"\u0001\u0000\u0000\u0000\u0019\u007f\u0001\u0000\u0000\u0000\u001b\u0089"+
		"\u0001\u0000\u0000\u0000\u001d\u008f\u0001\u0000\u0000\u0000\u001f\u0091"+
		"\u0001\u0000\u0000\u0000!\u0093\u0001\u0000\u0000\u0000#\u0095\u0001\u0000"+
		"\u0000\u0000%\u0097\u0001\u0000\u0000\u0000\'\u0099\u0001\u0000\u0000"+
		"\u0000)\u009b\u0001\u0000\u0000\u0000+\u009d\u0001\u0000\u0000\u0000-"+
		"\u009f\u0001\u0000\u0000\u0000/\u00a1\u0001\u0000\u0000\u00001\u00a3\u0001"+
		"\u0000\u0000\u00003\u00a5\u0001\u0000\u0000\u00005\u00a7\u0001\u0000\u0000"+
		"\u00007\u00a9\u0001\u0000\u0000\u00009:\u00033\u0019\u0000:;\u0003\'\u0013"+
		"\u0000;<\u00035\u001a\u0000<\u0002\u0001\u0000\u0000\u0000=>\u00033\u0019"+
		"\u0000>?\u0003+\u0015\u0000?@\u00031\u0018\u0000@A\u00037\u001b\u0000"+
		"A\u0004\u0001\u0000\u0000\u0000BC\u0003/\u0017\u0000CD\u0003)\u0014\u0000"+
		"DE\u0005_\u0000\u0000EF\u0003-\u0016\u0000FG\u00031\u0018\u0000GH\u0003"+
		"%\u0012\u0000HI\u0003\'\u0013\u0000I\u0006\u0001\u0000\u0000\u0000JK\u0005"+
		"2\u0000\u0000KL\u0003/\u0017\u0000LM\u0003)\u0014\u0000M\b\u0001\u0000"+
		"\u0000\u0000NO\u00053\u0000\u0000OP\u0003/\u0017\u0000PQ\u0003)\u0014"+
		"\u0000Q\n\u0001\u0000\u0000\u0000RS\u0003!\u0010\u0000ST\u0003#\u0011"+
		"\u0000TU\u0003/\u0017\u0000UV\u0003)\u0014\u0000V\f\u0001\u0000\u0000"+
		"\u0000WX\u00031\u0018\u0000XY\u0003)\u0014\u0000YZ\u0003)\u0014\u0000"+
		"Z\u000e\u0001\u0000\u0000\u0000[\\\u0005=\u0000\u0000\\\u0010\u0001\u0000"+
		"\u0000\u0000]^\u0005;\u0000\u0000^\u0012\u0001\u0000\u0000\u0000_c\u0007"+
		"\u0000\u0000\u0000`b\u0007\u0001\u0000\u0000a`\u0001\u0000\u0000\u0000"+
		"be\u0001\u0000\u0000\u0000ca\u0001\u0000\u0000\u0000cd\u0001\u0000\u0000"+
		"\u0000d\u0014\u0001\u0000\u0000\u0000ec\u0001\u0000\u0000\u0000fg\u0005"+
		"/\u0000\u0000gh\u0005*\u0000\u0000hl\u0001\u0000\u0000\u0000ik\t\u0000"+
		"\u0000\u0000ji\u0001\u0000\u0000\u0000kn\u0001\u0000\u0000\u0000lm\u0001"+
		"\u0000\u0000\u0000lj\u0001\u0000\u0000\u0000mo\u0001\u0000\u0000\u0000"+
		"nl\u0001\u0000\u0000\u0000op\u0005*\u0000\u0000pq\u0005/\u0000\u0000q"+
		"r\u0001\u0000\u0000\u0000rs\u0006\n\u0000\u0000s\u0016\u0001\u0000\u0000"+
		"\u0000tu\u0005-\u0000\u0000uv\u0005-\u0000\u0000vz\u0001\u0000\u0000\u0000"+
		"wy\b\u0002\u0000\u0000xw\u0001\u0000\u0000\u0000y|\u0001\u0000\u0000\u0000"+
		"zx\u0001\u0000\u0000\u0000z{\u0001\u0000\u0000\u0000{}\u0001\u0000\u0000"+
		"\u0000|z\u0001\u0000\u0000\u0000}~\u0006\u000b\u0000\u0000~\u0018\u0001"+
		"\u0000\u0000\u0000\u007f\u0083\u0005#\u0000\u0000\u0080\u0082\b\u0002"+
		"\u0000\u0000\u0081\u0080\u0001\u0000\u0000\u0000\u0082\u0085\u0001\u0000"+
		"\u0000\u0000\u0083\u0081\u0001\u0000\u0000\u0000\u0083\u0084\u0001\u0000"+
		"\u0000\u0000\u0084\u0086\u0001\u0000\u0000\u0000\u0085\u0083\u0001\u0000"+
		"\u0000\u0000\u0086\u0087\u0006\f\u0000\u0000\u0087\u001a\u0001\u0000\u0000"+
		"\u0000\u0088\u008a\u0007\u0003\u0000\u0000\u0089\u0088\u0001\u0000\u0000"+
		"\u0000\u008a\u008b\u0001\u0000\u0000\u0000\u008b\u0089\u0001\u0000\u0000"+
		"\u0000\u008b\u008c\u0001\u0000\u0000\u0000\u008c\u008d\u0001\u0000\u0000"+
		"\u0000\u008d\u008e\u0006\r\u0000\u0000\u008e\u001c\u0001\u0000\u0000\u0000"+
		"\u008f\u0090\t\u0000\u0000\u0000\u0090\u001e\u0001\u0000\u0000\u0000\u0091"+
		"\u0092\u0007\u0004\u0000\u0000\u0092 \u0001\u0000\u0000\u0000\u0093\u0094"+
		"\u0007\u0005\u0000\u0000\u0094\"\u0001\u0000\u0000\u0000\u0095\u0096\u0007"+
		"\u0006\u0000\u0000\u0096$\u0001\u0000\u0000\u0000\u0097\u0098\u0007\u0007"+
		"\u0000\u0000\u0098&\u0001\u0000\u0000\u0000\u0099\u009a\u0007\b\u0000"+
		"\u0000\u009a(\u0001\u0000\u0000\u0000\u009b\u009c\u0007\t\u0000\u0000"+
		"\u009c*\u0001\u0000\u0000\u0000\u009d\u009e\u0007\n\u0000\u0000\u009e"+
		",\u0001\u0000\u0000\u0000\u009f\u00a0\u0007\u000b\u0000\u0000\u00a0.\u0001"+
		"\u0000\u0000\u0000\u00a1\u00a2\u0007\f\u0000\u0000\u00a20\u0001\u0000"+
		"\u0000\u0000\u00a3\u00a4\u0007\r\u0000\u0000\u00a42\u0001\u0000\u0000"+
		"\u0000\u00a5\u00a6\u0007\u000e\u0000\u0000\u00a64\u0001\u0000\u0000\u0000"+
		"\u00a7\u00a8\u0007\u000f\u0000\u0000\u00a86\u0001\u0000\u0000\u0000\u00a9"+
		"\u00aa\u0007\u0010\u0000\u0000\u00aa8\u0001\u0000\u0000\u0000\u0006\u0000"+
		"clz\u0083\u008b\u0001\u0006\u0000\u0000";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}