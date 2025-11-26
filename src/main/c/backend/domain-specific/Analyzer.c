#include "Analyzer.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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
        logError(_logger, "Error: El Quiz debe tener un título.");
        valid = false;
    }

    // Sin preguntas
    if (program->quiz->questions == NULL || program->quiz->questions->questions == NULL) {
        logError(_logger, "Error: El Quiz debe tener al menos una pregunta.");
        valid = false;
    }

    // Tiempo global inválido
    if (_isTimeNegative(program->quiz->time)) {
        logError(_logger, "Error: El tiempo global no puede ser negativo.");
        valid = false;
    }

    return valid;
}

static bool _validateQuestionLogic(QuestionNode * q) {
    bool valid = true;

    // Falta Type
    if (q->type == NULL) {
        logError(_logger, "Error: Pregunta (ID: %s) sin tipo definido.", q->id ? q->id : "?");
        return false; // No podemos seguir validando sin tipo
    }

    // No tiene text
    if (q->text == NULL || strlen(q->text) == 0) {
        logError(_logger, "Error: Pregunta (ID: %s) sin texto.", q->id ? q->id : "?");
        valid = false;
    }

    // Falta Answer
    if (q->answer == NULL) {
        logError(_logger, "Error: Pregunta (ID: %s) sin respuesta definida.", q->id ? q->id : "?");
        valid = false;
    }

    // Puntos negativos
    if (q->points != NULL && q->points->numberValue < 0) {
        logError(_logger, "Error: Pregunta (ID: %s) tiene puntos negativos.", q->id ? q->id : "?");
        valid = false;
    }

    // Tiempo local inválido
    if (_isTimeNegative(q->time)) {
        logError(_logger, "Error: Pregunta (ID: %s) tiene tiempo negativo.", q->id ? q->id : "?");
        valid = false;
    }

    // VALIDACION POR TIPO DE PREGUNTA

    if (strcmp(q->type, "trueFalse") == 0) {
        // trueFalse (string en vez de bool)
        if (q->answer != NULL) {
             ValueNode * ansVal = (ValueNode *)q->answer->data;
             if (ansVal->type != VAL_BOOLEAN) {
                 logError(_logger, "Error: Pregunta TrueFalse (ID: %s) debe tener respuesta booleana (true/false).", q->id ? q->id : "?");
                 valid = false;
             }
        }
    }
    else if (strcmp(q->type, "multipleChoice") == 0) {
        // Sin opciones
        if (q->options == NULL) {
            logError(_logger, "Error: Pregunta MultipleChoice (ID: %s) debe tener opciones.", q->id ? q->id : "?");
            valid = false;
        } else {
            // multipleChoice con una sola opcion
            if (_countList(q->options) < 2) {
                logError(_logger, "Error: Pregunta MultipleChoice (ID: %s) debe tener al menos 2 opciones.", q->id ? q->id : "?");
                valid = false;
            }

            // Answer no es una opción
            // Iteramos sobre las respuestas
            ListNode * ansNode = q->answer;
            while (ansNode != NULL) {
                ValueNode * ansVal = (ValueNode *)ansNode->data;
                if (!_isValueInOptions(ansVal, q->options)) {
                    logError(_logger, "Error: La respuesta no coincide con ninguna opción en la pregunta (ID: %s).", q->id ? q->id : "?");
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
        
        // Next no existe
        if (cond->ifTargetId != NULL) {
            if (!_questionIdExists(cond->ifTargetId, questionsList)) {
                logError(_logger, "Error: El condicional apunta a una pregunta inexistente (ID: %s).", cond->ifTargetId);
                valid = false;
            }
        }
        if (cond->elseTargetId != NULL) {
            if (!_questionIdExists(cond->elseTargetId, questionsList)) {
                logError(_logger, "Error: El condicional apunta a una pregunta inexistente (ID: %s).", cond->elseTargetId);
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
                    logError(_logger, "Error: ID duplicado '%s'.", q1->id);
                    valid = false;
                }
                checker = checker->next;
            }
        }
        current = current->next;
    }
    return valid;
}

/* --- FUNCION PUBLICA --- */

bool executeAnalyzer(CompilerState * compilerState) {
    logDebugging(_logger, "Iniciando Validaciones Semánticas...");
    bool isValid = true;
    Program * program = compilerState->abstractSyntaxtTree;

    if (program == NULL || program->quiz == NULL) return false;

    if (!_validateGlobalConstraints(program)) isValid = false;

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
    }
    
    if (isValid) {
        logDebugging(_logger, "Validación Semántica Exitosa.");
    } else {
        logError(_logger, "Se encontraron errores semánticos.");
    }

    return isValid;
}