//
// Created by Bill Evans on 11/3/24.
//

#include <string.h>
#include <assert.h>

#include "amd64.h"

static int amd64_function_print(struct Amd64Function *amd64Function, FILE *out);
void amd64_program_emit(struct Amd64Program *amd64Program, FILE *out) {
    for (int ix=0; ix < amd64Program->functions.num_items; ++ix) {
        struct Amd64Function *amd64Function = amd64Program->functions.items[ix];
        amd64_function_print(amd64Function, out);
    }
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
    fprintf(out, "\n       .globl _%s\n", amd64Function->name);
    fprintf(out, "_%s:\n", amd64Function->name);
    fprintf(out, inst_fmt "%%rbp\n", "pushq");
    fprintf(out, inst_fmt "%%rsp, %%rbp\n", "movq");
    for (int ix=0; ix < amd64Function->instructions.num_items; ++ix) {
        struct Amd64Instruction *inst = amd64Function->instructions.items[ix];
        amd64_instruction_print(inst, out);
    }
    return 1;
}

static char* operand_fmt(char* buf, enum OPCODE op, struct Amd64Operand operand, int operand_no, int operand_size);
static int amd64_instruction_print(struct Amd64Instruction *instruction, FILE *out) {
    const int OPERAND_BUF_SIZE = 128;
    char buf1[OPERAND_BUF_SIZE];
    char buf2[OPERAND_BUF_SIZE];
    enum OPCODE opcode = instruction->opcode;
    switch (instruction->instruction) {
        case INST_MOV:
            fprintf(out, inst_fmt "%s, %s\n",
                    inst_op_fmt(opcode, 4),
                    operand_fmt(buf1, opcode, instruction->operand1, 0, 4),
                    operand_fmt(buf2, opcode, instruction->operand2, 1, 4) );
            break;
        case INST_UNARY:
            fprintf(out, inst_fmt "%s\n",
                    inst_op_fmt(opcode, 4),
                    operand_fmt(buf1, opcode, instruction->operand1, 0, 4) );
            break;
        case INST_BINARY:
            fprintf(out, inst_fmt "%s, %s\n",
                    inst_op_fmt(opcode, 4),
                    operand_fmt(buf1, opcode, instruction->operand1, 0, 4),
                    operand_fmt(buf2, opcode, instruction->operand2, 1, 4) );
            break;
        case INST_CMP:
            fprintf(out, inst_fmt "%s, %s\n",
                    inst_op_fmt(opcode, 4),
                    operand_fmt(buf1, opcode, instruction->operand1, 0, 4),
                    operand_fmt(buf2, opcode, instruction->operand2, 1, 4) );
            break;
        case INST_IDIV:
            fprintf(out, inst_fmt "%s\n",
                    inst_op_fmt(opcode, 4),
                    operand_fmt(buf1, opcode, instruction->operand1, 0, 4) );
            break;
        case INST_CDQ:
            fprintf(out, inst_fmt "\n", inst_op_fmt(instruction->opcode, 0));
            break;
        case INST_JMP:
            fprintf(out, inst_fmt "%s\n", inst_op_fmt(instruction->opcode, 0), instruction->operand1.name);
            break;
        case INST_JMPCC:
            fprintf(out, inst_fmt "%s\n", inst_op_fmt(instruction->opcode, 0), instruction->operand1.name);
            break;
        case INST_SETCC:
            fprintf(out, inst_fmt "%s\n",
                    inst_op_fmt(opcode, 0),
                    operand_fmt(buf1, opcode, instruction->operand1, 0, 4) );
            break;
        case INST_LABEL:
            fprintf(out, "%s:\n", instruction->operand1.name);
            break;
        case INST_ALLOC_STACK:
            fprintf(out, inst_fmt "$%d, %%rsp\n", "subq", instruction->bytes);
            break;
        case INST_RET:
            fprintf(out, "       # epilog\n");
            fprintf(out, inst_fmt "%%rbp, %%rsp\n", inst_op_fmt(OPCODE_MOV, 8));
            fprintf(out, inst_fmt "%%rbp\n", inst_op_fmt(OPCODE_POP, 8));
            fprintf(out, "       ret\n");
            break;
        case INST_COMMENT:
            fprintf(out, "     # %s\n", instruction->text);
            break;
        case INST_CALL:
            fprintf(out, inst_fmt "_%s\n",
                    inst_op_fmt(instruction->opcode, 0),
                    instruction->operand1.name);
            break;
        case INST_DEALLOC_STACK:
            fprintf(out, inst_fmt "$%d, %%rsp\n", "addq", instruction->bytes);
            break;
        case INST_PUSH:
            fprintf(out, inst_fmt "%s\n",
                    inst_op_fmt(instruction->opcode, 8),
                    operand_fmt(buf1, opcode, instruction->operand1, 0, 8));
            break;
    }
    return 1;
}

static char* operand_fmt(char* buf, enum OPCODE op, struct Amd64Operand operand, int operand_no, int operand_size) {
    int size_ix;
    switch (operand.operand_kind) {
        case OPERAND_IMM_INT:
            sprintf(buf, "$%d", operand.int_val);
            break;
        case OPERAND_REGISTER:
            switch (operand_size) {
                case 1: size_ix = 3; break;
                case 2: size_ix = 2; break;
                case 4: size_ix = 1; break;
                case 8: size_ix = 0; break;
                default:
                    assert(0 && "invalid operand1 size");
            }
            // No register other than CL can hold a shift amount.
            if ((op == OPCODE_SAR || op == OPCODE_SAL) && operand_no == 0 && operand.reg==REG_CX) {
                size_ix = 3;
            } else if (op >= OPCODE_SETE && op <= OPCODE_SETGE) {
                // SET CC needs an 8 bit register
                size_ix = 3;
            }
            sprintf(buf, "%s", register_names[operand.reg][size_ix]);
            break;
        case OPERAND_PSEUDO:
            sprintf(buf, "%%%s", operand.name);
            break;
        case OPERAND_LABEL:
            sprintf(buf, "%s", operand.name);
            break;
        case OPERAND_STACK:
            sprintf(buf, "%d(%%rbp)", operand.offset);
            break;
        case OPERAND_NONE:
            break;
        case OPERAND_FUNC:
            break;
    }

    return buf;
}

