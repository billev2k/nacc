//
// Created by Bill Evans on 8/28/24.
//

#ifndef BCC_AST_H
#define BCC_AST_H
#include "../ir/ir.h"
#include "../utils/utils.h"

/*
 *  Current AST:
    program = Program(function_definition)
    function_definition = Function(identifier name, block_item* body)
    block_item = S(statement) | D(declaration)
    declaration = Declaration(identifier name, exp? init)
    statement = Return(exp) | Expression(exp) | Null
    exp = Constant(int)
        | Var(identifier)
        | Unary(unary_operator, exp)
        | Binary(binary_operator, exp, exp)
        | Assignment(exp, exp)
    unary_operator = Complement | Negate | Not
    binary_operator = Add | Subtract | Multiply | Divide | Remainder | And | Or
                    | Equal | NotEqual | LessThan | LessOrEqual
                    | GreaterThan | GreaterOrEqual

    Excerpt From
    Writing a C Compiler
    Nora Sandler
 */

enum AST_BLOCK_ITEM {
    AST_BI_STATEMENT,
    AST_BI_DECLARATION,
};

enum AST_STMT {
    STMT_RETURN,
    STMT_EXP,
    STMT_NULL,
};

enum AST_EXP_TYPE {
    AST_EXP_CONST,
    AST_EXP_UNOP,
    AST_EXP_BINOP,
    AST_EXP_VAR,
    AST_EXP_ASSIGNMENT,
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
    X(ASSIGN,     "=",     1),      \
    

DYN_LIST_OF_P_DECL(CBlockItem) // CBlockItem_list, ..._append, ..._free

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

struct CBlockItem;
struct CStatement;
struct CFunction {
    const char *name;
//    struct CStatement *statement;
    struct CBlockItem_list body;
};
extern struct CFunction* c_function_new(const char* name);
extern void c_function_append_block_item(struct CFunction* function, struct CBlockItem* item);
extern void c_function_free(struct CFunction *function);

struct CStatement {
    enum AST_STMT type;
    struct CExpression *expression;
};
extern struct CStatement* c_statement_new_return(struct CExpression* expression);
extern struct CStatement* c_statement_new_exp(struct CExpression* expression);
extern struct CStatement* c_statement_new_null(void);
extern void c_statement_free(struct CStatement *statement);

struct CExpression {
    enum AST_EXP_TYPE type;
    union {
        struct {
            enum AST_BINARY_OP op;
            struct CExpression *left;
            struct CExpression *right;
        } binary;
        struct {
            enum AST_CONST_TYPE type;
            const char *value;
        } literal;
        struct {
            enum AST_UNARY_OP op;
            struct CExpression *operand;
        } unary;
        struct CExpression *exp;
        struct {
            const char *name;
        } var;
        struct {
            struct CExpression* dst;
            struct CExpression* src;
        } assign;
    };
};
extern struct CExpression* c_expression_new_const(enum AST_CONST_TYPE const_type, const char *value);
extern struct CExpression* c_expression_new_unop(enum AST_UNARY_OP op, struct CExpression* operand);
extern struct CExpression* c_expression_new_binop(enum AST_BINARY_OP op, struct CExpression* left, struct CExpression* right);
extern struct CExpression* c_expression_new_assign(struct CExpression* dst, struct CExpression* src);
extern struct CExpression* c_expression_new_var(const char* name);
extern void c_expression_free(struct CExpression *expression);

struct CDeclaration {
    const char *name;
    struct CExpression* initializer;
};
extern struct CDeclaration* c_declaration_new(const char* identifier);
extern struct CDeclaration* c_declaration_new_init(const char* identifier, struct CExpression* initializer);
extern void c_declaration_free(struct CDeclaration* declaration);

struct CBlockItem {
    enum AST_BLOCK_ITEM type;
    union {
        struct CDeclaration* declaration;
        struct CStatement* statement;
    };
};
extern struct CBlockItem* c_block_item_new_decl(struct CDeclaration* declaration);
extern struct CBlockItem* c_block_item_new_stmt(struct CStatement* statement);
extern void c_block_item_free(struct CBlockItem* blockItem);
extern void CBlockItem_free(struct CBlockItem* blockItem);

#endif //BCC_AST_H
