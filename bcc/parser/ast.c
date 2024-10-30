#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 8/28/24.
//

#include <stdlib.h>
#include <printf.h>

#include "ast.h"
#include "../lexer/tokens.h"
#include "../utils/startup.h"

const char * const ASM_CONST_TYPE_NAMES[] = {
#define X(a,b) b 
    AST_CONST_LIST__
#undef X
};

const char * const AST_BINARY_NAMES[] = {
#define X(a,b,c) b
    AST_BINARY_LIST__
#undef X
};
const int AST_BINARY_PRECEDENCE[] = {
#define X(a,b,c) c
    AST_BINARY_LIST__
#undef X
};
const int AST_TO_IR_BINARY[] = {
#define X(a,b,c) IR_BINARY_##a 
    AST_BINARY_LIST__
#undef X
};


const char * const AST_UNARY_NAMES[] = {
#define X(a,b) b
        AST_UNARY_LIST__
#undef X
};
enum IR_UNARY_OP const AST_TO_IR_UNARY[] = {
#define X(a,b) IR_UNARY_##a
        AST_UNARY_LIST__
#undef X
};


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

static struct CExpression* c_expression_new(enum AST_EXP_TYPE type) {
    struct CExpression* expression = (struct CExpression*)malloc(sizeof(struct CExpression));
    expression->type = type;
    return expression;
}
struct CExpression* c_expression_new_const(enum AST_CONST_TYPE const_type, const char *value) {
    struct CExpression* expression = c_expression_new(AST_EXP_CONST);
    expression->const_type = const_type;
    expression->value = value;
    return expression;
}
struct CExpression* c_expression_new_unop(enum AST_UNARY_OP op, struct CExpression* operand) {
    struct CExpression* expression = c_expression_new(AST_EXP_UNOP);
    expression->unary_op = op;
    expression->operand = operand;
    return expression;
}
struct CExpression* c_expression_new_binop(enum AST_BINARY_OP op, struct CExpression* left, struct CExpression* right) {
    struct CExpression* expression = c_expression_new(AST_EXP_BINOP);
    expression->binary_op = op;
    expression->left = left;
    expression->right = right;
    return expression;
}
void c_expression_free(struct CExpression *expression) {
    if (!expression) return;
    if (traceAstMem) printf("Free expression @ %p\n", expression);
    switch (expression->type) {
        case AST_EXP_BINOP:
            if (expression->left) c_expression_free(expression->left);
            if (expression->right) c_expression_free(expression->right);
            break;
        case AST_EXP_CONST:
            // Nothing to do; strings owned by global string table.
            break;
        case AST_EXP_UNOP:
            if (expression->operand) c_expression_free(expression->operand);
            break;
    }
    free(expression);
}

#pragma clang diagnostic pop