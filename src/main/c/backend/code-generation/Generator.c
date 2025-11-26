#include "Generator.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static Logger * _logger = NULL;

void _shutdownGeneratorModule() {
    if (_logger != NULL) {
        destroyLogger(_logger);
        _logger = NULL;
    }
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

// Limpia comillas para IDs
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

// Estructura HTML
static void _generateHeader(FILE * out, Program * p) {
    _output(out, "<!DOCTYPE html>\n<html lang='es'>\n<head>\n");
    _output(out, "<meta charset='UTF-8'>\n<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");
    _output(out, "<title>Quizzy</title>\n");
    _output(out, "<style>\n");
    _output(out, "  :root { --primary: #6366f1; --bg: #f3f4f6; --text: #1f2937; --green: #10b981; --red: #ef4444; --orange: #f59e0b; }\n");
    _output(out, "  body { font-family: 'Segoe UI', sans-serif; background: var(--bg); color: var(--text); padding: 20px; display: flex; justify-content: center; }\n");
    _output(out, "  .quiz-container { background: white; width: 100%%; max-width: 700px; padding: 40px; border-radius: 20px; box-shadow: 0 10px 25px rgba(0,0,0,0.1); position: relative; min-height: 400px; display: flex; flex-direction: column; overflow: hidden; }\n");
    _output(out, "  h1 { text-align: center; color: var(--primary); border-bottom: 2px solid #eef2ff; padding-bottom: 20px; margin-top: 0; }\n");
    _output(out, "  .question-card { display: none; flex-grow: 1; animation: slideUp 0.4s ease-out; position: relative; }\n");
    _output(out, "  .question-card.active { display: block; }\n");
    _output(out, "  @keyframes slideUp { from { opacity: 0; transform: translateY(20px); } to { opacity: 1; transform: translateY(0); } }\n");
    
    // Pregunta
    _output(out, "  .q-header { display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 20px; }\n");
    _output(out, "  .q-text { font-size: 1.3rem; font-weight: 600; line-height: 1.5; color: #111; flex: 1; margin-right: 15px; }\n");
    _output(out, "  .opt-label { display: flex; align-items: center; padding: 15px; border: 2px solid #e5e7eb; border-radius: 10px; margin-bottom: 12px; cursor: pointer; transition: all 0.2s; font-weight: 500; }\n");
    _output(out, "  .opt-label:hover { background: #eef2ff; border-color: var(--primary); transform: translateX(5px); }\n");
    _output(out, "  .opt-label input { margin-right: 15px; transform: scale(1.3); accent-color: var(--primary); }\n");
    _output(out, "  input[type='text'] { width: 100%%; padding: 15px; border-radius: 10px; border: 2px solid #e5e7eb; font-size: 1rem; transition: 0.3s; outline: none; box-sizing: border-box; }\n");
    _output(out, "  input[type='text']:focus { border-color: var(--primary); box-shadow: 0 0 0 4px rgba(99, 102, 241, 0.1); }\n");
    
    // Boton
    _output(out, "  .btn-next { width: 100%%; background: var(--primary); color: white; padding: 15px; border: none; border-radius: 12px; font-size: 1.1rem; font-weight: bold; cursor: pointer; margin-top: 25px; transition: transform 0.1s, background 0.2s; box-shadow: 0 4px 6px rgba(99, 102, 241, 0.2); }\n");
    _output(out, "  .btn-next:hover { background: #4f46e5; transform: translateY(-2px); }\n");
    _output(out, "  .btn-next:active { transform: translateY(0); }\n");
    
    // Timer global
    _output(out, "  .timer { position: absolute; top: 20px; right: 20px; background: white; color: var(--text); padding: 8px 16px; border-radius: 50px; font-weight: 700; font-family: monospace; font-size: 1.2rem; display: none; box-shadow: 0 4px 10px rgba(0,0,0,0.1); border: 2px solid var(--primary); z-index: 10; transition: all 0.3s ease; }\n");
    _output(out, "  .timer.danger { border-color: var(--red); color: var(--red); background: #fef2f2; animation: pulse 1s infinite; }\n");
    
    // Timer pregunta
    _output(out, "  .q-timer-badge { display: inline-flex; align-items: center; background: #fffbeb; color: #b45309; border: 2px solid var(--orange); padding: 5px 12px; border-radius: 20px; font-weight: bold; font-family: monospace; font-size: 1rem; box-shadow: 0 2px 4px rgba(0,0,0,0.05); white-space: nowrap; }\n");
    _output(out, "  .q-timer-badge span { margin-left: 5px; font-size: 1.1em; }\n");
    
    _output(out, "  @keyframes pulse { 0%% { transform: scale(1); } 50%% { transform: scale(1.05); } 100%% { transform: scale(1); } }\n");

    // (Verde/Rojo)
    _output(out, "  .result-box { text-align: center; padding: 40px; border-radius: 12px; display: none; flex-direction: column; justify-content: center; height: 100%%; animation: fadeIn 0.5s; }\n");
    _output(out, "  @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }\n");
    _output(out, "  .pass { background: #ecfdf5; border: 2px solid var(--green); color: #065f46; }\n");
    _output(out, "  .fail { background: #fef2f2; border: 2px solid var(--red); color: #991b1b; }\n");
    _output(out, "  .score-num { font-size: 3.5rem; font-weight: 800; margin: 10px 0; letter-spacing: -1px; }\n");
    _output(out, "</style>\n");
    _output(out, "</head>\n<body>\n");
    _output(out, "<div class='quiz-container'>\n");

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

//Logica saltos

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

static void _generateFooter(FILE * out, Program * p) {
    _output(out, "  <div id='result' class='result-box'></div>\n");
    _output(out, "</div>\n");
    _output(out, "<script>\n");
    
    if (p->quiz->questions) _printJumps(p->quiz->questions->conditionals, out);
    else _output(out, "const jumps = [];\n");
    
    _output(out, "const cards = Array.from(document.querySelectorAll('.question-card'));\n");
    _output(out, "let idx = 0;\n");
    _output(out, "let score = 0;\n");
    _output(out, "let maxScore = 0;\n");
    
    _output(out, "let qInterval = null;\n");
    
    _output(out, "if(cards.length > 0) {\n");
    _output(out, "  cards[0].classList.add('active');\n");
    _output(out, "  startQTimer(cards[0]);\n");
    _output(out, "}\n");
    
    // Identificar Bonus
    _output(out, "const lockedIds = new Set(jumps.map(j => j.target));\n");
    
    // Timer GLOBAL
    _output(out, "const timerEl = document.getElementById('timer');\n");
    _output(out, "let time = timerEl ? parseInt(timerEl.dataset.sec) : 0;\n");
    _output(out, "let interval;\n");
    _output(out, "if(time > 0) {\n");
    _output(out, "  timerEl.style.display='block';\n");
    _output(out, "  updateTimerDisplay();\n");
    _output(out, "  interval = setInterval(()=>{\n");
    _output(out, "    time--;\n");
    _output(out, "    updateTimerDisplay();\n");
    _output(out, "    if(time<=0) { clearInterval(interval); finish(true); }\n");
    _output(out, "  }, 1000);\n");
    _output(out, "}\n");

    _output(out, "function updateTimerDisplay() {\n");
    _output(out, "    let m=Math.floor(time/60), s=time%%60;\n");
    _output(out, "    timerEl.innerText = `${m}:${s<10?'0':''}${s}`;\n");
    _output(out, "    if(time <= 10) timerEl.classList.add('danger');\n");
    _output(out, "}\n");
    
    // TIMER PREGUNTA
    _output(out, "function startQTimer(card) {\n");
    _output(out, "  if(!card) return;\n");
    _output(out, "  const tVal = parseInt(card.dataset.qtime) || 0;\n");
    _output(out, "  if(tVal <= 0) return;\n");
    _output(out, "  let curr = tVal;\n");
    _output(out, "  const display = card.querySelector('.q-val');\n");
    _output(out, "  if(display) display.innerText = curr;\n");
    _output(out, "  qInterval = setInterval(()=>{\n");
    _output(out, "    curr--;\n");
    _output(out, "    if(display) display.innerText = curr;\n");
    _output(out, "    if(curr <= 0) {\n");
    _output(out, "      clearInterval(qInterval);\n");
    _output(out, "      // Forzar siguiente pregunta\n");
    _output(out, "      const btn = card.querySelector('.btn-next');\n");
    _output(out, "      if(btn) btn.click();\n");
    _output(out, "    }\n");
    _output(out, "  }, 1000);\n");
    _output(out, "}\n");
    
    _output(out, "function next(btn) {\n");
    _output(out, "  // Detener timer de la pregunta actual si existe\n");
    _output(out, "  if(qInterval) { clearInterval(qInterval); qInterval=null; }\n");

    _output(out, "  const card = btn.closest('.question-card');\n");
    _output(out, "  if(card.dataset.processed) return;\n");
    _output(out, "  card.dataset.processed = 'true';\n");

    _output(out, "  const pts = parseFloat(card.dataset.p) || 0;\n");
    _output(out, "  const pWrong = parseFloat(document.getElementById('p-wrong').value);\n");
    _output(out, "  const ans = card.dataset.ans;\n");
    _output(out, "  const type = card.dataset.type;\n");
    _output(out, "  const caseSens = card.dataset.case === 'true';\n");

    _output(out, "  const isBonus = lockedIds.has(card.id);\n");

    _output(out, "  let user = '';\n");
    _output(out, "  if(type.includes('multiple') || type.includes('true')) {\n");
    _output(out, "    const el = card.querySelector('input:checked');\n");
    _output(out, "    if(el) user = el.value;\n");
    _output(out, "  } else {\n");
    _output(out, "    const el = card.querySelector('input[type=text]');\n");
    _output(out, "    if(el) user = el.value.trim();\n");
    _output(out, "  }\n");

    _output(out, "  let correct = false;\n");
    _output(out, "  if(user) {\n");
    _output(out, "    if(caseSens) correct = (user === ans);\n");
    _output(out, "    else correct = (user.toLowerCase() === ans.toLowerCase());\n");
    _output(out, "    if(!correct && ans.includes(',')) {\n");
    _output(out, "       if(ans.split(',').some(a=>a.trim().toLowerCase()===user.toLowerCase())) correct = true;\n");
    _output(out, "    }\n");
    _output(out, "  }\n");

    _output(out, "  if(correct) { \n");
    _output(out, "      score += pts; \n");
    _output(out, "      if(!isBonus) maxScore += pts; \n");
    _output(out, "  } else { \n");
    _output(out, "      if(!isBonus) { \n");
    _output(out, "          score += pWrong; \n");
    _output(out, "          maxScore += pts; \n");
    _output(out, "      }\n");
    _output(out, "  }\n");

    _output(out, "  card.classList.remove('active');\n");

    // Lógica de Salto
    _output(out, "  let targetId = null;\n");
    _output(out, "  for(let j of jumps) {\n");
    _output(out, "    const destCard = document.getElementById(j.target);\n");
    _output(out, "    if(destCard && cards.indexOf(destCard) === idx + 1) {\n");
    _output(out, "       if(score >= j.threshold) targetId = j.target;\n");
    _output(out, "    }\n");
    _output(out, "  }\n");

    _output(out, "  let nextCard = null;\n");
    _output(out, "  if(targetId) {\n");
    _output(out, "    nextCard = document.getElementById(targetId);\n");
    _output(out, "    if(nextCard) idx = cards.indexOf(nextCard);\n");
    _output(out, "  }\n");

    _output(out, "  if(!nextCard) {\n");
    _output(out, "    let nextIdx = idx + 1;\n");
    _output(out, "    while(nextIdx < cards.length && lockedIds.has(cards[nextIdx].id)) {\n");
    _output(out, "        nextIdx++;\n");
    _output(out, "    }\n");
    _output(out, "    if(nextIdx < cards.length) {\n");
    _output(out, "        idx = nextIdx;\n");
    _output(out, "        nextCard = cards[idx];\n");
    _output(out, "    }\n");
    _output(out, "  }\n");

    _output(out, "  if(nextCard) {\n");
    _output(out, "      nextCard.classList.add('active');\n");
    _output(out, "      startQTimer(nextCard);\n");
    _output(out, "  } else finish(false);\n");
    _output(out, "}\n");

    _output(out, "function finish(timeout) {\n");
    _output(out, "  clearInterval(interval);\n");
    _output(out, "  if(qInterval) clearInterval(qInterval);\n");
    _output(out, "  const res = document.getElementById('result');\n");
    _output(out, "  cards.forEach(c => c.style.display='none');\n");
    _output(out, "  document.querySelector('h1').style.display='none';\n");
    _output(out, "  if(timerEl) timerEl.style.display='none';\n");

    _output(out, "  const isPass = maxScore > 0 ? (score/maxScore >= 0.5) : false;\n");
    _output(out, "  res.className = 'result-box ' + (isPass ? 'pass' : 'fail');\n");
    _output(out, "  res.style.display = 'flex';\n");
    _output(out, "  let html = timeout ? '<h3>¡Tiempo Agotado!</h3>' : '';\n");
    _output(out, "  html += isPass ? '<h1>¡Felicidades!</h1>' : '<h1>Inténtalo de Nuevo</h1>';\n");
    _output(out, "  html += `<div class='score-num'>${score.toFixed(1)} / ${maxScore.toFixed(1)}</div>`;\n");
    _output(out, "  html += `<p>Puntos Finales</p>`;\n");
    _output(out, "  res.innerHTML = html;\n");
    _output(out, "}\n");
    _output(out, "</script>\n</body>\n</html>\n");
}


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

static void _generateQuestion(QuestionNode * q, int idx, FILE * out) {
    if(!q) return;
    double pts = q->points ? q->points->numberValue : 0;
    
    // Calcular tiempo de la pregunta
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
    
    if(qSec > 0) {
        _output(out, "  <div class='q-timer-badge'>⏳ <span class='q-val'>%d</span>s</div>\n", qSec);
    }
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

static void _generateBody(Program * p, FILE * out) {
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

void executeGenerator(CompilerState * cs) {
    logDebugging(_logger, "Generando quiz.html...");
    FILE * out = fopen("quiz.html", "w");
    if(out && cs->abstractSyntaxtTree) {
        _generateHeader(out, cs->abstractSyntaxtTree);
        _generateBody(cs->abstractSyntaxtTree, out);
        _generateFooter(out, cs->abstractSyntaxtTree);
        fclose(out);
    }
}