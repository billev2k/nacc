//
// Created by Bill Evans on 11/3/24.
//

#include <string.h>

#include "amd64.h"

static int amd64_function_print(struct Amd64Function *amd64Function, FILE *out);
void amd64_program_emit(struct Amd64Program *amd64Program, FILE *out) {
    amd64_function_print(amd64Program->function, out);
}

#define inst_fmt "       %-8s"
static char * inst_op_fmt(enum OPCODE opcode, int nbytes) {
    static char buf[OPCODE_BUF_SIZE];
    strcpy(buf, opcode_names[opcode]);
    if (nbytes == 4) {
        strcat(buf, "l");
    } else if (nbytes == 8) {
        strcat(buf, "q");
    }
    return buf;
}

static int amd64_instruction_print(struct Amd64Instruction *instruction, FILE *out);
static int amd64_function_print(struct Amd64Function *amd64Function, FILE *out) {
    fprintf(out, "       .globl _%s\n", amd64Function->name);
    fprintf(out, "_%s:\n", amd64Function->name);
    fprintf(out, inst_fmt "%%rbp\n", "pushq");
    fprintf(out, inst_fmt "%%rsp, %%rbp\n", "movq");
    for (int ix=0; ix < amd64Function->instructions.num_items; ++ix) {
        struct Amd64Instruction *inst = amd64Function->instructions.items[ix];
        amd64_instruction_print(inst, out);
    }
    return 1;
}

static int amd64_operand_print(struct Amd64Operand operand, FILE *out);
static int amd64_instruction_print(struct Amd64Instruction *instruction, FILE *out) {
    enum OPCODE opcode = instruction->opcode;
    switch (instruction->instruction) {
        case INST_MOV:
            fprintf(out, inst_fmt, inst_op_fmt(opcode, 4));
            amd64_operand_print(instruction->mov.src, out);
            fprintf(out, ",");
            amd64_operand_print(instruction->mov.dst, out);
            fprintf(out, "\n");
            break;
        case INST_UNARY:
            fprintf(out, inst_fmt, inst_op_fmt(opcode, 4));
            amd64_operand_print(instruction->unary.operand, out);
            fprintf(out, "\n");
            break;
        case INST_BINARY:
            fprintf(out, inst_fmt, inst_op_fmt(instruction->opcode, 4));
            amd64_operand_print(instruction->binary.operand1, out);
            fprintf(out, ",");
            amd64_operand_print(instruction->binary.operand2, out);
            fprintf(out, "\n");
            break;
        case INST_CMP:
            fprintf(out, inst_fmt, inst_op_fmt(instruction->opcode, 4));
            amd64_operand_print(instruction->cmp.operand1, out);
            fprintf(out, ",");
            amd64_operand_print(instruction->cmp.operand2, out);
            fprintf(out, "\n");
            break;
        case INST_IDIV:
            fprintf(out, inst_fmt, inst_op_fmt(instruction->opcode, 4));
            amd64_operand_print(instruction->idiv.operand, out);
            fprintf(out, "\n");
            break;
        case INST_CDQ:
            fprintf(out, inst_fmt "\n", inst_op_fmt(instruction->opcode, 0));
            break;
        case INST_JMP:
            fprintf(out, inst_fmt "%s\n", inst_op_fmt(instruction->opcode, 0), instruction->jmp.identifier.name);
            break;
        case INST_JMPCC:
            fprintf(out, inst_fmt "%s\n", inst_op_fmt(instruction->opcode, 0), instruction->jmpcc.identifier.name);
            break;
        case INST_SETCC:
            fprintf(out, inst_fmt, inst_op_fmt(instruction->opcode, 0));
            amd64_operand_print(instruction->setcc.operand, out);
            fprintf(out, "\n");
            break;
        case INST_LABEL:
            fprintf(out, "%s:\n", instruction->label.identifier.name);
            break;
        case INST_ALLOC_STACK:
            fprintf(out, inst_fmt "$%d, %%rsp\n", "subq", instruction->stack.bytes);
            break;
        case INST_RET:
            fprintf(out, "       # epilog\n");
            fprintf(out, inst_fmt "%%rbp, %%rsp\n", inst_op_fmt(OPCODE_MOV, 8));
            fprintf(out, inst_fmt "%%rbp\n", inst_op_fmt(OPCODE_POP, 8));
            fprintf(out, "       ret\n");
            break;
        case INST_COMMENT:
            fprintf(out, "     # %s\n", instruction->comment.text);
            break;
    }
    return 1;
}

static int amd64_operand_print(struct Amd64Operand operand, FILE *out) {
    switch (operand.operand_type) {
        case OPERAND_IMM_LIT:
            fprintf(out, "$%s", operand.name);
            break;
        case OPERAND_REGISTER:
            fprintf(out, "%s", register_names[operand.reg]);
            break;
        case OPERAND_PSEUDO:
            fprintf(out, "%%%s", operand.name);
            break;
        case OPERAND_LABEL:
            fprintf(out, "%s", operand.name);
            break;
        case OPERAND_STACK:
            fprintf(out, "%d(%%rbp)", operand.offset);
            break;
        case OPERAND_NONE:
            break;
    }
    return 1;
}
