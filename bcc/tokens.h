//
// Created by Bill Evans on 8/27/24.
//

#ifndef BCC_TOKENS_H
#define BCC_TOKENS_H

#define _KEYWORDS \
    X(TK_INT, "int"),       \
    X(TK_VOID, "void"),     \
    X(TK_RETURN, "return"),

enum TK {
    TK_UNKNOWN,

#define X(a,b) a
    _KEYWORDS
#undef X

//    TK_INT,
//    TK_VOID,
//    TK_RETURN,

    TK_SEMI,
    TK_L_PAREN,
    TK_R_PAREN,
    TK_L_BRACE,
    TK_R_BRACE,

    TK_ID,
    TK_CONSTANT,

    TK_EOF,

    NUM_TOKEN_TYPES
};

#endif //BCC_TOKENS_H
