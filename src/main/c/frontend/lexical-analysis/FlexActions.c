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
    token->semanticValue->string = strdup(token->lexeme);
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus StringLiteralLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, T_STRING_LITERAL);
    // Remove surrounding quotes and duplicate
    size_t len = strlen(token->lexeme);
    if (len >= 2) {
        size_t innerLen = len - 2;
        char *copy = (char *)calloc(innerLen + 1, sizeof(char));
        if (copy != NULL) {
            memcpy(copy, token->lexeme + 1, innerLen);
            copy[innerLen] = '\0';
        }
        token->semanticValue->string = copy;
    } else {
        token->semanticValue->string = strdup("");
    }
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
    token->semanticValue->string = strdup(token->lexeme);
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus SymbolRefLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, T_SYMBOL_REF);
    token->semanticValue->string = strdup(token->lexeme);
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
            logError(_logger, "El archivo terminó inesperadamente dentro de un contexto (eg. un comentario sin cerrar). Contexto=%d", context);
            status = FAILED;
        }
    }
    destroyToken(token);
    return status;
}

static char * stringBuffer = NULL;
static int stringLength = 0;
static long stringAllocated = 0;

#define STRING_BLOCK_SIZE 1024

CompilationStatus EnterStringLexemeAction(FlexContext context) {
    if (_logIgnoredLexemes) {
        Token * token = createToken(_lexicalAnalyzer, T_OPEN_QUOTE);
        _logTokenAction(__FUNCTION__, token);
        destroyToken(token);
    }

    stringLength = 0;
    if (stringBuffer == NULL) {
        stringAllocated = STRING_BLOCK_SIZE;
        stringBuffer = (char *)malloc(stringAllocated);
    }
    stringBuffer[0] = '\0';
    
    enterLexicalAnalyzerContext(_lexicalAnalyzer, context);
    return IN_PROGRESS;
}

CompilationStatus AppendStringLexemeAction(const char * text, int len) {
    while (stringLength + len + 1 > stringAllocated) {
        stringAllocated *= 2;
        stringBuffer = (char *)realloc(stringBuffer, stringAllocated);
    }
    
    memcpy(stringBuffer + stringLength, text, len);
    stringLength += len;
    stringBuffer[stringLength] = '\0';

    return IN_PROGRESS;
}

CompilationStatus LeaveStringLexemeAction() {
    if (_logIgnoredLexemes) {
        Token * token = createToken(_lexicalAnalyzer, T_CLOSE_QUOTE);
        _logTokenAction(__FUNCTION__, token);
        destroyToken(token);
    }
    
    Token * token = createToken(_lexicalAnalyzer, T_STRING_LITERAL);
    free(token->lexeme);
    token->lexeme = strdup(stringBuffer);
    token->length = stringLength;
    token->semanticValue->string = strdup(stringBuffer);
    
    _logTokenAction(__FUNCTION__, token);
    leaveLexicalAnalyzerContext(_lexicalAnalyzer);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus UnterminatedStringLexemeAction() {
    Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
    _logTokenAction(__FUNCTION__, token);
    logError(_logger, "String sin cerrar");
    destroyToken(token);
    leaveLexicalAnalyzerContext(_lexicalAnalyzer);
    return FAILED;
}