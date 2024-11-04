//
// Created by Bill Evans on 8/20/24.
//
#include <stdlib.h>
#include <string.h>

#include "startup.h"

#define TEST_OPT "--test"
#define LEX_OPT "--lex"
#define PARSE_OPT "--parse"
#define TACKY_OPT "--ir"
#define TACKY_OPT2 "--tacky"
#define CODEGEN_OPT "--codegen"
#define ASM_OPT "-S"

// if 1, run unit tests.
int configOptTest = 0;
// if 1, lex only, then stop
int configOptLex = 0;
// if 1, parse only (build C AST), then stop
int configOptParse = 0;
// if 1, parse, generate TACKY, then stop
int configOptTacky = 0;
// if 1, generate assembly (build asm AST), but don't write any files
int configOptCodegen = 0;
// if 1, create assembly output; don't assemble or link
int configOptAsm = 0;

int traceAstMem = 0;
int traceTokens = 0;

// Name of the input file (.c file)
char const *inputFname;
// Name of the output file(s) (constructed from input file name)
char const *ppFname;
char const *asmFname;
char const *executableFname;

int parseInputFilename(char *string) {
    inputFname = strdup(string);
    char const *pExt = strrchr(string, '.');
    if (!pExt) return 0; // no extension

    // Length to where the extension goes.
    int nameLen = (int)(pExt-string);
    // pre-processed file
    ppFname = malloc(nameLen + 3);
    strcpy((char*)ppFname, string);
    strcpy((char*)ppFname+nameLen, ".i");
    // assembly file name
    asmFname = malloc(nameLen + 3);
    strcpy((char*)asmFname, string);
    strcpy((char*)asmFname+nameLen, ".s");
    // executable file name
    executableFname = strdup(string);
    strcpy((char*)executableFname, inputFname);
    strcpy((char*)executableFname+nameLen, "");

    return 1;
}

int parseConfig(int argc, char **argv) {
    int configOptsFound = 0;
    int ok = 1;
    for (int i=1; i<argc; ++i) {
        if (strcasecmp(argv[i], TEST_OPT) == 0) {
            ++configOptsFound;
            configOptTest = 1;
        } else if (strcasecmp(argv[i], LEX_OPT) == 0) {
            ++configOptsFound;
            configOptLex = 1;
        } else if (strcasecmp(argv[i], PARSE_OPT) == 0) {
            ++configOptsFound;
            configOptParse = 1;
        } else if (strcasecmp(argv[i], TACKY_OPT) == 0 || strcasecmp(argv[i], TACKY_OPT2) == 0) {
            ++configOptsFound;
            configOptTacky = 1;
        } else if (strcasecmp(argv[i], CODEGEN_OPT) == 0) {
            ++configOptsFound;
            configOptCodegen = 1;
        } else if (strcasecmp(argv[i], ASM_OPT) == 0) {
            ++configOptsFound;
            configOptAsm = 1;
        } else {
            ok |= parseInputFilename(argv[i]);
        }
    }
    return ok && (inputFname && *inputFname || configOptTest);
}

