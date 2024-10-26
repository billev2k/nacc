//
// Created by Bill Evans on 9/23/24.
//

#ifndef BCC_IR_H
#define BCC_IR_H

#include "../utils/utils.h"

enum IR_OP {
    IR_OP_RET,
    IR_OP_UNARY,
};

enum IR_UNARY_OP {
    IR_UNARY_COMPLEMENT,
    IR_UNARY_NEGATE,
};

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
    };
    struct IrValue *a;
    struct IrValue *b;
    struct IrValue *c;
};
extern struct IrInstruction *ir_instruction_new_nonary(enum IR_OP inst, struct IrValue *src);
extern struct IrInstruction *ir_instruction_new_unary(enum IR_UNARY_OP op, struct IrValue *src, struct IrValue *dst);
extern void IrInstruction_free(struct IrInstruction *instruction);

struct IrValue {
    enum IR_VAL type;
    const char *text;
};
extern struct IrValue * IrValue_new(enum IR_VAL valType, const char *valText);
extern void IrValue_free(struct IrValue *value);

#endif //BCC_IR_H
