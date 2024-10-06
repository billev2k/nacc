//
// Created by Bill Evans on 8/31/24.
//

#include <stdlib.h>
#include "amd64.h"

DYN_LIST_OF_P_IMPL(Amd64Instruction)

void amd64_program_free(struct Amd64Program *program) {
    if (program->function) amd64_function_free(program->function);
    free(program);
}

void amd64_function_append_instruction(struct Amd64Function *function, struct Amd64Instruction *instruction) {
    Amd64Instruction_list_append(&function->instructions, instruction);
}
void amd64_function_free(struct Amd64Function *function) {
    Amd64Instruction_list_free(&function->instructions);
    free(function);
}

void Amd64Instruction_free(struct Amd64Instruction *instruction) {
    if (instruction->src) amd64_operand_free(instruction->src);
    if (instruction->dst) amd64_operand_free(instruction->dst);
    free(instruction);
}

void amd64_operand_free(struct Amd64Operand *operand) {
    free(operand);
}

static int amd64_function_print(struct Amd64Function *Amd64Function, FILE *out);
void amd64_program_emit(struct Amd64Program *asmProgram, FILE *out) {
    amd64_function_print(asmProgram->function, out);
}

static int amd64_instruction_print(struct Amd64Instruction *inst, FILE *out);
static int amd64_function_print(struct Amd64Function *asmFunction, FILE *out) {
    fprintf(out, "       .globl _%s\n", asmFunction->name);
    fprintf(out, "_%s:\n", asmFunction->name);
    for (int ix=0; ix<asmFunction->instructions.list_count; ++ix) {
        struct Amd64Instruction *inst = asmFunction->instructions.items[ix];
        amd64_instruction_print(inst, out);
    }
    return 1;
}

static int amd64_operand_print(struct Amd64Operand *opnd, FILE *out);
static int amd64_instruction_print(struct Amd64Instruction *inst, FILE *out) {
    switch (inst->instruction) {
        case INST_MOV:
            fprintf(out, "       movl    ");
            amd64_operand_print(inst->src, out);
            fprintf(out, ",");
            amd64_operand_print(inst->dst, out);
            fprintf(out, "\n");
            break;
        case INST_RET:
            fprintf(out, "       ret\n");
            break;
    }
    return 1;
}
static int amd64_operand_print(struct Amd64Operand *opnd, FILE *out) {
    switch (opnd->operand_type) {
        case OPND_IMM:
            fprintf(out, "$%s", opnd->name);
            break;
        case OPND_REGISTER:
            fprintf(out, "%%%s", opnd->name);
            break;
    }
    return 1;
}
