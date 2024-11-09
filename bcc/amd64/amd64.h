//
// Created by Bill Evans on 8/31/24.
//

#ifndef BCC_AMD64_H
#define BCC_AMD64_H

#include <stdio.h>
#include "../ir/ir.h"
#include "../utils/utils.h"
 
enum INSTRUCTION_FLAGS {
    IF_NO_2_REG             = 0x01,
    IF_SIZED                = 0x02,
    IF_DEST_IS_REG          = 0x04,
    IF_SHIFT_CONST_OR_CL   = 0x08,
};
#define IS_NO_2_REG(inst)           (instruction_flags[inst]&IF_NO_2_REG?1:0)
#define IS_SIZED(inst)              (instruction_flags[inst]&IF_SIZED?1:0)
#define IS_DEST_IS_REG(inst)        (instruction_flags[inst]&IF_DEST_IS_REG?1:0)
#define IS_SHIFT_CONST_OR_CL(inst) (instruction_flags[inst]&IF_SHIFT_CONST_OR_CL?1:0)
#define INSTRUCTION_LIST__ \
    X(MOV,      "mov",          IF_NO_2_REG|IF_SIZED),          \
    X(RET,      "ret",          0),                             \
    X(NEG,      "neg",          IF_SIZED),                      \
    X(NOT,      "not",          IF_SIZED),                      \
    X(ADD,      "add",          IF_NO_2_REG|IF_SIZED),          \
    X(SUB,      "sub",          IF_NO_2_REG|IF_SIZED),          \
    X(MULT,     "imul",         IF_DEST_IS_REG|IF_SIZED),       \
    X(IDIV,     "idiv",         IF_SIZED),                      \
    X(CDQ,      "cdq",          0),                             \
    X(POP,      "pop",          IF_SIZED),                      \
    X(SAL,      "sal",          IF_SHIFT_CONST_OR_CL|IF_SIZED), \
    X(SAR,      "sar",          IF_SHIFT_CONST_OR_CL|IF_SIZED), \
    X(AND,      "and",          IF_NO_2_REG|IF_SIZED),          \
    X(OR,       "or",           IF_NO_2_REG|IF_SIZED),          \
    X(XOR,      "xor",          IF_NO_2_REG|IF_SIZED),          \

enum INST {
#define X(a,b,c) INST_##a
    INSTRUCTION_LIST__
#undef X
};
extern const enum INSTRUCTION_FLAGS instruction_flags[];
extern const char * const instruction_names[];

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
    X(CL,  "%cl"),          \
    X(CX,  "%ecx"),         \
    X(DX,  "%edx"),         \
    X(R10, "%r10d"),        \
    X(R11, "%r11d"),        \
    X(BP,  "%rbp"),         \
    X(SP,  "%rsp")

enum REGISTER {
#define X(r,n) REG_##r
    REGISTERS__
#undef X
};
extern char const * register_names[];

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
