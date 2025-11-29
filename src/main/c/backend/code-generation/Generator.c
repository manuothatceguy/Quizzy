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
    _output(out, "<div style='display:flex; flex-direction:column; gap:10px;'>\n");
    ListNode * cur = q->options;
    while(cur) {
        ValueNode * v = (ValueNode*)cur->data;
        if(v) {
            char * s = (v->type == VAL_STRING || v->type == VAL_SYMBOL) ? v->stringValue : "Opción";
            if(v->type == VAL_NUMBER) { static char b[32]; snprintf(b,32,"%.2f",v->numberValue); s=b; }
            _output(out, "<label class='opt-label'><input type='radio' name='q%d' value='%s'> %s</label>\n", idx, s, s);
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
        else if(v->type==VAL_BOOLEAN) _output(out,"%s",v->booleanValue?"true":"false");
        else if(v->type==VAL_NUMBER) _output(out,"%.2f",v->numberValue);
    }
}

//Funciones principales

static void _generatePrologue(FILE * out, Program * p) {
    
    fprintf(out, "%s", HTML_HEAD);

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
    
    const char * cs = (q->caseSensitive && q->caseSensitive->booleanValue) ? "true":"false";
    _output(out, "data-type='%s' data-p='%.2f' data-case='%s' data-qtime='%d' data-ans='",
            q->type ? q->type : "shortAnswer", pts, cs, qSec);
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
            double t = _extractValue(c->condition->right);
            _output(out, "  { threshold: %.2f, target: '", t);
            _printCleanId(out, c->ifTargetId);
            _output(out, "' },\n");
        }
        list = list->next;
    }
    _output(out, "];\n");
}

static void _generateEpilogue(FILE * out, Program * p) {
    // Cerramos contenedores
    _output(out, "  <div id='result' class='result-box'></div>\n</div>\n");
    
    // Script
    _output(out, "<script>\n");
    
    // 1. Datos dinámicos (Saltos)
    if (p->quiz->questions) _printJumps(p->quiz->questions->conditionals, out);
    else _output(out, "const jumps = [];\n");
    
    // 2. Lógica Estática (Plantilla JS)
    fprintf(out, "%s", JS_SCRIPT);
    
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