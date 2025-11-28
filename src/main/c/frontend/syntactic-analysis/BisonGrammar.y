%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

/**
 * The error reporting function for Bison parser.
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 */
void yyerror(const YYLTYPE * location, const char * message) {}

%}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	char * string;
	double number;
	int boolean;

	Program * programNode;
	QuizNode * quizNode;
	ScoringNode * scoringNode;
	ScoringRuleNode * scoringRuleNode;
	QuestionNode * questionNode;
	ConditionalNode * conditionalNode;
	MediaNode * mediaNode;
	ExpressionNode * expressionNode;
    AttributeNode * attributeNode; 
    BodyNode * bodyNode;           
	
	ValueNode * valueNode;
	ListNode * list;
}

/** Terminals. */
%token <string> T_STRING_LITERAL T_IDENTIFIER T_DURATION T_SYMBOL_REF
%token <number> T_NUMBER
%token <boolean> T_TRUE T_FALSE

/* Keywords */
%token T_QUIZ T_TITLE T_SHUFFLE T_TIME T_SCORING T_CORRECT T_WRONG T_TIMEOUT
%token T_QUESTIONS T_QUESTION T_ID T_TYPE T_TEXT T_OPTIONS T_ANSWER T_POINTS
%token T_PARTIAL_CREDIT T_CASE_SENSITIVE T_MEDIA T_IMAGE T_AUDIO T_VIDEO T_AS
%token T_IF T_ELSE T_NEXT T_SCORE_VAR T_TIMELEFT_VAR
%token T_QTYPE_SHORT T_QTYPE_MULTIPLE T_QTYPE_BOOLEAN

/* Punctuation */
%token T_LBRACE T_RBRACE T_LBRACKET T_RBRACKET T_LPAREN T_RPAREN
%token T_ASSIGN T_COMMA

/* Operators */
%token T_ADD T_SUB T_MUL T_DIV
%token T_EQ T_NEQ T_GT T_LT T_GTE T_LTE

%token T_IGNORED
%token T_OPEN_COMMENT
%token T_CLOSE_COMMENT
%token UNKNOWN

/** Non-terminals. */
%type <programNode> program
%type <quizNode> quiz_block
%type <questionNode> question
%type <bodyNode> quiz_item q_attr scoring_block questions_block media_block
%type <conditionalNode> conditional

%type <list> quiz_body question_list question_body media_items scoring_rules
%type <list> value_list list_literal set_literal

%type <attributeNode> quiz_attr question_attr

%type <expressionNode> expression term
%type <scoringRuleNode> scoring_rule
%type <valueNode> value question_type boolean

%destructor { free($$); } <string>
%destructor { destroyValueNode($$); } <valueNode>
%destructor { destroyExpressionNode($$); } <expressionNode>
%destructor { destroyExpressionNode($$); } <scoringRuleNode>
%destructor { destroyBodyNode($$); } <bodyNode>
%destructor { destroyConditionalNode($$); } <conditionalNode>
%destructor { destroyQuestionNode($$); } <questionNode>
%destructor { destroyQuizNode($$); } <quizNode>

%destructor { destroyListNode($$, (void (*)(void*))destroyValueNode); } value_list list_literal set_literal

%destructor { destroyListNode($$, destroyBodyNode); } quiz_body question_body
%destructor { destroyListNode($$, destroyScoringRuleNode); } scoring_rules
%destructor { destroyListNode($$, destroyMediaItemNode); } media_items
%destructor {
	ListNode * cur = $$;
	while (cur != NULL) {
		ListNode * next = cur->next;
		if (cur->data != NULL) {
			QuestionListItem * item = (QuestionListItem *) cur->data;
			if (item->isConditional) {
				destroyConditionalNode((ConditionalNode *) item->node);
			} else {
				destroyQuestionNode((QuestionNode *) item->node);
			}
			free(item);
		}
		free(cur);
		cur = next;
	}
} question_list

/**
 * Precedence and associativity.
 * @see https://www.gnu.org/software/bison/manual/html_node/Precedence.html
 */
%nonassoc T_EQ T_NEQ T_GT T_LT T_GTE T_LTE // Operadores relacionales
%left T_ADD T_SUB
%left T_MUL T_DIV

%%

program
	: quiz_block
	{
		$$ = ProgramSemanticAction($1);
	}
	| %empty
	{
		$$ = ProgramSemanticAction(NULL);
	}
;

quiz_block
	: T_QUIZ T_LBRACE quiz_body T_RBRACE
	{
		$$ = QuizBlockSemanticAction($3);
	}
;

quiz_body
	: %empty										{ $$ = NULL; }
	| quiz_body quiz_item							{ $$ = AppendToList($1, $2); }
;

quiz_item
	: quiz_attr										{ $$ = CreateBodyNodeFromAttribute($1); }
	| scoring_block									{ $$ = $1; } // es un body node
	| questions_block								{ $$ = $1; } 
	| conditional									{ $$ = CreateBodyNodeFromConditional($1); }
;

quiz_attr
	: T_TITLE T_ASSIGN T_STRING_LITERAL				{ $$ = TitleAttributeSemanticAction($3); free($3); }
	| T_SHUFFLE T_ASSIGN boolean					{ $$ = ShuffleAttributeSemanticAction($3); }
	| T_TIME T_ASSIGN T_DURATION					{ $$ = TimeAttributeSemanticAction($3); free($3); }
;

boolean
	: T_TRUE										{ $$ = ValueSemanticActionFromBoolean(true); }
	| T_FALSE										{ $$ = ValueSemanticActionFromBoolean(false); }
;

scoring_block
	: T_SCORING T_LBRACE scoring_rules T_RBRACE
    {
        ScoringNode* scoring = ScoringBlockSemanticAction($3);
        $$ = CreateBodyNodeFromScoring(scoring);
    }
;

scoring_rules
	: %empty										{ $$ = NULL; }
	| scoring_rules scoring_rule					{ $$ = AppendToList($1, $2); }
;

scoring_rule
	: T_CORRECT T_ASSIGN expression					{ $$ = ScoringRuleSemanticAction(SCORING_CORRECT, $3); }
	| T_WRONG T_ASSIGN expression					{ $$ = ScoringRuleSemanticAction(SCORING_WRONG, $3); }
	| T_TIMEOUT T_ASSIGN expression					{ $$ = ScoringRuleSemanticAction(SCORING_TIMEOUT, $3); }
;

questions_block
	: T_QUESTIONS T_ASSIGN T_LBRACKET question_list T_RBRACKET
	{
		QuestionsBlockNode* qblock = QuestionsBlockSemanticAction($4, 0); // 0 = false 
		$$ = CreateBodyNodeFromQuestions(qblock);
	}
	| T_QUESTIONS T_ASSIGN T_LBRACE question_list T_RBRACE
	{
		QuestionsBlockNode* qblock = QuestionsBlockSemanticAction($4, 1); // 1 = true 
		$$ = CreateBodyNodeFromQuestions(qblock);
	}
;

question_list
	: %empty										{ $$ = NULL; }
	| question_list question							{ $$ = AppendToList($1, CreateQuestionListItemFromQuestion($2)); }
	| question_list conditional							{ $$ = AppendToList($1, CreateQuestionListItemFromConditional($2)); }
;

question
	: T_QUESTION T_LBRACE question_body T_RBRACE
	{
		$$ = QuestionSemanticAction($3);
	}
;

question_body
	: %empty										{ $$ = NULL; }
	| question_body q_attr							{ $$ = AppendToList($1, $2); }
;

q_attr
	: question_attr									{ $$ = CreateBodyNodeFromAttribute($1); }
	| media_block									{ $$ = $1; } // es un body node
;

question_attr
	: T_ID T_ASSIGN T_IDENTIFIER					{ $$ = IdAttributeSemanticAction($3); free($3); }
	| T_TYPE T_ASSIGN question_type					{ $$ = TypeAttributeSemanticAction($3); }
	| T_TEXT T_ASSIGN T_STRING_LITERAL				{ $$ = TextAttributeSemanticAction($3); free($3); }
	| T_OPTIONS T_ASSIGN set_literal				{ $$ = OptionsAttributeSemanticAction($3); }
	| T_OPTIONS T_ASSIGN list_literal				{ $$ = OptionsAttributeSemanticAction($3); }
	| T_ANSWER T_ASSIGN value						{ $$ = AnswerAttributeSemanticAction(CreateList($3)); }
	| T_ANSWER T_ASSIGN list_literal				{ $$ = AnswerAttributeSemanticAction($3); }
	| T_POINTS T_ASSIGN T_NUMBER					{ $$ = PointsAttributeSemanticAction($3); }
	| T_PARTIAL_CREDIT T_ASSIGN boolean				{ $$ = PartialCreditAttributeSemanticAction($3); }
	| T_CASE_SENSITIVE T_ASSIGN boolean				{ $$ = CaseSensitiveAttributeSemanticAction($3); }
    | T_TIME T_ASSIGN T_DURATION                    { $$ = TimeAttributeSemanticAction($3); free($3); }
;

question_type
	: T_QTYPE_SHORT									{ $$ = ValueSemanticActionFromString("shortAnswer"); }
	| T_QTYPE_MULTIPLE								{ $$ = ValueSemanticActionFromString("multipleChoice"); }
	| T_QTYPE_BOOLEAN								{ $$ = ValueSemanticActionFromString("trueFalse"); }
	| T_IDENTIFIER								{ $$ = ValueSemanticActionFromString($1); free($1); }
;

media_block
	: T_MEDIA T_LBRACE media_items T_RBRACE
    {
        MediaNode* media = MediaBlockSemanticAction($3);
        $$ = CreateBodyNodeFromMedia(media);
    }
;

media_items
	: %empty										{ $$ = NULL; }
	| media_items T_IMAGE T_LPAREN T_STRING_LITERAL T_RPAREN { $$ = AppendToList($1, MediaItemSemanticAction(MEDIA_IMAGE, $4, NULL)); free($4); }
	| media_items T_IMAGE T_LPAREN T_STRING_LITERAL T_AS T_IDENTIFIER T_RPAREN { $$ = AppendToList($1, MediaItemSemanticAction(MEDIA_IMAGE, $4, $6)); free($4); free($6); }
	| media_items T_AUDIO T_LPAREN T_STRING_LITERAL T_RPAREN { $$ = AppendToList($1, MediaItemSemanticAction(MEDIA_AUDIO, $4, NULL)); free($4); }
	| media_items T_AUDIO T_LPAREN T_STRING_LITERAL T_AS T_IDENTIFIER T_RPAREN { $$ = AppendToList($1, MediaItemSemanticAction(MEDIA_AUDIO, $4, $6)); free($4); free($6); }
	| media_items T_VIDEO T_LPAREN T_STRING_LITERAL T_RPAREN { $$ = AppendToList($1, MediaItemSemanticAction(MEDIA_VIDEO, $4, NULL)); free($4); }
	| media_items T_VIDEO T_LPAREN T_STRING_LITERAL T_AS T_IDENTIFIER T_RPAREN { $$ = AppendToList($1, MediaItemSemanticAction(MEDIA_VIDEO, $4, $6)); free($4); free($6); }
;

list_literal
	: T_LBRACKET value_list T_RBRACKET				{ $$ = $2; }
;

set_literal
	: T_LBRACE value_list T_RBRACE					{ $$ = $2; }
;

value_list
	: %empty										{ $$ = NULL; }
	| value											{ $$ = CreateList($1); }
	| value_list T_COMMA value						{ $$ = AppendToList($1, $3); }
;

value
	: T_STRING_LITERAL							{ $$ = ValueSemanticActionFromString($1); free($1); }
	| T_NUMBER										{ $$ = ValueSemanticActionFromNumber($1); }
	| boolean										{ $$ = $1; } 
	| T_SYMBOL_REF							{ $$ = ValueSemanticActionFromSymbol($1); free($1); }
;

conditional
	: T_IF T_LPAREN expression T_RPAREN T_NEXT T_ASSIGN T_QUESTION T_LPAREN T_STRING_LITERAL T_RPAREN
	{
		$$ = ConditionalSemanticAction($3, $9, NULL); free($9);
	}
	| T_IF T_LPAREN expression T_RPAREN T_NEXT T_ASSIGN T_QUESTION T_LPAREN T_STRING_LITERAL T_RPAREN T_ELSE T_NEXT T_ASSIGN T_QUESTION T_LPAREN T_STRING_LITERAL T_RPAREN
	{
		$$ = ConditionalSemanticAction($3, $9, $16); free($9); free($16);
	}
;

expression
	: term											{ $$ = $1; }
	| expression T_ADD term							{ $$ = BinaryExpressionSemanticAction(OP_ADD, $1, $3); }
	| expression T_SUB term							{ $$ = BinaryExpressionSemanticAction(OP_SUB, $1, $3); }
	| expression T_MUL term							{ $$ = BinaryExpressionSemanticAction(OP_MUL, $1, $3); }
	| expression T_DIV term							{ $$ = BinaryExpressionSemanticAction(OP_DIV, $1, $3); }
	| expression T_EQ term							{ $$ = BinaryExpressionSemanticAction(OP_EQ, $1, $3); }
	| expression T_NEQ term							{ $$ = BinaryExpressionSemanticAction(OP_NEQ, $1, $3); }
	| expression T_GT term							{ $$ = BinaryExpressionSemanticAction(OP_GT, $1, $3); }
	| expression T_LT term							{ $$ = BinaryExpressionSemanticAction(OP_LT, $1, $3); }
	| expression T_GTE term							{ $$ = BinaryExpressionSemanticAction(OP_GTE, $1, $3); }
	| expression T_LTE term							{ $$ = BinaryExpressionSemanticAction(OP_LTE, $1, $3); }
;

term
	: T_NUMBER										{ $$ = NumberTermSemanticAction($1); }
	| T_IDENTIFIER							{ $$ = IdentifierTermSemanticAction($1); free($1); }
	| T_POINTS										{ $$ = KeywordTermSemanticAction(KEYWORD_POINTS); }
	| T_SCORE_VAR									{ $$ = KeywordTermSemanticAction(KEYWORD_SCORE); }
	| T_TIMELEFT_VAR								{ $$ = KeywordTermSemanticAction(KEYWORD_TIMELEFT); }
	| T_LPAREN expression T_RPAREN					{ $$ = $2; }
	| T_ADD term									{ $$ = BinaryExpressionSemanticAction(OP_ADD, NumberTermSemanticAction(0), $2); }
	| T_SUB term									{ $$ = BinaryExpressionSemanticAction(OP_SUB, NumberTermSemanticAction(0), $2); }
;

%%