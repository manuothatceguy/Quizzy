#ifndef GENERATOR_HEADER
#define GENERATOR_HEADER

#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"

/**
 * Inicializa el estado interno del módulo Generator.
 * Crea el logger y prepara recursos si es necesario.
 */
ModuleDestructor initializeGeneratorModule();

/**
 * Ejecuta la generación de código (Backend).
 * Toma el AST del compilerState y genera código HTML en la salida estándar (stdout).
 * * @param compilerState El estado actual del compilador que contiene el AST.
 */
void executeGenerator(CompilerState * compilerState);

#endif