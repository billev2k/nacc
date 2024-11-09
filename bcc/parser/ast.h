//
// Created by Bill Evans on 8/28/24.
//

#ifndef BCC_AST_H
#define BCC_AST_H
#include "../ir/ir.h"

enum AST_STMT {
    STMT_RETURN
};

enum AST_EXP_TYPE {
    AST_EXP_CONST,
    AST_EXP_UNOP,
    AST_EXP_BINOP,
};

#define AST_CONST_LIST__ \
    X(AST_CONST_INT, "int")

enum AST_CONST_TYPE {
#define X(a,b) a
    AST_CONST_LIST__
#undef X
};
extern const char * const ASM_CONST_TYPE_NAMES[];

// This list also expands to reference items from IR_UNARY_OP_LIST__
// Every unary operator here must have a corresponding IR unary
// operator; the converse is not true, IR can support unary ops that
// C does not need.
#define AST_UNARY_LIST__ \
    X(NEGATE,       "-"),       \
    X(COMPLEMENT,   "~"),       \
    X(L_NOT,        "!"),       
enum AST_UNARY_OP {
#define X(a,b) AST_UNARY_##a
    AST_UNARY_LIST__
#undef X
};
extern const char * const AST_UNARY_NAMES[];
extern enum IR_UNARY_OP const AST_TO_IR_UNARY[];

// This list also expands to reference items from IR_BINARY_OP_LIST__
// Every binary operator here must have a corresponding IR binary
// operator; the converse is not true, IR can support binary ops that
// C does not need.
#define AST_BINARY_LIST__ \
    X(MULTIPLY,   "*",    50),      \
    X(DIVIDE,     "/",    50),      \
    X(REMAINDER,  "%",    50),      \
    X(ADD,        "+",    45),      \
    X(SUBTRACT,   "-",    45),      \
    X(OR,         "|",    20),      \
    X(AND,        "&",    25),      \
    X(XOR,        "^",    23),      \
    X(LSHIFT,     "<<",   40),      \
    X(RSHIFT,     ">>",   40),      \
    X(LT,         "<",    35),      \
    X(LE,         "<=",   35),      \
    X(GT,         ">",    35),      \
    X(GE,         ">=",   35),      \
    X(EQ,         "==",   30),      \
    X(NE,         "!=",   30),      \
    X(L_AND,      "&&",   10),      \
    X(L_OR,       "||",    5),      \




enum AST_BINARY_OP {
#define X(a,b,c) AST_BINARY_##a
    AST_BINARY_LIST__
#undef X
};
extern const char * const AST_BINARY_NAMES[];
extern const int AST_BINARY_PRECEDENCE[];
extern const int AST_TO_IR_BINARY[];

struct CFunction;
struct CProgram {
    struct CFunction *function;
};
extern void c_program_free(struct CProgram *program);

struct CStatement;
struct CFunction {
    const char *name;
    struct CStatement *statement;
};
extern void c_function_free(struct CFunction *function);

struct CStatement {
    enum AST_STMT type;
    struct CExpression *expression;
};
extern void c_statement_free(struct CStatement *statement);

struct CExpression {
    enum AST_EXP_TYPE type;
    union {
        struct {
            enum AST_BINARY_OP binary_op;
            struct CExpression *left;
            struct CExpression *right;
        };
        struct {
            enum AST_CONST_TYPE const_type;
            const char *value;
        };
        struct {
            enum AST_UNARY_OP unary_op;
            struct CExpression *operand;
        };
        struct CExpression *exp;
    };
};
extern struct CExpression* c_expression_new_const(enum AST_CONST_TYPE const_type, const char *value);
extern struct CExpression* c_expression_new_unop(enum AST_UNARY_OP op, struct CExpression* operand);
extern struct CExpression* c_expression_new_binop(enum AST_BINARY_OP op, struct CExpression* left, struct CExpression* right);
extern void c_expression_free(struct CExpression *expression);

#endif //BCC_AST_H
