#include "backend/code-generation/Generator.h"
#include "backend/domain-specific/Analyzer.h"
#include "frontend/Frontend.h"
#include "frontend/lexical-analysis/FlexActions.h"
#include "frontend/syntactic-analysis/BisonActions.h"
#include "support/logging/Logger.h"
#include "support/type/CompilationStatus.h"
#include "support/type/CompilerState.h"
#include "support/type/ModuleDestructor.h"

/**
 * The main entry-point of the entire application.
 */
const int main(const int length, const char ** arguments) {
    LexicalAnalyzer * lexicalAnalyzer = createLexicalAnalyzer();
    Logger * logger = createLogger("EntryPoint");
    for (int k = 0; k < length; ++k) {
        logDebugging(logger, "Argument %d: \"%s\"", k, arguments[k]);
    }
    CompilerState compilerState = {
        .abstractSyntaxtTree = NULL,
        .value = 0
    };
    
    ModuleDestructor moduleDestructors[] = {
        initializeAbstractSyntaxTreeModule(),
        initializeFlexActionsModule(lexicalAnalyzer),
        initializeBisonActionsModule(&compilerState),
        initializeFrontendModule(lexicalAnalyzer),
        initializeAnalyzerModule(),
        initializeGeneratorModule()
    };
    
    CompilationStatus compilationStatus = executeSyntacticAnalysis();
    Program * program = compilerState.abstractSyntaxtTree;
    
if (compilationStatus == SUCCEEDED) {
        logDebugging(logger, "Syntactic analysis succeeded. Running Semantic Analysis...");

        bool isSemanticallyValid = executeAnalyzer(&compilerState);

        if (isSemanticallyValid) {
             logDebugging(logger, "Semantic checks passed. Generating output...");
             executeGenerator(&compilerState);
        } 
        else {
             logError(logger, "Semantic analysis failed.");
             compilationStatus = FAILED;
        }
    }
    else {
        logError(logger, "The syntactic-analysis phase rejects the input program.");
        compilationStatus = FAILED;
    }
    
    logDebugging(logger, "Releasing AST resources...");
    destroyProgram(program);
    for (int k = (sizeof(moduleDestructors)/sizeof(ModuleDestructor)) - 1; 0 <= k; --k) {
        moduleDestructors[k]();
    }
    logDebugging(logger, "Compilation is done.");
    destroyLogger(logger);
    destroyLexicalAnalyzer(lexicalAnalyzer);
    return compilationStatus;
}