//
// Created by Bill Evans on 8/31/24.
//

#ifndef BCC_AMD64_H
#define BCC_AMD64_H

#include <stdio.h>
#include "utils.h"

enum INST {
    INST_MOV,
    INST_RET
};

enum OPND {
    OPND_IMM,
    OPND_REGISTER
};

DYN_LIST_OF_P_DECL(Amd64Instruction) // Amd64Instruction_list, ..._append, ..._free

struct Amd64Function;
struct Amd64Program {
    struct Amd64Function *function;
};
extern void amd64_program_free(struct Amd64Program *program);
extern void amd64_program_emit(struct Amd64Program *program, FILE *out);

struct Amd64Function {
    const char *name;
    struct Amd64Instruction_list instructions;
};
extern void amd64_function_append_instruction(struct Amd64Function *function, struct Amd64Instruction *instruction);
extern void amd64_function_free(struct Amd64Function *function);

struct Amd64Instruction {
    enum INST instruction;
    struct Amd64Operand *src;
    struct Amd64Operand *dst;
};
extern void Amd64Instruction_free(struct Amd64Instruction *instruction);

struct Amd64Operand {
    enum OPND operand_type;
    const char *name;
};
extern void amd64_operand_free(struct Amd64Operand *operand);

#endif //BCC_AMD64_H
