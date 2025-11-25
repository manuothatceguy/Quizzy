#include "Analyzer.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <string.h>

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

/* --- VALIDACIONES PRIVADAS --- */

// Validación 1: Verificar IDs duplicados
static bool _validateDuplicateIDs(ListNode * questionsList) {
    bool isValid = true;
    ListNode * current = questionsList;

    while (current != NULL) {
        QuestionNode * q1 = (QuestionNode *) current->data;
        
        ListNode * checker = current->next;
        while (checker != NULL) {
            QuestionNode * q2 = (QuestionNode *) checker->data;
            
            // Si ambos tienen ID y son iguales -> ERROR
            if (q1->id && q2->id && strcmp(q1->id, q2->id) == 0) {
                logError(_logger, "Error Semántico: El ID de pregunta '%s' está duplicado.", q1->id);
                isValid = false;
            }
            checker = checker->next;
        }
        current = current->next;
    }
    return isValid;
}

// Validación 2: Verificar que MultipleChoice tenga opciones
static bool _validateQuestionIntegrity(ListNode * questionsList) {
    bool isValid = true;
    ListNode * current = questionsList;

    while (current != NULL) {
        QuestionNode * q = (QuestionNode *) current->data;
        
        if (q->text == NULL || strlen(q->text) == 0) {
            logError(_logger, "Error Semántico: La pregunta con ID '%s' no tiene texto.", q->id ? q->id : "???");
            isValid = false;
        }

        // Chequeo de Opciones para Multiple Choice
        if (q->type && strcmp(q->type, "multipleChoice") == 0) {
            if (q->options == NULL) {
                logError(_logger, "Error Semántico: La pregunta '%s' es MultipleChoice pero no tiene opciones.", q->id ? q->id : "???");
                isValid = false;
            }
        }

        current = current->next;
    }
    return isValid;
}

/* --- FUNCION PUBLICA --- */

bool executeAnalyzer(CompilerState * compilerState) {
    logDebugging(_logger, "Ejecutando Análisis Semántico...");
    bool isValid = true;
    Program * program = compilerState->abstractSyntaxtTree;

    if (program != NULL && program->quiz != NULL && program->quiz->questions != NULL) {
        ListNode * questionsList = program->quiz->questions->questions;
        if (!_validateDuplicateIDs(questionsList)) isValid = false;
        if (!_validateQuestionIntegrity(questionsList)) isValid = false;
        
    } else {
        logWarning(_logger, "El Quiz no tiene preguntas para validar.");
    }

    if (isValid) {
        logDebugging(_logger, "Análisis Semántico exitoso.");
    } else {
        logError(_logger, "El Análisis Semántico falló. No se generará código.");
    }

    return isValid;
}