//
// Created by Bill Evans on 8/20/24.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "startup.h"

#define TEST_OPT "--test"
#define LEX_OPT "--lex"
#define PARSE_OPT "--parse"
#define VALIDATE_OPT "--validate"
#define TACKY_OPT "--ir"
#define TACKY_OPT2 "--tacky"
#define CODEGEN_OPT "--codegen"
#define NO_ASSEMBLE_OPT "-S"
#define NO_LINK_OPT "-c"
#define PP_ONLY_OPT "-E"
#define ONAME_OPT "-o"

// if 1, run unit tests.
int configOptTest = 0;
// if 1, run preprocessor, then stop. "-E"
int configOptPpOnly = 0;
// if 1, lex only, then stop
int configOptLexOnly = 0;
// if 1, parse only (build & print C AST), then stop
int configOptParseOnly = 0;
// if 1, parse, perform semantic analysis, and stop
int configOptValidateOnly = 0;
// if 1, parse, generate TACKY, then stop
int configOptTackyOnly = 0;
// if 1, generate assembly (build && print asm AST), but don't write any files
int configOptCodegenOnly = 0;
// if 1, don't assemble (or link)
int configOptNoAssemble = 0;
// if 1, don't invoke the linker. "-c"
int configOptNoLink = 0;

int traceAstMem = 0;
int traceTokens = 1;
int traceResolution = 1;

// Name of the input file (.c file)
char const *inputFname;
int inputFileIsC;
// Any provided output file name.
char const* oFname = NULL;
// Name of the output file(s) (constructed from input file name)
char const *ppFname;
char const *asmFname;
char const *executableFname;
char const* assembleAndLinkCommand;

char const **inputFileNames;
int numInputFileNames = 0;

int parseInputFilename(int ix) {
    assert(ix < numInputFileNames);
    const char *string = inputFileNames[ix];

    char const *pExt = strrchr(string, '.');
    if (!pExt) return 0; // no extension
    inputFileIsC = strcmp(pExt, ".c") == 0;
    // TOD: don't leak
    inputFname = strdup(string);

    // Length to where the extension goes.
    int nameLen = (int)(pExt-string);
    // pre-processed file
    // TOD: don't leak
    ppFname = malloc(nameLen + 3);
    strcpy((char*)ppFname, string);
    strcpy((char*)ppFname+nameLen, ".i");

    // assembly file name
    // TOD: don't leak
    asmFname = malloc(nameLen + 3);
    strcpy((char*)asmFname, string);
    strcpy((char*)asmFname+nameLen, ".s");

    // executable file name
    // TOD: don't leak
    executableFname = strdup(string);
    strcpy((char*)executableFname, inputFname);
    if (configOptNoLink) {
        strcpy((char *) executableFname + nameLen, ".o");
    } else {
        strcpy((char *) executableFname + nameLen, "");
    }

    return 1;
}

static int parseArgs(int argc, char **argv) {
    inputFileNames = malloc(sizeof(char*) * argc); // may wind up with empty slots at the end.
    int configOptsFound = 0;
    int ok = 1;
    for (int i=1; i<argc; ++i) {
        if (*argv[i] == '-') {
            if (strcasecmp(argv[i], TEST_OPT) == 0) {
                // --test
                ++configOptsFound;
                configOptTest = 1;
            } else if (strcasecmp(argv[i], LEX_OPT) == 0) {
                // --lex
                ++configOptsFound;
                configOptLexOnly = 1;
                configOptNoAssemble = 1;
            } else if (strcasecmp(argv[i], PARSE_OPT) == 0) {
                // --parse
                ++configOptsFound;
                configOptParseOnly = 1;
                configOptNoAssemble = 1;
            } else if (strcasecmp(argv[i], VALIDATE_OPT) == 0) {
                // --validate
                ++configOptsFound;
                configOptNoAssemble = 1;
                configOptValidateOnly = 1;
            } else if (strcasecmp(argv[i], TACKY_OPT) == 0 || strcasecmp(argv[i], TACKY_OPT2) == 0) {
                // --tacky or --ir
                ++configOptsFound;
                configOptTackyOnly = 1;
                configOptNoAssemble = 1;
            } else if (strcasecmp(argv[i], CODEGEN_OPT) == 0) {
                // --codegen
                ++configOptsFound;
                configOptCodegenOnly = 1;
                configOptNoAssemble = 1;
            } else if (strcmp(argv[i], NO_ASSEMBLE_OPT) == 0) {
                // -S
                ++configOptsFound;
                configOptNoAssemble = 1;
            } else if (strcmp(argv[i], NO_LINK_OPT) == 0) {
                // -c
                ++configOptsFound;
                configOptNoLink = 1;
            } else if (strcmp(argv[i], PP_ONLY_OPT) == 0) {
                // -E
                ++configOptsFound;
                configOptPpOnly = 1;
            } else if (strcmp(argv[i], ONAME_OPT) == 0) {
                // -o oname
                ++configOptsFound;
                oFname = argv[++i];
            } else {
                fprintf(stderr, "error: unknown command line argument: %s\n", argv[i]);
                ok = 0;
            }
        } else {
            inputFileNames[numInputFileNames++] = argv[i];
        }
    }
    return ok;
}

static int validateArgs() {
    int ok = 1;
    if (numInputFileNames == 0) {
        fprintf(stderr, "error: no input files provided.\n");
    } else {
        if ((configOptPpOnly || configOptNoLink) && numInputFileNames > 1 && oFname != NULL) {
            fprintf(stderr, "error: cannot specify -o when generating multiple output files.\n");
        }
        for (int i = 0; i < numInputFileNames; ++i) {
            if (access(inputFileNames[i], F_OK) != 0) {
                fprintf(stderr, "error: file does not exist or can't be read: %s\n", inputFileNames[i]);
                ok = 0;
            }
        }
    }
    return ok;
}

const char * buildAssembleAndLinkCommand() {
    size_t totalLen = 0;
    for (int ix=0; ix<numInputFileNames; ++ix) {
        totalLen += strlen(inputFileNames[ix]) + 1;
    }
    const char* oName = oFname ? oFname : executableFname;
    totalLen += strlen(oName) + 1 + strlen("gcc -o  -c ");
    char* cmd = malloc(totalLen);

    strcpy(cmd, "gcc ");
    if (configOptNoLink) {
        strcat(cmd, "-c ");
    }
    for (int ix=0; ix<numInputFileNames; ++ix) {
        strcat(cmd, inputFileNames[ix]);
        // If this is a ".c" file, use the ".s" name instead. ".s" and ".o" pass through unchanged.
        char* ext = strrchr(cmd, '.');
        if (strcmp(ext, ".c") == 0) strcpy(ext, ".s");
        strcat(cmd, " ");
    }
    strcat(cmd, "-o ");
    strcat(cmd, oName);

    assembleAndLinkCommand = cmd;
    return assembleAndLinkCommand;
}

int parseConfig(int argc, char **argv) {
    int ok = parseArgs(argc, argv);
    ok = ok && validateArgs();
    if (!ok) {
        exit(-1);
    }
    return ok;
}
