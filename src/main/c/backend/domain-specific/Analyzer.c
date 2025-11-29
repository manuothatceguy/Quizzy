#include "Analyzer.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>

static Logger * _logger = NULL;

void _shutdownAnalyzerModule() {
    if (_logger != NULL) {
        destroyLogger(_logger);
        _logger = NULL;
    }
}

ModuleDestructor initializeAnalyzerModule() {
    _logger = createLogger("Analyzer");
    return _shutdownAnalyzerModule;
}

// Verifica si un tiempo es negativo
static bool _isTimeNegative(char * timeStr) {
    if (timeStr == NULL) return false;
    while (*timeStr == ' ') timeStr++;
    return *timeStr == '-';
}

// Verifica si un ValueNode es igual a otro (para comparar respuestas con opciones)
static bool _areValuesEqual(ValueNode * v1, ValueNode * v2) {
    if (v1->type != v2->type) return false;
    switch (v1->type) {
        case VAL_STRING:
        case VAL_SYMBOL: return strcmp(v1->stringValue, v2->stringValue) == 0;
        case VAL_NUMBER: return v1->numberValue == v2->numberValue;
        case VAL_BOOLEAN: return v1->booleanValue == v2->booleanValue;
        default: return false;
    }
}

// Busca si un valor existe dentro de una lista de opciones
static bool _isValueInOptions(ValueNode * target, ListNode * options) {
    ListNode * current = options;
    while (current != NULL) {
        ValueNode * optionVal = (ValueNode *)current->data;
        if (_areValuesEqual(target, optionVal)) return true;
        current = current->next;
    }
    return false;
}

// Cuenta elementos en una lista
static int _countList(ListNode * list) {
    int count = 0;
    ListNode * current = list;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

// Verifica si un ID existe en la lista de preguntas (para los conditionals)
static bool _questionIdExists(char * id, ListNode * questionsList) {
    if (id == NULL) return false;
    ListNode * current = questionsList;
    while (current != NULL) {
        QuestionNode * q = (QuestionNode *)current->data;
        if (q->id && strcmp(q->id, id) == 0) return true;
        current = current->next;
    }
    return false;
}

static bool _validateGlobalConstraints(Program * program) {
    bool valid = true;
    
    // Falta Title
    if (program->quiz->title == NULL) {
        logError(_logger, "El Quiz debe tener un título.");
        valid = false;
    }

    // Sin preguntas
    if (program->quiz->questions == NULL || program->quiz->questions->questions == NULL) {
        logError(_logger, "El Quiz debe tener al menos una pregunta.");
        valid = false;
    }

    // Tiempo global inválido
    if (_isTimeNegative(program->quiz->time)) {
        logError(_logger, "El tiempo global no puede ser negativo.");
        valid = false;
    }

    return valid;
}

static bool _fileExists(const char *path) {
    if (!path || !*path) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

static bool _validateMedia(const MediaNode *media) {
    if (!media) return true; // media opcional
    for (ListNode *it = media->items; it; it = it->next) {
        MediaItemNode *item = (MediaItemNode *) it->data;
        if (!item || !item->path || !*item->path) {
            logError(_logger, "Media inválido. Item sin path.");
            return false;
        }
        if (!_fileExists(item->path)) {
            logError(_logger, "Media inválido. Archivo no existe: %s", item->path);
            return false;
        }
    }
    return true;
}

static bool _aliasExistsInMedia(const char * alias, const MediaNode * media) {
    if (!media || !alias) return false;
    for (ListNode * it = media->items; it; it = it->next) {
        MediaItemNode * item = (MediaItemNode *)it->data;
        if (item && item->alias && strcmp(item->alias, alias) == 0) {
            return true;
        }
    }
    return false;
}

static bool _validateSymbolReferences(QuestionNode * q) {
    if (!q->options || !q->media) return true;
    
    bool valid = true;
    ListNode * current = q->options;
    while (current) {
        ValueNode * val = (ValueNode *)current->data;
        if (val && val->type == VAL_SYMBOL) {
            // el símbolo es ":alias", necesitamos extraer "alias"
            const char * symbolStr = val->stringValue;
            if (symbolStr && symbolStr[0] == ':') {
                const char * alias = symbolStr + 1; // Saltar el ':'
                if (!_aliasExistsInMedia(alias, q->media)) {
                    logError(_logger, "Pregunta (ID: %s) usa símbolo ':%s' que no existe en media.", 
                             q->id ? q->id : "?", alias);
                    valid = false;
                }
            }
        }
        current = current->next;
    }
    return valid;
}

static bool _validatePartialCredit(QuestionNode * q) {
    if (!q->partialCredit || !q->partialCredit->booleanValue) return true; // No usa partial credit
    
    bool valid = true;
    
    if (q->type && strcmp(q->type, "multipleChoice") != 0) {
        logWarning(_logger, "Pregunta (ID: %s) usa partialCredit pero no es multipleChoice. Se va a ignorar.", 
                   q->id ? q->id : "?");
    }
    
    int answerCount = _countList(q->answer);
    if (answerCount < 2) {
        logWarning(_logger, "Pregunta (ID: %s) usa partialCredit pero solo tiene una respuesta correcta. No tiene efecto.", 
                   q->id ? q->id : "?");
    }
    
    return valid;
}

static bool _validateExpressionVariables(ExpressionNode * expr, const char * context) {
    if (!expr) return true;
    
    if (expr->nodeType == EXPRESSION_NODE_TERM) {
        if (expr->termType == TERM_IDENTIFIER || expr->termType == TERM_KEYWORD) {
            const char * varName = expr->identifier;
            if (strcmp(varName, "points") != 0 && 
                strcmp(varName, "score") != 0 && 
                strcmp(varName, "timeLeft") != 0) {
                logError(_logger, "Variable inválida '%s' en %s. Solo se permiten: points, score, timeLeft.", 
                         varName, context);
                return false;
            }
        }
    } else if (expr->nodeType == EXPRESSION_NODE_BINARY) {
        bool leftValid = _validateExpressionVariables(expr->left, context);
        bool rightValid = _validateExpressionVariables(expr->right, context);
        
        // Detectar división por cero literal
        if (expr->op == OP_DIV && expr->right) {
            if (expr->right->nodeType == EXPRESSION_NODE_TERM && 
                expr->right->termType == TERM_NUMBER &&
                expr->right->value->numberValue == 0.0) {
                logError(_logger, "División por cero en %s.", context);
                return false;
            }
        }
        
        return leftValid && rightValid;
    }
    return true;
}

static bool _validateScoring(ScoringNode * scoring) {
    if (!scoring) return true;
    
    bool valid = true;
    ListNode * current = scoring->rules;
    
    while (current) {
        ScoringRuleNode * rule = (ScoringRuleNode *)current->data;
        const char * ruleType = (rule->type == SCORING_CORRECT) ? "correct" :
                                (rule->type == SCORING_WRONG) ? "wrong" : "timeout";
        
        char context[64];
        snprintf(context, sizeof(context), "regla de scoring '%s'", ruleType);
        
        if (!_validateExpressionVariables(rule->expression, context)) {
            valid = false;
        }
        
        current = current->next;
    }
    
    return valid;
}

static bool _validateQuestionLogic(QuestionNode * q) {
    bool valid = true;

    // Falta Type
    if (q->type == NULL) {
        logError(_logger, "Pregunta (ID: %s) sin tipo definido.", q->id ? q->id : "?");
        return false; // No podemos seguir validando sin tipo
    }

    if (q->text == NULL || strlen(q->text) == 0) {
        logError(_logger, "Pregunta (ID: %s) sin texto.", q->id ? q->id : "?");
        valid = false;
    }

    if (q->answer == NULL) {
        logError(_logger, "Pregunta (ID: %s) sin respuesta definida.", q->id ? q->id : "?");
        valid = false;
    }

    if (q->points != NULL && q->points->numberValue < 0) {
        logError(_logger, "Pregunta (ID: %s) tiene puntos negativos.", q->id ? q->id : "?");
        valid = false;
    }

    if (_isTimeNegative(q->time)) {
        logError(_logger, "Pregunta (ID: %s) tiene tiempo negativo.", q->id ? q->id : "?");
        valid = false;
    }

    if (!_validateMedia(q->media)) {
        logError(_logger, "Pregunta (ID: %s) tiene media inválida.", q->id ? q->id : "?");
        valid = false;
    }

    if (!_validateSymbolReferences(q)) {
        valid = false;
    }

    if (!_validatePartialCredit(q)) {
        valid = false;
    }


    if (strcmp(q->type, "trueFalse") == 0) {
        if (q->answer != NULL) {
             ValueNode * ansVal = (ValueNode *)q->answer->data;
             if (ansVal->type != VAL_BOOLEAN) {
                 logError(_logger, "Pregunta TrueFalse (ID: %s) debe tener respuesta booleana (true/false).", q->id ? q->id : "?");
                 valid = false;
             }
        }
    }
    else if (strcmp(q->type, "multipleChoice") == 0) {
        if (q->options == NULL) {
            logError(_logger, "Pregunta MultipleChoice (ID: %s) debe tener opciones.", q->id ? q->id : "?");
            valid = false;
        } else {
            if (_countList(q->options) < 2) {
                logError(_logger, "Pregunta MultipleChoice (ID: %s) debe tener al menos 2 opciones.", q->id ? q->id : "?");
                valid = false;
            }

            ListNode * ansNode = q->answer;
            while (ansNode != NULL) {
                ValueNode * ansVal = (ValueNode *)ansNode->data;
                if (!_isValueInOptions(ansVal, q->options)) {
                    logError(_logger, "La respuesta no coincide con ninguna opción en la pregunta (ID: %s).", q->id ? q->id : "?");
                    valid = false;
                }
                ansNode = ansNode->next;
            }
        }
    }
    
    return valid;
}

static bool _validateConditionals(Program * program) {
    bool valid = true;
    if (program->quiz->questions == NULL) return true;
    
    ListNode * questionsList = program->quiz->questions->questions;
    ListNode * condList = program->quiz->questions->conditionals;
    
    ListNode * current = condList;
    while (current != NULL) {
        ConditionalNode * cond = (ConditionalNode *)current->data;
        
        if (cond->ifTargetId != NULL) {
            if (!_questionIdExists(cond->ifTargetId, questionsList)) {
                logError(_logger, "El condicional apunta a una pregunta inexistente (ID: %s).", cond->ifTargetId);
                valid = false;
            }
        }
        if (cond->elseTargetId != NULL) {
            if (!_questionIdExists(cond->elseTargetId, questionsList)) {
                logError(_logger, "El condicional apunta a una pregunta inexistente (ID: %s).", cond->elseTargetId);
                valid = false;
            }
        }
        
        if (cond->condition) {
            if (!_validateExpressionVariables(cond->condition, "condicional")) {
                valid = false;
            }
        }
        
        current = current->next;
    }
    return valid;
}

static bool _validateDuplicates(ListNode * questionsList) {
    bool valid = true;
    ListNode * current = questionsList;
    while (current != NULL) {
        QuestionNode * q1 = (QuestionNode *)current->data;
        if (q1->id != NULL) {
            ListNode * checker = current->next;
            while (checker != NULL) {
                QuestionNode * q2 = (QuestionNode *)checker->data;
                // IDs duplicados
                if (q2->id != NULL && strcmp(q1->id, q2->id) == 0) {
                    logError(_logger, "ID duplicado '%s'.", q1->id);
                    valid = false;
                }
                checker = checker->next;
            }
        }
        current = current->next;
    }
    return valid;
}

static bool _validateShuffleConfig(Program * program) {
    if (!program->quiz->questions) return true;
    
    bool isShuffled = program->quiz->questions->isShuffled;
    
    int totalQuestions = 0;
    int questionsWithId = 0;
    ListNode * current = program->quiz->questions->questions;
    while (current) {
        totalQuestions++;
        QuestionNode * q = (QuestionNode *)current->data;
        if (q->id) questionsWithId++;
        current = current->next;
    }
    
    if (!isShuffled && questionsWithId > 0 && program->quiz->questions->conditionals) {
        logWarning(_logger, "Hay %d preguntas con ID (usadas en condicionales) pero el quiz no usa shuffle. Las preguntas con ID solo tienen significado especial con shuffle activado.", questionsWithId);
    }
    
    return true;
}

/* --- FUNCION PUBLICA --- */

bool executeAnalyzer(CompilerState * compilerState) {
    logDebugging(_logger, "Iniciando Validaciones Semánticas...");
    bool isValid = true;
    Program * program = compilerState->abstractSyntaxtTree;

    if (program == NULL || program->quiz == NULL) return false;

    if (!_validateGlobalConstraints(program)) isValid = false;

    // ✨ NUEVA: Validar scoring
    if (program->quiz->scoring) {
        if (!_validateScoring(program->quiz->scoring)) isValid = false;
    }

    if (program->quiz->questions != NULL) {
        ListNode * questionsList = program->quiz->questions->questions;
        
        if (!_validateDuplicates(questionsList)) isValid = false;
        
        ListNode * current = questionsList;
        while (current != NULL) {
            QuestionNode * q = (QuestionNode *)current->data;
            if (!_validateQuestionLogic(q)) isValid = false;
            current = current->next;
        }
        
        if (!_validateConditionals(program)) isValid = false;
        
        if (!_validateShuffleConfig(program)) isValid = false;
    }
    
    if (isValid) {
        logDebugging(_logger, "Validación Semántica Exitosa.");
    } else {
        logError(_logger, "Se encontraron errores semánticos.");
    }

    return isValid;
}