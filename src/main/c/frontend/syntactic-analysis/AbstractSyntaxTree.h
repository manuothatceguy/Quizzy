#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdlib.h>
#include <string.h>

ModuleDestructor initializeAbstractSyntaxTreeModule();

typedef struct Program Program;
typedef struct QuizNode QuizNode;
typedef struct ScoringNode ScoringNode;
typedef struct ScoringRuleNode ScoringRuleNode;
typedef struct QuestionsBlockNode QuestionsBlockNode;
typedef struct QuestionNode QuestionNode;
typedef struct MediaNode MediaNode;
typedef struct MediaItemNode MediaItemNode;
typedef struct ConditionalNode ConditionalNode;
typedef struct ExpressionNode ExpressionNode;
typedef struct ValueNode ValueNode;
typedef struct AttributeNode AttributeNode;
typedef struct ListNode ListNode;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GTE, OP_LTE
} OperatorType;

typedef enum {
    TERM_NUMBER, TERM_IDENTIFIER, TERM_KEYWORD
} TermType;

typedef enum {
    VAL_STRING, VAL_NUMBER, VAL_BOOLEAN, VAL_SYMBOL
} ValueType;

typedef enum {
    MEDIA_IMAGE, MEDIA_AUDIO, MEDIA_VIDEO
} MediaType;

typedef enum {
    SCORING_CORRECT, SCORING_WRONG, SCORING_TIMEOUT
} ScoringRuleType;

typedef enum {
    ATTR_ID, ATTR_TYPE, ATTR_TEXT, ATTR_OPTIONS, ATTR_ANSWER, ATTR_POINTS,
    ATTR_PARTIAL_CREDIT, ATTR_CASE_SENSITIVE, ATTR_TITLE, ATTR_SHUFFLE, ATTR_TIME
} AttributeType;

typedef enum {
    BODY_NODE_ATTRIBUTE,
    BODY_NODE_SCORING,
    BODY_NODE_QUESTIONS,
    BODY_NODE_MEDIA,
    BODY_NODE_CONDITIONAL
} BodyNodeType;

typedef struct BodyNode {
    BodyNodeType type;
    union {
        AttributeNode * attributeNode;
        ScoringNode * scoringNode;
        QuestionsBlockNode * questionsBlockNode;
        MediaNode * mediaNode;
        ConditionalNode * conditionalNode;
    } data;
} BodyNode;

struct AttributeNode {
    AttributeType type;
    union {
        char * stringValue;
        ValueNode * valueNode;
        ListNode * listValue;
    };
};

struct ListNode {
    void * data;
    ListNode * next;
};

struct ValueNode {
    ValueType type;
    union {
        char * stringValue;
        double numberValue;
        int booleanValue;
    };
};

typedef enum {
    EXPRESSION_NODE_BINARY,
    EXPRESSION_NODE_TERM
} ExpressionNodeType;

struct ExpressionNode {
    ExpressionNodeType nodeType; 
    
    OperatorType op;
    TermType termType;
    union {
        struct {
            ExpressionNode * left;
            ExpressionNode * right;
        };
        ValueNode * value;
        char* identifier;
    };
};

struct ScoringRuleNode {
    ScoringRuleType type;
    ExpressionNode * expression;
};

struct ScoringNode {
    ListNode * rules;
};

struct MediaItemNode {
    MediaType type;
    char * path;
    char * alias; // nulleable
};

struct MediaNode {
    ListNode * items; 
};

struct QuestionNode {
    char * id;
    char * type;
    char * text;
    ListNode * options; 
    ListNode * answer;  
    ValueNode * points;
    ValueNode * partialCredit;
    ValueNode * caseSensitive;
    char* time;
    MediaNode * media;
};

struct QuestionsBlockNode {
    bool isShuffled; // 1 si es {}, 0 si es []
    ListNode * questions; 
    ListNode * conditionals; 
};

// Nodo para un if
struct ConditionalNode {
    ExpressionNode * condition;
    char * ifTargetId;
    char * elseTargetId; // nulleable
};

struct QuizNode {
    char * title;
    ValueNode * shuffle;
    char * time;
    ScoringNode * scoring;
    QuestionsBlockNode * questions;
    ListNode * conditionals;
};

struct Program {
    QuizNode * quiz;
};

ListNode * CreateList(void * data);
ListNode * AppendToList(ListNode * list, void * data);


void destroyProgram(Program * program);
void destroyQuizNode(QuizNode * quiz);
void destroyScoringNode(ScoringNode * scoring);
void destroyScoringRuleNode(void * data);
void destroyQuestionsBlockNode(QuestionsBlockNode * questions);
void destroyQuestionNode(QuestionNode * question);
void destroyMediaNode(MediaNode * media);
void destroyMediaItemNode(void * data);
void destroyConditionalNode(ConditionalNode * conditional);
void destroyExpressionNode(ExpressionNode * expression);
void destroyValueNode(ValueNode * value);
void destroyListNode(ListNode * list, void (*destroyData)(void *));
void destroyAttributeNode(AttributeNode * attr);
void destroyBodyNode(void * data);

#endif