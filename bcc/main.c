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

#include "SetOfItemTest.h"
#include "parser/print_ast.h"
#include "parser/semantics.h"

void preProcess();

void compile();

void assembleAndLink();

void cleanup();

//int dumbtest() {
//    // allocate locals
//    asm("subq $12,%rsp");
//    // test code
//    asm("movl    $0,%r11d");
//    asm("cmpl    $2,%r11d");
//    asm("movl    $0,-4(%rbp)");
//    asm("setle   -4(%rbp)");
//    asm("movl    $0,%r11d");
//    asm("cmpl    $0,%r11d");
//    asm("movl    $0,-8(%rbp)");
//    asm("setle   -8(%rbp)");
//    asm("movl    -4(%rbp),%r10d");
//    asm("movl    %r10d,-12(%rbp)");
//    asm("movl    -8(%rbp),%r10d");
//    asm("addl    %r10d,-12(%rbp)");
//    asm("movl    -12(%rbp),%eax");
//    // epilog
//    asm("movq    %rbp, %rsp");
//    asm("popq %rbp");
//    asm("retq");
//    // not reached, makes the compiler happy.
//    return 0;
//}

int main(int argc, char **argv, char **envv) {
//    int x = dumbtest();
    if (!parseConfig(argc, argv)) {
        fprintf(stderr, "Error in command line args.");
        return -1;
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
        struct CProgram *cProgram = c_program_parse();
        if (configOptParse) {
            c_program_print(cProgram);
            c_program_free(cProgram);
            return;
        }
        if (configOptValidate) {
            c_program_print(cProgram);
            analyze_program(cProgram);
            c_program_print(cProgram);
            c_program_free(cProgram);
            return;
        }
        analyze_program(cProgram);
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
            c_program_print(cProgram);
            print_ir(irProgram, stdout);
            amd64_program_emit(asmProgram, stdout);
            amd64_program_free(asmProgram);
            c_program_free(cProgram);
            return;
        }
        c_program_print(cProgram);
        print_ir(irProgram, stdout);
        amd64_program_emit(asmProgram, stdout);

        FILE *asmf = fopen(asmFname, "w");
        amd64_program_emit(asmProgram, asmf);
        fclose(asmf);
        amd64_program_free(asmProgram);
        c_program_free(cProgram);

        assembleAndLink();
    }

}

void assembleAndLink() {
    // system("gcc {asmFname} -o {executableFname")
    int cmdLength = 10 + strlen(asmFname) + strlen(executableFname);
    char *cmd = malloc(cmdLength);
    strcpy(cmd, "gcc ");
    if (configOptNoLink) {
        strcat(cmd, "-c ");
    }
    strcat(cmd, asmFname);
    strcat(cmd, " -o ");
    strcat(cmd, executableFname);

    int rc = system(cmd);

    free(cmd);

}

void cleanup() {

}
