#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 10/27/24.
//

#include <stdio.h>
#include <assert.h>

#include "ast.h"
#include "print_ast.h"

static void print_ast_block(const struct CBlock *block, int depth);
static void print_ast_funcdecl(const struct CFuncDecl *function);
static void print_ast_vardecl(struct CVarDecl *vardecl, int depth);
static void print_ast_statement(struct CStatement *statement, int depth);
static void print_ast_expression(const struct CExpression *expression, int depth);
void c_program_print(const struct CProgram *program) {
    printf("\n\nAST:\nProgram(\n");
    for (int ix=0; ix<program->declarations.num_items; ix++) {
        if (ix > 0) printf("\n");
        struct CDeclaration* decl = program->declarations.items[ix];
        switch (decl->decl_kind) {
            case FUNC_DECL:
                print_ast_funcdecl(decl->func);
                break;
            case VAR_DECL:
                print_ast_vardecl(decl->var, 0);
                break;
        }
    }
}
static void print_ast_funcdecl(const struct CFuncDecl *function) {
    printf("int %s(", function->name);
    if (function->params.num_items == 0) {
        printf("void");
    } else {
        for (int ix = 0; ix < function->params.num_items; ix++) {
            if (ix > 0) printf(", ");
            printf("int %s(%s)", function->params.items[ix].name, function->params.items[ix].source_name);
        }
    }
    if (!function->body) {
        printf(");\n");
    } else {
        printf(") ");
        print_ast_block(function->body, 0);
    }
}

static void indent4(int n) {
    for (int i=0; i<=n; ++i) printf("    ");
}

static void print_ast_vardecl(struct CVarDecl *vardecl, int depth) {
    if (!vardecl) return;
//    indent4(depth);
    printf("int %s", vardecl->var.source_name);
    if (vardecl->initializer) {
        printf(" = ");
        print_ast_expression(vardecl->initializer, depth);
    }
//    printf(";\n");
}

void print_ast_forinit(struct CForInit * init) {
    if (!init) return;
    switch (init->kind) {
        case FOR_INIT_DECL:
            print_ast_vardecl(init->declaration->var, -1);
            break;
        case FOR_INIT_EXPR:
            print_ast_expression(init->expression, 0);
            break;
    }
}

static void print_ast_block(const struct CBlock *block, int depth) {
    indent4(depth-1);
    printf("{\n");
    for (int ix=0; ix<block->items.num_items; ix++) {
        struct CBlockItem* bi = block->items.items[ix];
        switch (bi->kind) {
            case AST_BI_STATEMENT:
                print_ast_statement(bi->statement, depth);
                break;
            case AST_BI_DECLARATION:
                indent4(depth);
                switch (bi->declaration->decl_kind) {
                    case FUNC_DECL:
                        print_ast_funcdecl(bi->declaration->func);
                        break;
                    case VAR_DECL:
                        print_ast_vardecl(bi->declaration->var, depth);
                        printf(";\n");
                        break;
                }
                break;
        }
    }
    indent4(depth-1);
    printf("}\n");
}

static void print_ast_statement(struct CStatement *statement, int depth) {
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
            print_ast_expression(statement->expression, depth);
            printf(" ;\n");
            break;
        case STMT_EXP:
            indent4(depth);
            print_ast_expression(statement->expression, 0);
            printf(" ;\n");
            break;
        case STMT_NULL:
            break;
        case STMT_GOTO:
            indent4(depth);
            printf("goto ");
            print_ast_expression(statement->goto_statement.label, 0);
            printf(";\n");
            break;
        case STMT_IF:
            indent4(depth);
            printf("if (");
            print_ast_expression(statement->if_statement.condition, 0);
            printf(")\n");
            print_ast_statement(statement->if_statement.then_statement, depth + 1);
            if (statement->if_statement.else_statement) {
                indent4(depth);
                printf("else\n");
                print_ast_statement(statement->if_statement.else_statement, depth + 1);
            }
            break;
        case STMT_COMPOUND:
            print_ast_block(statement->compound, depth);
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
            print_ast_statement(statement->while_or_do_statement.body, depth + 1);
            indent4(depth);
            printf("while (");
            print_ast_expression(statement->while_or_do_statement.condition, 0);
            printf(" )");
            break;
        case STMT_FOR:
            indent4(depth);
            printf("for (");
            print_ast_forinit(statement->for_statement.init);
            printf(" ; ");
            print_ast_expression(statement->for_statement.condition, 0);
            printf(" ; ");
            print_ast_expression(statement->for_statement.post, 0);
            printf(" )\n");
            print_ast_statement(statement->for_statement.body, depth + 1);
            break;
        case STMT_SWITCH:
            indent4(depth);
            printf("switch (");
            print_ast_expression(statement->switch_statement.expression, 0);
            printf(")\n");
            print_ast_statement(statement->switch_statement.body, depth + 1);
            break;
        case STMT_WHILE:
            indent4(depth);
            printf("while (");
            print_ast_expression(statement->while_or_do_statement.condition, 0);
            printf(")\n");
            print_ast_statement(statement->while_or_do_statement.body, depth + 1);
            break;
    }
}

static int expression_needs_parens(const struct CExpression *pExpression) {
    assert(pExpression->kind == AST_EXP_BINOP);
    int this_precedence = AST_BINARY_PRECEDENCE[pExpression->kind];
    if (pExpression->binop.left->kind == AST_EXP_BINOP && AST_BINARY_PRECEDENCE[pExpression->binop.left->kind] < this_precedence) {return 1;}
    if (pExpression->binop.right->kind == AST_EXP_BINOP && AST_BINARY_PRECEDENCE[pExpression->binop.right->kind] < this_precedence) {return 1;}
    return 0;
}

static void print_ast_expression(const struct CExpression *expression, int depth) {
    if (!expression) return;
    int needs_parens;
    int has_binop;
    switch (expression->kind) {
        case AST_EXP_CONST:
            printf("%d", expression->literal.int_val);
            break;
        case AST_EXP_UNOP:
            // Is the target of the unary operator a binop operation? If so, parenthesize.
            has_binop = expression->unary.operand->kind == AST_EXP_BINOP;
            printf("%s", AST_UNARY_NAMES[expression->unary.op]);
            if (has_binop) { printf("("); }
            print_ast_expression(expression->unary.operand, depth + 1);
            if (has_binop) { printf(")"); }
            break;
        case AST_EXP_BINOP:
            needs_parens = expression_needs_parens(expression);
            if (needs_parens) printf("(");
            print_ast_expression(expression->binop.left, depth + 1);
            printf(" %s ", AST_BINARY_NAMES[expression->binop.op]);
            print_ast_expression(expression->binop.right, depth + 1);
            if (needs_parens) printf(")");
            break;
        case AST_EXP_VAR:
            printf("%s", expression->var.source_name);
            break;
        case AST_EXP_ASSIGNMENT:
            if (depth > 0) { printf("("); }
            print_ast_expression(expression->assign.dst, depth + 1);
            printf(" = ");
            print_ast_expression(expression->assign.src, depth + 1);
            if (depth > 0) { printf(")"); }
            break;
        case AST_EXP_INCREMENT:
            if (expression->increment.op == AST_PRE_INCR || expression->increment.op == AST_PRE_DECR) {
                printf("%s", (expression->increment.op == AST_PRE_INCR) ? "++" : "--");
            }
            print_ast_expression(expression->increment.operand, depth + 1);
            if (expression->increment.op == AST_POST_INCR || expression->increment.op == AST_POST_DECR) {
                printf("%s", (expression->increment.op == AST_POST_INCR) ? "++" : "--");
            }
            break;
        case AST_EXP_CONDITIONAL:
            printf("(");
            print_ast_expression(expression->conditional.left_exp, depth + 1);
            printf(" ? ");
            print_ast_expression(expression->conditional.middle_exp, depth + 1);
            printf(" : ");
            print_ast_expression(expression->conditional.right_exp, depth + 1);
            printf(")");
            break;
        case AST_EXP_FUNCTION_CALL:
            printf("%s(", expression->function_call.func.source_name);
            for (int ix=0; ix<expression->function_call.args.num_items; ix++) {
                if (ix > 0) printf(", ");
                print_ast_expression(expression->function_call.args.items[ix], depth + 1);
            }
            printf(")");
            break;
    }
}

#pragma clang diagnostic pop