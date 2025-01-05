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
    for (int ix=0; ix<function->block->items.num_items; ix++) {
        struct CBlockItem* bi = function->block->items.items[ix];
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
    printf("    int %s", declaration->var.name);
    if (declaration->initializer) {
        printf(" = ( ");
        expression_print(declaration->initializer, depth);
        printf(" )");
    }
    printf(";\n");
}
static void indent4(int n) {
    for (int i=0; i<n; ++i) printf("    ");
}
static void c_statement_print(struct CStatement *statement, int depth) {
    if (!statement) return;
    indent4(depth);
    if (c_statement_has_labels(statement)) {
        struct CVariable * labels = c_statement_get_labels(statement);
        while (labels->source_name) {
            printf("%s:\n", labels->source_name);
            indent4(depth);
            ++labels;
        }
    }
    switch (statement->type) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            printf("  Return( ");
            expression_print(statement->expression, depth);
            printf(" )\n");
            break;
        case STMT_EXP:
            printf("  Expr( ");
            expression_print(statement->expression, depth);
            printf(" )\n");
            break;
        case STMT_NULL:
            break;
        case STMT_GOTO:
            printf("  Goto ");
            expression_print(statement->goto_statement.label, depth);
            printf(";\n");
            break;
        case STMT_IF:
            printf("  If (");
            expression_print(statement->if_statement.condition, depth);
            printf(")\n");
            c_statement_print(statement->if_statement.then_statement, depth+1);
            if (statement->if_statement.else_statement) {
                indent4(depth);
                printf("  else\n");
                c_statement_print(statement->if_statement.else_statement, depth+1);
            }
            break;
        case STMT_LABEL:
            expression_print(statement->label_statement.label, depth);
            printf(":\n");
            break;
        case STMT_COMPOUND:
            printf("  {\n");
            for (int ix=0; ix<statement->compound->items.num_items; ix++) {
                struct CBlockItem* bi = statement->compound->items.items[ix];
                if (bi->type == AST_BI_STATEMENT) {
                    c_statement_print(bi->statement, depth+1);
                } else {
                    c_declaration_print(bi->declaration, depth+1);
                }
            }
            printf("  }\n");
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
        case AST_EXP_CONDITIONAL:
            printf("(");
            expression_print(expression->conditional.left_exp, depth+1);
            printf(" ? ");
            expression_print(expression->conditional.middle_exp, depth+1);
            printf(" : ");
            expression_print(expression->conditional.right_exp, depth+1);
            printf(")");
            break;
    }
}

#pragma clang diagnostic pop