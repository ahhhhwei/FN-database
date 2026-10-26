// Generated from /root/Desktop/FN-database/grammar/FNCommand.g4 by ANTLR 4.13.1
import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link FNCommandParser}.
 */
public interface FNCommandListener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link FNCommandParser#statement}.
	 * @param ctx the parse tree
	 */
	void enterStatement(FNCommandParser.StatementContext ctx);
	/**
	 * Exit a parse tree produced by {@link FNCommandParser#statement}.
	 * @param ctx the parse tree
	 */
	void exitStatement(FNCommandParser.StatementContext ctx);
	/**
	 * Enter a parse tree produced by {@link FNCommandParser#setModeStatement}.
	 * @param ctx the parse tree
	 */
	void enterSetModeStatement(FNCommandParser.SetModeStatementContext ctx);
	/**
	 * Exit a parse tree produced by {@link FNCommandParser#setModeStatement}.
	 * @param ctx the parse tree
	 */
	void exitSetModeStatement(FNCommandParser.SetModeStatementContext ctx);
	/**
	 * Enter a parse tree produced by {@link FNCommandParser#showModeStatement}.
	 * @param ctx the parse tree
	 */
	void enterShowModeStatement(FNCommandParser.ShowModeStatementContext ctx);
	/**
	 * Exit a parse tree produced by {@link FNCommandParser#showModeStatement}.
	 * @param ctx the parse tree
	 */
	void exitShowModeStatement(FNCommandParser.ShowModeStatementContext ctx);
	/**
	 * Enter a parse tree produced by {@link FNCommandParser#normalizationMode}.
	 * @param ctx the parse tree
	 */
	void enterNormalizationMode(FNCommandParser.NormalizationModeContext ctx);
	/**
	 * Exit a parse tree produced by {@link FNCommandParser#normalizationMode}.
	 * @param ctx the parse tree
	 */
	void exitNormalizationMode(FNCommandParser.NormalizationModeContext ctx);
}