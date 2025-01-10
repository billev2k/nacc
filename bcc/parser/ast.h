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

enum FOR_INIT_TYPE {
    FOR_INIT_DECL,
    FOR_INIT_EXPR,
};

enum LABEL_TYPE {
    LABEL_DECL,
    LABEL_CASE,
    LABEL_DEFAULT,
};

enum AST_STMT {
    STMT_BREAK,
    STMT_COMPOUND,
    STMT_CONTINUE,
    STMT_DOWHILE,
    STMT_EXP,
    STMT_FOR,
    STMT_GOTO,
    STMT_IF,
    STMT_NULL,
    STMT_RETURN,
    STMT_SWITCH,
    STMT_WHILE,
    STMT_AUTO_RETURN,
};

enum AST_EXP_TYPE {
    AST_EXP_CONST,
    AST_EXP_UNOP,
    AST_EXP_BINOP,
    AST_EXP_VAR,
    AST_EXP_ASSIGNMENT,
    AST_EXP_INCREMENT,
    AST_EXP_CONDITIONAL,
};

enum AST_INCREMENT_OP {
    AST_PRE_INCR,
    AST_PRE_DECR,
    AST_POST_INCR,
    AST_POST_DECR,
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
    X(L_NOT,        "!"),       \

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
    X(MULTIPLY,         "*",    50,	    IR_BINARY_MULTIPLY),    \
    X(DIVIDE,           "/",    50,	    IR_BINARY_DIVIDE),      \
    X(REMAINDER,        "%",    50,	    IR_BINARY_REMAINDER),   \
    X(ADD,              "+",    45,	    IR_BINARY_ADD),         \
    X(SUBTRACT,         "-",    45,	    IR_BINARY_SUBTRACT),    \
    X(OR,               "|",    20,	    IR_BINARY_OR),          \
    X(AND,              "&",    25,	    IR_BINARY_AND),         \
    X(XOR,              "^",    23,	    IR_BINARY_XOR),         \
    X(LSHIFT,           "<<",   40,	    IR_BINARY_LSHIFT),      \
    X(RSHIFT,           ">>",   40,	    IR_BINARY_RSHIFT),      \
    X(LT,               "<",    35,	    IR_BINARY_LT),          \
    X(LE,               "<=",   35,	    IR_BINARY_LE),          \
    X(GT,               ">",    35,	    IR_BINARY_GT),          \
    X(GE,               ">=",   35,	    IR_BINARY_GE),          \
    X(EQ,               "==",   30,	    IR_BINARY_EQ),          \
    X(NE,               "!=",   30,	    IR_BINARY_NE),          \
    X(L_AND,            "&&",   10,	    IR_BINARY_L_AND),       \
    X(L_OR,             "||",    5,	    IR_BINARY_L_OR),        \
    X(QUESTION,         "?",     3,	    0),                     \
    X(ASSIGN,           "=",     2,	    0),                     \

#define COMPOUND_ASSIGN_LIST_ = \
    X(ASSIGN_PLUS,      "+=",    2),      \
    X(ASSIGN_MINUS,     "-=",    2),      \
    X(ASSIGN_MULT,      "*=",    2),      \
    X(ASSIGN_DIV,       "/=",    2),      \
    X(ASSIGN_MOD,       "%=",    2),      \
    X(ASSIGN_AND,       "&=",    2),      \
    X(ASSIGN_OR,        "|=",    2),      \
    X(ASSIGN_XOR,       "^=",    2),      \
    X(ASSIGN_LSHIFT,    "<<=",   2),      \
    X(ASSIGN_RSHIFT,    ">>=",   2),      \


enum AST_BINARY_OP {
#define X(a,b,c,d) AST_BINARY_##a
    AST_BINARY_LIST__
#undef X
};
extern const char * const AST_BINARY_NAMES[];
extern const int AST_BINARY_PRECEDENCE[];
extern const int AST_TO_IR_BINARY[];

/**
 * Holds the declared name and the uniqufied name for a variable, including labels.
 */
struct CVariable {
    const char* name;
    const char* source_name;
};

struct CLabel {
    enum LABEL_TYPE type;
    struct CVariable label;
};

LIST_OF_ITEM_DECL(CLabel, struct CLabel);


//region struct CExpression
struct CExpression {
    enum AST_EXP_TYPE type;
    union {
        struct {
            enum AST_UNARY_OP op;
            struct CExpression *operand;
        } unary;
        struct {
            enum AST_BINARY_OP op;
            struct CExpression *left;
            struct CExpression *right;
        } binary;
        struct {
            enum AST_CONST_TYPE type;
            const char *value;
        } literal;
        struct CVariable var;
        struct {
            struct CExpression* dst;
            struct CExpression* src;
        } assign;
        struct {
            enum AST_INCREMENT_OP op;
            struct CExpression* operand;
        } increment;
        struct {
            struct CExpression* left_exp;       // condition
            struct CExpression* middle_exp;     // if true value
            struct CExpression* right_exp;      // if false value
        } conditional;
    };
};
extern struct CExpression* c_expression_new_unop(enum AST_UNARY_OP op, struct CExpression* operand);
extern struct CExpression* c_expression_new_binop(enum AST_BINARY_OP op, struct CExpression* left, struct CExpression* right);
extern struct CExpression* c_expression_new_const(enum AST_CONST_TYPE const_type, const char *value);
extern struct CExpression* c_expression_new_var(const char* name);
extern struct CExpression* c_expression_new_assign(struct CExpression* src, struct CExpression* dst);
extern struct CExpression* c_expression_new_increment(enum AST_INCREMENT_OP op, struct CExpression* operand);
extern struct CExpression* c_expression_new_conditional(struct CExpression* left_exp, struct CExpression* middle_exp, struct CExpression* right_exp);
extern struct CExpression* c_expression_clone(const struct CExpression* expression);
extern void c_expression_free(struct CExpression *expression);
//endregion

//region struct CDeclaration
struct CDeclaration {
    struct CVariable var;
    struct CExpression* initializer;
};
extern struct CDeclaration* c_declaration_new(const char* identifier);
extern struct CDeclaration* c_declaration_new_init(const char* identifier, struct CExpression* initializer);
extern void c_declaration_free(struct CDeclaration* declaration);
//endregion

//region struct CForInit
struct CForInit {
    enum FOR_INIT_TYPE type;
    union {
        struct CDeclaration* declaration;
        struct CExpression* expression;
    };
};
extern struct CForInit* c_for_init_new_declaration(struct CDeclaration* declaration);
extern struct CForInit* c_for_init_new_expression(struct CExpression* expression);
extern void c_for_init_free(struct CForInit* for_init);
//endregion

//region struct CBlockItem
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
extern void CBlockItem_free(struct CBlockItem* blockItem);  // actually used in c_blockitem_helpers
//endregion

// Implementation of List<CBlockItem> (?list_of_CBlockItem")
LIST_OF_ITEM_DECL(CBlockItem, struct CBlockItem*)

//region CBlock
struct CBlock {
    struct list_of_CBlockItem items;
    int is_function_block;
};
extern struct CBlock* c_block_new(int is_function);
extern void c_block_append_item(struct CBlock* block, struct CBlockItem* item);
extern void c_block_free(struct CBlock *block);
//endregion

//region struct CStatement
struct CStatement {
    enum AST_STMT type;
    union {
        struct CExpression* expression;
        struct {
            struct CExpression* condition;
            struct CStatement* then_statement;
            struct CStatement* else_statement;
        } if_statement;
        struct {
            struct CExpression* label;
        } goto_statement;
        struct {
            struct CExpression* condition;
            struct CStatement* body;
        } while_or_do_statement;
        struct {
            struct CForInit* init;
            struct CExpression* condition;
            struct CExpression* post;
            struct CStatement* body;
        } for_statement;
        struct {
            struct CExpression* expression;
            struct CStatement* body;
        } switch_statement;
        struct CBlock* compound;
    };
    struct list_of_CLabel* labels;
};
extern struct CStatement* c_statement_new_break(void);
extern struct CStatement* c_statement_new_compound(struct CBlock* block);
extern struct CStatement* c_statement_new_continue(void);
extern struct CStatement* c_statement_new_do(struct CStatement* body, struct CExpression* condition);
extern struct CStatement* c_statement_new_for(struct CForInit* init, struct CExpression* condition, struct CExpression* post, struct CStatement* body);
extern struct CStatement* c_statement_new_exp(struct CExpression* expression);
extern struct CStatement* c_statement_new_goto(struct CExpression* label);
extern struct CStatement* c_statement_new_if(struct CExpression* condition, struct CStatement* then_statement,
                                      struct CStatement* else_statement);
extern struct CStatement* c_statement_new_null(void);
extern struct CStatement* c_statement_new_return(struct CExpression* expression);
extern struct CStatement* c_statement_new_switch(struct CExpression* expression, struct CStatement* body);
extern struct CStatement* c_statement_new_while(struct CExpression* condition, struct CStatement* body);
extern int c_statement_has_labels(const struct CStatement * statement);
extern void c_statement_add_labels(struct CStatement *pStatement, struct CLabel *labels);
extern struct CLabel * c_statement_get_labels(const struct CStatement * statement);
extern void c_statement_free(struct CStatement *statement);
//endregion

//region struct CFunction
struct CFunction {
    const char *name;
    struct CBlock* block;
};
extern struct CFunction* c_function_new(const char* name, struct CBlock* block);
extern void c_function_append_block_item(const struct CFunction* function, struct CBlockItem* item);
extern void c_function_free(struct CFunction *function);
//endregion

//region struct CProgram
struct CProgram {
    struct CFunction *function;
};
extern void c_program_free(struct CProgram *program);
//endregion

#endif //BCC_AST_H
