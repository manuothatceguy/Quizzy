#include "BisonActions.h"
#include <string.h>
#include <stdio.h>

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

void _shutdownBisonActionsModule() {
    if (_logger != NULL) {
        logDebugging(_logger, "Destroying module: BisonActions...");
        destroyLogger(_logger);
        _logger = NULL;
    }
    _compilerState = NULL;
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
    _compilerState = compilerState;
    _logger = createLogger("BisonActions");
    return _shutdownBisonActionsModule;
}


static void _logSyntacticAnalyzerAction(const char * functionName) {
    logDebugging(_logger, "BisonAction: %s", functionName);
}


Program * ProgramSemanticAction(QuizNode * quiz) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    Program * program = calloc(1, sizeof(Program));
    program->quiz = quiz;
    _compilerState->abstractSyntaxtTree = program;
    return program;
}

QuizNode * QuizBlockSemanticAction(ListNode * body) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    QuizNode * quiz = (QuizNode *) calloc(1, sizeof(QuizNode));
    ListNode * current = body;

    while (current != NULL) {
        BodyNode * bodyNode = (BodyNode *) current->data;
        switch(bodyNode->type) {
            case BODY_NODE_ATTRIBUTE: {
                AttributeNode * attr = bodyNode->data.attributeNode;
                switch (attr->type) {
                    case ATTR_TITLE:   quiz->title = attr->stringValue; break;
                    case ATTR_SHUFFLE: quiz->shuffle = attr->valueNode; break;
                    case ATTR_TIME:    quiz->time = attr->stringValue; break;
                    default:           logWarning(_logger, "Attribute type not handled in QuizBlockSemanticAction."); break;
                }
                free(attr);
                break;
            }
            case BODY_NODE_SCORING:
                quiz->scoring = bodyNode->data.scoringNode;
                break;
            case BODY_NODE_QUESTIONS:
                quiz->questions = bodyNode->data.questionsBlockNode;
                break;
            case BODY_NODE_CONDITIONAL:
                quiz->conditionals = AppendToList(quiz->conditionals, bodyNode->data.conditionalNode);
                break;
            default:
                 logWarning(_logger, "BodyNode type not handled in QuizBlockSemanticAction.");
                 break;
        }
        free(bodyNode);
        current = current->next;
    }
    destroyListNode(body, NULL);
    return quiz;
}

QuestionNode * QuestionSemanticAction(ListNode * body) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    QuestionNode * question = (QuestionNode *) calloc(1, sizeof(QuestionNode));
    ListNode * current = body;

    while (current != NULL) {
        BodyNode * bodyNode = (BodyNode *) current->data;
        if (bodyNode->type == BODY_NODE_ATTRIBUTE) {
            AttributeNode * attr = bodyNode->data.attributeNode;
            switch (attr->type) {
                case ATTR_ID:             question->id = attr->stringValue; break;
                case ATTR_TYPE:           question->type = attr->valueNode->stringValue; free(attr->valueNode); break;
                case ATTR_TEXT:           question->text = attr->stringValue; break;
                case ATTR_OPTIONS:        question->options = attr->listValue; break;
                case ATTR_ANSWER:         question->answer = attr->listValue; break;
                case ATTR_POINTS:         question->points = attr->valueNode; break;
                case ATTR_PARTIAL_CREDIT: question->partialCredit = attr->valueNode; break;
                case ATTR_CASE_SENSITIVE: question->caseSensitive = attr->valueNode; break;
                case ATTR_TIME:           question->time = attr->stringValue; break;
                default:                  logWarning(_logger, "Attribute type not handled in QuestionSemanticAction."); break;
            }
            free(attr);
        } else if (bodyNode->type == BODY_NODE_MEDIA) {
            question->media = bodyNode->data.mediaNode;
        }
        free(bodyNode);
        current = current->next;
    }
    destroyListNode(body, NULL);
    return question;
}


BodyNode * CreateBodyNodeFromAttribute(AttributeNode * attr) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    BodyNode * node = calloc(1, sizeof(BodyNode));
    node->type = BODY_NODE_ATTRIBUTE;
    node->data.attributeNode = attr;
    return node;
}

BodyNode * CreateBodyNodeFromScoring(ScoringNode * scoring) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    BodyNode * node = calloc(1, sizeof(BodyNode));
    node->type = BODY_NODE_SCORING;
    node->data.scoringNode = scoring;
    return node;
}

BodyNode * CreateBodyNodeFromQuestions(QuestionsBlockNode * questions) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    BodyNode * node = calloc(1, sizeof(BodyNode));
    node->type = BODY_NODE_QUESTIONS;
    node->data.questionsBlockNode = questions;
    return node;
}

BodyNode * CreateBodyNodeFromMedia(MediaNode * media) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    BodyNode * node = calloc(1, sizeof(BodyNode));
    node->type = BODY_NODE_MEDIA;
    node->data.mediaNode = media;
    return node;
}

BodyNode * CreateBodyNodeFromConditional(ConditionalNode * cond) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    BodyNode * node = calloc(1, sizeof(BodyNode));
    node->type = BODY_NODE_CONDITIONAL;
    node->data.conditionalNode = cond;
    return node;
}


ScoringNode * ScoringBlockSemanticAction(ListNode * rules) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ScoringNode * node = calloc(1, sizeof(ScoringNode));
    node->rules = rules;
    return node;
}

QuestionsBlockNode * QuestionsBlockSemanticAction(ListNode * questions, int isShuffled) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    QuestionsBlockNode * node = calloc(1, sizeof(QuestionsBlockNode));
    node->isShuffled = isShuffled;
    node->questions = NULL;
    node->conditionals = NULL;
    ListNode * current = questions;
    while (current != NULL) {
        QuestionListItem * item = (QuestionListItem *) current->data;
        if (item != NULL && item->isConditional) {
            node->conditionals = AppendToList(node->conditionals, (ConditionalNode *) item->node);
        } else if (item != NULL) {
            node->questions = AppendToList(node->questions, (QuestionNode *) item->node);
        }
        free(item);
        current = current->next;
    }
    destroyListNode(questions, NULL);
    node->isShuffled = isShuffled;
    return node;
}

QuestionListItem * CreateQuestionListItemFromQuestion(QuestionNode * q) {
    QuestionListItem * item = (QuestionListItem *) calloc(1, sizeof(QuestionListItem));
    item->isConditional = 0;
    item->node = q;
    return item;
}

QuestionListItem * CreateQuestionListItemFromConditional(ConditionalNode * c) {
    QuestionListItem * item = (QuestionListItem *) calloc(1, sizeof(QuestionListItem));
    item->isConditional = 1;
    item->node = c;
    return item;
}

MediaNode * MediaBlockSemanticAction(ListNode * items) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    MediaNode * node = calloc(1, sizeof(MediaNode));
    node->items = items;
    return node;
}

MediaItemNode * MediaItemSemanticAction(MediaType type, char * path, char * alias) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    MediaItemNode * item = calloc(1, sizeof(MediaItemNode));
    item->type = type;
    item->path = path ? strdup(path) : NULL;
    if (alias) {
        item->alias = strdup(alias);
    }
    return item;
}

ConditionalNode * ConditionalSemanticAction(ExpressionNode * condition, char * ifTarget, char * elseTarget) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ConditionalNode * node = calloc(1, sizeof(ConditionalNode));
    node->condition = condition;
    node->ifTargetId = ifTarget ? strdup(ifTarget) : NULL;
    if (elseTarget) {
        node->elseTargetId = strdup(elseTarget);
    }
    return node;
}


AttributeNode * TitleAttributeSemanticAction(char * title) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_TITLE;
    attr->stringValue = strdup(title);
    return attr;
}

AttributeNode * ShuffleAttributeSemanticAction(ValueNode * value) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_SHUFFLE;
    attr->valueNode = value;
    return attr;
}

AttributeNode * TimeAttributeSemanticAction(char * duration) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_TIME;
    attr->stringValue = strdup(duration);
    return attr;
}

AttributeNode * IdAttributeSemanticAction(char * id) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_ID;
    attr->stringValue = strdup(id);
    return attr;
}

AttributeNode * TypeAttributeSemanticAction(ValueNode * type) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_TYPE;
    attr->valueNode = type;
    return attr;
}

AttributeNode * TextAttributeSemanticAction(char * text) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_TEXT;
    attr->stringValue = strdup(text);
    return attr;
}

AttributeNode * OptionsAttributeSemanticAction(ListNode * options) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_OPTIONS;
    attr->listValue = options;
    return attr;
}

AttributeNode * AnswerAttributeSemanticAction(ListNode * answer) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_ANSWER;
    attr->listValue = answer;
    return attr;
}

AttributeNode * PointsAttributeSemanticAction(double points) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_POINTS;
    attr->valueNode = ValueSemanticActionFromNumber(points);
    return attr;
}

AttributeNode * PartialCreditAttributeSemanticAction(ValueNode * value) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_PARTIAL_CREDIT;
    attr->valueNode = value;
    return attr;
}

AttributeNode * CaseSensitiveAttributeSemanticAction(ValueNode * value) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    AttributeNode * attr = calloc(1, sizeof(AttributeNode));
    attr->type = ATTR_CASE_SENSITIVE;
    attr->valueNode = value;
    return attr;
}

ExpressionNode * BinaryExpressionSemanticAction(OperatorType op, ExpressionNode * left, ExpressionNode * right) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ExpressionNode * node = calloc(1, sizeof(ExpressionNode));
    node->nodeType = EXPRESSION_NODE_BINARY;
    node->op = op;
    node->left = left;
    node->right = right;
    return node;
}

ExpressionNode * NumberTermSemanticAction(double number) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ExpressionNode * node = calloc(1, sizeof(ExpressionNode));
    node->nodeType = EXPRESSION_NODE_TERM;
    node->termType = TERM_NUMBER;
    node->value = ValueSemanticActionFromNumber(number);
    return node;
}

ExpressionNode * IdentifierTermSemanticAction(char * identifier) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ExpressionNode * node = calloc(1, sizeof(ExpressionNode));
    node->nodeType = EXPRESSION_NODE_TERM;
    node->termType = TERM_IDENTIFIER;
    node->identifier = strdup(identifier);
    return node;
}

ExpressionNode * KeywordTermSemanticAction(KeywordType type) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ExpressionNode * node = calloc(1, sizeof(ExpressionNode));
    node->termType = TERM_KEYWORD;
    node->nodeType = EXPRESSION_NODE_TERM;
    switch (type) {
        case KEYWORD_POINTS:   node->identifier = strdup("points"); break;
        case KEYWORD_SCORE:    node->identifier = strdup("score"); break;
        case KEYWORD_TIMELEFT: node->identifier = strdup("timeLeft"); break;
    }
    return node;
}

ScoringRuleNode * ScoringRuleSemanticAction(ScoringRuleType type, ExpressionNode * expression) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ScoringRuleNode * rule = calloc(1, sizeof(ScoringRuleNode));
    rule->type = type;
    rule->expression = expression;
    return rule;
}

ValueNode * ValueSemanticActionFromString(const char * string) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ValueNode * node = calloc(1, sizeof(ValueNode));
    node->type = VAL_STRING;
    node->stringValue = strdup(string);
    return node;
}

ValueNode * ValueSemanticActionFromSymbol(char * symbol) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ValueNode * node = calloc(1, sizeof(ValueNode));
    node->type = VAL_SYMBOL;
    node->stringValue = strdup(symbol);
    return node;
}

ValueNode * ValueSemanticActionFromNumber(double number) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ValueNode * node = calloc(1, sizeof(ValueNode));
    node->type = VAL_NUMBER;
    node->numberValue = number;
    return node;
}

ValueNode * ValueSemanticActionFromBoolean(int boolean) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    ValueNode * node = calloc(1, sizeof(ValueNode));
    node->type = VAL_BOOLEAN;
    node->booleanValue = boolean;
    return node;
}