#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 8/28/24.
//

#include <assert.h>
#include <stdlib.h>
#include "ast.h"
#include "parser.h"

#include <stdio.h>

#include "../lexer/tokens.h"
#include "../lexer/lexer.h"
#include "semantics.h"

// List of un-owned strings ("persistent strings" aka "pstr")
#define NAME list_of_pstr
#define TYPE const char*
#include "../utils/list_of_item.h"
#undef NAME
#undef TYPE
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
void no_op(const char * x) {}
#pragma clang diagnostic pop
struct list_of_pstr_helpers list_of_pstr_helpers = {
        .free = no_op,
        .null = NULL,
};
#define NAME list_of_pstr
#define TYPE const char*
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

static struct CProgram * parse_program(void);
static struct CFunction * parse_function(void);
static struct CBlock* parse_block(int is_function);
static struct CBlockItem* parse_block_item(void);
static struct CDeclaration* parse_declaration(void);
static struct CStatement * parse_statement(void);
static struct CExpression * parse_expression(int minimum_precedence);
static struct CExpression * parse_factor(void);

static struct Token expect(enum TK expected);
static void fail(const char * msg);

struct CProgram * c_program_parse(void) {
    struct CProgram *program = parse_program();
//    c_program_print(program);
    return program;
}

void analyze_program(const struct CProgram* program) {
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
    struct CBlock* block = parse_block(1 /* is_function */);
    struct CFunction *function = c_function_new(name, block);
    return function;
}

struct CBlock* parse_block(int is_function) {
    struct CBlock* result = c_block_new(is_function);
    struct Token token = lex_peek_token();
    while (token.token != TK_R_BRACE) {
        struct CBlockItem *item = parse_block_item();
        c_block_append_item(result, item);
        token = lex_peek_token();
    }
    lex_take_token();
    return result;
}

struct CBlockItem* parse_block_item() {
    struct Token token = lex_peek_token();
    if (token.token == TK_INT) {
        struct CDeclaration* decl = parse_declaration();
        return c_block_item_new_decl(decl);
    } else {
        // Get the statement. Will include any preceeding labels;
        struct CStatement *stmt = parse_statement();
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

/**
 * Parses a 'for' loop initialization expression or declaration.
 *
 * This function determines whether the initialization part of a 'for' loop is
 * a declaration or an expression. If the token at the current position is of
 * type `TK_INT`, it is identified as a declaration, which is parsed
 * accordingly. If not, it is treated as an expression and parsed as such.
 * The parsed result is then wrapped into an appropriate `CForInit` structure
 * and returned.
 *
 * @return A pointer to a `CForInit` structure representing the parsed
 *         initialization component of the 'for' loop.
 */
struct CForInit* parse_for_init() {
    struct CForInit* result = NULL;
    struct Token next_token = lex_peek_token();
    if (next_token.token == TK_INT) {
        struct CDeclaration* decl = parse_declaration(); // expects to end with (and consumes) ';'
        result = c_for_init_new_declaration(decl);
    }
    else if (next_token.token != TK_SEMI) {
        struct CExpression* expression = parse_expression(0);
        result = c_for_init_new_expression(expression);
        if (lex_peek_token().token == TK_SEMI)
            lex_take_token();
    } else {
        lex_take_token();
        // and return NULL; no init-clause
    }
    return result;
}

struct CExpression * parse_optional_expression(enum TK tk) {
    struct CExpression* result = NULL;
    struct Token next_token = lex_peek_token();
    if (next_token.token == tk) {
        return result;
    }
    result = parse_expression(0);
    return result;
}

struct CStatement *parse_statement() {
    struct list_of_CLabel labels;
    int have_labels = 0;
    struct CStatement* result = NULL;
    struct CExpression* dst = NULL;
    struct Token next_token = lex_peek_token();

    // Gather any labels.
    while ((next_token.token == TK_ID && lex_peek_ahead(2).token == TK_COLON ) ||
            next_token.token == TK_CASE || next_token.token == TK_DEFAULT) {
        struct CLabel label;
        if (next_token.token == TK_CASE) {
            // TODO: support constant expressions
            lex_take_token();   // case
            next_token = lex_take_token();   // get value
            label = (struct CLabel){.type = LABEL_CASE, .case_value = next_token.text};
        } else if (next_token.token == TK_DEFAULT) {
            lex_take_token();   // default
            label = (struct CLabel){.type = LABEL_DEFAULT};
        } else {
            lex_take_token();   // label name (TK_ID)
            label = (struct CLabel){.type = LABEL_DECL, .label = {.name = next_token.text, .source_name = next_token.text}};
        }
        expect(TK_COLON);
        if (!have_labels) {
            list_of_CLabel_init(&labels, 3);
            have_labels = 1;
        }
        list_of_CLabel_append(&labels, label);
        next_token = lex_peek_token();
    }

    if (next_token.token == TK_L_BRACE) {
        lex_take_token();
        struct CBlock* block = parse_block(0 /* is_function */);
        result = c_statement_new_compound(block);
    }
    else if (next_token.token == TK_IF) {
        struct CStatement* else_statement = NULL;
        lex_take_token();
        expect(TK_L_PAREN);
        struct CExpression* condition = parse_expression(0);
        expect(TK_R_PAREN);
        struct CStatement* then_statement = parse_statement();
        next_token = lex_peek_token();
        if (next_token.token == TK_ELSE) {
            lex_take_token();
            else_statement = parse_statement();
        }
        result = c_statement_new_if(condition, then_statement, else_statement);
    }
    else if (next_token.token == TK_WHILE) {
        lex_take_token();
        expect(TK_L_PAREN);
        struct CExpression* condition = parse_expression(0);
        expect(TK_R_PAREN);
        struct CStatement* body = parse_statement();
        result = c_statement_new_while(condition, body);
    }
    else if (next_token.token == TK_DO) {
        lex_take_token();
        struct CStatement* body = parse_statement();
        expect(TK_WHILE);
        expect(TK_L_PAREN);
        struct CExpression* condition = parse_expression(0);
        expect(TK_R_PAREN);
        expect(TK_SEMI);
        result = c_statement_new_do(body, condition);
    }
    else if (next_token.token == TK_FOR) {
        lex_take_token();
        expect(TK_L_PAREN);
        struct CForInit* for_init = parse_for_init();
        struct CExpression* condition = parse_optional_expression(TK_SEMI);
        expect(TK_SEMI);
        struct CExpression* post = parse_optional_expression(TK_R_PAREN);
        expect(TK_R_PAREN);
        struct CStatement* body = parse_statement();
        result = c_statement_new_for(for_init, condition, post, body);
    }
    else if (next_token.token == TK_SWITCH) {
        lex_take_token();
        expect(TK_L_PAREN);
        struct CExpression* condition = parse_expression(0);
        expect(TK_R_PAREN);
        struct CStatement* body = parse_statement();
        result = c_statement_new_switch(condition, body);
    }
    else if (next_token.token == TK_BREAK) {
        lex_take_token();
        expect(TK_SEMI);
        result = c_statement_new_break();
    }
    else if (next_token.token == TK_CONTINUE) {
        lex_take_token();
        expect(TK_SEMI);
        result = c_statement_new_continue();
    }
    else if (next_token.token == TK_RETURN) {
        lex_take_token();
        struct CExpression* expression = parse_expression(0);
        result = c_statement_new_return(expression);

        expect(TK_SEMI);
    }
    else if (next_token.token == TK_GOTO) {
        lex_take_token();
        next_token = expect(TK_ID);
        dst = c_expression_new_var(next_token.text);
        expect(TK_SEMI);
        result = c_statement_new_goto(dst);
    }
    else if (next_token.token != TK_SEMI) {
        struct CExpression* expression = parse_expression(0);
        result = c_statement_new_exp(expression);
        expect(TK_SEMI);
    }
    else {
        result = c_statement_new_null();
        lex_take_token();
    }

    // Apply labels from above.
    if (have_labels) {
        c_statement_add_labels(result, labels);
        list_of_CLabel_free(&labels);
    }

    // Don't do this; any production that expects a ';' should itself code for it.
    // next_token = lex_peek_token();
    // if (next_token.token == TK_SEMI) { lex_take_token(); }
    return result;
}


static int get_binop_precedence(struct Token token) {
    enum TK tk = token.token;
    if (!TK_IS_BINOP(tk)) return -1;
    // simple assignment (a = 5) or compound assignment (a += 5)?
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

struct CExpression *parse_conditional_middle() {
    lex_take_token(); // the '?'
    struct CExpression* middle_exp = parse_expression(0);
    expect(TK_COLON);
    return middle_exp;
}

struct CExpression *parse_expression(int minimum_precedence) {
    struct CExpression* left_exp = parse_factor();
    struct Token next_token = lex_peek_token();
    int op_precedence;
    while (TK_IS_BINOP(next_token.token) && (op_precedence=get_binop_precedence(next_token)) >= minimum_precedence) {
        if (TK_IS_ASSIGNMENT(next_token.token)) {
            enum AST_BINARY_OP binary_op = parse_binop();
            struct CExpression *right_exp = parse_expression(op_precedence);
            if (next_token.token == TK_ASSIGN) {
                // simple assignment; assign right_exp to lvalue_exp.
                left_exp = c_expression_new_assign(right_exp, left_exp);
            } else {
                // compound assignment; perform binary op on lvalue_exp op right_exp, then assign result to lvalue_exp
                struct CExpression* op_result_exp = c_expression_new_binop(binary_op, c_expression_clone(left_exp), right_exp);
                left_exp = c_expression_new_assign(op_result_exp, left_exp);
            }
        } else if (next_token.token == TK_QUESTION) {
            struct CExpression* middle_exp = parse_conditional_middle();
            struct CExpression* right_exp = parse_expression(op_precedence);
            left_exp = c_expression_new_conditional(left_exp, middle_exp, right_exp);
        }
        else {
            enum AST_BINARY_OP binary_op = parse_binop();
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
        result = c_expression_new_const(AST_CONST_INT, atoi(next_token.text));
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
    else if (next_token.token == TK_INCREMENT || next_token.token == TK_DECREMENT) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_increment((next_token.token == TK_INCREMENT)?AST_PRE_INCR:AST_PRE_DECR, operand);
    }
    else {
        result = NULL;
        fail("Malformed factor");
    }
    next_token = lex_peek_token();
    while (next_token.token == TK_INCREMENT || next_token.token == TK_DECREMENT) {
        lex_take_token();
        result = c_expression_new_increment((next_token.token == TK_INCREMENT)?AST_POST_INCR:AST_POST_DECR, result);
        next_token = lex_peek_token();
    }
    return result;
}

static struct Token expect(enum TK expected) {
    struct Token next = lex_take_token();
    if (next.token != expected) {
        fprintf(stderr, "Expected %s but got %s\n", lex_token_name(expected), next.text);
        exit(1);
    }
    return next;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"
static void fail(const char * msg) {
    fprintf(stderr, "Fail: %s\n", msg);
    exit(1);
}
#pragma clang diagnostic pop

#pragma clang diagnostic pop