//
// Created by Bill Evans on 8/31/24.
//

#ifndef BCC_AMD64_H
#define BCC_AMD64_H

#include <stdio.h>
#include "../ir/ir.h"
#include "../utils/utils.h"

enum INSTRUCTION {
    INST_ALLOC_STACK,
    INST_BINARY,
    INST_CALL,
    INST_CDQ,
    INST_CMP,
    INST_COMMENT,
    INST_DEALLOC_STACK,
    INST_IDIV,
    INST_JMP,
    INST_JMPCC,
    INST_LABEL,
    INST_MOV,
    INST_PUSH,
    INST_RET,
    INST_SETCC,
    INST_UNARY,
};

enum UNARY_OP {
#define X(a,b) UNARY_OP_##a
    IR_UNARY_OP_LIST__
#undef X
};

enum BINARY_OP {
#define X(a,b) BINARY_OP_##a
    IR_BINARY_OP_LIST__
#undef X
};

#define COND_CODE_LIST__ \
    X(EQ,   "eq",   e, E),       \
    X(NE,   "ne",   ne, NE),      \
    X(GT,   "gt",   g, G),       \
    X(GE,   "ge",   ge, GE),      \
    X(LT,   "lt",   l, L),       \
    X(LE,   "le",   le, LE),

enum COND_CODE {
#define X(a,b,c,d) CC_##a
    COND_CODE_LIST__
#undef X
};
extern const char* const cond_code_suffix[];

// Large enough for any opcode + suffix
#define OPCODE_BUF_SIZE 8
#define OPCODE_LIST__ \
    X(CALL,     "call",     1),         \
    X(MOV,      "mov",      2),         \
    X(RET,      "ret",      0),         \
    X(NEG,      "neg",      1),         \
    X(NOT,      "not",      1),         \
    X(ADD,      "add",      2),         \
    X(SUB,      "sub",      2),         \
    X(MULT,     "imul",     2),         \
    X(IDIV,     "idiv",     1),         \
    X(CDQ,      "cdq",      1),         \
    X(PUSH,     "push",     1),         \
    X(POP,      "pop",      1),         \
    X(SAL,      "sal",      2),         \
    X(SAR,      "sar",      2),         \
    X(AND,      "and",      2),         \
    X(OR,       "or",       2),         \
    X(XOR,      "xor",      2),         \
    X(CMP,      "cmp",      2),         \
    X(JMP,      "jmp",      1),         \
    X(JE,       "je",       1),         \
    X(JNE,      "jne",      1),         \
    X(JL,       "jl",       1),         \
    X(JLE,      "jle",      1),         \
    X(JG,       "jg",       1),         \
    X(JGE,      "jge",      1),         \
    X(SETE,     "sete",     1),         \
    X(SETNE,    "setne",    1),         \
    X(SETL,     "setl",     1),         \
    X(SETLE,    "setle",    1),         \
    X(SETG,     "setg",     1),         \
    X(SETGE,    "setge",    1),         

enum OPCODE {
#define X(a,b,c) OPCODE_##a
    OPCODE_LIST__
#undef X
    OPCODE_NONE,
};
extern const char * const opcode_names[];
extern const unsigned char opcode_num_operands[];

extern const enum OPCODE cond_code_to_jXX[];
extern const enum OPCODE cond_code_to_setXX[];

enum OPERAND {
    OPERAND_NONE,
    OPERAND_IMM_INT,
    OPERAND_REGISTER,
    OPERAND_PSEUDO,
    OPERAND_STACK,
    OPERAND_LABEL,
    OPERAND_FUNC,
};

// The registers are all named by their popular names, so "AX", "SP, "R10" are 2, 2, and 8 bytes
// The names are for 8, 4, 2, and 1 byte sizes, so rax==8 bytes, sil==1 byte
#define REGISTERS__ \
    X(AX,       "%rax",   "%eax",    "%ax",     "%al"),     \
    X(BX,       "%rbx",   "%ebx",    "%bx",     "%bl"),     \
    X(CX,       "%rcx",   "%ecx",    "%cx",     "%cl"),     \
    X(DX,       "%rdx",   "%edx",    "%dx",     "%dl"),     \
    X(SI,       "%rsi",   "%esi",    "%si",     "%sil"),    \
    X(DI,       "%rdi",   "%edi",    "%di",     "%dil"),    \
    X(BP,       "%rbp",   "%ebp",    "%bp",     "%bpl"),    \
    X(SP,       "%rsp",   "%esp",    "%sp",     "%spl"),    \
    X(R8,       "%r8",    "%r8d",    "%r8w",    "%r8b"),    \
    X(R9,       "%r9",    "%r9d",    "%r9w",    "%r9b"),    \
    X(R10,      "%r10",   "%r10d",   "%r10w",   "%r10b"),   \
    X(R11,      "%r11",   "%r11d",   "%r11w",   "%r11b"),   \
    X(R12,      "%r12",   "%r12d",   "%r12w",   "%r12b"),   \
    X(R13,      "%r13",   "%r13d",   "%r13w",   "%r13b"),   \
    X(R14,      "%r14",   "%r14d",   "%r14w",   "%r14b"),   \
    X(R15,      "%r15",   "%r15d",   "%r15w",   "%r15b"),


enum REGISTER {
#define X(n, r8, r4, r2, r1) REG_##n
    REGISTERS__
#undef X
};
extern char const * register_names[16][4];

//region struct Amd64Operand
struct Amd64Operand {
    enum OPERAND operand_kind;
    union {
        const char *name;
        int offset;
        enum REGISTER reg;
        int int_val;
    };
};
extern struct Amd64Operand amd64_operand_func(const char* func_name);
extern struct Amd64Operand amd64_operand_imm_int(int int_val);
extern struct Amd64Operand amd64_operand_label(const char* label);
extern struct Amd64Operand amd64_operand_none;
extern struct Amd64Operand amd64_operand_pseudo(const char* pseudo_name);
extern struct Amd64Operand amd64_operand_reg(enum REGISTER reg);
extern struct Amd64Operand amd64_operand_stack(int offset);
//endregion

//region struct Amd64Instruction
struct Amd64Instruction {
    /**
     * CRITICAL: The various (first) Amd64Operand members must all occur at
     * the same offset, for use by fixup_stack_accesses. If keeping these
     * members in sync becomes a problem, refactor that function.
     */
    enum INSTRUCTION instruction; // BINARY, IDIV, ...
    enum OPCODE opcode;           // add, sub, idiv, ...
    union {
        enum UNARY_OP unary_op;
        enum BINARY_OP binary_op;
        enum COND_CODE cc;
    };
    union {
        struct Amd64Operand operand1;
        const char* text;
        int bytes;
    };
    struct Amd64Operand operand2;
};
extern struct Amd64Instruction* amd64_instruction_new_alloc_stack(int bytes);
extern struct Amd64Instruction* amd64_instruction_new_dealloc_stack(int bytes);
extern struct Amd64Instruction* amd64_instruction_new_binary(enum BINARY_OP op, struct Amd64Operand operand1, struct Amd64Operand operand2);
extern struct Amd64Instruction* amd64_instruction_new_call(struct Amd64Operand identifier);
extern struct Amd64Instruction* amd64_instruction_new_cdq();
extern struct Amd64Instruction* amd64_instruction_new_cmp(struct Amd64Operand operand1, struct Amd64Operand operand2);
extern struct Amd64Instruction* amd64_instruction_new_comment(const char *text);
extern struct Amd64Instruction* amd64_instruction_new_idiv(struct Amd64Operand operand);
extern struct Amd64Instruction* amd64_instruction_new_jmp(struct Amd64Operand identifier);
extern struct Amd64Instruction* amd64_instruction_new_jmpcc(enum COND_CODE cc, struct Amd64Operand identifier);
extern struct Amd64Instruction* amd64_instruction_new_label(struct Amd64Operand identifier);
extern struct Amd64Instruction* amd64_instruction_new_mov(struct Amd64Operand src, struct Amd64Operand dst);
extern struct Amd64Instruction* amd64_instruction_new_push(struct Amd64Operand operand);
extern struct Amd64Instruction* amd64_instruction_new_ret();
extern struct Amd64Instruction* amd64_instruction_new_setcc(enum COND_CODE cc, struct Amd64Operand operand);
extern struct Amd64Instruction* amd64_instruction_new_unary(enum UNARY_OP op, struct Amd64Operand operand);
extern void Amd64Instruction_delete(struct Amd64Instruction *instruction);
//endregion

//region struct Amd64Function
#define NAME list_of_Amd64Instruction
#define TYPE struct Amd64Instruction*
#include "../utils/list_of_item.h"
#undef NAME
#undef TYPE

struct Amd64Function {
    const char *name;
    int stack_allocations;
    struct list_of_Amd64Instruction instructions;
};
extern struct Amd64Function* amd64_function_new(const char *name);
extern void amd64_function_append_instruction(struct Amd64Function *function, struct Amd64Instruction *instruction);
extern void amd64_function_delete(struct Amd64Function *function);
//endregion

//region struct Amd64Program
#define NAME list_of_Amd64Function
#define TYPE struct Amd64Function*
#include "../utils/list_of_item.h"
#undef NAME
#undef TYPE
struct Amd64Program {
    struct list_of_Amd64Function functions;
};
extern struct Amd64Program* amd64_program_new(void );
extern void amd64_program_add_function(struct Amd64Program* program, struct Amd64Function* function);
extern void amd64_program_delete(struct Amd64Program *program);
extern void amd64_program_emit(struct Amd64Program *amd64Program, FILE *out);
//endregion

#endif //BCC_AMD64_H
