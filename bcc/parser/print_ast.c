#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 10/27/24.
//

#include <stdio.h>
#include <assert.h>

#include "ast.h"
#include "print_ast.h"

static void print_block(const struct CBlock *block, int depth);
static void c_function_print(const struct CFunction *function);
static void c_vardecl_print(struct CVarDecl *vardecl, int depth);
static void c_statement_print(struct CStatement *statement, int depth);
static void expression_print(const struct CExpression *expression, int depth);
void c_program_print(const struct CProgram *program) {
    printf("\n\nAST:\nProgram(\n");
    for (int ix=0; ix<program->functions.num_items; ix++) {
        if (ix > 0) printf("\n");
        c_function_print(program->functions.items[ix]);
    }
}
static void c_function_print(const struct CFunction *function) {
    printf("int %s(", function->name);
    if (function->params.num_items == 0) {
        printf("void");
    } else {
        for (int ix = 0; ix < function->params.num_items; ix++) {
            if (ix > 0) printf(", ");
            printf("int %s", function->params.items[ix].source_name);
        }
    }
    if (!function->body) {
        printf(");\n");
    } else {
        print_block(function->body, 0);
    }
}

static void indent4(int n) {
    for (int i=0; i<=n; ++i) printf("    ");
}

static void c_vardecl_print(struct CVarDecl *vardecl, int depth) {
    if (!vardecl) return;
    indent4(depth);
    printf("int %s", vardecl->var.source_name);
    if (vardecl->initializer) {
        printf(" = ");
        expression_print(vardecl->initializer, depth);
    }
    printf(";\n");
}

void for_init_print(struct CForInit * init) {
    if (!init) return;
    switch (init->kind) {
        case FOR_INIT_DECL:
            c_vardecl_print(init->vardecl, -1);
            break;
        case FOR_INIT_EXPR:
            expression_print(init->expression, 0);
            break;
    }
}

static void print_block(const struct CBlock *block, int depth) {
    indent4(depth-1);
    printf("{\n");
    for (int ix=0; ix<block->items.num_items; ix++) {
        struct CBlockItem* bi = block->items.items[ix];
        switch (bi->kind) {
            case AST_BI_STATEMENT:
                c_statement_print(bi->statement, depth);
                break;
            case AST_BI_VAR_DECL:
                c_vardecl_print(bi->vardecl, depth);
                break;
            case AST_BI_FUNC_DECL:
                c_function_print(bi->funcdecl);
                break;
        }
    }
    indent4(depth-1);
    printf("}\n");
}

static void c_statement_print(struct CStatement *statement, int depth) {
    if (!statement) return;
    if (c_statement_has_labels(statement)) {
        struct CLabel * labels = c_statement_get_labels(statement);
        for (int i=0; i<c_statement_num_labels(statement); ++i) {
            indent4(depth-1);
            if (labels[i].kind == LABEL_DECL)
                printf("%s:\n", labels[i].label.source_name);
            else if (labels[i].kind == LABEL_DEFAULT)
                printf("default:\n");
            else if (labels[i].kind == LABEL_CASE)
                printf("case %s:\n", labels[i].case_value);
        }
    }
    switch (statement->kind) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            indent4(depth);
            printf("return%s", statement->expression ? " " : "");
            expression_print(statement->expression, depth);
            printf(" ;\n");
            break;
        case STMT_EXP:
            indent4(depth);
            expression_print(statement->expression, 0);
            printf(" ;\n");
            break;
        case STMT_NULL:
            break;
        case STMT_GOTO:
            indent4(depth);
            printf("goto ");
            expression_print(statement->goto_statement.label, 0);
            printf(";\n");
            break;
        case STMT_IF:
            indent4(depth);
            printf("if (");
            expression_print(statement->if_statement.condition, 0);
            printf(")\n");
            c_statement_print(statement->if_statement.then_statement, depth+1);
            if (statement->if_statement.else_statement) {
                indent4(depth);
                printf("else\n");
                c_statement_print(statement->if_statement.else_statement, depth+1);
            }
            break;
        case STMT_COMPOUND:
            print_block(statement->compound, depth);
            break;
        case STMT_BREAK:
            indent4(depth);
            printf("break;\n");
            break;
        case STMT_CONTINUE:
            indent4(depth);
            printf("continue;\n");
            break;
        case STMT_DOWHILE:
            indent4(depth);
            printf("do\n");
            c_statement_print(statement->while_or_do_statement.body, depth+1);
            indent4(depth);
            printf("while (");
            expression_print(statement->while_or_do_statement.condition, 0);
            printf(" )");
            break;
        case STMT_FOR:
            indent4(depth);
            printf("for (");
            for_init_print(statement->for_statement.init);
            printf(" ; ");
            expression_print(statement->for_statement.condition, 0);
            printf(" ; ");
            expression_print(statement->for_statement.post, 0);
            printf(" )\n");
            c_statement_print(statement->for_statement.body, depth+1);
            break;
        case STMT_SWITCH:
            indent4(depth);
            printf("switch (");
            expression_print(statement->switch_statement.expression, 0);
            printf(")\n");
            c_statement_print(statement->switch_statement.body, depth+1);
            break;
        case STMT_WHILE:
            indent4(depth);
            printf("while (");
            expression_print(statement->while_or_do_statement.condition, 0);
            printf(")\n");
            c_statement_print(statement->while_or_do_statement.body, depth+1);
            break;
    }
}

static int expression_needs_parens(const struct CExpression *pExpression) {
    assert(pExpression->kind == AST_EXP_BINOP);
    int this_precedence = AST_BINARY_PRECEDENCE[pExpression->kind];
    if (pExpression->binary.left->kind == AST_EXP_BINOP && AST_BINARY_PRECEDENCE[pExpression->binary.left->kind] < this_precedence) {return 1;}
    if (pExpression->binary.right->kind == AST_EXP_BINOP && AST_BINARY_PRECEDENCE[pExpression->binary.right->kind] < this_precedence) {return 1;}
    return 0;
}

static void expression_print(const struct CExpression *expression, int depth) {
    if (!expression) return;
    int needs_parens;
    int has_binop;
    switch (expression->kind) {
        case AST_EXP_CONST:
            printf("%d", expression->literal.int_val);
            break;
        case AST_EXP_UNOP:
            // Is the target of the unary operator a binary operation? If so, parenthesize.
            has_binop = expression->unary.operand->kind == AST_EXP_BINOP;
            printf("%s", AST_UNARY_NAMES[expression->unary.op]);
            if (has_binop) { printf("("); }
            expression_print(expression->unary.operand, depth+1);
            if (has_binop) { printf(")"); }
            break;
        case AST_EXP_BINOP:
            needs_parens = expression_needs_parens(expression);
            if (needs_parens) printf("(");
            expression_print(expression->binary.left, depth+1);
            printf(" %s ", AST_BINARY_NAMES[expression->binary.op]);
            expression_print(expression->binary.right, depth+1);
            if (needs_parens) printf(")");
            break;
        case AST_EXP_VAR:
            printf("%s", expression->var.source_name);
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
        case AST_EXP_FUNCTION_CALL:
            printf("%s(", expression->function_call.func.source_name);
            for (int ix=0; ix<expression->function_call.args.num_items; ix++) {
                if (ix > 0) printf(", ");
                expression_print(expression->function_call.args.items[ix], depth+1);
            }
            printf(")");
            break;
    }
}

#pragma clang diagnostic pop