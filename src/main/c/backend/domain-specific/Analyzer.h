#ifndef ANALYZER_HEADER
#define ANALYZER_HEADER

#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdbool.h>

/** Inicializa el módulo */
ModuleDestructor initializeAnalyzerModule();

/**
 * Ejecuta las validaciones semánticas.
 * @return true si el programa es válido, false si tiene errores lógicos.
 */
bool executeAnalyzer(CompilerState * compilerState);

#endif