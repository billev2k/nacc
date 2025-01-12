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
    INST_CDQ,
    INST_CMP,
    INST_COMMENT,
    INST_IDIV,
    INST_JMP,
    INST_JMPCC,
    INST_LABEL,
    INST_MOV,
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
    X(MOV,      "mov"),         \
    X(RET,      "ret"),         \
    X(NEG,      "neg"),         \
    X(NOT,      "not"),         \
    X(ADD,      "add"),         \
    X(SUB,      "sub"),         \
    X(MULT,     "imul"),        \
    X(IDIV,     "idiv"),        \
    X(CDQ,      "cdq"),         \
    X(POP,      "pop"),         \
    X(SAL,      "sal"),         \
    X(SAR,      "sar"),         \
    X(AND,      "and"),         \
    X(OR,       "or"),          \
    X(XOR,      "xor"),         \
    X(CMP,      "cmp"),         \
    X(JMP,      "jmp"),         \
    X(JE,       "je"),          \
    X(JNE,      "jne"),         \
    X(JL,       "jl"),          \
    X(JLE,      "jle"),         \
    X(JG,       "jg"),          \
    X(JGE,      "jge"),         \
    X(SETE,     "sete"),        \
    X(SETNE,    "setne"),       \
    X(SETL,     "setl"),        \
    X(SETLE,    "setle"),       \
    X(SETG,     "setg"),        \
    X(SETGE,    "setge"),       \
    \

enum OPCODE {
#define X(a,b) OPCODE_##a
    OPCODE_LIST__
#undef X
    OPCODE_NONE,
};
extern const char * const opcode_names[];

extern const enum OPCODE cond_code_to_jXX[];
extern const enum OPCODE cond_code_to_setXX[];

enum OPERAND {
    OPERAND_NONE,
    OPERAND_IMM_LIT,
    OPERAND_REGISTER,
    OPERAND_PSEUDO,
    OPERAND_STACK,
    OPERAND_LABEL,
};

#define REGISTERS__ \
    X(AX,  "%eax"),         \
    X(CL,  "%cl"),          \
    X(CX,  "%ecx"),         \
    X(DX,  "%edx"),         \
    X(R10, "%r10d"),        \
    X(R11, "%r11d"),        \
    X(BP,  "%rbp"),         \
    X(SP,  "%rsp")

enum REGISTER {
#define X(r,n) REG_##r
    REGISTERS__
#undef X
};
extern char const * register_names[];

//region struct Amd64Operand
struct Amd64Operand {
    enum OPERAND operand_type;
    union {
        const char *name;
        int offset;
        enum REGISTER reg;
        int value;
    };
};
extern struct Amd64Operand amd64_operand_none;
extern struct Amd64Operand amd64_operand_imm_literal(const char* value_str);
extern struct Amd64Operand amd64_operand_reg(enum REGISTER reg);
extern struct Amd64Operand amd64_operand_pseudo(const char* pseudo_name);
extern struct Amd64Operand amd64_operand_stack(int offset);
extern struct Amd64Operand amd64_operand_label(const char* label);
//endregion

//region struct Amd64Instruction
struct Amd64Instruction {
    /**
     * CRITICAL: The various (first) Amd64Operand members must all occur at
     * the same offset, for use by fixup_stack_accesses. If keeping these
     * members in sync becomes a problem, refactor that function.
     */
    enum INSTRUCTION instruction;
    enum OPCODE opcode;
    union {
        struct {
            int unused_subcode__;
            struct Amd64Operand src;
            struct Amd64Operand dst;
        } mov;
        struct {
            enum UNARY_OP op;
            struct Amd64Operand operand;
        } unary;
        struct {
            enum BINARY_OP op;
            struct Amd64Operand operand1;
            struct Amd64Operand operand2;
        } binary;
        struct {
            int unused_subcode__;
            struct Amd64Operand operand1;
            struct Amd64Operand operand2;
        } cmp;
        struct {
            int unused_subcode__;
            struct Amd64Operand operand;
        } idiv;
        struct {
            int unused_subcode__;
            struct Amd64Operand identifier;
        } jmp;
        struct {
            enum COND_CODE cc;
            struct Amd64Operand identifier;
        } jmpcc;
        struct {
            enum COND_CODE cc;
            struct Amd64Operand operand;
        } setcc;
        struct {
            int unused_subcode__;
            struct Amd64Operand identifier;
        } label;
        struct {
            int unused_subcode__;
            int bytes;
        } stack;
        struct {
            int unused_subcode__;
            const char *text;
        } comment;
    };
};
extern struct Amd64Instruction* amd64_instruction_new_alloc_stack(int bytes);
extern struct Amd64Instruction* amd64_instruction_new_binary(enum BINARY_OP op, struct Amd64Operand operand1, struct Amd64Operand operand2);
extern struct Amd64Instruction* amd64_instruction_new_cdq();
extern struct Amd64Instruction* amd64_instruction_new_cmp(struct Amd64Operand operand1, struct Amd64Operand operand2);
extern struct Amd64Instruction* amd64_instruction_new_comment(const char *text);
extern struct Amd64Instruction* amd64_instruction_new_idiv(struct Amd64Operand operand);
extern struct Amd64Instruction* amd64_instruction_new_jmp(struct Amd64Operand identifier);
extern struct Amd64Instruction* amd64_instruction_new_jmpcc(enum COND_CODE cc, struct Amd64Operand identifier);
extern struct Amd64Instruction* amd64_instruction_new_label(struct Amd64Operand identifier);
extern struct Amd64Instruction* amd64_instruction_new_mov(struct Amd64Operand src, struct Amd64Operand dst);
extern struct Amd64Instruction* amd64_instruction_new_ret();
extern struct Amd64Instruction* amd64_instruction_new_setcc(enum COND_CODE cc, struct Amd64Operand operand);
extern struct Amd64Instruction* amd64_instruction_new_unary(enum UNARY_OP op, struct Amd64Operand operand);
extern void Amd64Instruction_free(struct Amd64Instruction *instruction);
//endregion

//region struct Amd64Function
LIST_OF_ITEM_DECL(Amd64Instruction, struct Amd64Instruction*)
struct Amd64Function {
    const char *name;
    int stack_allocations;
    struct list_of_Amd64Instruction instructions;
};
extern struct Amd64Function* amd64_function_new(const char *name);
extern void amd64_function_append_instruction(struct Amd64Function *function, struct Amd64Instruction *instruction);
extern void amd64_function_free(struct Amd64Function *function);
//endregion

//region struct Amd64Program
struct Amd64Program {
    struct Amd64Function *function;
};
extern struct Amd64Program* amd64_program_new(void );
extern void amd64_program_free(struct Amd64Program *program);
extern void amd64_program_emit(struct Amd64Program *amd64Program, FILE *out);
//endregion

#endif //BCC_AMD64_H
