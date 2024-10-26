//
// Created by Bill Evans on 8/31/24.
//

#ifndef BCC_AMD64_H
#define BCC_AMD64_H

#include <stdio.h>
#include "utils.h"

enum INST {
    INST_MOV,
    INST_RET,
    INST_NEG,
    INST_NOT,
};

enum UNARY_OP {
    UNARY_OP_NEGATE,
    UNARY_OP_NOT,
};

enum OPERAND {
    OPERAND_NONE,
    OPERAND_IMM_LIT,
    OPERAND_IMM_CONST,
    OPERAND_REGISTER,
    OPERAND_PSEUDO,
    OPERAND_STACK,
};

#define REGISTERS__ \
    X(AX,  "%eax"),         \
    X(R10, "%r10d"),        \
    X(BP,  "%rbp"),         \
    X(SP,  "%rsp")

enum REGISTER {
#define X(r,n) REG_##r
    REGISTERS__
#undef X
};
extern char const * register_names_[];

DYN_LIST_OF_P_DECL(Amd64Instruction) // Amd64Instruction_list, ..._append, ..._free

struct Amd64Function;
struct Amd64Program {
    struct Amd64Function *function;
};
extern struct Amd64Program* amd64_program_new(void );
extern void amd64_program_free(struct Amd64Program *program);
extern void amd64_program_emit(struct Amd64Program *amd64Program, FILE *out);

struct Amd64Function {
    const char *name;
    int stack_allocations;
    struct Amd64Instruction_list instructions;
};
extern struct Amd64Function* amd64_function_new(const char *name);
extern void amd64_function_append_instruction(struct Amd64Function *function, struct Amd64Instruction *instruction);
extern void amd64_function_free(struct Amd64Function *function);

struct Amd64Operand {
    enum OPERAND operand_type;
    union {
        const char *name;
        int offset;
        enum REGISTER reg;
        int value;
    };
};
extern struct Amd64Operand amd64_operand_none;
extern struct Amd64Operand amd64_operand_imm_literal(const char* value_str);
extern struct Amd64Operand amd64_operand_imm_const(int value);
extern struct Amd64Operand amd64_operand_reg(enum REGISTER reg);
extern struct Amd64Operand amd64_operand_pseudo(const char* pseudo_name);
extern struct Amd64Operand amd64_operand_stack(int offset);



struct Amd64Instruction {
    enum INST instruction;
    struct Amd64Operand src;
    struct Amd64Operand dst;
};
extern struct Amd64Instruction* amd64_instruction_new(enum INST instruction, struct Amd64Operand src, struct Amd64Operand dst);
extern void Amd64Instruction_free(struct Amd64Instruction *instruction);

#endif //BCC_AMD64_H
