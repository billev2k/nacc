#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "startup.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "ir2amd64.h"
#include "ast2ir.h"
#include "print_ir.h"

#include "SetOfItemTest.h"

void preProcess();

void compile();

void assembleAndLink();

void cleanup();

int main(int argc, char **argv, char **envv) {
    if (!parseConfig(argc, argv)) {
        fprintf(stderr, "Error in command line args.");
        return -1;
    }
    if (configOptTest) {
        unit_tests(argc, argv);
        exit(0);
    }
    preProcess();
    compile();
    //assembleAndLink();
    //cleanup();

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

    if (configOptLex) {
        struct Token tk;
        while ((tk = lex_take_token()).token != TK_EOF) {
            printf("Token %d: %s\n", tk.token, tk.text);
            if (tk.token == TK_UNKNOWN) {
                printf("Unknown token_text!\n");
                exit(1);
            }
        }
    } else {
        struct CProgram *cProgram = parser_go();
        if (configOptParse) {
            c_program_print(cProgram);
            c_program_free(cProgram);
            return;
        }
        struct IrProgram *irProgram = ast2ir(cProgram);
        if (configOptTacky) {
            c_program_print(cProgram);
            print_ir(irProgram, stdout);
            c_program_free(cProgram);
            IrProgram_free(irProgram);
            return;
        }
        struct Amd64Program *asmProgram = ir2amd64(irProgram);
        if (configOptCodegen) {
            amd64_program_emit(asmProgram, stdout);
            amd64_program_free(asmProgram);
            c_program_free(cProgram);
            return;
        }
        FILE *asmf = fopen(asmFname, "w");
        amd64_program_emit(asmProgram, asmf);
        fclose(asmf);

        assembleAndLink();
    }

}

void assembleAndLink() {
    // system("gcc {asmFname} -o {executableFname")
    int cmdLength = 10 + strlen(asmFname) + strlen(executableFname);
    char *cmd = malloc(cmdLength);
    strcpy(cmd, "gcc ");
    strcat(cmd, asmFname);
    strcat(cmd, " -o ");
    strcat(cmd, executableFname);

    int rc = system(cmd);

    free(cmd);

}

void cleanup() {

}
