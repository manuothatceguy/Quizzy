#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h" 
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeBisonActionsModule();

typedef enum {
    KEYWORD_POINTS,
    KEYWORD_SCORE,
    KEYWORD_TIMELEFT
} KeywordType;

typedef struct QuestionListItem {
    int isConditional;
    void * node;
} QuestionListItem;

QuestionListItem * CreateQuestionListItemFromQuestion(QuestionNode * q);
QuestionListItem * CreateQuestionListItemFromConditional(ConditionalNode * c);

Program * ProgramSemanticAction(QuizNode * quiz);
QuizNode * QuizBlockSemanticAction(ListNode * body);
QuestionNode * QuestionSemanticAction(ListNode * body);

BodyNode * CreateBodyNodeFromAttribute(AttributeNode * attr);
BodyNode * CreateBodyNodeFromScoring(ScoringNode * scoring);
BodyNode * CreateBodyNodeFromQuestions(QuestionsBlockNode * questions);
BodyNode * CreateBodyNodeFromMedia(MediaNode * media);
BodyNode * CreateBodyNodeFromConditional(ConditionalNode * cond);

ScoringNode * ScoringBlockSemanticAction(ListNode * rules);
QuestionsBlockNode * QuestionsBlockSemanticAction(ListNode * questions, bool isShuffled);
MediaNode * MediaBlockSemanticAction(ListNode * items);
MediaItemNode * MediaItemSemanticAction(MediaType type, char * path, char * alias);
ConditionalNode * ConditionalSemanticAction(ExpressionNode * condition, char * ifTarget, char * elseTarget);

AttributeNode * TitleAttributeSemanticAction(char * title);
AttributeNode * ShuffleAttributeSemanticAction(ValueNode * value);
AttributeNode * TimeAttributeSemanticAction(char * duration);
AttributeNode * IdAttributeSemanticAction(char * id);
AttributeNode * TypeAttributeSemanticAction(ValueNode * type);
AttributeNode * TextAttributeSemanticAction(char * text);
AttributeNode * OptionsAttributeSemanticAction(ListNode * options);
AttributeNode * AnswerAttributeSemanticAction(ListNode * answer);
AttributeNode * PointsAttributeSemanticAction(double points);
AttributeNode * PartialCreditAttributeSemanticAction(ValueNode * value);
AttributeNode * CaseSensitiveAttributeSemanticAction(ValueNode * value);

ExpressionNode * BinaryExpressionSemanticAction(OperatorType op, ExpressionNode * left, ExpressionNode * right);
ExpressionNode * NumberTermSemanticAction(double number);
ExpressionNode * IdentifierTermSemanticAction(char * identifier);
ExpressionNode * KeywordTermSemanticAction(KeywordType type);
ScoringRuleNode * ScoringRuleSemanticAction(ScoringRuleType type, ExpressionNode * expression);

ValueNode * ValueSemanticActionFromString(const char * string);
ValueNode * ValueSemanticActionFromSymbol(char * symbol);
ValueNode * ValueSemanticActionFromNumber(double number);
ValueNode * ValueSemanticActionFromBoolean(bool boolean);

#endif