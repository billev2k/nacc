//
// Created by Bill Evans on 9/23/24.
//

#ifndef BCC_IR_H
#define BCC_IR_H

#include "../utils/utils.h"

enum IR_OP {
    IR_OP_RET,
    IR_OP_UNARY,
    IR_OP_BINARY,
};

#define IR_UNARY_OP_LIST__ \
    X(NEGATE,       "CMP"),        \
    X(COMPLEMENT,   "NOT")
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
    X(DIVIDE,       "DIV"),             \
    X(REMAINDER,    "MOD"),             \
    X(OR,           "OR"),              \
    X(AND,          "AND"),             \
    X(XOR,          "XOR"),             \
    X(LSHIFT,       "LSHIFT"),          \
    X(RSHIFT,       "RSHIFT"),

enum IR_BINARY_OP {
#define X(a,b) IR_BINARY_##a
    IR_BINARY_OP_LIST__
#undef X
};
extern const char * const IR_BINARY_NAMES[];

enum IR_VAL {
    IR_VAL_CONST_INT,
    IR_VAL_ID
};

DYN_LIST_OF_P_DECL(IrInstruction) // Amd64Instruction_list, ..._append, ..._free

struct IrProgram {
    struct IrFunction *function;
};
extern struct IrProgram * ir_program_new();
extern void IrProgram_free(struct IrProgram *program);

struct IrFunction {
    const char *name;
    struct IrInstruction_list body;
};
extern struct IrFunction * ir_function_new(const char *name);
extern void IrFunction_free(struct IrFunction *function);
extern void ir_function_append_instruction(struct IrFunction *function, struct IrInstruction *instruction);

struct IrInstruction {
    enum IR_OP inst;
    union {
        enum IR_UNARY_OP unary_op;
        enum IR_BINARY_OP binary_op;
    };
    struct IrValue *a;
    struct IrValue *b;
    struct IrValue *c;
};
extern struct IrInstruction *ir_instruction_new_nonary(enum IR_OP inst, struct IrValue *src);
extern struct IrInstruction *ir_instruction_new_unary(enum IR_UNARY_OP op, struct IrValue *src, struct IrValue *dst);
extern struct IrInstruction *ir_instruction_new_binary(enum IR_BINARY_OP op, struct IrValue *src1, struct IrValue *src2, struct IrValue *dst);
extern void IrInstruction_free(struct IrInstruction *instruction);

struct IrValue {
    enum IR_VAL type;
    const char *text;
};
extern struct IrValue * ir_value_new(enum IR_VAL valType, const char *valText);
extern void ir_value_free(struct IrValue *value);

#endif //BCC_IR_H
