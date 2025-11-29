#include "Generator.h"
#include "Templates.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static Logger * _logger = NULL;

const char * JS_PATH = "src/main/c/backend/domain-specific/quiz-utils/script.js";
const char * HTML_PATH = "src/main/c/backend/domain-specific/quiz-utils/base.html";
const char * booleanStrings[] = {"false", "true"};

// Funciones helpers

void _shutdownGeneratorModule() {
    if (_logger != NULL) { destroyLogger(_logger); _logger = NULL; }
}

ModuleDestructor initializeGeneratorModule() {
    _logger = createLogger("Generator");
    return _shutdownGeneratorModule;
}

static void _output(FILE * file, const char * const format, ...) {
    va_list arguments;
    va_start(arguments, format);
    vfprintf(file, format, arguments);
    va_end(arguments);
}

static void _printCleanId(FILE * out, char * str) {
    if (!str) return;
    int len = strlen(str);
    if (len >= 2 && (str[0] == '"' || str[0] == '\'')) {
        char buffer[256];
        strncpy(buffer, str + 1, len - 2);
        buffer[len - 2] = '\0';
        fprintf(out, "%s", buffer);
    } else {
        fprintf(out, "%s", str);
    }
}

static int _parseTimeToSeconds(char * timeStr) {
    if (!timeStr) return 0;
    char * temp = strdup(timeStr);
    char * valPart = strtok(temp, " ");
    char * unitPart = strtok(NULL, " ");
    if (!valPart) { free(temp); return 0; }
    double val = atof(valPart);
    int sec = (int)val;
    if (unitPart && strstr(unitPart,"m")) sec = (int)(val * 60);
    free(temp);
    return sec;
}

static double _extractValue(ExpressionNode * expr) {
    if (!expr) return 0.0;
    if (expr->nodeType == EXPRESSION_NODE_TERM && expr->termType == TERM_NUMBER)
        return expr->value->numberValue;
    if (expr->nodeType == EXPRESSION_NODE_BINARY) {
        double l = _extractValue(expr->left);
        double r = _extractValue(expr->right);
        if (expr->op == OP_ADD) return l + r;
        if (expr->op == OP_SUB) return l - r;
        if (expr->op == OP_MUL) return l * r;
        if (expr->op == OP_DIV && r != 0) return l / r;
    }
    return 0.0;
}

static const char * _extractOperator(ExpressionNode * expr) {
    if (!expr || expr->nodeType != EXPRESSION_NODE_BINARY) return ">=";
    switch (expr->op) {
        case OP_EQ:  return "==";
        case OP_NEQ: return "!=";
        case OP_GT:  return ">";
        case OP_LT:  return "<";
        case OP_GTE: return ">=";
        case OP_LTE: return "<=";
        default:     return ">=";
    }
}

static const char * _extractLeftVariable(ExpressionNode * expr) {
    if (!expr || expr->nodeType != EXPRESSION_NODE_BINARY) return "score";
    if (!expr->left) return "score";
    
    if (expr->left->nodeType == EXPRESSION_NODE_TERM) {
        if (expr->left->termType == TERM_IDENTIFIER || expr->left->termType == TERM_KEYWORD) {
            return expr->left->identifier;
        }
    }
    return "score";
}

static char * _readFile(const char * filepath) {
    FILE * f = fopen(filepath, "r");
    if (!f) {
        logCritical(_logger, "No se pudo abrir %s", filepath); // debería explotar todo porque supuestamente estos archivos no cambian con el tiempo ergo difícil llegar a este caso
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char * content = malloc(size + 1);
    if (!content) {
        logCritical(_logger, "No hay memoria para leer %s", filepath);
        fclose(f);
        return NULL;
    }
    
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);
    
    logDebugging(_logger, "Archivo %s leído correctamente (%ld bytes)", filepath, size);
    return content;
}

// Funciones de generacion de cuerpo

static void _genMedia(MediaNode * m, FILE * out) {
    if(!m || !m->items) return;
    _output(out, "<div style='text-align:center; margin-bottom:20px;'>\n");
    ListNode * cur = m->items;
    while(cur) {
        MediaItemNode * i = (MediaItemNode*)cur->data;
        if(i->path) {
            if(i->type == MEDIA_AUDIO) _output(out, "<audio controls src='%s' style='width:100%%; margin-top:10px;'></audio>\n", i->path);
            else if(i->type == MEDIA_IMAGE) _output(out, "<img src='%s' style='max-width:100%%;border-radius:12px;box-shadow:0 4px 6px rgba(0,0,0,0.1);'>\n", i->path);
        }
        cur = cur->next;
    }
    _output(out, "</div>\n");
}

static void _genOpts(QuestionNode * q, int idx, FILE * out) {
    bool multipleAnswers = (q->answer && q->answer->next) ? true : false;
    const char * inputType = multipleAnswers ? "checkbox" : "radio";
    
    _output(out, "<div style='display:flex; flex-direction:column; gap:10px;'>\n");
    ListNode * cur = q->options;
    while(cur) {
        ValueNode * v = (ValueNode*)cur->data;
        if(v) {
            char * s = (v->type == VAL_STRING || v->type == VAL_SYMBOL) ? v->stringValue : "Opción";
            if(v->type == VAL_NUMBER) { static char b[32]; snprintf(b,32,"%.2f",v->numberValue); s=b; }
            _output(out, "<label class='opt-label'><input type='%s' name='q%d' value='%s'> %s</label>\n", 
                    inputType, idx, s, s);
        }
        cur = cur->next;
    }
    _output(out, "</div>\n");
}

static void _genAns(QuestionNode * q, FILE * out) {
    if(!q->answer) return;
    if(q->answer->next) {
        ListNode * n = q->answer;
        int f=1;
        while(n) {
            ValueNode * v = (ValueNode*)n->data;
            if(!f) _output(out, ",");
            if(v->type==VAL_STRING || v->type==VAL_SYMBOL) _output(out,"%s",v->stringValue);
            f=0; n=n->next;
        }
    } else if(q->answer->data) {
        ValueNode * v = (ValueNode*)q->answer->data;
        if(v->type==VAL_STRING || v->type==VAL_SYMBOL) _output(out,"%s",v->stringValue);
        else if(v->type==VAL_BOOLEAN) _output(out,"%s",booleanStrings[v->booleanValue]);
        else if(v->type==VAL_NUMBER) _output(out,"%.2f",v->numberValue);
    }
}

//Funciones principales

static void _generatePrologue(FILE * out, Program * p) {
    
    char * htmlContent = _readFile(HTML_PATH);
    if(htmlContent){
        _output(out, "%s", htmlContent);
        free(htmlContent);
        logInformation(_logger, "Archivo html leído exitosamente");
    } else {
        logCritical(_logger, "No se pudo leer el archivo html");
        exit(1); // no tiene sentido seguir con la ejecución sin el html, por eso el exit
    }

    if (p->quiz->time) {
        int sec = _parseTimeToSeconds(p->quiz->time);
        _output(out, "  <div id='timer' class='timer' data-sec='%d'></div>\n", sec);
    }
    double pWrong=0, pTimeout=0;
    if (p->quiz->scoring) {
        ListNode * r = p->quiz->scoring->rules;
        while(r) {
            ScoringRuleNode * rule = (ScoringRuleNode*)r->data;
            if (rule->type == SCORING_WRONG) pWrong = _extractValue(rule->expression);
            if (rule->type == SCORING_TIMEOUT) pTimeout = _extractValue(rule->expression);
            r = r->next;
        }
    }
    _output(out, "  <input type='hidden' id='p-wrong' value='%.2f'>\n", pWrong);
    _output(out, "  <input type='hidden' id='p-timeout' value='%.2f'>\n", pTimeout);
}

static void _generateQuestion(QuestionNode * q, int idx, FILE * out) {
    if(!q) return;
    double pts = q->points ? q->points->numberValue : 0;
    int qSec = q->time ? _parseTimeToSeconds(q->time) : 0;
    
    _output(out, "<div class='question-card' id='");
    if (q->id) _printCleanId(out, q->id);
    else _output(out, "q_%d", idx);
    _output(out, "' ");
    
    const char * cs = (q->caseSensitive && q->caseSensitive->booleanValue) ? booleanStrings[true]:booleanStrings[false];
    const char * pc = (q->partialCredit && q->partialCredit->booleanValue) ? booleanStrings[true]:booleanStrings[false];

    bool multipleAnswers = (q->answer && q->answer->next) ? true : false;

    _output(out, "data-type='%s' data-p='%.2f' data-case='%s' data-qtime='%d' data-partial='%s' data-multi='%s' data-ans='",
            q->type ? q->type : "shortAnswer", pts, cs, qSec, pc, booleanStrings[multipleAnswers]);
    _genAns(q, out);
    _output(out, "'>\n");
    
    _output(out, "<div class='q-header'>\n");
    if(q->text) _output(out, "  <span class='q-text'>%s</span>\n", q->text);
    if(qSec > 0) _output(out, "  <div class='q-timer-badge'>⏳ <span class='q-val'>%d</span>s</div>\n", qSec);
    _output(out, "</div>\n");

    _genMedia(q->media, out);
    
    if(q->type && !strcmp(q->type, "multipleChoice")) _genOpts(q, idx, out);
    else if(q->type && !strcmp(q->type, "trueFalse")) {
        _output(out, "<div style='display:flex; gap:15px;'>");
        _output(out, "<label class='opt-label' style='flex:1;justify-content:center;'><input type='radio' name='q%d' value='true'> Verdadero</label>", idx);
        _output(out, "<label class='opt-label' style='flex:1;justify-content:center;'><input type='radio' name='q%d' value='false'> Falso</label>", idx);
        _output(out, "</div>");
    } else {
        _output(out, "<input type='text' name='q%d' placeholder='Escribe tu respuesta aquí...'>\n", idx);
    }
    _output(out, "<button class='btn-next' onclick='next(this)'>Continuar</button>\n");
    _output(out, "</div>\n");
}

static void _generateBody(FILE * out, Program * p) {
    if(!p || !p->quiz) return;
    if(p->quiz->title) _output(out, "<h1>%s</h1>\n", p->quiz->title);
    if(p->quiz->questions) {
        ListNode * c = p->quiz->questions->questions;
        int i=0;
        while(c) {
            _generateQuestion((QuestionNode*)c->data, i++, out);
            c = c->next;
        }
    }
}

static void _printJumps(ListNode * list, FILE * out) {
    _output(out, "const jumps = [\n");
    while(list) {
        ConditionalNode * c = (ConditionalNode*)list->data;
        if (c->condition->nodeType == EXPRESSION_NODE_BINARY && c->ifTargetId) {
            const char * op = _extractOperator(c->condition);
            const char * leftVar = _extractLeftVariable(c->condition);
            double threshold = _extractValue(c->condition->right);
            
            _output(out, "  { op: '%s', variable: '%s', threshold: %.2f, target: '", 
                    op, leftVar, threshold);
            _printCleanId(out, c->ifTargetId);
            _output(out, "'");
            
            if (c->elseTargetId) {
                _output(out, ", elseTarget: '");
                _printCleanId(out, c->elseTargetId);
                _output(out, "'");
            }
            
            _output(out, " },\n");
        }
        list = list->next;
    }
    _output(out, "];\n");
}

static void _generateEpilogue(FILE * out, Program * p) {
    _output(out, "  <div id='result' class='result-box'></div>\n</div>\n");
    
    _output(out, "<script>\n");
    
    if (p->quiz->questions) {
        _printJumps(p->quiz->questions->conditionals, out);
    } else {
        _output(out, "const jumps = [];\n");
    }

    bool shouldShuffle = (p->quiz->questions && p->quiz->questions->isShuffled);
    _output(out, "const shouldShuffle = %s;\n", booleanStrings[shouldShuffle]);
    
    char * jsContent = _readFile(JS_PATH);
    if (jsContent) {
        _output(out, "%s", jsContent);
        free(jsContent);
        logInformation(_logger, "Archivo js leído exitosamente");
    } else {
        logCritical(_logger, "No se pudo leer el archivo js");
        exit(1); // cuando no puedo leer archivos explota todo con este exit
    }
    
    _output(out, "</script>\n</body>\n</html>\n");
}

void executeGenerator(CompilerState * cs) {
    logDebugging(_logger, "Generando quiz.html...");
    FILE * out = fopen("quiz.html", "w");
    if(out && cs->abstractSyntaxtTree) {
        _generatePrologue(out, cs->abstractSyntaxtTree);
        _generateBody(out, cs->abstractSyntaxtTree);
        _generateEpilogue(out, cs->abstractSyntaxtTree);
        fclose(out);
    }
}