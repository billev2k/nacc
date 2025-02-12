//
// Created by Bill Evans on 8/20/24.
//

#ifndef BCC_LEXER_H
#define BCC_LEXER_H

#include "tokens.h"

struct Token {
    enum TK tk;
    const char *text;
};

#define TYPE struct Token
#define NAME list_of_token
#include "../utils/list_of_item.h"

extern int lex_openFile(char const *fname);

extern struct Token lex_peek_ahead(int n);
extern struct Token lex_peek_token(void);
extern struct Token lex_take_token(void);

extern const char *lex_token_name(enum TK token);
extern struct Token current_token;

#endif //BCC_LEXER_H
