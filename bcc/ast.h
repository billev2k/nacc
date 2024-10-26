//
// Created by Bill Evans on 8/28/24.
//

#ifndef BCC_AST_H
#define BCC_AST_H

enum STMT {
    STMT_RETURN
};

enum EXP {
    EXP_CONST,
    EXP_UNARY,
};

enum EXP_CONST_TYPE {
    EXP_CONST_INT,
};
enum EXP_UNARY_OP {
    EXP_UNARY_NEGATE,
    EXP_UNARY_COMPLEMENT,
};

struct CFunction;
struct CProgram {
    struct CFunction *function;
};
extern void c_program_free(struct CProgram *program);
extern void c_program_print(struct CProgram *program);

struct CStatement;
struct CFunction {
    const char *name;
    struct CStatement *statement;
};
extern void c_function_free(struct CFunction *function);

struct CStatement {
    enum STMT type;
    struct CExpression *expression;
};
extern void c_statement_free(struct CStatement *statement);

struct CExpression {
    enum EXP type;
    union {
        enum EXP_CONST_TYPE const_type;
        enum EXP_UNARY_OP unary_op;
    };
    const char *value;
    struct CExpression *exp;
};
extern void c_expression_free(struct CExpression *expression);

#endif //BCC_AST_H
