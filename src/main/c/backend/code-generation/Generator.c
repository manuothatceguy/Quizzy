#include "Generator.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static Logger * _logger = NULL;

// Constantes de la plantilla HTML
static const char * HTML_HEAD = 
"<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Quizzy</title><style>"
":root{--primary:#6366f1;--bg:#f3f4f6;--text:#1f2937;--green:#10b981;--red:#ef4444;--orange:#f59e0b;}"
"body{font-family:'Segoe UI',sans-serif;background:var(--bg);color:var(--text);padding:20px;display:flex;justify-content:center;}"
".quiz-container{background:white;width:100%;max-width:700px;padding:40px;border-radius:20px;box-shadow:0 10px 25px rgba(0,0,0,0.1);position:relative;min-height:400px;display:flex;flex-direction:column;overflow:hidden;}"
"h1{text-align:center;color:var(--primary);border-bottom:2px solid #eef2ff;padding-bottom:20px;margin-top:0;}"
".question-card{display:none;flex-grow:1;animation:slideUp 0.4s ease-out;position:relative;}.question-card.active{display:block;}"
"@keyframes slideUp{from{opacity:0;transform:translateY(20px);}to{opacity:1;transform:translateY(0);}}"
".q-header{display:flex;justify-content:space-between;align-items:flex-start;margin-bottom:20px;}"
".q-text{font-size:1.3rem;font-weight:600;line-height:1.5;color:#111;flex:1;margin-right:15px;}"
".opt-label{display:flex;align-items:center;padding:15px;border:2px solid #e5e7eb;border-radius:10px;margin-bottom:12px;cursor:pointer;transition:all 0.2s;font-weight:500;}"
".opt-label:hover{background:#eef2ff;border-color:var(--primary);transform:translateX(5px);}"
".opt-label input{margin-right:15px;transform:scale(1.3);accent-color:var(--primary);}"
"input[type='text']{width:100%;padding:15px;border-radius:10px;border:2px solid #e5e7eb;font-size:1rem;transition:0.3s;outline:none;box-sizing:border-box;}"
"input[type='text']:focus{border-color:var(--primary);box-shadow:0 0 0 4px rgba(99,102,241,0.1);}"
".btn-next{width:100%;background:var(--primary);color:white;padding:15px;border:none;border-radius:12px;font-size:1.1rem;font-weight:bold;cursor:pointer;margin-top:25px;transition:0.2s;}"
".btn-next:hover{background:#4f46e5;transform:translateY(-2px);}"
".timer{position:absolute;top:20px;right:20px;background:white;color:var(--text);padding:8px 16px;border-radius:50px;font-weight:700;font-family:monospace;font-size:1.2rem;display:none;border:2px solid var(--primary);z-index:10;}"
".timer.danger{border-color:var(--red);color:var(--red);background:#fef2f2;animation:pulse 1s infinite;}"
".q-timer-badge{display:inline-flex;align-items:center;background:#fffbeb;color:#b45309;border:2px solid var(--orange);padding:5px 12px;border-radius:20px;font-weight:bold;font-family:monospace;font-size:1rem;}"
"@keyframes pulse{0%{transform:scale(1);}50%{transform:scale(1.05);}100%{transform:scale(1);}}"
".result-box{text-align:center;padding:40px;border-radius:12px;display:none;flex-direction:column;justify-content:center;height:100%;animation:fadeIn 0.5s;}"
"@keyframes fadeIn{from{opacity:0;}to{opacity:1;}}.pass{background:#ecfdf5;border:2px solid var(--green);color:#065f46;}.fail{background:#fef2f2;border:2px solid var(--red);color:#991b1b;}"
".score-num{font-size:3.5rem;font-weight:800;margin:10px 0;letter-spacing:-1px;}"
"</style></head><body><div class='quiz-container'>";

// Logica JS para el quiz
static const char * JS_SCRIPT = 
"const cards=Array.from(document.querySelectorAll('.question-card'));let idx=0,score=0,maxScore=0,qInterval=null;"
"const lockedIds=new Set(jumps.map(j=>j.target));"
// Bonus
"lockedIds.forEach(id=>{"
"  let el=document.getElementById(id);"
"  if(el){"
"    let b=document.createElement('div');"
"    b.innerText='★ Pregunta Bonus';b.style.cssText='color:#f59e0b;font-weight:bold;margin-bottom:10px';"
"    el.querySelector('.q-header').before(b);"
"  }"
"});"
"if(cards.length>0){cards[0].classList.add('active');startQTimer(cards[0]);}"
// Timer Global
"const timerEl=document.getElementById('timer');let time=timerEl?parseInt(timerEl.dataset.sec):0;let interval;"
"if(time>0){timerEl.style.display='block';updateTimerDisplay();interval=setInterval(()=>{time--;updateTimerDisplay();if(time<=0){clearInterval(interval);finish(true);}},1000);}"
"function updateTimerDisplay(){let m=Math.floor(time/60),s=time%60;timerEl.innerText=`${m}:${s<10?'0':''}${s}`;if(time<=10)timerEl.classList.add('danger');}"
// Timer Pregunta
"function startQTimer(c){if(!c)return;let t=parseInt(c.dataset.qtime)||0;if(t<=0)return;let cur=t,d=c.querySelector('.q-val');if(d)d.innerText=cur;"
"qInterval=setInterval(()=>{cur--;if(d)d.innerText=cur;if(cur<=0){clearInterval(qInterval);c.querySelector('.btn-next')?.click();}},1000);}"
// Logica Next
"function next(btn){if(qInterval){clearInterval(qInterval);qInterval=null;}let card=btn.closest('.question-card');if(card.dataset.processed)return;card.dataset.processed='true';"
"let pts=parseFloat(card.dataset.p)||0,pWrong=parseFloat(document.getElementById('p-wrong').value);"
"let ans=card.dataset.ans,type=card.dataset.type,caseSens=card.dataset.case==='true',isBonus=lockedIds.has(card.id);"
"let user='',correct=false;"
"if(type.includes('multiple')||type.includes('true')){let el=card.querySelector('input:checked');if(el)user=el.value;}"
"else{let el=card.querySelector('input[type=text]');if(el)user=el.value.trim();}"
"if(user){if(caseSens)correct=(user===ans);else correct=(user.toLowerCase()===ans.toLowerCase());if(!correct&&ans.includes(','))if(ans.split(',').some(a=>a.trim().toLowerCase()===user.toLowerCase()))correct=true;}"
"card.dataset.result=correct?'correct':'wrong';"
"if(correct){score+=pts;if(!isBonus)maxScore+=pts;}else{if(!isBonus){score+=pWrong;maxScore+=pts;}}"
"card.classList.remove('active');let tid=null;for(let j of jumps){let d=document.getElementById(j.target);if(d&&cards.indexOf(d)===idx+1&&score>=j.threshold)tid=j.target;}"
"let nextCard=null;if(tid){nextCard=document.getElementById(tid);if(nextCard)idx=cards.indexOf(nextCard);}"
"if(!nextCard){let n=idx+1;while(n<cards.length&&lockedIds.has(cards[n].id))n++;if(n<cards.length){idx=n;nextCard=cards[idx];}}"
"if(nextCard){nextCard.classList.add('active');startQTimer(nextCard);}else finish(false);}"
// Finish
"function finish(to){clearInterval(interval);if(qInterval)clearInterval(qInterval);"
"document.querySelector('h1').style.display='none';if(timerEl)timerEl.style.display='none';"
"let res=document.getElementById('result');let pass=maxScore>0?(score/maxScore>=0.5):false;"
"res.className='result-box '+(pass?'pass':'fail');res.style.display='flex';"
"res.innerHTML=(to?'<h3>¡Tiempo Agotado!</h3>':'')+(pass?'<h1>¡Felicidades!</h1>':'<h1>Inténtalo de Nuevo</h1>')+`<div class='score-num'>${score.toFixed(1)} / ${maxScore.toFixed(1)}</div><p>Puntaje Final</p><h3 style='margin-top:30px;border-top:1px solid #ccc;padding-top:20px;'>Revisión</h3>`;"
"let cont=document.querySelector('.quiz-container');cont.style.overflowY='auto';cont.style.height='auto';cont.insertBefore(res,cont.firstChild);"
"cards.forEach(c=>{c.style.display='block';c.classList.remove('active');c.style.marginBottom='20px';c.style.pointerEvents='none';c.style.opacity='1';c.style.animation='none';"
"if(c.dataset.result==='correct'){c.style.border='3px solid #10b981';c.style.background='#ecfdf5';}else{c.style.border='3px solid #ef4444';c.style.background='#fef2f2';}c.querySelector('.btn-next').style.display='none';});}";

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