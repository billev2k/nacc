//
// Created by Bill Evans on 8/31/24.
//

#include <stdlib.h>
#include "asm.h"

void asm_program_free(struct AsmProgram *program) {
    if (program->function) asm_function_free(program->function);
    free(program);
}

void asm_function_append_instruction(struct AsmFunction *function, struct AsmInstruction *instruction) {
    if (function->instructionCount == function->instructionsSize) {
        function->instructionsSize = function->instructionsSize ? function->instructionsSize*2 : 8;
        struct AsmInstruction **newInstructions = malloc(function->instructionsSize * sizeof(struct AsmInstruction*));
        for (int i=0; i<function->instructionCount; ++i)
            newInstructions[i] = function->instructions[i];
        free(function->instructions);
        function->instructions = newInstructions;
    }
    function->instructions[function->instructionCount++] = instruction;
}
void asm_function_free(struct AsmFunction *function) {
    for (int i=0; i<function->instructionCount; ++i)
        asm_instruction_free(function->instructions[i]);
    free(function);
}

void asm_instruction_free(struct AsmInstruction *instruction) {
    if (instruction->src) asm_operand_free(instruction->src);
    if (instruction->dst) asm_operand_free(instruction->dst);
    free(instruction);
}

void asm_operand_free(struct AsmOperand *operand) {
    free(operand);
}

static int asm_function_print(struct AsmFunction *AsmFunction, FILE *out);
void asm_program_emit(struct AsmProgram *asmProgram, FILE *out) {
    asm_function_print(asmProgram->function, out);
}

static int asm_instruction_print(struct AsmInstruction *inst, FILE *out);
static int asm_function_print(struct AsmFunction *asmFunction, FILE *out) {
    fprintf(out, "       .globl _%s\n", asmFunction->name);
    fprintf(out, "_%s:\n", asmFunction->name);
    for (int ix=0; ix<asmFunction->instructionCount; ++ix) {
        struct AsmInstruction *inst = asmFunction->instructions[ix];
        asm_instruction_print(inst, out);
    }
    return 1;
}

static int asm_operand_print(struct AsmOperand *opnd, FILE *out);
static int asm_instruction_print(struct AsmInstruction *inst, FILE *out) {
    switch (inst->instruction) {
        case INST_MOV:
            fprintf(out, "       movl    ");
            asm_operand_print(inst->src, out);
            fprintf(out, ",");
            asm_operand_print(inst->dst, out);
            fprintf(out, "\n");
            break;
        case INST_RET:
            fprintf(out, "       ret\n");
            break;
    }
    return 1;
}
static int asm_operand_print(struct AsmOperand *opnd, FILE *out) {
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
