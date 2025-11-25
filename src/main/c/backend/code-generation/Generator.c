#include "Generator.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
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

static void _generateHeader(FILE * out) {
    _output(out, "<!DOCTYPE html>\n<html lang='es'>\n<head>\n");
    _output(out, "<meta charset='UTF-8'>\n<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");
    _output(out, "<title>Quizzy</title>\n");
    _output(out, "<style>\n");
    _output(out, "  :root { --primary: #6366f1; --surface: #ffffff; --bg: #f3f4f6; --text: #1f2937; --success: #22c55e; --error: #ef4444; }\n");
    _output(out, "  body { font-family: 'Inter', system-ui, sans-serif; background: var(--bg); color: var(--text); padding: 2rem; display: flex; justify-content: center; }\n");
    _output(out, "  .quiz-wrapper { background: var(--surface); width: 100%%; max-width: 700px; padding: 2rem; border-radius: 16px; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1); }\n");
    _output(out, "  h1 { text-align: center; color: var(--primary); margin-bottom: 2rem; border-bottom: 2px solid #e5e7eb; padding-bottom: 1rem; }\n");
    _output(out, "  .question-card { margin-bottom: 2rem; padding: 1.5rem; border: 2px solid #e5e7eb; border-radius: 12px; background: #fafafa; transition: all 0.3s ease; }\n");
    _output(out, "  .question-text { font-size: 1.15rem; font-weight: 600; margin-bottom: 1rem; display: block; }\n");
    _output(out, "  .options-list { display: flex; flex-direction: column; gap: 0.75rem; }\n");
    _output(out, "  .option-item { display: flex; align-items: center; padding: 0.75rem; border-radius: 8px; border: 1px solid #d1d5db; background: white; cursor: pointer; transition: all 0.2s; }\n");
    _output(out, "  .option-item:hover { border-color: var(--primary); background: #eef2ff; }\n");
    _output(out, "  .option-item input { margin-right: 10px; accent-color: var(--primary); transform: scale(1.2); }\n");
    _output(out, "  input[type='text'] { width: 100%%; padding: 12px; border: 1px solid #d1d5db; border-radius: 8px; font-size: 1rem; margin-top: 10px; box-sizing: border-box; }\n");
    _output(out, "  input[type='text']:focus { outline: none; border-color: var(--primary); ring: 2px solid var(--primary); }\n");
    
    _output(out, "  .correct { border-color: var(--success); background-color: #f0fdf4; }\n");
    _output(out, "  .incorrect { border-color: var(--error); background-color: #fef2f2; }\n");
    _output(out, "  .feedback { font-weight: bold; margin-top: 10px; display: none; }\n");
    
    _output(out, "  .btn-submit { background: var(--primary); color: white; width: 100%%; padding: 1rem; border: none; border-radius: 8px; font-size: 1.1rem; font-weight: bold; cursor: pointer; margin-top: 1rem; transition: background 0.2s; }\n");
    _output(out, "  .btn-submit:hover { background: #4f46e5; }\n");
    _output(out, "</style>\n");
    _output(out, "</head>\n<body>\n");
    _output(out, "<div class='quiz-wrapper'>\n");
}

static void _generateFooter(FILE * out) {
    _output(out, "  <div id='result-display' class='result-box'></div>\n");
    
    // Botón
    _output(out, "  <button class='btn-submit' onclick='evaluateQuiz()'>Enviar Quiz</button>\n");
    _output(out, "</div>\n");

    // Estilos extra para la caja de resultados
    _output(out, "<style>\n");
    _output(out, "  .result-box { margin-top: 20px; padding: 20px; border-radius: 10px; text-align: center; display: none; animation: fadeIn 0.5s; }\n");
    _output(out, "  .result-score { font-size: 2.5rem; font-weight: bold; color: var(--primary); display: block; margin: 10px 0; }\n");
    _output(out, "  @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }\n");
    _output(out, "</style>\n");

    // SCRIPT JS
    _output(out, "<script>\n");
    _output(out, "function evaluateQuiz() {\n");
    _output(out, "  let score = 0;\n");
    _output(out, "  let total = 0;\n");
    _output(out, "  const cards = document.querySelectorAll('.question-card');\n");
    _output(out, "  \n");
    _output(out, "  cards.forEach(card => {\n");
    _output(out, "    total++;\n");
    _output(out, "    const correctAnswer = card.dataset.answer;\n");
    _output(out, "    const type = card.dataset.type;\n");
    _output(out, "    const feedback = card.querySelector('.feedback');\n");
    _output(out, "    let userAnswer = '';\n");
    _output(out, "    \n");
    _output(out, "    if (type === 'multipleChoice' || type === 'trueFalse') {\n");
    _output(out, "      const selected = card.querySelector('input:checked');\n");
    _output(out, "      if (selected) userAnswer = selected.value;\n");
    _output(out, "    } else {\n");
    _output(out, "      const input = card.querySelector('input[type=\"text\"]');\n");
    _output(out, "      if (input) userAnswer = input.value.trim();\n");
    _output(out, "    }\n");
    _output(out, "    \n");
    _output(out, "    // Lógica de comparación simple\n");
    _output(out, "    if (userAnswer && userAnswer.toLowerCase() === correctAnswer.toLowerCase()) {\n");
    _output(out, "      score++;\n");
    _output(out, "      card.classList.add('correct');\n");
    _output(out, "      card.classList.remove('incorrect');\n");
    _output(out, "      feedback.style.color = 'var(--success)';\n");
    _output(out, "      feedback.textContent = '¡Correcto!';\n");
    _output(out, "    } else {\n");
    _output(out, "      card.classList.add('incorrect');\n");
    _output(out, "      card.classList.remove('correct');\n");
    _output(out, "      feedback.style.color = 'var(--error)';\n");
    _output(out, "      feedback.textContent = 'Incorrecto. La respuesta era: ' + correctAnswer;\n");
    _output(out, "    }\n");
    _output(out, "    feedback.style.display = 'block';\n");
    _output(out, "  });\n");
    _output(out, "  \n");
    
    _output(out, "  const resultDisplay = document.getElementById('result-display');\n");
    _output(out, "  resultDisplay.style.display = 'block';\n");
    
    // Color de fondo segun la nota
    _output(out, "  if (score / total >= 0.5) {\n");
    _output(out, "     resultDisplay.style.backgroundColor = '#dcfce7';\n"); // Verde
    _output(out, "     resultDisplay.style.border = '2px solid #22c55e';\n");
    _output(out, "     resultDisplay.innerHTML = `<h3>¡Bien hecho!</h3><span class='result-score'>${score} / ${total}</span><p>Has aprobado el cuestionario.</p>`;\n");
    _output(out, "  } else {\n");
    _output(out, "     resultDisplay.style.backgroundColor = '#fee2e2';\n"); // Rojo
    _output(out, "     resultDisplay.style.border = '2px solid #ef4444';\n");
    _output(out, "     resultDisplay.innerHTML = `<h3>Sigue intentando</h3><span class='result-score'>${score} / ${total}</span><p>Repasa los conceptos e inténtalo de nuevo.</p>`;\n");
    _output(out, "  }\n");
    
    _output(out, "  resultDisplay.scrollIntoView({ behavior: 'smooth' });\n");
    _output(out, "}\n");
    _output(out, "</script>\n");
    
    _output(out, "</body>\n</html>\n");
}


static void _generateOptions(QuestionNode * question, int index, FILE * out) {
    _output(out, "    <div class='options-list'>\n");
    
    ListNode * current = question->options;
    while (current != NULL) {
        ValueNode * valNode = (ValueNode *) current->data;
        if (valNode && valNode->stringValue) {
            _output(out, "      <label class='option-item'>\n");
            _output(out, "        <input type='radio' name='q%d' value='%s'>\n", index, valNode->stringValue);
            _output(out, "        <span>%s</span>\n", valNode->stringValue);
            _output(out, "      </label>\n");
        }
        current = current->next;
    }
    _output(out, "    </div>\n");
}

static void _generateTrueFalse(int index, FILE * out) {
    _output(out, "    <div class='options-list' style='flex-direction: row; gap: 1rem;'>\n");
    _output(out, "      <label class='option-item' style='flex:1; justify-content: center;'>\n");
    _output(out, "        <input type='radio' name='q%d' value='true'> Verdadero\n", index);
    _output(out, "      </label>\n");
    _output(out, "      <label class='option-item' style='flex:1; justify-content: center;'>\n");
    _output(out, "        <input type='radio' name='q%d' value='false'> Falso\n", index);
    _output(out, "      </label>\n");
    _output(out, "    </div>\n");
}

static void _generateShortAnswer(int index, FILE * out) {
    _output(out, "    <input type='text' name='q%d' placeholder='Escribe tu respuesta aquí...'>\n", index);
}

static void _printCorrectAnswer(QuestionNode * question, FILE * out) {
    if (question->answer != NULL && question->answer->data != NULL) {
        ValueNode * ansNode = (ValueNode *) question->answer->data;
        
        if (ansNode->type == VAL_STRING || ansNode->type == VAL_SYMBOL) {
            _output(out, "%s", ansNode->stringValue);
        } else if (ansNode->type == VAL_NUMBER) {
            _output(out, "%.2f", ansNode->numberValue);
        } else if (ansNode->type == VAL_BOOLEAN) {
            _output(out, "%s", ansNode->booleanValue ? "true" : "false");
        }
    } else {
        _output(out, "undefined");
    }
}

static void _generateQuestion(QuestionNode * question, int index, FILE * out) {
    if (!question) return;

    _output(out, "  <div class='question-card' id='%s' data-type='%s' data-answer='", 
            question->id ? question->id : "q",
            question->type ? question->type : "shortAnswer");
    
    _printCorrectAnswer(question, out);
    
    _output(out, "'>\n");
    
    // Texto de la pregunta
    if (question->text) {
        _output(out, "    <span class='question-text'>%d. %s</span>\n", index + 1, question->text);
    }

    // Logica basada en el TIPO
    if (question->type != NULL) {
        if (strcmp(question->type, "multipleChoice") == 0) {
            _generateOptions(question, index, out);
        } else if (strcmp(question->type, "trueFalse") == 0) {
            _generateTrueFalse(index, out);
        } else {
            _generateShortAnswer(index, out);
        }
    } else {
        if (question->options != NULL) _generateOptions(question, index, out);
        else _generateShortAnswer(index, out);
    }

    _output(out, "    <div class='feedback'></div>\n");
    _output(out, "  </div>\n");
}

static void _generateBody(Program * program, FILE * out) {
    if (!program || !program->quiz) return;

    if (program->quiz->title) {
        _output(out, "  <h1>%s</h1>\n", program->quiz->title);
    } else {
        _output(out, "  <h1>Cuestionario</h1>\n");
    }

    if (program->quiz->questions != NULL) {
        ListNode * currentList = program->quiz->questions->questions;
        int index = 0;
        while (currentList != NULL) {
            QuestionNode * q = (QuestionNode *) currentList->data;
            _generateQuestion(q, index, out);
            currentList = currentList->next;
            index++;
        }
    }
}

/* --- FUNCION PUBLICA --- */

void executeGenerator(CompilerState * compilerState) {
    logDebugging(_logger, "Generando archivo de salida (quiz.html)...");
    
    FILE * out = fopen("quiz.html", "w");
    
    if (out == NULL) {
        logError(_logger, "Error: No se pudo crear el archivo quiz.html");
        return;
    }

    if (compilerState->abstractSyntaxtTree != NULL) {
        _generateHeader(out);
        _generateBody(compilerState->abstractSyntaxtTree, out);
        _generateFooter(out);
        logDebugging(_logger, "¡Éxito! El archivo 'quiz.html' ha sido generado.");
    } else {
        logError(_logger, "El AST es NULL. No se puede generar código.");
    }
    
    fclose(out);
}