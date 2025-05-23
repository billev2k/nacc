//
// Created by Bill Evans on 8/27/24.
//

#ifndef BCC_TOKENS_H
#define BCC_TOKENS_H

// Token attributes:
//  Flags for: (can be) unop, binop, keyword, identifier, literal
//  Fields for corresponding unop, binop, kind of literal
enum TOKEN_FLAGS  {
    TF_UNOP         = 0x00000001,
    TF_BINOP        = 0x00000002,
    TF_ASSIGNMENT   = 0x00000004,   // If set, is an assignment token, "=", "+=", etc
    TF_UNOP_MASK    = 0x0000ff00,   // Mask for unop operation.
    TF_UNOP_SHIFT   = 8,            // How much to shift unop operation to move it to LSB position.
    TF_BINOP_MASK   = 0x00ff0000,   // Mask for the binop flags
    TF_BINOP_SHIFT  = 16,           // // How much to shift binop operation to move it to LSB position.
};
extern enum TOKEN_FLAGS TOKEN_FLAGS[];

#define TK_UNOP(unop) (TF_UNOP | AST_UNARY_##unop<<TF_UNOP_SHIFT)
#define TK_IS_UNOP(tk) (TOKEN_FLAGS[tk] & TF_UNOP)
#define TK_GET_UNOP(tk) ((enum AST_UNARY_OP)(TK_IS_UNOP(tk) ? ((TOKEN_FLAGS[tk] & TF_UNOP_MASK)>>TF_UNOP_SHIFT) : -1))

#define TK_BINOP(binop) (TF_BINOP | AST_BINARY_##binop<<TF_BINOP_SHIFT)
#define TK_IS_BINOP(tk) (TOKEN_FLAGS[tk] & TF_BINOP)
#define TK_GET_BINOP(tk) ((enum AST_BINARY_OP)(TK_IS_BINOP(tk) ? ((TOKEN_FLAGS[tk] & TF_BINOP_MASK)>>TF_BINOP_SHIFT) : -1))
#define TK_IS_ASSIGNMENT(tk) (TOKEN_FLAGS[tk] & TF_ASSIGNMENT)

// token ID, print value, corresponding AST UNOP or BINOP (or both, eg, '-' is negate unop and subtract binop)
// Token id, text representation, token flags correlating with unary and binary operators.
#define TOKENS__ \
    X(TK_UNKNOWN,       "!! unknown !!",    0),                                      \
    X(TK_ELSE,          "else",             0),                                      \
    X(TK_GOTO,          "goto",             0),                                      \
    X(TK_IF,            "if",               0),                                      \
    X(TK_CONST,         "const",            0),                                      \
    X(TK_INT,           "int",              0),                                      \
    X(TK_LONG,          "long",             0),                                      \
    X(TK_SHORT,         "short",            0),                                      \
    X(TK_CHAR,          "char",             0),                                      \
    X(TK_SIGNED,        "signed",           0),                                      \
    X(TK_UNSIGNED,      "unsigned",         0),                                      \
    X(TK_FLOAT,         "float",            0),                                      \
    X(TK_DOUBLE,        "double",           0),                                      \
    X(TK_RETURN,        "return",           0),                                      \
    X(TK_VOID,          "void",             0),                                      \
    X(TK_STATIC,        "static",           0),                                      \
    X(TK_EXTERN,        "extern",           0),                                      \
    X(TK_WHILE,         "while",            0),                                      \
    X(TK_DO,            "do",               0),                                      \
    X(TK_FOR,           "for",              0),                                      \
    X(TK_BREAK,         "break",            0),                                      \
    X(TK_CONTINUE,      "continue",         0),                                      \
    X(TK_SWITCH,        "switch",           0),                                      \
    X(TK_CASE,          "case",             0),                                      \
    X(TK_DEFAULT,       "default",          0),                                      \
    X(TK_SEMI,          ";",                0),                                      \
    X(TK_L_PAREN,       "(",                0),                                      \
    X(TK_R_PAREN,       ")",                0),                                      \
    X(TK_L_BRACE,       "{",                0),                                      \
    X(TK_R_BRACE,       "}",                0),                                      \
    X(TK_COMMA,         ",",                0),                                      \
    X(TK_COLON,         ":",                0),                                      \
    X(TK_QUESTION,      "?",                TK_BINOP(QUESTION)),                     \
    X(TK_PLUS,          "+",                TK_BINOP(ADD)),                          \
    X(TK_HYPHEN,        "-",                TK_UNOP(NEGATE) | TK_BINOP(SUBTRACT)),   \
    X(TK_ASTERISK,      "*",                TK_BINOP(MULTIPLY)),                     \
    X(TK_SLASH,         "/",                TK_BINOP(DIVIDE)),                       \
    X(TK_PERCENT,       "%",                TK_BINOP(REMAINDER)),                    \
    X(TK_AMPERSAND,     "&",                TK_BINOP(AND)),                          \
    X(TK_CARET,         "^",                TK_BINOP(XOR)),                          \
    X(TK_OR,            "|",                TK_BINOP(OR)),                           \
    X(TK_L_AND,         "&&",               TK_BINOP(L_AND)),                        \
    X(TK_L_OR,          "||",               TK_BINOP(L_OR)),                         \
    X(TK_EQ,            "==",               TK_BINOP(EQ)),                           \
    X(TK_NE,            "!=",               TK_BINOP(NE)),                           \
    X(TK_LT,            "<",                TK_BINOP(LT)),                           \
    X(TK_GT,            ">",                TK_BINOP(GT)),                           \
    X(TK_LE,            "<=",               TK_BINOP(LE)),                           \
    X(TK_GE,            ">=",               TK_BINOP(GE)),                           \
    X(TK_LSHIFT,        "<<",               TK_BINOP(LSHIFT)),                       \
    X(TK_RSHIFT,        ">>",               TK_BINOP(RSHIFT)),                       \
    X(TK_INCREMENT,     "++",               0),                                      \
    X(TK_DECREMENT,     "--",               0),                                      \
    X(TK_L_NOT,         "!",                TK_UNOP(L_NOT)),                         \
    X(TK_TILDE,         "~",                TK_UNOP(COMPLEMENT)),                    \
    X(TK_ASSIGN,        "=",                TF_ASSIGNMENT|TK_BINOP(ASSIGN)),         \
    X(TK_ASSIGN_PLUS,   "+=",               TF_ASSIGNMENT|TK_BINOP(ADD)),            \
    X(TK_ASSIGN_MINUS,  "-=",               TF_ASSIGNMENT|TK_BINOP(SUBTRACT)),       \
    X(TK_ASSIGN_MULT,   "*=",               TF_ASSIGNMENT|TK_BINOP(MULTIPLY)),       \
    X(TK_ASSIGN_DIV,    "/=",               TF_ASSIGNMENT|TK_BINOP(DIVIDE)),         \
    X(TK_ASSIGN_MOD,    "%=",               TF_ASSIGNMENT|TK_BINOP(REMAINDER)),      \
    X(TK_ASSIGN_AND,    "&=",               TF_ASSIGNMENT|TK_BINOP(AND)),            \
    X(TK_ASSIGN_OR,     "|=",               TF_ASSIGNMENT|TK_BINOP(OR)),             \
    X(TK_ASSIGN_XOR,    "^=",               TF_ASSIGNMENT|TK_BINOP(XOR)),            \
    X(TK_ASSIGN_LSHIFT, "<<=",              TF_ASSIGNMENT|TK_BINOP(LSHIFT)),         \
    X(TK_ASSIGN_RSHIFT, ">>=",              TF_ASSIGNMENT|TK_BINOP(RSHIFT)),         \
    X(TK_ID,            "an identifier",    0),                                      \
    X(TK_LITERAL,       "a literal",        0),                                      \
    X(TK_EOF,           "eof",              0),

 enum TK {
#define X(a,b,c) a
    TOKENS__
#undef X
    NUM_TOKEN_TYPES,
    TK_KEYWORDS_BEGIN=TK_ELSE,
    TK_KEYWORDS_END=TK_DEFAULT+1,
};

extern const char *token_names[NUM_TOKEN_TYPES];

#endif //BCC_TOKENS_H
