//
// Created by Bill Evans on 8/28/24.
//

#include <stdlib.h>
#include <printf.h>
#include "ast.h"

void c_program_free(struct CProgram *program) {
    if (!program) return;
    if (program->function) c_function_free(program->function);
    free(program);
}

void c_function_free(struct CFunction *function) {
    if (!function) return;
    // Don't free 'name'; owned by global name table.
    if (function->statement) c_statement_free(function->statement);
    free(function);
}

void c_statement_free(struct CStatement *statement) {
    if (!statement) return;
    if (statement->expression) c_expression_free(statement->expression);
    free(statement);
}

void c_expression_free(struct CExpression *expression) {
    if (!expression) return;
    // Don't free expression's 'value'; owned by global name table.
    free(expression);
}


void c_function_print(struct CFunction *function);
void c_statement_print(struct CStatement *statement, int depth);
void expression_print(struct CExpression *expression, int depth);
void c_program_print(struct CProgram *program) {
    printf("Program(\n");
    if (program->function) c_function_print(program->function);
    printf(")\n");
}
void c_function_print(struct CFunction *function) {
    printf("  Function(\n");
    printf("    name=\"%s\"\n", function->name);
    printf("    body=");
    if (function->statement) c_statement_print(function->statement, 0);
    printf("  )\n");
}
void c_statement_print(struct CStatement *statement, int depth) {
    printf("Return(\n");
    expression_print(statement->expression, depth);
    printf("    )\n");
}
void expression_print(struct CExpression *expression, int depth) {
    printf("      Constant(%s)\n", expression->value);
}
