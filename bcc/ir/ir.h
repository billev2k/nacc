//
// Created by Bill Evans on 9/23/24.
//

#ifndef BCC_IR_H
#define BCC_IR_H

#include <limits.h>
#include <stdbool.h>
#include "inc/constant.h"
#include "inc/utils.h"

enum IR_OP {
    IR_OP_VAR,
    IR_OP_RET,
    IR_OP_UNARY,
    IR_OP_BINARY,
    IR_OP_FUNCALL,
    IR_OP_COPY,
    IR_OP_JUMP,
    IR_OP_JUMP_ZERO,
    IR_OP_JUMP_NZERO,
    IR_OP_JUMP_EQ,
    IR_OP_LABEL,
    IR_OP_COMMENT,
};

#define IR_UNARY_OP_LIST__ \
    X(NEGATE,       "neg"),         \
    X(COMPLEMENT,   "cmpl"),         \
    X(L_NOT,        "not")
enum IR_UNARY_OP {
#define X(a,b) IR_UNARY_##a
    IR_UNARY_OP_LIST__
#undef X            
};
extern const char * const IR_UNARY_NAMES[];

#define IR_BINARY_OP_LIST__ \
    X(ADD,          "ADD"),             \
    X(SUBTRACT,     "SUB"),             \
    X(MULTIPLY,     "MULT"),            \
    X(DIVIDE,       "IDIV"),            \
    X(REMAINDER,    "MOD"),             \
    X(OR,           "OR"),              \
    X(AND,          "AND"),             \
    X(XOR,          "XOR"),             \
    X(LSHIFT,       "LSHIFT"),          \
    X(RSHIFT,       "RSHIFT"),          \
    X(L_AND,        "&&"),              \
    X(L_OR,         "||"),              \
    X(EQ,           "=="),              \
    X(NE,           "!="),              \
    X(LT,           "<"),               \
    X(LE,           "<="),              \
    X(GT,           ">"),               \
    X(GE,           ">="),              \



enum IR_BINARY_OP {
#define X(a,b) IR_BINARY_##a
    IR_BINARY_OP_LIST__
#undef X
};
extern const char * const IR_BINARY_NAMES[];


//region struct IrValue
enum IR_VAL {
    IR_VAL_CONST,
    IR_VAL_ID,
    IR_VAL_LABEL,
};

struct IrValue {
    enum IR_VAL kind;
    union {
        struct Constant const_value;
        const char *text;
    };
};
extern struct IrValue ir_value_new_id(const char* id);
extern struct IrValue ir_value_new_label(const char* label_name);
extern struct IrValue ir_value_new_int(int int_val);
extern struct IrValue ir_value_new_const(struct Constant value);

LIST_OF_ITEM_DECL(list_of_IrValue,struct IrValue)
//endregion VALUE

//region struct IrInstruction
struct IrInstruction {
    enum IR_OP inst;
    union {
        struct {
            struct IrValue value;
        } var;
        struct {
            struct IrValue value;
        } ret;
        struct {
            enum IR_UNARY_OP op;
            struct IrValue src;
            struct IrValue dst;
        } unary;
        struct {
            enum IR_BINARY_OP op;
            struct IrValue src1;
            struct IrValue src2;
            struct IrValue dst;
        } binary;
        struct {
            struct IrValue src;
            struct IrValue dst;
        } copy;
        struct {
            struct IrValue target; // must be "IR_VAL_LABEL"
        } jump;
        struct {
            struct IrValue value;
            struct IrValue comparand;
            struct IrValue target; // must be "IR_VAL_LABEL"
        } cjump;
        struct {
            struct IrValue label;
        } label;
        struct {
            const char *text;
        } comment;
        struct {
            struct IrValue func_name;
            struct list_of_IrValue args;
            struct IrValue dst;
        } funcall;
    };
};
extern struct IrInstruction* ir_instruction_new_var(struct IrValue value);
extern struct IrInstruction* ir_instruction_new_ret(struct IrValue value);
extern struct IrInstruction *ir_instruction_new_unary(enum IR_UNARY_OP op, struct IrValue src, struct IrValue dst);
extern struct IrInstruction *ir_instruction_new_binary(enum IR_BINARY_OP op, struct IrValue src1, struct IrValue src2, struct IrValue dst);
extern struct IrInstruction* ir_instruction_new_copy(struct IrValue src, struct IrValue dst);
extern struct IrInstruction* ir_instruction_new_jump(struct IrValue target);
extern struct IrInstruction *
ir_instruction_new_jumpeq(struct IrValue value, struct IrValue comparand, struct IrValue target);
extern struct IrInstruction* ir_instruction_new_jumpz(struct IrValue value, struct IrValue target);
extern struct IrInstruction* ir_instruction_new_jumpnz(struct IrValue value, struct IrValue target);
extern struct IrInstruction* ir_instruction_new_label(struct IrValue label);
extern struct IrInstruction* ir_instruction_new_funcall(struct IrValue func_name, struct list_of_IrValue* args, struct IrValue dst);
extern struct IrInstruction* ir_instruction_new_comment(const char* text);
extern void IrInstruction_delete(struct IrInstruction *instruction);

LIST_OF_ITEM_DECL(list_of_IrInstruction, struct IrInstruction*)
//endregion

//region struct IrStaticVar
struct IrStaticVar {
    const char *name;
    bool global;
    struct Constant init_value;
};
extern struct IrStaticVar *ir_static_var_new(const char *name, bool global, struct Constant init_value);
extern void ir_static_var_delete(struct IrStaticVar *static_var);
//endregion

//region struct IrFunction
struct IrFunction {
    const char *name;
    bool global;
    struct list_of_IrValue params;
    struct list_of_IrInstruction body;
};
extern struct IrFunction *ir_function_new(const char *name, bool global);
extern void IrFunction_delete(struct IrFunction *function);
extern void IrFunction_add_param(struct IrFunction* function, const char* param_name);
extern void ir_function_append_instruction(struct IrFunction *function, struct IrInstruction *instruction);
//endregion

//region struct IrTopLevel
enum IR_TOP_LEVEL_KIND {
    IR_FUNCTION,
    IR_STATIC_VAR,
};
struct IrTopLevel {
    enum IR_TOP_LEVEL_KIND kind;
    union {
        struct IrFunction *function;
        struct IrStaticVar *static_var;
    };
};
extern struct IrTopLevel* ir_top_level_new_function(struct IrFunction *function);
extern struct IrTopLevel* ir_top_level_new_static_var(struct IrStaticVar *static_var);
extern void ir_top_level_delete(struct IrTopLevel *top_level);
LIST_OF_ITEM_DECL(list_of_top_level, struct IrTopLevel*)
#include "inc/constant.h"
//endregion

//region struct IrProgram
struct IrProgram {
    struct list_of_top_level top_level;
};
extern struct IrProgram * ir_program_new();
extern void ir_program_add_function(struct IrProgram* program, struct IrFunction* function);
extern void ir_program_add_static_var(struct IrProgram *program, struct IrStaticVar *static_var);
extern void IrProgram_delete(struct IrProgram *program);
//endregion

#endif //BCC_IR_H
