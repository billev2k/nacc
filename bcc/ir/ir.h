//
// Created by Bill Evans on 9/23/24.
//

#ifndef BCC_IR_H
#define BCC_IR_H

#include "../utils/utils.h"

enum IR_OP {
    IR_OP_VAR,
    IR_OP_RET,
    IR_OP_UNARY,
    IR_OP_BINARY,
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
    IR_VAL_CONST_INT,
    IR_VAL_ID,
    IR_VAL_LABEL,
};

struct IrValue {
    enum IR_VAL kind;
    union {
        int int_val;
        const char *text;
    };
};
extern struct IrValue ir_value_new(enum IR_VAL valKind, const char *valText);
extern struct IrValue ir_value_new_id(const char* id);
extern struct IrValue ir_value_new_label(const char* label_name);
extern struct IrValue ir_value_new_int(int int_val);
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
extern struct IrInstruction* ir_instruction_new_comment(const char* text);
extern void IrInstruction_free(struct IrInstruction *instruction);
//endregion

//region struct IrFunction
#define NAME list_of_IrInstruction
#define TYPE struct IrInstruction*
#include "../utils/list_of_item.h"
#undef NAME
#undef TYPE

struct IrFunction {
    const char *name;
    struct list_of_IrInstruction body;
};
extern struct IrFunction * ir_function_new(const char *name);
extern void IrFunction_free(struct IrFunction *function);
extern void ir_function_append_instruction(struct IrFunction *function, struct IrInstruction *instruction);
//endregion

//region struct IrProgram
struct IrProgram {
    struct IrFunction *function;
};
extern struct IrProgram * ir_program_new();
extern void IrProgram_free(struct IrProgram *program);
//endregion

#endif //BCC_IR_H
