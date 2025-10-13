#include "AbstractSyntaxTree.h"
#include <stdio.h>

static Logger * _logger = NULL;

void _shutdownAbstractSyntaxTreeModule() {
    if (_logger != NULL) {
        logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
        destroyLogger(_logger);
        _logger = NULL;
    }
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
    _logger = createLogger("AbstractSyntaxTree");
    logDebugging(_logger, "Initializing module: AbstractSyntaxTree...");
    return _shutdownAbstractSyntaxTreeModule;
}

ListNode * CreateList(void * data) {
    ListNode * newNode = (ListNode *) malloc(sizeof(ListNode));
    if (newNode == NULL) {
        logError(_logger, "Failed to allocate memory for ListNode.");
        return NULL;
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

ListNode * AppendToList(ListNode * list, void * data) {
    if (list == NULL) {
        return CreateList(data);
    }
    ListNode * current = list;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = CreateList(data);
    return list;
}

void destroyValueNode(ValueNode * value) {
    if (value == NULL) return;
    if (value->type == VAL_STRING || value->type == VAL_SYMBOL) {
        free(value->stringValue);
    }
    free(value);
}

void destroyExpressionNode(ExpressionNode * expression) {
    if (expression == NULL) return;

    if (expression->nodeType == EXPRESSION_NODE_BINARY) {
        destroyExpressionNode(expression->left);
        destroyExpressionNode(expression->right);
    } 
    else { 
        switch (expression->termType) {
            case TERM_NUMBER:
                destroyValueNode(expression->value);
                break;
            case TERM_IDENTIFIER:
            case TERM_KEYWORD:
                free(expression->identifier);
                break;
        }
    }
    
    free(expression);
}

void destroyListNode(ListNode * list, void (*destroyData)(void *)) {
    while (list != NULL) {
        ListNode * next = list->next;
        if (destroyData != NULL) {
            destroyData(list->data);
        }
        free(list);
        list = next;
    }
}

void destroyScoringRuleNode(void * data) {
    ScoringRuleNode * rule = (ScoringRuleNode *)data;
    if (rule == NULL) return;
    destroyExpressionNode(rule->expression);
    free(rule);
}

void destroyScoringNode(ScoringNode * scoring) {
    if (scoring == NULL) return;
    destroyListNode(scoring->rules, destroyScoringRuleNode);
    free(scoring);
}

void destroyMediaItemNode(void * data) {
    MediaItemNode * item = (MediaItemNode *)data;
    if (item == NULL) return;
    free(item->path);
    if (item->alias != NULL) {
        free(item->alias);
    }
    free(item);
}

void destroyMediaNode(MediaNode * media) {
    if (media == NULL) return;
    destroyListNode(media->items, destroyMediaItemNode);
    free(media);
}

void destroyQuestionNode(QuestionNode * question) {
    if (question == NULL) return;

    if (question->id) free(question->id);
    if (question->type) free(question->type);
    if (question->text) free(question->text);
    if (question->time) free(question->time);
    
    destroyListNode(question->options, (void (*)(void*))destroyValueNode);
    destroyListNode(question->answer, (void (*)(void*))destroyValueNode);
    destroyValueNode(question->points);
    destroyValueNode(question->partialCredit);
    destroyValueNode(question->caseSensitive);
    destroyMediaNode(question->media);

    free(question);
}

void destroyQuestionsBlockNode(QuestionsBlockNode * questionsBlock) {
    if (questionsBlock == NULL) return;
    destroyListNode(questionsBlock->questions, (void (*)(void *))destroyQuestionNode);
    destroyListNode(questionsBlock->conditionals, (void (*)(void *))destroyConditionalNode);
    free(questionsBlock);
}

void destroyConditionalNode(ConditionalNode * conditional) {
    if (conditional == NULL) return;
    destroyExpressionNode(conditional->condition);
    free(conditional);
}

void destroyQuizNode(QuizNode * quiz) {
    if (quiz == NULL) return;

    if (quiz->title) free(quiz->title);
    if (quiz->time) free(quiz->time);
    
    destroyValueNode(quiz->shuffle);
    destroyScoringNode(quiz->scoring);
    destroyQuestionsBlockNode(quiz->questions);
    destroyListNode(quiz->conditionals, (void (*)(void *))destroyConditionalNode);

    free(quiz);
}

void destroyProgram(Program * program) {
    logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
    if (program != NULL) {
        destroyQuizNode(program->quiz);
        free(program);
    }
}