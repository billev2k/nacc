//
// Created by Bill Evans on 8/31/24.
//

#include <stdlib.h>
#include <string.h>
#include "amd64.h"

struct Amd64Operand amd64_operand_none = {.operand_type = OPERAND_NONE};
char const * register_names[] = {
#define X(r,n) n
        REGISTERS__
#undef X
};
const char * const instruction_names[] = {
#define X(a,b,c) b
    INSTRUCTION_LIST__
#undef X
};
const enum INSTRUCTION_FLAGS instruction_flags[] = {
#define X(a,b,c) c
    INSTRUCTION_LIST__
#undef X
};

DYN_LIST_OF_P_IMPL(Amd64Instruction)

struct Amd64Program* amd64_program_new(void ) {
    struct Amd64Program *result = (struct Amd64Program *)malloc(sizeof(struct Amd64Program));
    return result;
}
void amd64_program_free(struct Amd64Program *program) {
    if (program->function) amd64_function_free(program->function);
    free(program);
}

struct Amd64Function* amd64_function_new(const char* name) {
    struct Amd64Function* result = (struct Amd64Function*)malloc(sizeof(struct Amd64Function));
    result->name = name;
    return result;
}
void amd64_function_append_instruction(struct Amd64Function *function, struct Amd64Instruction *instruction) {
    Amd64Instruction_list_append(&function->instructions, instruction);
}
void amd64_function_free(struct Amd64Function *function) {
    Amd64Instruction_list_free(&function->instructions);
    free(function);
}

struct Amd64Instruction* amd64_instruction_new(enum INST instruction, struct Amd64Operand src, struct Amd64Operand dst) {
    struct Amd64Instruction* inst = (struct Amd64Instruction *)malloc(sizeof(struct Amd64Instruction));
    inst->instruction = instruction;
    inst->src = src;
    inst->dst = dst;
    return inst;
}
void Amd64Instruction_free(struct Amd64Instruction *instruction) {
    free(instruction);
}

struct Amd64Operand amd64_operand_imm_literal(const char* value_str) {
    struct Amd64Operand imm_operand = {
        .operand_type = OPERAND_IMM_LIT,
        .name = value_str
    };
    return imm_operand;
}
struct Amd64Operand amd64_operand_imm_const(int value) {
    struct Amd64Operand imm_operand = {
            .operand_type = OPERAND_IMM_CONST,
            .value = value
    };
    return imm_operand;
}
struct Amd64Operand amd64_operand_reg(enum REGISTER reg) {
    struct Amd64Operand reg_operand = {
            .operand_type = OPERAND_REGISTER,
            .reg = reg
    };
    return reg_operand;
};
struct Amd64Operand amd64_operand_pseudo(const char* pseudo_name) {
    struct Amd64Operand pseudo_operand = {
            .operand_type = OPERAND_PSEUDO,
            .name = pseudo_name
    };
    return pseudo_operand;
};
struct Amd64Operand amd64_operand_stack(int offset) {
    struct Amd64Operand stack_operand = {
            .operand_type = OPERAND_STACK,
            .offset = offset
    };
    return stack_operand;
};

