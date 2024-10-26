//
// Created by Bill Evans on 8/31/24.
//

#include <stdlib.h>
#include "amd64.h"

struct Amd64Operand amd64_operand_none = {.operand_type = OPERAND_NONE};
char const * register_names[] = {
#define X(r,n) n
        REGISTERS__
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

static int amd64_function_print(struct Amd64Function *amd64Function, FILE *out);
void amd64_program_emit(struct Amd64Program *amd64Program, FILE *out) {
    amd64_function_print(amd64Program->function, out);
}

#define inst_fmt "       %-8s"

static int amd64_instruction_print(struct Amd64Instruction *inst, FILE *out);
static int amd64_function_print(struct Amd64Function *amd64Function, FILE *out) {
    fprintf(out, "       .globl _%s\n", amd64Function->name);
    fprintf(out, "_%s:\n", amd64Function->name);
    fprintf(out, inst_fmt "%%rbp\n", "pushq");
    fprintf(out, inst_fmt "%%rsp, %%rbp\n", "movq");
    if (amd64Function->stack_allocations) {
        fprintf(out, inst_fmt "$%d, %%rsp\n", "subq", amd64Function->stack_allocations);
    }
    for (int ix=0; ix < amd64Function->instructions.list_count; ++ix) {
        struct Amd64Instruction *inst = amd64Function->instructions.items[ix];
        amd64_instruction_print(inst, out);
    }
    return 1;
}


static int amd64_operand_print(struct Amd64Operand operand, FILE *out);
static int amd64_instruction_print(struct Amd64Instruction *inst, FILE *out) {
    switch (inst->instruction) {
        case INST_MOV:
            fprintf(out, inst_fmt, "movl");
            amd64_operand_print(inst->src, out);
            fprintf(out, ",");
            amd64_operand_print(inst->dst, out);
            fprintf(out, "\n");
            break;
        case INST_RET:
            fprintf(out, inst_fmt "%%rbp, %%rsp\n", "movq");
            fprintf(out, inst_fmt "%%rbp\n", "popq");
            fprintf(out, "       ret\n");
            break;
        case INST_NEG:
            fprintf(out, "       negl    ");
            amd64_operand_print(inst->src, out);
            fprintf(out, "\n");
            break;
        case INST_NOT:
            fprintf(out, "       notl    ");
            amd64_operand_print(inst->src, out);
            fprintf(out, "\n");
            break;
    }
    return 1;
}
static int amd64_operand_print(struct Amd64Operand operand, FILE *out) {
    switch (operand.operand_type) {
        case OPERAND_IMM_LIT:
            fprintf(out, "$%s", operand.name);
            break;
        case OPERAND_IMM_CONST:
            fprintf(out, "$%d", operand.value);
            break;
        case OPERAND_REGISTER:
            fprintf(out, "%s", register_names[operand.reg]);
            break;
        case OPERAND_PSEUDO:
            fprintf(out, "%%%s", operand.name);
            break;
        case OPERAND_STACK:
            fprintf(out, "%d(%%rbp)", operand.offset);
            break;
        case OPERAND_NONE:
            break;
    }
    return 1;
}
