//
// Created by Bill Evans on 8/31/24.
//

#include <stdlib.h>
#include <string.h>
#include "amd64.h"

struct Amd64Operand amd64_operand_none = {.operand_kind = OPERAND_NONE};

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

// TODO: Need all 4 names for every register.
char const * register_names[16][4] = {
#define X(n, r8, r4, r2, r1) { r8, r4, r2, r1 }
        REGISTERS__
#undef X
};

const char * const opcode_names[] = {
#define X(a,b,c) b
        OPCODE_LIST__
#undef X
};
const unsigned char opcode_num_operands[] = {
#define X(a,b,c) c
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

struct list_of_Amd64Instruction_helpers list_of_Amd64Instruction_helpers = {
    .delete = Amd64Instruction_delete
};
#define NAME list_of_Amd64Instruction
#define TYPE struct Amd64Instruction*
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

#define NAME list_of_Amd64Function
#define TYPE struct Amd64Function*
struct list_of_Amd64Function_helpers list_of_Amd64Function_helpers = {
        .delete = amd64_function_delete,
};
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

struct Amd64Program* amd64_program_new(void ) {
    struct Amd64Program *result = (struct Amd64Program *)malloc(sizeof(struct Amd64Program));
    list_of_Amd64Function_init(&result->functions, 10);
    return result;
}
void amd64_program_add_function(struct Amd64Program* program, struct Amd64Function* function) {
    list_of_Amd64Function_append(&program->functions, function);
}
void amd64_program_delete(struct Amd64Program *program) {
    list_of_Amd64Function_delete(&program->functions);
    free(program);
}

struct Amd64Function* amd64_function_new(const char* name) {
    struct Amd64Function* result = (struct Amd64Function*)malloc(sizeof(struct Amd64Function));
    result->name = name;
    list_of_Amd64Instruction_init(&result->instructions, 101);
    return result;
}
void amd64_function_append_instruction(struct Amd64Function *function, struct Amd64Instruction *instruction) {
    list_of_Amd64Instruction_append(&function->instructions, instruction);
}
void amd64_function_delete(struct Amd64Function *function) {
    list_of_Amd64Instruction_delete(&function->instructions);
    free(function);
}

static struct Amd64Instruction* amd64_instruction_new(enum INSTRUCTION instruction, enum OPCODE opcode) {
    struct Amd64Instruction* inst = (struct Amd64Instruction *)malloc(sizeof(struct Amd64Instruction));
    inst->instruction = instruction;
    inst->opcode = opcode;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_call(struct Amd64Operand identifier) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_CALL, OPCODE_CALL);
    inst->operand1 = identifier;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_mov(struct Amd64Operand src, struct Amd64Operand dst) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_MOV, OPCODE_MOV);
    inst->operand1 = src;
    inst->operand2 = dst;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_unary(enum UNARY_OP op, struct Amd64Operand operand) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_UNARY, opcode_for_unary_op(op));
    inst->unary_op = op;
    inst->operand1 = operand;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_binary(enum BINARY_OP op, struct Amd64Operand operand1, struct Amd64Operand operand2) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_BINARY, opcode_for_binary_op(op));
    inst->binary_op = op;
    inst->operand1 = operand1;
    inst->operand2 = operand2;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_cmp(struct Amd64Operand operand1, struct Amd64Operand operand2) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_CMP, OPCODE_CMP);
    inst->operand1 = operand1;
    inst->operand2 = operand2;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_comment(const char *text) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_COMMENT, 0 /* None */);
    inst->text = text;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_idiv(struct Amd64Operand operand) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_IDIV, OPCODE_IDIV);
    inst->operand1 = operand;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_cdq() {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_CDQ, OPCODE_CDQ);
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_jmp(struct Amd64Operand identifier) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_JMP, OPCODE_JMP);
    inst->operand1 = identifier;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_jmpcc(enum COND_CODE cc, struct Amd64Operand identifier) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_JMPCC, cond_code_to_jXX[cc]);
    inst->cc = cc;
    inst->operand1 = identifier;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_setcc(enum COND_CODE cc, struct Amd64Operand operand) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_SETCC, cond_code_to_setXX[cc]);
    inst->cc = cc;
    inst->operand1 = operand;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_label(struct Amd64Operand identifier) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_LABEL, OPCODE_NONE);
    inst->operand1 = identifier;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_alloc_stack(int bytes) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_ALLOC_STACK, OPCODE_NONE);
    inst->bytes = bytes;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_dealloc_stack(int bytes) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_DEALLOC_STACK, OPCODE_NONE);
    inst->bytes = bytes;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_push(struct Amd64Operand operand) {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_PUSH, OPCODE_PUSH);
    inst->operand1 = operand;
    return inst;
}
struct Amd64Instruction* amd64_instruction_new_ret() {
    struct Amd64Instruction* inst = amd64_instruction_new(INST_RET, OPCODE_RET);
    return inst;
}

void Amd64Instruction_delete(struct Amd64Instruction *instruction) {
    free(instruction);
}

struct Amd64Operand amd64_operand_imm_int(int int_val) {
    struct Amd64Operand imm_operand = {
        .operand_kind = OPERAND_IMM_INT,
        .int_val = int_val
    };
    return imm_operand;
}
struct Amd64Operand amd64_operand_reg(enum REGISTER reg) {
    struct Amd64Operand reg_operand = {
            .operand_kind = OPERAND_REGISTER,
            .reg = reg
    };
    return reg_operand;
};
struct Amd64Operand amd64_operand_pseudo(const char* pseudo_name) {
    struct Amd64Operand pseudo_operand = {
            .operand_kind = OPERAND_PSEUDO,
            .name = pseudo_name
    };
    return pseudo_operand;
};
struct Amd64Operand amd64_operand_stack(int offset) {
    struct Amd64Operand stack_operand = {
            .operand_kind = OPERAND_STACK,
            .offset = offset
    };
    return stack_operand;
};
struct Amd64Operand amd64_operand_func(const char* func_name) {
    struct Amd64Operand func_operand = {
            .operand_kind = OPERAND_FUNC,
            .name = func_name
    };
    return func_operand;
}
struct Amd64Operand amd64_operand_label(const char* label) {
    struct Amd64Operand pseudo_operand = {
            .operand_kind = OPERAND_LABEL,
            .name = label
    };
    return pseudo_operand;
};

