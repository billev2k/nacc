#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 10/27/24.
//

#include <stdio.h>

#include "ast.h"
#include "print_ast.h"

static void c_function_print(struct CFunction *function);
static void c_declaration_print(struct CDeclaration *declaration, int depth);
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
    printf("    body=\n");
    for (int ix=0; ix<function->body.num_items; ix++) {
        struct CBlockItem* bi = function->body.items[ix];
        if (bi->type == AST_BI_STATEMENT) {
            c_statement_print(bi->statement, 0);
        } else {
            c_declaration_print(bi->declaration, 0);
        }
    }
    printf("  )\n");
}
static void c_declaration_print(struct CDeclaration *declaration, int depth) {
    if (!declaration) return;
    printf("    int %s", declaration->name);
    if (declaration->initializer) {
        printf(" = ( ");
        expression_print(declaration->initializer, depth);
        printf(" )");
    }
    printf(";\n");
}
static void c_statement_print(struct CStatement *statement, int depth) {
    if (!statement) return;
    switch (statement->type) {
        case STMT_RETURN:
            printf("    Return( ");
            expression_print(statement->expression, depth);
            printf(" )\n");
            break;
        case STMT_EXP:
            printf("    Expr( ");
            expression_print(statement->expression, depth);
            printf(" )\n");
            break;
        case STMT_NULL:
            break;
    }
}
static void expression_print(struct CExpression *expression, int depth) {
    int has_binop;
    switch (expression->type) {
        case AST_EXP_CONST:
            printf("%s", expression->literal.value);
            break;
        case AST_EXP_UNOP:
            has_binop = expression->unary.operand->type == AST_EXP_BINOP;
            printf("%s", AST_UNARY_NAMES[expression->unary.op]);
            if (has_binop) { printf("*"); }
            expression_print(expression->unary.operand, depth+1);
            if (has_binop) { printf(")"); }
            break;
        case AST_EXP_BINOP:
            printf("(");
            expression_print(expression->binary.left, depth+1);
            printf(" %s ", AST_BINARY_NAMES[expression->binary.op]);
            expression_print(expression->binary.right, depth+1);
            printf(")");
            break;
        case AST_EXP_VAR:
            printf("%s", expression->var.name);
            break;
        case AST_EXP_ASSIGNMENT:
            if (depth > 0) { printf("("); }
            expression_print(expression->assign.dst, depth+1);
            printf(" = ");
            expression_print(expression->assign.src, depth+1);
            if (depth > 0) { printf(")"); }
            break;
        case AST_EXP_INCREMENT:
            if (expression->increment.op == AST_PRE_INCR || expression->increment.op == AST_PRE_DECR) {
                printf("%s", (expression->increment.op == AST_PRE_INCR) ? "++" : "--");
            }
            expression_print(expression->increment.operand, depth+1);
            if (expression->increment.op == AST_POST_INCR || expression->increment.op == AST_POST_DECR) {
                printf("%s", (expression->increment.op == AST_POST_INCR) ? "++" : "--");
            }
            break;
    }
}

#pragma clang diagnostic pop