#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 10/27/24.
//

#include <stdio.h>

#include "ast.h"
#include "print_ast.h"

static void c_function_print(struct CFunction *function);
static void c_statement_print(struct CStatement *statement, int depth);
static void expression_print(struct CExpression *expression, int depth);
void c_program_print(struct CProgram *program) {
    printf("Program(\n");
    if (program->function) c_function_print(program->function);
    printf(")\n");
}
static void c_function_print(struct CFunction *function) {
    printf("  Function(\n");
    printf("    name=\"%s\"\n", function->name);
    printf("    body=");
    if (function->statement) c_statement_print(function->statement, 0);
    printf("  )\n");
}
static void c_statement_print(struct CStatement *statement, int depth) {
    printf("Return(\n");
    expression_print(statement->expression, depth);
    printf("    )\n");
}
#define PR(depth) do {for (int i=0; i<depth; ++i) printf("   "); } while (0)
static void expression_print(struct CExpression *expression, int depth) {
    PR(depth);
    switch (expression->type) {
        case AST_EXP_CONST:
            printf("      %s\n", expression->value);
            break;
        case AST_EXP_UNOP:
            printf("      %s (\n", AST_UNARY_NAMES[expression->unary_op]);
            expression_print(expression->operand, depth+1);
            PR(depth);
            printf("      )\n");
            break;
        case AST_EXP_BINOP:
            printf("      %s (\n", AST_BINARY_NAMES[expression->binary_op]);
            expression_print(expression->left, depth+1);
            expression_print(expression->right, depth+1);
            PR(depth);
            printf("      )\n");
            break;
    }
}

#pragma clang diagnostic pop