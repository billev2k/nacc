//
// Created by Bill Evans on 8/27/24.
//

#ifndef BCC_TOKENS_H
#define BCC_TOKENS_H

// Token attributes:
//  Flags for: (can be) unop, binop, keyword, identifier, literal
//  Fields for corresponding unop, binop, type of literal
enum TOKEN_FLAGS  {
    TF_UNOP         = 0x00000001,
    TF_BINOP        = 0x00000002,
    TF_UNOP_MASK    = 0x0000ff00,
    TF_UNOP_SHIFT   = 8,
    TF_BINOP_MASK   = 0x00ff0000,
    TF_BINOP_SHIFT  = 16,
};
extern enum TOKEN_FLAGS TOKEN_FLAGS[];

#define TK_UNOP(unop) (TF_UNOP | AST_UNARY_##unop<<TF_UNOP_SHIFT)
#define TK_IS_UNOP(tk) (TOKEN_FLAGS[tk] & TF_UNOP)
#define TK_GET_UNOP(tk) ((enum AST_UNARY_OP)(TK_IS_UNOP(tk) ? ((TOKEN_FLAGS[tk] & TF_UNOP_MASK)>>TF_UNOP_SHIFT) : -1))

#define TK_BINOP(binop) (TF_BINOP | AST_BINARY_##binop<<TF_BINOP_SHIFT)
#define TK_IS_BINOP(tk) (TOKEN_FLAGS[tk] & TF_BINOP)
#define TK_GET_BINOP(tk) ((enum AST_BINARY_OP)(TK_IS_BINOP(tk) ? ((TOKEN_FLAGS[tk] & TF_BINOP_MASK)>>TF_BINOP_SHIFT) : -1))

// token ID, print value, corresponding AST UNOP, BINOP
#define TOKENS__ \
    X(TK_UNKNOWN, "!! unknown !!",  0),                                      \
    X(TK_INT, "int",                0),                                      \
    X(TK_VOID, "void",              0),                                      \
    X(TK_RETURN, "return",          0),                                      \
    X(TK_SEMI, ";",                 0),                                      \
    X(TK_L_PAREN, "(",              0),                                      \
    X(TK_R_PAREN, ")",              0),                                      \
    X(TK_L_BRACE, "{",              0),                                      \
    X(TK_R_BRACE, "}",              0),                                      \
    X(TK_PLUS, "+",                 TK_BINOP(ADD)),                          \
    X(TK_HYPHEN, "-",               TK_UNOP(NEGATE) | TK_BINOP(SUBTRACT)),   \
    X(TK_ASTERISK, "*",             TK_BINOP(MULTIPLY)),                     \
    X(TK_SLASH, "/",                TK_BINOP(DIVIDE)),                       \
    X(TK_PERCENT, "%",              TK_BINOP(REMAINDER)),                    \
    X(TK_AMPERSAND, "&",            TK_BINOP(AND)),                          \
    X(TK_CARET, "^",                TK_BINOP(XOR)),                          \
    X(TK_BAR, "|",                  TK_BINOP(OR)),                           \
    X(TK_L_ANGLE, "<",              0),                                      \
    X(TK_R_ANGLE, ">",              0),                                      \
    X(TK_LSHIFT, "<<",              TK_BINOP(LSHIFT)),                       \
    X(TK_RSHIFT, ">>",              TK_BINOP(RSHIFT)),                       \
    X(TK_INCREMENT, "++",           0),                                      \
    X(TK_DECREMENT, "--",           0),                                      \
    X(TK_TILDE, "~",                TK_UNOP(COMPLEMENT)),                    \
    X(TK_ID, "an identifier",       0),                                      \
    X(TK_LITERAL, "a literal",      0),                                      \
    X(TK_EOF, "eof",                0),

 enum TK {
#define X(a,b,c) a
    TOKENS__
#undef X
    NUM_TOKEN_TYPES,
    TK_KEYWORDS_BEGIN=TK_INT,
    TK_KEYWORDS_END=TK_RETURN+1,
};

extern const char *token_names[NUM_TOKEN_TYPES];

#endif //BCC_TOKENS_H
