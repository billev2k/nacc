#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 8/28/24.
//

#include <stdlib.h>
#include "ast.h"
#include "parser.h"

#include <stdio.h>

#include "../lexer/tokens.h"
#include "../lexer/lexer.h"
#include "semantics.h"

// List of un-owned strings ("persistent strings" aka "pstr")
LIST_OF_ITEM_DECL(list_of_pstr, const char*)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
void no_op(const char * x) {}
#pragma clang diagnostic pop
struct list_of_pstr_helpers list_of_pstr_helpers = {
        .delete = no_op,
        .null = NULL,
};
LIST_OF_ITEM_DEFN(list_of_pstr,const char*)

static struct CProgram * parse_program(void);
static struct CDeclaration *parse_declaration(void);
static struct CFuncDecl *parse_funcdecl(struct Token idToken, enum STORAGE_CLASS sc, int type);
static struct CBlock* parse_block(int is_function);
static struct CBlockItem* parse_block_item(void);
static struct CVarDecl *parse_vardecl(struct Token idToken, enum STORAGE_CLASS sc, int type);
static struct CStatement * parse_statement(void);
static struct CExpression * parse_expression(int minimum_precedence);
static struct CExpression * parse_factor(void);

static struct Token expect(enum TK expected);

struct list_of_token specifier_list;

static void initialize_parser(void) {
    static int initialized = 0;
    if (!initialized) {
        list_of_token_init(&specifier_list, 3);
        initialized = 1;
    } else {
        list_of_token_clear(&specifier_list);
    }
}


struct CProgram * c_program_parse(void) {
    initialize_parser();
    struct CProgram *program = parse_program();
    return program;
}

void analyze_program(const struct CProgram* program) {
    semantic_analysis(program);
}

struct CProgram *parse_program() {
    struct Token token = lex_peek_token();
    struct CProgram *program = c_program_new();
    while (token.tk != TK_EOF) {
        struct CDeclaration *declaration = parse_declaration();
        c_program_add_decl(program, declaration);
//        struct CFuncDecl *func = parse_funcdecl(NULL, SC_STATIC, 0);
//        c_program_add_func(program, func);
        token = lex_peek_token();
    }
    expect(TK_EOF);
    return program;
}

static int is_specifier(struct Token token) {
    // When we support more types, they go here.
    if (token.tk == TK_INT) return 1;
    if (token.tk == TK_EXTERN || token.tk == TK_STATIC) return 1;
    return 0;
}

static void parse_type_and_storage_class(struct list_of_token *specifiers, enum STORAGE_CLASS *storage_class, int *type_kind) {
    enum STORAGE_CLASS sc = SC_NONE;
    int type = TK_INT;
    int num_sc = 0;
    int num_types = 0;
    for (int i = 0; i < specifiers->num_items; i++) {
        struct Token token = specifiers->items[i];
        if (token.tk == TK_EXTERN) {
            sc |= SC_EXTERN;
            ++num_sc;
        } else if (token.tk == TK_STATIC) {
            sc |= SC_STATIC;
            ++num_sc;
        } else if (token.tk == TK_INT) {
            type = TK_INT;
            ++num_types;
        }
    }
    if (num_sc > 1) {
        failf("Multiple storage classes");
    }
    if (num_types == 0) {
        failf("No type specified");
    } else if (num_types > 1) {
        failf("Multiple types");
    }
    *storage_class = sc;
    *type_kind = type;
}

struct CDeclaration *parse_declaration(void) {
    struct CDeclaration *declaration = NULL;
    // parse base type and storage classes.
    list_of_token_clear(&specifier_list);
    while (is_specifier(lex_peek_token())) {
        struct Token token = lex_take_token();
        list_of_token_append(&specifier_list, token);
    }
    int type;
    enum STORAGE_CLASS storage_class;
    parse_type_and_storage_class(&specifier_list, &storage_class, &type);

    // func or var name (soon, we'll need to parse pointer and array declarations, so this will totally change).
    struct Token idToken = expect(TK_ID);

    // Looks like function?
    if (lex_peek_token().tk == TK_L_PAREN) {
        struct CFuncDecl *func = parse_funcdecl(idToken, storage_class, type);
        declaration = c_declaration_new_func(func);
    } else {
        struct CVarDecl *var = parse_vardecl(idToken, storage_class, type);
        declaration = c_declaration_new_var(var);
    }
    return declaration;
}


struct CFuncDecl *parse_funcdecl(struct Token idToken, enum STORAGE_CLASS sc, int type) {
    expect(TK_L_PAREN);
    struct CFuncDecl* function = c_function_new(idToken.text, sc);

    struct Token token = lex_peek_token();
    if (token.tk == TK_VOID && lex_peek_ahead(2).tk == TK_R_PAREN) {
        lex_take_token(); // void
    } else {
        int num_params = 0;
        while (token.tk != TK_R_PAREN) {
            if (num_params++ > 0) {
                expect(TK_COMMA);
            }
            // parse kind
            expect(TK_INT);
            // parse name [todo: optional if declaration]
            token = lex_take_token();
            c_function_add_param(function, token.text);
            // optional comma, if more params
            token = lex_peek_token();
        }
    }
    expect(TK_R_PAREN);

    token = lex_peek_token();
    if (token.tk == TK_L_BRACE) {
        // function definition
        lex_take_token();
        struct CBlock *block = parse_block(1 /* is_function */);
        c_function_add_body(function, block);
    } else {
        // function declaration
        expect(TK_SEMI);
    }
    return function;
}

struct CVarDecl *parse_vardecl(struct Token idToken, enum STORAGE_CLASS sc, int type) {
    struct CVarDecl* result;
    if (idToken.tk != TK_ID) {
        fprintf(stderr, "Expected id: %s\n", idToken.text);
        exit(1);
    }
    struct Token init = lex_peek_token();
    if (init.tk == TK_ASSIGN) {
        lex_take_token();
        struct CExpression* initializer = parse_expression(0);
        result = c_vardecl_new_init(idToken.text, initializer, sc);
    } else {
        result = c_vardecl_new(idToken.text, sc);
    }
    expect(TK_SEMI);
    return result;
}

/**
 * Parse a block of declarations/definitions and/or statements.
 *
 * Assumes opening brace already consumed; consumes the closing brace.
 *
 * @param is_function non-zero if the block is a function's block.
 * @return a pointer to the new block object.
 */
struct CBlock* parse_block(int is_function) {
    struct CBlock* result = c_block_new(is_function);
    struct Token token = lex_peek_token();
    while (token.tk != TK_R_BRACE) {
        struct CBlockItem *item = parse_block_item();
        c_block_append_item(result, item);
        token = lex_peek_token();
    }
    lex_take_token(); // TK_R_BRACE
    return result;
}

struct CBlockItem* parse_block_item() {
    struct Token token = lex_peek_token();
    if (is_specifier(token)) {
        struct CDeclaration *decl = parse_declaration();
        return c_block_item_new_decl(decl);
    } else {
        // Get the statement. Will include any preceeding labels;
        struct CStatement *stmt = parse_statement();
        return c_block_item_new_stmt(stmt);
    }
}


/**
 * Parses a 'for' loop initialization expression or declaration.
 *
 * This function determines whether the initialization part of a 'for' loop is
 * a declaration or an expression. If the token at the current position is of
 * kind `TK_INT`, it is identified as a declaration, which is parsed
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
    if (is_specifier(next_token)) {
        struct CDeclaration *decl = parse_declaration();
        if (decl->decl_kind == FUNC_DECL) {
            failf("Function declaration in for-init");
        }
        result = c_for_init_new_vardecl(decl);
    }
    else if (next_token.tk != TK_SEMI) {
        struct CExpression* expression = parse_expression(0);
        result = c_for_init_new_expression(expression);
        if (lex_peek_token().tk == TK_SEMI)
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
    if (next_token.tk == tk) {
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
    while ((next_token.tk == TK_ID && lex_peek_ahead(2).tk == TK_COLON ) ||
           next_token.tk == TK_CASE || next_token.tk == TK_DEFAULT) {
        struct CLabel label;
        if (next_token.tk == TK_CASE) {
            // TODO: support constant expressions
            lex_take_token();   // "case"
            struct CExpression* expr = parse_expression(0);
            label = c_label_new_switch_case(expr);
        } else if (next_token.tk == TK_DEFAULT) {
            lex_take_token();   // "default"
            label = c_label_new_switch_default();
        } else {
            lex_take_token();   // identifier name (TK_ID)
            label = c_label_new_label((struct CIdentifier){.name = next_token.text, .source_name = next_token.text});
        }
        expect(TK_COLON);
        if (!have_labels) {
            list_of_CLabel_init(&labels, 3);
            have_labels = 1;
        }
        list_of_CLabel_append(&labels, label);
        next_token = lex_peek_token();
    }

    if (next_token.tk == TK_L_BRACE) {
        lex_take_token();
        struct CBlock* block = parse_block(0 /* is_function */);
        result = c_statement_new_compound(block);
    }
    else if (next_token.tk == TK_IF) {
        struct CStatement* else_statement = NULL;
        lex_take_token();
        expect(TK_L_PAREN);
        struct CExpression* condition = parse_expression(0);
        expect(TK_R_PAREN);
        struct CStatement* then_statement = parse_statement();
        next_token = lex_peek_token();
        if (next_token.tk == TK_ELSE) {
            lex_take_token();
            else_statement = parse_statement();
        }
        result = c_statement_new_if(condition, then_statement, else_statement);
    }
    else if (next_token.tk == TK_WHILE) {
        lex_take_token();
        expect(TK_L_PAREN);
        struct CExpression* condition = parse_expression(0);
        expect(TK_R_PAREN);
        struct CStatement* body = parse_statement();
        result = c_statement_new_while(condition, body);
    }
    else if (next_token.tk == TK_DO) {
        lex_take_token();
        struct CStatement* body = parse_statement();
        expect(TK_WHILE);
        expect(TK_L_PAREN);
        struct CExpression* condition = parse_expression(0);
        expect(TK_R_PAREN);
        expect(TK_SEMI);
        result = c_statement_new_do(body, condition);
    }
    else if (next_token.tk == TK_FOR) {
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
    else if (next_token.tk == TK_SWITCH) {
        lex_take_token();
        expect(TK_L_PAREN);
        struct CExpression* condition = parse_expression(0);
        expect(TK_R_PAREN);
        struct CStatement* body = parse_statement();
        result = c_statement_new_switch(condition, body);
    }
    else if (next_token.tk == TK_BREAK) {
        lex_take_token();
        expect(TK_SEMI);
        result = c_statement_new_break();
    }
    else if (next_token.tk == TK_CONTINUE) {
        lex_take_token();
        expect(TK_SEMI);
        result = c_statement_new_continue();
    }
    else if (next_token.tk == TK_RETURN) {
        lex_take_token();
        struct CExpression* expression = parse_expression(0);
        result = c_statement_new_return(expression);

        expect(TK_SEMI);
    }
    else if (next_token.tk == TK_GOTO) {
        lex_take_token();
        next_token = expect(TK_ID);
        dst = c_expression_new_var(next_token.text);
        expect(TK_SEMI);
        result = c_statement_new_goto(dst);
    }
    else if (next_token.tk != TK_SEMI) {
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
        list_of_CLabel_delete(&labels);
    }

    // Don't do this; any production that expects a ';' should itself code for it.
    // next_token = lex_peek_token();
    // if (next_token.tk == TK_SEMI) { lex_take_token(); }
    return result;
}


static int get_binop_precedence(struct Token token) {
    enum TK tk = token.tk;
    if (!TK_IS_BINOP(tk)) return -1;
    // simple assignment (a = 5) or compound assignment (a += 5)?
    enum AST_BINARY_OP binop = TK_IS_ASSIGNMENT(tk) ? AST_BINARY_ASSIGN : TK_GET_BINOP(tk);
    int precedence = AST_BINARY_PRECEDENCE[binop];
    return precedence;
}

static enum AST_BINARY_OP parse_binop() {
    struct Token token = lex_take_token();
    if (!TK_IS_BINOP(token.tk)) {
        failf("Expected binop-op but got %s", token.text);
    }
    enum AST_BINARY_OP binop = TK_GET_BINOP(token.tk);
    return binop;
}

struct CExpression *parse_conditional_middle() {
    lex_take_token(); // the '?'
    struct CExpression* middle_exp = parse_expression(0);
    expect(TK_COLON);
    return middle_exp;
}

struct CExpression* parse_function_call(const char *name) {
    struct CIdentifier func = { .name = name, .source_name = name};
    struct CExpression* function_call = c_expression_new_function_call(func);
    expect(TK_L_PAREN);
    struct Token token = lex_peek_token();
    int num_args = 0;
    while (token.tk != TK_R_PAREN) {
        if (num_args++ > 0) {
            expect(TK_COMMA);
        }
        // gather args; here TK_COMMA is arg separator, not comma-operator!
        struct CExpression* arg = parse_expression(0);
        c_expression_function_call_add_arg(function_call, arg);
        token = lex_peek_token();
    }
    lex_take_token(); // TK_R_PAREN
    return function_call;
}

struct CExpression *parse_expression(int minimum_precedence) {
    struct CExpression* left_exp = parse_factor();
    struct Token next_token = lex_peek_token();
    int op_precedence;
    while (TK_IS_BINOP(next_token.tk) && (op_precedence=get_binop_precedence(next_token)) >= minimum_precedence) {
        if (TK_IS_ASSIGNMENT(next_token.tk)) {
            enum AST_BINARY_OP binary_op = parse_binop();
            struct CExpression *right_exp = parse_expression(op_precedence);
            if (next_token.tk == TK_ASSIGN) {
                // simple assignment; assign right_exp to lvalue_exp.
                left_exp = c_expression_new_assign(right_exp, left_exp);
            } else {
                // compound assignment; perform binop op on lvalue_exp op right_exp, then assign result to lvalue_exp
                struct CExpression* op_result_exp = c_expression_new_binop(binary_op, c_expression_clone(left_exp), right_exp);
                left_exp = c_expression_new_assign(op_result_exp, left_exp);
            }
        } else if (next_token.tk == TK_QUESTION) {
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
    if (next_token.tk == TK_LITERAL) {
        result = c_expression_new_const(AST_CONST_INT, atoi(next_token.text));
    }
    else if (next_token.tk == TK_HYPHEN) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_unop(AST_UNARY_NEGATE, operand);
    }
    else if (next_token.tk == TK_TILDE) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_unop(AST_UNARY_COMPLEMENT, operand);
    }
    else if (next_token.tk == TK_PLUS) {
        // "+x" is simply "x". Skip the '+' and return the factor.
        result = parse_factor();
    }
    else if (next_token.tk == TK_L_NOT) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_unop(AST_UNARY_L_NOT, operand);
    }
    else if (next_token.tk == TK_L_PAREN) {
        result = parse_expression(0);
        expect(TK_R_PAREN);
    }
    else if (next_token.tk == TK_ID) {
        if (lex_peek_token().tk == TK_L_PAREN) {
            result = parse_function_call(next_token.text);
        } else {
            result = c_expression_new_var(next_token.text);
        }
    }
    else if (next_token.tk == TK_INCREMENT || next_token.tk == TK_DECREMENT) {
        struct CExpression *operand = parse_factor();
        result = c_expression_new_increment((next_token.tk == TK_INCREMENT) ? AST_PRE_INCR : AST_PRE_DECR, operand);
    }
    else {
        result = NULL;
        failf("Malformed factor, token = '%s'", next_token.text);
    }
    next_token = lex_peek_token();
    while (next_token.tk == TK_INCREMENT || next_token.tk == TK_DECREMENT) {
        lex_take_token();
        result = c_expression_new_increment((next_token.tk == TK_INCREMENT) ? AST_POST_INCR : AST_POST_DECR, result);
        next_token = lex_peek_token();
    }
    return result;
}

static struct Token expect(enum TK expected) {
    struct Token token = lex_take_token();
    if (token.tk != expected) {
        failf("Expected %s but got %s", lex_token_name(expected), token.text);
    }
    return token;
}

#pragma clang diagnostic pop