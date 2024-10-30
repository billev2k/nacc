#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 8/28/24.
//

#include <stdlib.h>
#include "ast.h"
#include "parser.h"
#include "../lexer/tokens.h"
#include "../lexer/lexer.h"


static struct CProgram * parse_program();
static struct CFunction * parse_function();
static struct CStatement * parse_statement();
static struct CExpression * parse_expression(int minimum_precedence);
static struct CExpression * parse_factor();

static int expect(enum TK expected);
static void fail(const char * msg);

struct CProgram * parser_go(void) {
    struct CProgram *program = parse_program();
    return program;
}

struct CProgram *parse_program() {
    struct CProgram *result = (struct CProgram *)malloc(sizeof(struct CProgram));
    result->function = parse_function();
    expect(TK_EOF);
    return result;
}

struct CFunction *parse_function() {
    expect(TK_INT);
    expect(TK_ID);
    const char *name = current_token.text;
    expect(TK_L_PAREN);
    expect(TK_VOID);
    expect(TK_R_PAREN);
    expect(TK_L_BRACE);
    struct CFunction *result = (struct CFunction *)malloc(sizeof(struct CFunction));
    result->name = name;
    result->statement = parse_statement();
    expect(TK_R_BRACE);
    return result;
}

struct CStatement *parse_statement() {
    expect(TK_RETURN);
    struct CStatement *result = (struct CStatement *)malloc(sizeof(struct CStatement));
    result->expression = parse_expression(0);
    result->type = STMT_RETURN;
    expect(TK_SEMI);
    return result;
}

static int get_binop_precedence(enum TK tk) {
    if (!TK_IS_BINOP(tk)) return -1;
    enum AST_BINARY_OP binop = TK_GET_BINOP(tk);
    int precedence = AST_BINARY_PRECEDENCE[binop];
    return precedence;
}

static enum AST_BINARY_OP parse_binop() {
    struct Token token = lex_take_token();
    if (!TK_IS_BINOP(token.token)) {
        fprintf(stderr, "Expected binary-op but got %s\n", token.text);
        exit(1);
    }
    enum AST_BINARY_OP binop = TK_GET_BINOP(token.token);
    return binop;
}

struct CExpression *parse_expression(int minimum_precedence) {
    struct CExpression* left_exp = parse_factor();
    struct Token next_token = lex_peek_token();
    int op_precedence;
    while (TK_IS_BINOP(next_token.token) && (op_precedence=get_binop_precedence(next_token.token)) >= minimum_precedence) {
        enum AST_BINARY_OP binary_op = parse_binop();
        struct CExpression* right_exp = parse_expression(op_precedence+1);
        left_exp = c_expression_new_binop(binary_op, left_exp, right_exp);
        next_token = lex_peek_token();
    }
    return left_exp;
}

static struct CExpression* parse_factor() {
    struct CExpression *result;
    struct Token next_token = lex_take_token();
    if (next_token.token == TK_CONSTANT) {
        result = c_expression_new_const(AST_CONST_INT, next_token.text);
    } else if (next_token.token == TK_HYPHEN) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_unop(AST_UNARY_NEGATE, operand);
    } else if (next_token.token == TK_TILDE) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_unop(AST_UNARY_COMPLEMENT, operand);
    } else if (next_token.token == TK_PLUS) {
        // "+x" is simply "x". Skip the '+' and return the factor.
        result = parse_factor();
    } else if (next_token.token == TK_L_PAREN) {
        result = parse_expression(0);
        expect(TK_R_PAREN);
    } else {
        result = NULL;
        fail("Malformed factor");
    }
    return result;
}

static int expect(enum TK expected) {
    struct Token next = lex_take_token();
    if (next.token != expected) {
        fprintf(stderr, "Expected %s but got %s\n", lex_token_name(expected), next.text);
        exit(1);
    }
    return 1;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"
static void fail(const char * msg) {
    fprintf(stderr, "Fail: %s\n", msg);
    exit(1);
}
#pragma clang diagnostic pop

#pragma clang diagnostic pop