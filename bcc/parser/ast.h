//
// Created by Bill Evans on 8/28/24.
//

#ifndef BCC_AST_H
#define BCC_AST_H
#include "../ir/ir.h"
#include "../utils/utils.h"

/*
 *  Current AST:
    program = Program(function_vardecl*)
    declaration = FunDecl(function_vardecl) | VarDecl(variable_vardecl)
    variable_vardecl = (identifier name, exp? init)
    function_vardecl = (identifier name, identifier* params, body? body)
    block_item = S(statement) | D(declaration)
    body = Block(block_item*)
    for_init = InitDecl(variable_vardecl) | InitExp(exp?)
    statement = Return(exp)
              | Expression(exp)
              | If(exp condition, statement then, statement? else)
              | Compound(body)
              | Break
              | Continue
              | While(exp condition, statement body)
              | DoWhile(statement body, exp condition)
              | For(for_init init, exp? condition, exp? post, statement body)
              | Null
    exp = Constant(int)
        | Var(identifier)
        | Unary(unary_operator, exp)
        | Binary(binary_operator, exp, exp)
        | Assignment(exp, exp)
        | Conditional(exp condition, exp, exp)
        | FunctionCall(identifier, exp* args)
    unary_operator = Complement | Negate | Not
    binary_operator = Add | Subtract | Multiply | Divide | Remainder | And | Or
                    | Equal | NotEqual | LessThan | LessOrEqual
                    | GreaterThan | GreaterOrEqual

    Excerpt From
    Writing a C Compiler
    Nora Sandler
    This material may be protected by copyright.
 */

enum AST_RESULT {
    AST_OK = 0,
    AST_DUPLICATE,
};

enum STORAGE_CLASS {
    SC_NONE         = 0x00,
    SC_STATIC       = 0x01,
    SC_EXTERN       = 0x02,
    // SC_THREAD       = 0x04,
};

enum DECL_KIND {
    FUNC_DECL,
    VAR_DECL,
};

enum AST_BLOCK_ITEM_KIND {
    AST_BI_STATEMENT,
    AST_BI_DECLARATION,
};

enum FOR_INIT_KIND {
    FOR_INIT_DECL,
    FOR_INIT_EXPR,
};

enum LABEL_KIND {
    LABEL_NONE,
    LABEL_DECL,         // "identifier:" before a statement.
    LABEL_CASE,         // a "case X:" in a switch() statement body.
    LABEL_DEFAULT,      // a "default:" in a switch() statement body.
};

enum AST_STMT_KIND {
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

enum AST_EXP_KIND {
    AST_EXP_ASSIGNMENT,
    AST_EXP_BINOP,
    AST_EXP_CONDITIONAL,
    AST_EXP_CONST,
    AST_EXP_FUNCTION_CALL,
    AST_EXP_INCREMENT,
    AST_EXP_UNOP,
    AST_EXP_VAR,
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

//region AST operators and precedence
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
extern const char* const AST_UNARY_NAMES[];
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

//#define COMPOUND_ASSIGN_LIST_ = \
//    X(ASSIGN_PLUS,      "+=",    2),      \
//    X(ASSIGN_MINUS,     "-=",    2),      \
//    X(ASSIGN_MULT,      "*=",    2),      \
//    X(ASSIGN_DIV,       "/=",    2),      \
//    X(ASSIGN_MOD,       "%=",    2),      \
//    X(ASSIGN_AND,       "&=",    2),      \
//    X(ASSIGN_OR,        "|=",    2),      \
//    X(ASSIGN_XOR,       "^=",    2),      \
//    X(ASSIGN_LSHIFT,    "<<=",   2),      \
//    X(ASSIGN_RSHIFT,    ">>=",   2),      \


enum AST_BINARY_OP {
#define X(a,b,c,d) AST_BINARY_##a
    AST_BINARY_LIST__
#undef X
};
extern const char* const AST_BINARY_NAMES[];
extern const int AST_BINARY_PRECEDENCE[];
extern const int AST_TO_IR_BINARY[];
//endregion

//region struct CIdentifier
/**
 * Holds the declared name and the uniqufied name for a variable, including labels.
 */
struct CIdentifier {
    const char* name;
    const char* source_name;
};
#define NAME list_of_CIdentifier
#define TYPE struct CIdentifier
#include "../utils/list_of_item.h"
//endregion CIdentifier
   
//region struct CLabel
struct CLabel {
    enum LABEL_KIND kind;
    struct CIdentifier identifier;
    struct CExpression *expr;
    int switch_flow_id;
};

struct CExpression;
extern struct CLabel c_label_new_label(struct CIdentifier identifier);
extern struct CLabel c_label_new_switch_default();
extern struct CLabel c_label_new_switch_case(struct CExpression *expr);
extern void c_label_delete(struct CLabel label);

#define NAME list_of_CLabel
#define TYPE struct CLabel
#include "../utils/list_of_item.h"
//endregion struct CLabel

//region struct CExpression
struct CExpression;
#define NAME list_of_CExpression
#define TYPE struct CExpression*
#include "../utils/list_of_item.h"
struct CExpression {
    enum AST_EXP_KIND kind;
    union {
        struct {
            enum AST_UNARY_OP op;
            struct CExpression* operand;
        } unary;
        struct {
            enum AST_BINARY_OP op;
            struct CExpression* left;
            struct CExpression* right;
        } binop;
        struct {
            enum AST_CONST_TYPE type;
            union {
                int int_val;
            };
        } literal;
        struct CIdentifier var;
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
            struct CExpression* middle_exp;     // if true int_val
            struct CExpression* right_exp;      // if false int_val
        } conditional;
        struct {
            struct CIdentifier func;
            struct list_of_CExpression args;
        } function_call;
    };
};
extern int c_expression_is_const(struct CExpression *exp);
extern int c_expression_get_const_value(struct CExpression *exp);
extern struct CExpression* c_expression_new_assign(struct CExpression* src, struct CExpression* dst);
extern struct CExpression* c_expression_new_binop(enum AST_BINARY_OP op, struct CExpression* left, struct CExpression* right);
extern struct CExpression* c_expression_new_conditional(struct CExpression* left_exp, struct CExpression* middle_exp, struct CExpression* right_exp);
extern struct CExpression* c_expression_new_const(enum AST_CONST_TYPE const_type, int int_val);
extern struct CExpression* c_expression_new_function_call(struct CIdentifier func);
extern struct CExpression* c_expression_new_increment(enum AST_INCREMENT_OP op, struct CExpression* operand);
extern struct CExpression* c_expression_new_unop(enum AST_UNARY_OP op, struct CExpression* operand);
extern struct CExpression* c_expression_new_var(const char* name);
extern void c_expression_function_call_add_arg(struct CExpression* call_expr, struct CExpression* arg);
extern struct CExpression* c_expression_clone(const struct CExpression* expression);
extern void c_expression_delete(struct CExpression* expression);
//endregion CExpression

//region struct CDeclaration
struct CDeclaration {
    enum DECL_KIND decl_kind;
    union {
        struct CVarDecl* var;
        struct CFuncDecl* func;
    };
};
extern struct CDeclaration* c_declaration_new_var(struct CVarDecl* vardecl);
extern struct CDeclaration* c_declaration_new_func(struct CFuncDecl* funcdecl);
extern void c_declaration_delete(struct CDeclaration* declaration);
#define TYPE struct CDeclaration*
#define NAME list_of_CDeclaration
#include "../utils/list_of_item.h"
//endregion CDeclaration

//region struct CVarDecl
struct CVarDecl {
    enum STORAGE_CLASS storage_class;
    struct CIdentifier var;
    struct CExpression* initializer;
};
extern struct CVarDecl* c_vardecl_new(const char* identifier, enum STORAGE_CLASS storage_class);
extern struct CVarDecl* c_vardecl_new_init(const char* identifier, struct CExpression* initializer, enum STORAGE_CLASS storage_class);
extern void c_vardecl_delete(struct CVarDecl* vardecl);
//endregion CVarDecl

//region struct CForInit
struct CForInit {
    enum FOR_INIT_KIND kind;
    union {
        struct CDeclaration* declaration;
        struct CExpression* expression;
    };
};
extern struct CForInit* c_for_init_new_vardecl(struct CDeclaration *declaration);
extern struct CForInit* c_for_init_new_expression(struct CExpression* expression);
extern void c_for_init_delete(struct CForInit* for_init);
//endregion  CForInit

//region struct CBlockItem
struct CBlockItem {
    enum AST_BLOCK_ITEM_KIND kind;
    union {
        struct CStatement* statement;
        struct CDeclaration *declaration;
    };
};
extern struct CBlockItem* c_block_item_new_decl(struct CDeclaration* declaration);

extern struct CBlockItem* c_block_item_new_stmt(struct CStatement* statement);
extern void c_block_item_delete(struct CBlockItem* blockItem);
extern void CBlockItem_delete(struct CBlockItem* blockItem);  // actually used in c_blockitem_helpers
// Implementation of List<CBlockItem> (?list_of_CBlockItem")
#define NAME list_of_CBlockItem
#define TYPE struct CBlockItem*
#include "../utils/list_of_item.h"
//endregion CBlockItem

//region struct CBlock
struct CBlock {
    struct list_of_CBlockItem items;
    int is_function_block;
};
extern struct CBlock* c_block_new(int is_function);
extern void c_block_append_item(struct CBlock* block, struct CBlockItem* item);
extern void c_block_delete(struct CBlock* block);
//endregion CBlock

//region struct CStatement
struct CStatement {
    enum AST_STMT_KIND kind;
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
            struct list_of_int* case_labels;
            int has_default;
        } switch_statement;
        struct CBlock* compound;
    };
    struct list_of_CLabel* labels;
    int flow_id;    // ID of while/do/for/switch statement, for break/continue/case/default handling.
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
extern int c_statement_has_labels(const struct CStatement* statement);
extern void c_statement_add_labels(struct CStatement* pStatement, struct list_of_CLabel labels);
extern struct CLabel* c_statement_get_labels(const struct CStatement* statement);
extern int c_statement_num_labels(const struct CStatement* statement);
extern enum AST_RESULT c_statement_set_flow_id(struct CStatement* statement, int flow_id);
extern enum AST_RESULT c_statement_set_switch_has_default(struct CStatement* statement);
extern enum AST_RESULT c_statement_register_switch_case(struct CStatement* statement, int case_value);
extern void c_statement_delete(struct CStatement* statement);
//endregion CStatement

//region struct CFuncDecl
struct CFuncDecl {
    enum STORAGE_CLASS storage_class;
    const char* name;
    struct CBlock* body;
    struct list_of_CIdentifier params;
};
#define NAME list_of_CFuncDecl
#define TYPE struct CFuncDecl*
#include "../utils/list_of_item.h"
extern struct CFuncDecl* c_function_new(const char* name, enum STORAGE_CLASS storage_class);
extern enum AST_RESULT c_function_add_param(struct CFuncDecl* function, const char* param_name);
extern enum AST_RESULT c_function_add_body(struct CFuncDecl* function, struct CBlock* body);
extern void c_function_delete(struct CFuncDecl* function);
//endregion CFuncDecl

//region struct CProgram
struct CProgram {
    struct list_of_CDeclaration declarations;
};
extern struct CProgram* c_program_new(void);
extern enum AST_RESULT c_program_add_decl(struct CProgram *program, struct CDeclaration *declaration);

extern void c_program_delete(struct CProgram* program);
//endregion CProgram

#endif //BCC_AST_H
