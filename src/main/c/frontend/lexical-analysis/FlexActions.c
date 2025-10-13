#include "FlexActions.h"
#include <string.h>
#include <stdlib.h>

static bool _logIgnoredLexemes = true;
static LexicalAnalyzer * _lexicalAnalyzer = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownFlexActionsModule() {
    if (_logger != NULL) {
        logDebugging(_logger, "Destroying module: FlexActions...");
        destroyLogger(_logger);
        _logger = NULL;
    }
    _lexicalAnalyzer = NULL;
}

ModuleDestructor initializeFlexActionsModule(LexicalAnalyzer * lexicalAnalyzer) {
    _lexicalAnalyzer = lexicalAnalyzer;
    _logger = createLogger("FlexActions");
    _logIgnoredLexemes = getBooleanOrDefault("LOG_IGNORED_LEXEMES", _logIgnoredLexemes);
    return _shutdownFlexActionsModule;
}

/* PRIVATE FUNCTION */
static void _logTokenAction(const char * actionName, Token * token) {
    char * _lexeme = escape(token->lexeme);
    logDebugging(_logger, "FlexAction: %s, Token(label=%d, lexeme=\"%s\")",
        actionName,
        token->label,
        _lexeme);
    free(_lexeme);
    _lexeme = NULL;
}

// crear, loguear, pushear, destruir
static CompilationStatus _GenericTokenAction(TokenLabel label, const char * functionName) {
    Token * token = createToken(_lexicalAnalyzer, label);
    _logTokenAction(functionName, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus KeywordLexemeAction(TokenLabel label) {
    return _GenericTokenAction(label, __FUNCTION__);
}

CompilationStatus PunctuationLexemeAction(TokenLabel label) {
    return _GenericTokenAction(label, __FUNCTION__);
}

CompilationStatus RelationalOperatorLexemeAction(TokenLabel label) {
    return _GenericTokenAction(label, __FUNCTION__);
}

CompilationStatus ArithmeticOperatorLexemeAction(TokenLabel label) {
    return _GenericTokenAction(label, __FUNCTION__);
}

CompilationStatus IdentifierLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, T_IDENTIFIER);
    token->semanticValue->string = token->lexeme;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus StringLiteralLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, T_STRING_LITERAL);
    char * lexeme = token->lexeme;
    lexeme[strlen(lexeme) - 1] = '\0'; 
    token->semanticValue->string = lexeme + 1;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus NumberLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, T_NUMBER);
    token->semanticValue->number = atof(token->lexeme);
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus DurationLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, T_DURATION);
    token->semanticValue->string = token->lexeme;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus SymbolRefLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, T_SYMBOL_REF);
    token->semanticValue->string = token->lexeme;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}


CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context) {
    if (_logIgnoredLexemes) {
        Token * token = createToken(_lexicalAnalyzer, T_OPEN_COMMENT);
        _logTokenAction(__FUNCTION__, token);
        destroyToken(token);
    }
    enterLexicalAnalyzerContext(_lexicalAnalyzer, context);
    return IN_PROGRESS;
}

CompilationStatus LeaveMultilineCommentLexemeAction() {
    if (_logIgnoredLexemes) {
        Token * token = createToken(_lexicalAnalyzer, T_CLOSE_COMMENT);
        _logTokenAction(__FUNCTION__, token);
        destroyToken(token);
    }
    leaveLexicalAnalyzerContext(_lexicalAnalyzer);
    return IN_PROGRESS;
}

CompilationStatus IgnoredLexemeAction() {
    if (_logIgnoredLexemes) {
        Token * token = createToken(_lexicalAnalyzer, T_IGNORED);
        _logTokenAction(__FUNCTION__, token);
        destroyToken(token);
    }
    return IN_PROGRESS;
}

CompilationStatus UnknownLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
    _logTokenAction(__FUNCTION__, token);
    destroyToken(token);
    return FAILED;
}

CompilationStatus EOFLexemeAction() {
    CompilationStatus status = IN_PROGRESS;
    Token * token = createToken(_lexicalAnalyzer, 0);
    _logTokenAction(__FUNCTION__, token);

    if (!popInputBuffer(_lexicalAnalyzer)) {
        status = pushToken(_lexicalAnalyzer, token);
        FlexContext context = currentLexicalAnalyzerContext(_lexicalAnalyzer);
        if (0 < context) {
            logError(_logger, "Error: El archivo terminó inesperadamente dentro de un contexto (ej. un comentario sin cerrar). Contexto=%d", context);
            status = FAILED;
        }
    }
    destroyToken(token);
    return status;
}