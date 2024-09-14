//
// Created by Bill Evans on 8/31/24.
//

#ifndef BCC_ASM_H
#define BCC_ASM_H

#include <stdio.h>

enum INST {
    INST_MOV,
    INST_RET
};

enum OPND {
    OPND_IMM,
    OPND_REGISTER
};


struct AsmFunction;
struct AsmProgram {
    struct AsmFunction *function;
};
extern void asm_program_free(struct AsmProgram *program);
extern void asm_program_emit(struct AsmProgram *program, FILE *out);

struct AsmFunction {
    const char *name;
    struct AsmInstruction **instructions;
    int instructionCount;
    int instructionsSize;
};
extern void asm_function_append_instruction(struct AsmFunction *function, struct AsmInstruction *instruction);
extern void asm_function_free(struct AsmFunction *function);

struct AsmInstruction {
    enum INST instruction;
    struct AsmOperand *src;
    struct AsmOperand *dst;
};
extern void asm_instruction_free(struct AsmInstruction *instruction);

struct AsmOperand {
    enum OPND operand_type;
    const char *name;
};
extern void asm_operand_free(struct AsmOperand *operand);

#endif //BCC_ASM_H
