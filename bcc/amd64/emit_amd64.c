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
static char * inst_op_fmt(enum INST inst, int nbytes, char *dst) {
    strcpy(dst, instruction_names[inst]);
    if (IS_SIZED(inst)) {
        if (nbytes == 4) {
            strcat(dst, "l");
        } else if (nbytes == 8) {
            strcat(dst, "q");
        }
    }
    return dst;
}

static int amd64_instruction_print(struct Amd64Instruction *inst, FILE *out);
static int amd64_function_print(struct Amd64Function *amd64Function, FILE *out) {
    fprintf(out, "       .globl _%s\n", amd64Function->name);
    fprintf(out, "_%s:\n", amd64Function->name);
    fprintf(out, inst_fmt "%%rbp\n", "pushq");
    fprintf(out, inst_fmt "%%rsp, %%rbp\n", "movq");
    if (amd64Function->stack_allocations) {
        fprintf(out, inst_fmt "$%d, %%rsp\n", "subq", amd64Function->stack_allocations);
    }
    fprintf(out, "       # end prolog\n");
    for (int ix=0; ix < amd64Function->instructions.list_count; ++ix) {
        struct Amd64Instruction *inst = amd64Function->instructions.items[ix];
        amd64_instruction_print(inst, out);
    }
    return 1;
}

static int amd64_operand_print(struct Amd64Operand operand, FILE *out);
static int amd64_instruction_print(struct Amd64Instruction *inst, FILE *out) {
    char op_buf[12];
    char op_fmt_buf[12];
    switch (inst->instruction) {
        case INST_MOV:
            fprintf(out, inst_fmt, inst_op_fmt(INST_MOV, 4, op_fmt_buf));
            amd64_operand_print(inst->src, out);
            fprintf(out, ",");
            amd64_operand_print(inst->dst, out);
            fprintf(out, "\n");
            break;
        case INST_RET:
            fprintf(out, "       # epilog\n");
            fprintf(out, inst_fmt "%%rbp, %%rsp\n", inst_op_fmt(INST_MOV, 8, op_fmt_buf));
            fprintf(out, inst_fmt "%%rbp\n", inst_op_fmt(INST_POP, 8, op_fmt_buf));
            fprintf(out, "       ret\n");
            break;
        default:
            strcpy(op_buf, instruction_names[inst->instruction]);
            fprintf(out, inst_fmt, inst_op_fmt(inst->instruction, 4, op_fmt_buf));
            if (inst->src.operand_type != OPERAND_NONE) {
                amd64_operand_print(inst->src, out);
                if (inst->dst.operand_type != OPERAND_NONE) {
                    fprintf(out, ",");
                    amd64_operand_print(inst->dst, out);
                }
            }
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
