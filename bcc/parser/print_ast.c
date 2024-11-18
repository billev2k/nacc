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
    for (int ix=0; ix<function->body.list_count; ix++) {
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
#define PR(depth) do {for (int i=0; i<depth; ++i) printf("   "); } while (0)
static void expression_print(struct CExpression *expression, int depth) {
//    PR(depth);
    switch (expression->type) {
        case AST_EXP_CONST:
            printf("%s", expression->literal.value);
//            printf("      %s\n", expression->literal.value);
            break;
        case AST_EXP_UNOP:
            printf("%s(", AST_UNARY_NAMES[expression->unary.op]);
//            printf("      %s (\n", AST_UNARY_NAMES[expression->unary.op]);
            expression_print(expression->unary.operand, depth+1);
//            PR(depth);
//            printf("      )\n");
            printf(")");
            break;
        case AST_EXP_BINOP:
            printf("(");
//            printf("      %s (\n", AST_BINARY_NAMES[expression->binary.op]);
            expression_print(expression->binary.left, depth+1);
            printf(") %s (", AST_BINARY_NAMES[expression->binary.op]);
            expression_print(expression->binary.right, depth+1);
//            PR(depth);
            printf(")");
//            printf("      )\n");
            break;
        case AST_EXP_VAR:
            printf("%s", expression->var.name);
//            printf("      %s\n", expression->var.name);
            break;
        case AST_EXP_ASSIGNMENT:
//            printf("      ASSIGN (\n");
            expression_print(expression->assign.dst, depth+1);
            printf(" = (");
            expression_print(expression->assign.src, depth+1);
//            PR(depth);
            printf(")");
//            printf("      )\n");
            break;
    }
}

#pragma clang diagnostic pop