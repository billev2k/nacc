#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils/startup.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast.h"
#include "amd64/ir2amd64.h"
#include "parser/ast2ir.h"
#include "ir/print_ir.h"

#include "parser/print_ast.h"

void preProcess();

void compile();

void assembleAndLink();

void cleanup();

int main(int argc, char **argv, char **envv) {
    if (!parseConfig(argc, argv)) {
        fprintf(stderr, "Error in command line args.");
        return -1;
    }
    // For each file on the command line
    for (int ix=0; ix<numInputFileNames; ++ix) {
        parseInputFilename(ix);
        // If it is a .c file
        if (inputFileIsC) {
            // Preprocess to .i file
            preProcess();
            // If "compileOpt"
            if (!configOptPpOnly) {
                // compile to .s file
                compile();
                // remove .i file
                remove(ppFname);
            }
        }
    }

    if (!(configOptNoAssemble | configOptNoLink)) {
        //     Gather all the .s, .o, and .c->.s files into one command line invocation to gcc.
        if (buildAssembleAndLinkCommand()) {
            assembleAndLink();
        }
    }

    return 0;
}

void preProcess() {
    // system("gcc -E -P {inputFname} -o {ppFname}")
    int cmdLength = 15 + strlen(inputFname) + strlen(ppFname);
    char *cmd = malloc(cmdLength);
    strcpy(cmd, "gcc -E -P ");
    strcat(cmd, inputFname);
    strcat(cmd, " -o ");
    strcat(cmd, ppFname);

    int rc = system(cmd);

    free(cmd);
}

void compile() {
    // system("gcc -S -O -fno-asynchronous-unwind-tables -fcd-protection=none {ppFname} -o {asmFname}")

    lex_openFile(ppFname);

    if (configOptLexOnly) {
        struct Token tk;
        while ((tk = lex_take_token()).tk != TK_EOF) {
            printf("Token %d: %s\n", tk.tk, tk.text);
            if (tk.tk == TK_UNKNOWN) {
                printf("Unknown token_text!\n");
                exit(1);
            }
        }
    } else {
        struct CProgram *cProgram = c_program_parse();
        if (configOptParseOnly) {
            c_program_print(cProgram);
            c_program_delete(cProgram);
            return;
        }
        if (configOptValidateOnly) {
            c_program_print(cProgram);
            analyze_program(cProgram);
            c_program_print(cProgram);
            c_program_delete(cProgram);
            return;
        }
        analyze_program(cProgram);
        struct IrProgram *irProgram = ast2ir(cProgram);
        if (configOptTackyOnly) {
            c_program_print(cProgram);
            print_ir(irProgram, stdout);
            c_program_delete(cProgram);
            IrProgram_delete(irProgram);
            return;
        }
        struct Amd64Program *asmProgram = ir2amd64(irProgram);
        if (configOptCodegenOnly) {
            c_program_print(cProgram);
            print_ir(irProgram, stdout);
            amd64_program_emit(asmProgram, stdout);
            amd64_program_delete(asmProgram);
            c_program_delete(cProgram);
            return;
        }
        c_program_print(cProgram);
        print_ir(irProgram, stdout);
        amd64_program_emit(asmProgram, stdout);

        FILE *asmf = fopen(asmFname, "w");
        amd64_program_emit(asmProgram, asmf);
        fclose(asmf);
        amd64_program_delete(asmProgram);
        c_program_delete(cProgram);
    }

}

void assembleAndLink() {
    // system("gcc {asmFname} -o {executableFname")
    int rc = system(assembleAndLinkCommand);
}

void cleanup() {

}
