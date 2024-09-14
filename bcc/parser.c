//
// Created by Bill Evans on 8/28/24.
//

#include <stdlib.h>
#include "ast.h"
#include "parser.h"
#include "tokens.h"
#include "lexer.h"


static struct CProgram * parse_program();
static struct CFunction * parse_function();
static struct CStatement * parse_statement();
static struct CExpression * parse_expression();

static int expect(enum TK expected);

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
    result->expression = parse_expression();
    result->type = STMT_RETURN;
    expect(TK_SEMI);
    return result;
}

struct CExpression *parse_expression() {
    expect(TK_CONSTANT);
    struct CExpression *result = (struct CExpression *)malloc(sizeof(struct CExpression));
    result->value = current_token.text;
    result->type = EXP_CONSTANT;
    return result;
}

int expect(enum TK expected) {
    struct Token next = lex_take_token();
    if (next.token != expected) {
        fprintf(stderr, "Expected %s but got %s\n", lex_token_name(expected), next.text);
        exit(1);
    }
    return 1;
}