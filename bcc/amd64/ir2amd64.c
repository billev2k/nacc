#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 9/2/24.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ir2amd64.h"

struct pseudo_register {
    const char *name;
    int offset;
};
SET_OF_ITEM_DECL(pseudo_register, struct pseudo_register*)
unsigned long pseudo_register_hash(struct pseudo_register* pl) {
    return hash_str(pl->name);
}
int pseudo_register_cmp(struct pseudo_register* l, struct pseudo_register* r) {
    return strcmp(l->name, r->name);
}
struct pseudo_register* pseudo_register_dup(struct pseudo_register* pl) {
    struct pseudo_register* plNew = (struct pseudo_register*)malloc(sizeof(struct pseudo_register));
    *plNew = *pl;
    return plNew;
}
void pseudo_register_free(struct pseudo_register* pl) {
    free(pl);
}
struct set_of_pseudo_register_helpers setOfPseudoLocationHelpers = {
        .hash=pseudo_register_hash,
        .cmp=pseudo_register_cmp,
        .dup=pseudo_register_dup,
        .free=pseudo_register_free,
        .is_null=(int (*)(struct pseudo_register *)) long_is_zero,
};
SET_OF_ITEM_DEFN(pseudo_register, struct pseudo_register*, setOfPseudoLocationHelpers)

static struct Amd64Function *compile_function(struct IrFunction *irFunction);
static int compile_instruction(struct Amd64Function *asmFunction, struct IrInstruction *irInstruction);
static struct Amd64Operand make_operand(struct IrValue value);
static void fixup_stack_accesses(struct Amd64Function* function);
static int allocate_pseudo_registers(struct Amd64Function* function);
static int fixup_pseudo_register(struct set_of_pseudo_register* locations, struct Amd64Operand* operand, int previously_allocated);

struct Amd64Operand zero = {
        .operand_type = OPERAND_IMM_LIT,
        .name = "0"
};
struct Amd64Operand one = {
        .operand_type = OPERAND_IMM_LIT,
        .name = "1"
};

static void validate_Amd54Instruction_offsets(void) {
    int offset = offsetof(struct Amd64Instruction, mov.src);
    if ((offsetof(struct Amd64Instruction, unary.operand) != offset) != 0 ||
        (offsetof(struct Amd64Instruction, cmp.operand1) != offset) ||
        (offsetof(struct Amd64Instruction, idiv.operand) != offset) ||
        (offsetof(struct Amd64Instruction, jmp.identifier) != offset) ||
        (offsetof(struct Amd64Instruction, jmpcc.identifier) != offset) ||
        (offsetof(struct Amd64Instruction, setcc.operand) != offset) ||
        (offsetof(struct Amd64Instruction, label.identifier) != offset)) {
        fprintf(stderr, "Internal error: operand offset mismatch in struct Amd64Instruction.\n");
        exit(1);
    }
}

struct Amd64Program *ir2amd64(struct IrProgram* irProgram) {
    validate_Amd54Instruction_offsets();
    struct Amd64Program *result = amd64_program_new();
    result->function = compile_function(irProgram->function);
    return result;
}

static struct Amd64Function *compile_function(struct IrFunction *irFunction) {
    struct Amd64Function *function = amd64_function_new(irFunction->name);
    for (int ix=0; ix<irFunction->body.num_items; ix++) {
        struct IrInstruction *irInstruction = irFunction->body.items[ix];
        compile_instruction(function, irInstruction);
    }
    // Allocate the pseudo registers
    function->stack_allocations = allocate_pseudo_registers(function);
    fixup_stack_accesses(function);
    return function;
}

static enum UNARY_OP unary_ir_to_inst(enum IR_UNARY_OP ir) {
    return (enum UNARY_OP) ir;
}
static enum BINARY_OP binary_ir_to_inst(enum IR_BINARY_OP ir) {
    return (enum BINARY_OP) ir;
}
static enum COND_CODE cc_for_binary_op(enum IR_BINARY_OP binary_op) {
    switch (binary_op) {
        case IR_BINARY_EQ:
            return CC_EQ;
        case IR_BINARY_NE:
            return CC_NE;
        case IR_BINARY_LT:
            return CC_LT;
        case IR_BINARY_LE:
            return CC_LE;
        case IR_BINARY_GT:
            return CC_GT;
        case IR_BINARY_GE:
            return CC_GE;
        default:
            return -1;
    }
} 

static int compile_instruction(struct Amd64Function *asmFunction, struct IrInstruction *irInstruction) {
    struct Amd64Instruction *inst;
    struct Amd64Operand src;
    struct Amd64Operand src2;
    struct Amd64Operand dst;
    struct Amd64Operand immediate;
    struct Amd64Operand result_register;
    enum COND_CODE cc;
    switch (irInstruction->inst) {
        case IR_OP_RET:
            src = make_operand(irInstruction->ret.value);
            dst = amd64_operand_reg(REG_AX);
            inst = amd64_instruction_new_mov(src, dst);
            amd64_function_append_instruction(asmFunction, inst);
            inst = amd64_instruction_new_ret();
            amd64_function_append_instruction(asmFunction, inst);
            break;
        case IR_OP_UNARY:
            src = make_operand(irInstruction->unary.src);
            dst = make_operand(irInstruction->unary.dst);
            switch (irInstruction->unary.op) {
                case IR_UNARY_NEGATE:
                case IR_UNARY_COMPLEMENT:
                    inst = amd64_instruction_new_mov(src, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_unary((enum UNARY_OP) irInstruction->unary.op, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
                case IR_UNARY_L_NOT:
                    // !x => x?0:1
                    // Cmp(Imm(0), src)
                    // Mov(Imm(0), dst)
                    // SetCC(E, dst)
                    inst = amd64_instruction_new_cmp(zero, src);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_mov(zero, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_setcc(CC_EQ, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
            }
            break;
        case IR_OP_BINARY:
            src = make_operand(irInstruction->binary.src1);
            src2 = make_operand(irInstruction->binary.src2);
            dst = make_operand(irInstruction->binary.dst);
            switch (irInstruction->binary.op) {
                case IR_BINARY_DIVIDE:
                case IR_BINARY_REMAINDER:
                    result_register = irInstruction->binary.op==IR_BINARY_DIVIDE
                            ? amd64_operand_reg(REG_AX)
                            : amd64_operand_reg(REG_DX);
                    inst = amd64_instruction_new_mov(src, amd64_operand_reg(REG_AX));
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_cdq();
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_idiv(src2);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_mov(result_register, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
                case IR_BINARY_LSHIFT:
                case IR_BINARY_RSHIFT:
                    inst = amd64_instruction_new_mov(src, amd64_operand_reg(REG_AX));
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_binary((enum BINARY_OP) irInstruction->binary.op, src2, amd64_operand_reg(REG_AX));
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_mov(amd64_operand_reg(REG_AX), dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
                case IR_BINARY_ADD:
                case IR_BINARY_SUBTRACT:
                case IR_BINARY_MULTIPLY:
                case IR_BINARY_OR:
                case IR_BINARY_AND:
                case IR_BINARY_XOR:
                    inst = amd64_instruction_new_mov(src, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_binary((enum BINARY_OP) irInstruction->binary.op, src2, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
                case IR_BINARY_EQ:
                case IR_BINARY_NE:
                case IR_BINARY_LT:
                case IR_BINARY_LE:
                case IR_BINARY_GT:
                case IR_BINARY_GE:
                    // x = a < b; becomes cmpl  b,a    (remember how "a - b" becomes "sub  b,a"
                    //                    setl  x
                    // a can't be immediate, and setXX only sets one byte, so we need to clear the entire destination.
                    cc = cc_for_binary_op(irInstruction->binary.op);
                    inst = amd64_instruction_new_cmp(src2, src);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_mov(zero, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new_setcc(cc, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
                case IR_BINARY_L_AND:
                case IR_BINARY_L_OR:
                    // internal error; && and || are translated earlier, to implement short circuits
                    break;
            }
            break;
        case IR_OP_COPY:
            src = make_operand(irInstruction->copy.src);
            dst = make_operand(irInstruction->copy.dst);
            inst = amd64_instruction_new_mov(src, dst);
            amd64_function_append_instruction(asmFunction, inst);
            break;
        case IR_OP_JUMP:
            dst = make_operand(irInstruction->jump.target);
            inst = amd64_instruction_new_jmp(dst);
            amd64_function_append_instruction(asmFunction, inst);
            break;
        case IR_OP_JUMP_ZERO:
            // Cmp(Imm(0), condition)
            // JmpCC(E, target)
        case IR_OP_JUMP_NZERO:
            // Cmp(Imm(0), condition)
            // JmpCC(NE, target)
            cc = irInstruction->inst==IR_OP_JUMP_ZERO ? CC_EQ : CC_NE;
            src = make_operand(irInstruction->cjump.value);
            dst = make_operand(irInstruction->cjump.target);
            inst = amd64_instruction_new_cmp(zero, src);
            amd64_function_append_instruction(asmFunction, inst);
            inst = amd64_instruction_new_jmpcc(cc, dst);
            amd64_function_append_instruction(asmFunction, inst);
            break;
        case IR_OP_LABEL:
            inst = amd64_instruction_new_label(make_operand(irInstruction->label.label));
            amd64_function_append_instruction(asmFunction, inst);
            break;
    }
    return 1;
}

static struct Amd64Operand make_operand(struct IrValue value) {
    struct Amd64Operand operand;
    switch (value.type) {
        case IR_VAL_CONST_INT:
            operand = amd64_operand_imm_literal(value.text);
            break;
        case IR_VAL_ID:
            operand = amd64_operand_pseudo(value.text);
            break;
        case IR_VAL_LABEL:
            operand = amd64_operand_label(value.text);
            break;
    }
    return operand;
}

static int is_multiply(struct Amd64Instruction* inst) {
    return inst->instruction == INST_BINARY && inst->binary.op == BINARY_OP_MULTIPLY;
}
static int is_shift(struct Amd64Instruction* inst) {
    return inst->instruction == INST_BINARY &&
            (inst->binary.op == BINARY_OP_LSHIFT || inst->binary.op == BINARY_OP_RSHIFT);
}

static void fixup_stack_accesses(struct Amd64Function* function) {
    for (int i = 0; i < function->instructions.num_items; ++i) {
        struct Amd64Instruction* inst = function->instructions.items[i];
        
        if (is_multiply(inst) && inst->binary.operand2.operand_type != OPERAND_REGISTER) {
            // The stack address that we must flow through a scratch register.
            struct Amd64Operand operand2 = inst->binary.operand2;
            inst->binary.operand2 = amd64_operand_reg(REG_R11);
            // Load the scratch register before the mult instruction
            struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand2, amd64_operand_reg(REG_R11));
            list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
            // Save the scratch register after the mult instruction
            fixup = amd64_instruction_new_mov(amd64_operand_reg(REG_R11), operand2);
            list_of_Amd64Instruction_insert(&function->instructions, fixup, i+2);
            // Fix up loop index to skip the instruction we've already fixed. Two instructions inserted.
            i+=2;
        }
        else if (inst->instruction == INST_IDIV) {
            if (inst->idiv.operand.operand_type != OPERAND_REGISTER) {
                struct Amd64Operand operand = inst->idiv.operand;
                inst->idiv.operand = amd64_operand_reg(REG_R10);
                // Load the scratch register before the instruction.
                struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand, amd64_operand_reg(REG_R10));
                list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
                i+=1;
            }
        }
        else if (is_shift(inst)) {
            if (! (inst->binary.operand1.operand_type == OPERAND_IMM_LIT ||
                    (inst->binary.operand1.operand_type==OPERAND_REGISTER && inst->binary.operand1.reg==REG_CL)) ) {
                // if the operand1 operand isn't a constant, and isn't CX, use CX.
                struct Amd64Operand operand1 = inst->binary.operand1;
                inst->binary.operand1 = amd64_operand_reg(REG_CL);
                struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand1, amd64_operand_reg(REG_CX));
                list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
                i+=1;
            }
        }
        else if ((inst->instruction == INST_BINARY || inst->instruction == INST_MOV) &&
                 inst->binary.operand1.operand_type == OPERAND_STACK &&
                 inst->binary.operand2.operand_type == OPERAND_STACK) {
            // Fix instructions that can't use two stack locations.
            // Look for "add -4(%rbp),-8(%rbp)" and use a scratch register.
            // Change the instruction to use the scratch register.
            struct Amd64Operand operand1 = inst->binary.operand1;
            inst->binary.operand1 = amd64_operand_reg(REG_R10);
            // Load the scratch register before the instruction.
            struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand1, amd64_operand_reg(REG_R10));
            list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
            // Fix up loop index to skip the instruction we've already fixed. One instruction inserted.
            i += 1;
        } else if (inst->instruction == INST_CMP) {
            if (inst->cmp.operand1.operand_type == OPERAND_STACK &&
                    inst->cmp.operand2.operand_type == OPERAND_STACK) {
                struct Amd64Operand operand1 = inst->cmp.operand1;
                inst->cmp.operand1 = amd64_operand_reg(REG_R10);
                // Load the scratch register before the instruction.
                struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand1, amd64_operand_reg(REG_R10));
                list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
                // Fix up loop index to skip the instruction we've already fixed. One instruction inserted.
                i += 1;
            } else if (inst->cmp.operand2.operand_type == OPERAND_IMM_LIT) {
                // The second operand of a cmp instruction can't be a literal. Load literals into R11
                struct Amd64Operand operand2 = inst->cmp.operand2;
                inst->cmp.operand2 = amd64_operand_reg(REG_R11);
                // Load the scratch register before the instruction.
                struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand2, amd64_operand_reg(REG_R11));
                list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
                // Fix up loop index to skip the instruction we've already fixed. One instruction inserted.
                i += 1;
            }
        }
    }
}

static int allocate_pseudo_registers(struct Amd64Function* function) {
    struct set_of_pseudo_register pseudo_registers;
    set_of_pseudo_register_init(&pseudo_registers, 10);
    int bytes_allocated = 0;
    for (int i=0; i<function->instructions.num_items; ++i) {
        struct Amd64Instruction* inst = function->instructions.items[i];
        if (inst->instruction != INST_ALLOC_STACK) {
            if (inst->binary.operand1.operand_type == OPERAND_PSEUDO) {
                bytes_allocated += fixup_pseudo_register(&pseudo_registers, &inst->binary.operand1, bytes_allocated);
            }
            if (inst->binary.operand2.operand_type == OPERAND_PSEUDO) {
                bytes_allocated += fixup_pseudo_register(&pseudo_registers, &inst->binary.operand2, bytes_allocated);
            }
        }
    }
    set_of_pseudo_register_free(&pseudo_registers);
    return bytes_allocated;
}

/**
 * Fixes up pseudo-registers in an instruction factor.
 * @param locations - a set of previously assigned offsets for pseudo-registers.
 * @param operand - the factor to be fixed.
 * @param previously_allocated - number of bytes already allocated in the functions frame.
 * @return number of bytes allocated for this pseudo-registers. Zero if space has already
 *      been allocated for this pseudo register.
 */
static int fixup_pseudo_register(struct set_of_pseudo_register* locations, struct Amd64Operand* operand, int previously_allocated) {
    int allocated = 0;
    struct pseudo_register pl = {.name = operand->name};
    struct pseudo_register* fixup = set_of_pseudo_register_find(locations, &pl);
    // If the space for this pseudo hasn't already been allocated, do so now.
    if (fixup == NULL) {
        allocated = 4; // when we have other sizes of stack variables, this will need to change.
        pl.offset = -(previously_allocated + allocated);
        fixup = set_of_pseudo_register_insert(locations, &pl);
    }
    operand->operand_type = OPERAND_STACK;
    operand->offset = fixup->offset;
    return allocated;
}


#pragma clang diagnostic pop