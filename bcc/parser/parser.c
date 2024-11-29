#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 8/28/24.
//

#include <assert.h>
#include <stdlib.h>
#include "ast.h"
#include "parser.h"
#include "../lexer/tokens.h"
#include "../lexer/lexer.h"
#include "semantics.h"
#include "print_ast.h"

static struct CProgram * parse_program(void);
static struct CFunction * parse_function(void);
static struct CBlockItem* parse_block_item(void);
static struct CDeclaration* parse_declaration(void);
static struct CStatement * parse_statement(void);
static struct CExpression * parse_expression(int minimum_precedence);
static struct CExpression * parse_factor(void);

static int expect(enum TK expected);
static void fail(const char * msg);

struct CProgram * parser_go(void) {
    struct CProgram *program = parse_program();
    return program;
}

void analyze_program(struct CProgram* program) {
    semantic_analysis(program);
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
    struct CFunction *function = c_function_new(name);
    struct Token token = lex_peek_token();
    while (token.token != TK_R_BRACE) {
        struct CBlockItem *item = parse_block_item();
        c_function_append_block_item(function, item);
        token = lex_peek_token();
    }
    expect(TK_R_BRACE);
    // Add a "return 0"; if there's already a return, this unreachable instruction will be removed
    // in a future pass.
    struct CExpression *zero = c_expression_new_const(AST_CONST_INT, "0");
    struct CStatement *ret_stmt = c_statement_new_return(zero);
    struct CBlockItem *ret_item = c_block_item_new_stmt(ret_stmt);
    c_function_append_block_item(function, ret_item);
    return function;
}

struct CBlockItem* parse_block_item() {
    struct Token token = lex_peek_token();
    if (token.token == TK_INT) {
        struct CDeclaration* decl = parse_declaration();
        return c_block_item_new_decl(decl);
    } else {
        struct CStatement* stmt = parse_statement();
        return c_block_item_new_stmt(stmt);
    }
}

struct CDeclaration* parse_declaration() {
    expect(TK_INT);
    struct CDeclaration* result;
    struct Token id = lex_take_token();
    if (id.token != TK_ID) {
        fprintf(stderr, "Expected id: %s\n", id.text);
        exit(1);
    }
    struct Token init = lex_peek_token();
    if (init.token == TK_ASSIGN) {
        lex_take_token();
        struct CExpression* initializer = parse_expression(0);
        result = c_declaration_new_init(id.text, initializer);
    } else {
        result = c_declaration_new(id.text);
    }
    expect(TK_SEMI);
    return result;
}

struct CStatement *parse_statement() {
    struct CStatement* result = NULL;
    struct Token next_token = lex_peek_token();
    if (next_token.token == TK_RETURN) {
        lex_take_token();
        struct CExpression* expression = parse_expression(0);
        result = c_statement_new_return(expression);
    } else if (next_token.token != TK_SEMI) {
        struct CExpression* expression = parse_expression(0);
        result = c_statement_new_exp(expression);
    } else {
        result = c_statement_new_null();
    }
    expect(TK_SEMI);
    return result;
}

static int get_binop_precedence(struct Token token) {
    enum TK tk = token.token;
    if (!TK_IS_BINOP(tk)) return -1;
    enum AST_BINARY_OP binop = TK_IS_ASSIGNMENT(tk) ? AST_BINARY_ASSIGN : TK_GET_BINOP(tk);
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
    while (TK_IS_BINOP(next_token.token) && (op_precedence=get_binop_precedence(next_token)) >= minimum_precedence) {
        enum AST_BINARY_OP binary_op = parse_binop();
        if (TK_IS_ASSIGNMENT(next_token.token)) {
            struct CExpression *right_exp = parse_expression(op_precedence);
            if (next_token.token == TK_ASSIGN) {
                // simple assignment; assign right_exp to lvalue_exp.
                left_exp = c_expression_new_assign(right_exp, left_exp);
            } else {
                // compound assignment; perform binary op on lvalue_exp op right_exp, then assign result to lvalue_exp
                struct CExpression* op_result_exp = c_expression_new_binop(binary_op, c_expression_clone(left_exp), right_exp);
                left_exp = c_expression_new_assign(op_result_exp, left_exp);
            }
        } else {
            struct CExpression *right_exp = parse_expression(op_precedence + 1);
            left_exp = c_expression_new_binop(binary_op, left_exp, right_exp);
        }
        next_token = lex_peek_token();
    }
    return left_exp;
}

static struct CExpression* parse_factor() {
    struct CExpression *result;
    struct Token next_token = lex_take_token();
    if (next_token.token == TK_LITERAL) {
        result = c_expression_new_const(AST_CONST_INT, next_token.text);
    }
    else if (next_token.token == TK_HYPHEN) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_unop(AST_UNARY_NEGATE, operand);
    }
    else if (next_token.token == TK_TILDE) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_unop(AST_UNARY_COMPLEMENT, operand);
    }
    else if (next_token.token == TK_PLUS) {
        // "+x" is simply "x". Skip the '+' and return the factor.
        result = parse_factor();
    }
    else if (next_token.token == TK_L_NOT) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_unop(AST_UNARY_L_NOT, operand);
    }
    else if (next_token.token == TK_L_PAREN) {
        result = parse_expression(0);
        expect(TK_R_PAREN);
    }
    else if (next_token.token == TK_ID) {
        result = c_expression_new_var(next_token.text);
    }
    else if (next_token.token == TK_INCREMENT) {
        int x=0;
        int y = (x)--;
    }
    else {
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