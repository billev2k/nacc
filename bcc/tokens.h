//
// Created by Bill Evans on 8/27/24.
//

#ifndef BCC_TOKENS_H
#define BCC_TOKENS_H

#define TOKENS__ \
    X(TK_UNKNOWN, "!! unknown !!"),            \
    X(TK_INT, "int"),                          \
    X(TK_VOID, "void"),                        \
    X(TK_RETURN, "return"),                    \
    X(TK_SEMI, ";"),                           \
    X(TK_L_PAREN, "("),                        \
    X(TK_R_PAREN, ")"),                        \
    X(TK_L_BRACE, "{"),                        \
    X(TK_R_BRACE, "}"),                        \
    X(TK_COMPLEMENT, "~"),                     \
    X(TK_MINUS, "-"),                          \
    X(TK_DECREMENT, "--"),                     \
    X(TK_ID, "an identifier"),                 \
    X(TK_CONSTANT, "a constant"),              \
    X(TK_EOF, "eof"),

 enum TK {
#define X(a,b) a
    TOKENS__
#undef X
    NUM_TOKEN_TYPES,
    TK_KEYWORDS_BEGIN=TK_INT,
    TK_KEYWORDS_END=TK_RETURN+1,
};


extern const char *token_names[NUM_TOKEN_TYPES];

#endif //BCC_TOKENS_H
