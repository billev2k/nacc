//
// Created by Bill Evans on 8/31/24.
//

#include <stdlib.h>
#include <string.h>
#include "amd64.h"

struct Amd64Operand amd64_operand_none = {.operand_type = OPERAND_NONE};

enum OPCODE opcode_for_unary_op(enum UNARY_OP op) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
    switch (op) {
        case UNARY_OP_NEGATE:
            return OPCODE_NEG;
        case UNARY_OP_COMPLEMENT:
            return OPCODE_NOT;
    }
#pragma clang diagnostic pop
    fprintf(stderr, "Internal error: Unexpected UNARY_OP.\n");
    exit(1);
}

enum OPCODE opcode_for_binary_op(enum BINARY_OP op) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
    switch (op) {
        case BINARY_OP_ADD:
            return OPCODE_ADD;
        case BINARY_OP_SUBTRACT:
            return OPCODE_SUB;
        case BINARY_OP_MULTIPLY:
            return OPCODE_MULT;
        case BINARY_OP_OR:
            return OPCODE_OR;
        case BINARY_OP_AND:
            return OPCODE_AND;
        case BINARY_OP_XOR:
            return OPCODE_XOR;
        case BINARY_OP_LSHIFT:
            return OPCODE_SAL;
        case BINARY_OP_RSHIFT:
            return OPCODE_SAR;
    }
#pragma clang diagnostic pop
    fprintf(stderr, "Internal error: Unexpected BINARY_OP.\n");
    exit(1);
}

char const * register_names[] = {
#define X(r,n) n
        REGISTERS__
#undef X
};

const char * const opcode_names[] = {
#define X(a,b) b
        OPCODE_LIST__
#undef X
};

const char* const cond_code_suffix[] = {
#define X(a,b,c,d) #c
    COND_CODE_LIST__
#undef X
};
const enum OPCODE cond_code_to_jXX[] = {
#define X(a,b,c,d) OPCODE_J##d
    COND_CODE_LIST__
#undef X
};
const enum OPCODE cond_code_to_setXX[] = {
#define X(a,b,c,d) OPCODE_SET##d
        COND_CODE_LIST__
#undef X
};

struct list_of_Amd64Instruction_helpers Amd64Helpers = {
    .free = Amd64Instruction_free
};
LIST_OF_ITEM_DEFN(Amd64Instruction, struct Amd64Instruction*, Amd64Helpers)


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
    list_of_Amd64Instruction_append(&function->instructions, instruction);
}
void amd64_function_free(struct Amd64Function *function) {
    list_of_Amd64Instruction_free(&function->instructions);
    free(function);
}

static struct Amd64Instruction* amd64_instruction_new(enum INSTRUCTION instruction, enum OPCODE opcode) {
    struct Amd64Instruction* inst = (struct Amd64Instruction *)malloc(sizeof(struct Amd64Instruction));
    inst->instruction = instruction;
    inst->opcode = opcode;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_mov(struct Amd64Operand src, struct Amd64Operand dst) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_MOV, OPCODE_MOV);
    inst->mov.src = src;
    inst->mov.dst = dst;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_unary(enum UNARY_OP op, struct Amd64Operand operand) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_UNARY, opcode_for_unary_op(op));
    inst->unary.op = op;
    inst->unary.operand = operand;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_binary(enum BINARY_OP op, struct Amd64Operand operand1, struct Amd64Operand operand2) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_BINARY, opcode_for_binary_op(op));
    inst->binary.op = op;
    inst->binary.operand1 = operand1;
    inst->binary.operand2 = operand2;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_cmp(struct Amd64Operand operand1, struct Amd64Operand operand2) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_CMP, OPCODE_CMP);
    inst->cmp.operand1 = operand1;
    inst->cmp.operand2 = operand2;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_idiv(struct Amd64Operand operand) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_IDIV, OPCODE_IDIV);
    inst->idiv.operand = operand;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_cdq() {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_CDQ, OPCODE_CDQ);
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_jmp(struct Amd64Operand identifier) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_JMP, OPCODE_JMP);
    inst->jmp.identifier = identifier;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_jmpcc(enum COND_CODE cc, struct Amd64Operand identifier) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_JMPCC, cond_code_to_jXX[cc]);
    inst->jmpcc.cc = cc;
    inst->jmpcc.identifier = identifier;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_setcc(enum COND_CODE cc, struct Amd64Operand operand) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_SETCC, cond_code_to_setXX[cc]);
    inst->setcc.cc = cc;
    inst->setcc.operand = operand;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_label(struct Amd64Operand identifier) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_LABEL, OPCODE_NONE);
    inst->label.identifier = identifier;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_alloc_stack(int bytes) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_ALLOC_STACK, OPCODE_NONE);
    inst->stack.bytes = bytes;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_ret() {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_RET, OPCODE_RET);
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
struct Amd64Operand amd64_operand_label(const char* label) {
    struct Amd64Operand pseudo_operand = {
            .operand_type = OPERAND_LABEL,
            .name = label
    };
    return pseudo_operand;
};

