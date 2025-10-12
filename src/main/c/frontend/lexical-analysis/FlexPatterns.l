%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

extern CompilationStatus ArithmeticOperatorLexemeAction(TokenLabel label);
extern CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
extern CompilationStatus EOFLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus LeaveMultilineCommentLexemeAction();
extern CompilationStatus UnknownLexemeAction();

extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus PunctuationLexemeAction(TokenLabel label);
extern CompilationStatus RelationalOperatorLexemeAction(TokenLabel label);
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus StringLiteralLexemeAction();
extern CompilationStatus NumberLexemeAction();
extern CompilationStatus DurationLexemeAction();
extern CompilationStatus SymbolRefLexemeAction();

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x MULTILINE_COMMENT

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace          [[:space:]]+
digit               [0-9]
letter              [a-zA-Z_]
identifier          {letter}({letter}|{digit})*
number              {digit}+(\.{digit}+)?
duration            {number}[[:space:]]*(s|min)

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}

"/*"                                { return EnterMultilineCommentLexemeAction(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/"             { return LeaveMultilineCommentLexemeAction(); }
<MULTILINE_COMMENT>[^*]+            { return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>.                { return IgnoredLexemeAction(); }
{whitespace}                        { return IgnoredLexemeAction(); }

"quiz"              { return KeywordLexemeAction(T_QUIZ); }
"title"             { return KeywordLexemeAction(T_TITLE); }
"shuffle"           { return KeywordLexemeAction(T_SHUFFLE); }
"time"              { return KeywordLexemeAction(T_TIME); }
"scoring"           { return KeywordLexemeAction(T_SCORING); }
"correct"           { return KeywordLexemeAction(T_CORRECT); }
"wrong"             { return KeywordLexemeAction(T_WRONG); }
"timeout"           { return KeywordLexemeAction(T_TIMEOUT); }
"questions"         { return KeywordLexemeAction(T_QUESTIONS); }
"question"          { return KeywordLexemeAction(T_QUESTION); }
"id"                { return KeywordLexemeAction(T_ID); }
"type"              { return KeywordLexemeAction(T_TYPE); }
"text"              { return KeywordLexemeAction(T_TEXT); }
"options"           { return KeywordLexemeAction(T_OPTIONS); }
"answer"            { return KeywordLexemeAction(T_ANSWER); }
"points"            { return KeywordLexemeAction(T_POINTS); }
"partialCredit"     { return KeywordLexemeAction(T_PARTIAL_CREDIT); }
"caseSensitive"     { return KeywordLexemeAction(T_CASE_SENSITIVE); }
"media"             { return KeywordLexemeAction(T_MEDIA); }
"image"             { return KeywordLexemeAction(T_IMAGE); }
"audio"             { return KeywordLexemeAction(T_AUDIO); }
"video"             { return KeywordLexemeAction(T_VIDEO); }
"as"                { return KeywordLexemeAction(T_AS); }
"if"                { return KeywordLexemeAction(T_IF); }
"else"              { return KeywordLexemeAction(T_ELSE); }
"next"              { return KeywordLexemeAction(T_NEXT); }
"true"              { return KeywordLexemeAction(T_TRUE); }
"false"             { return KeywordLexemeAction(T_FALSE); }
"score"             { return KeywordLexemeAction(T_SCORE_VAR); }
"timeLeft"          { return KeywordLexemeAction(T_TIMELEFT_VAR); }
"shortAnswer"       { return KeywordLexemeAction(T_QTYPE_SHORT); }
"multipleChoice"    { return KeywordLexemeAction(T_QTYPE_MULTIPLE); }
"trueFalse"         { return KeywordLexemeAction(T_QTYPE_BOOLEAN); }

{duration}                          { return DurationLexemeAction(); }
{number}                            { return NumberLexemeAction(); }
\"[^"\n]*\"                         { return StringLiteralLexemeAction(); }
:{identifier}                       { return SymbolRefLexemeAction(); }
{identifier}                        { return IdentifierLexemeAction(); }

"=="                { return RelationalOperatorLexemeAction(T_EQ); }
"!="                { return RelationalOperatorLexemeAction(T_NEQ); }
">="                { return RelationalOperatorLexemeAction(T_GTE); }
"<="                { return RelationalOperatorLexemeAction(T_LTE); }
">"                 { return PunctuationLexemeAction('>'); }
"<"                 { return PunctuationLexemeAction('<'); }

"-"                 { return ArithmeticOperatorLexemeAction(T_SUB); }
"*"                 { return ArithmeticOperatorLexemeAction(T_MUL); }
"/"                 { return ArithmeticOperatorLexemeAction(T_DIV); }
"+"                 { return ArithmeticOperatorLexemeAction(T_ADD); }

"{"                 { return PunctuationLexemeAction(T_LBRACE); }
"}"                 { return PunctuationLexemeAction(T_RBRACE); }
"["                 { return PunctuationLexemeAction(T_LBRACKET); }
"]"                 { return PunctuationLexemeAction(T_RBRACKET); }
"("                 { return PunctuationLexemeAction(T_LPAREN); }
")"                 { return PunctuationLexemeAction(T_RPAREN); }
"="                 { return PunctuationLexemeAction(T_ASSIGN); }
","                 { return PunctuationLexemeAction(T_COMMA); }
.                   { return UnknownLexemeAction(); }
<<EOF>>             { return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"