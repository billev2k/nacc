#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 9/2/24.
//

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
        .free=pseudo_register_free
};
SET_OF_ITEM_IMPL(pseudo_register, struct pseudo_register*, setOfPseudoLocationHelpers)

static struct Amd64Function *compile_function(struct IrFunction *irFunction);
static int compile_instruction(struct Amd64Function *asmFunction, struct IrInstruction *irInstruction);
static struct Amd64Operand make_operand(struct IrValue* value);
static void fixup_stack_to_stack_moves(struct Amd64Function* function);
static int allocate_pseudo_registers(struct Amd64Function* function);
static int fixup_pseudo_register(struct set_of_pseudo_register* locations, struct Amd64Operand* operand, int previously_allocated);

struct Amd64Program *ir2amd64(struct IrProgram* irProgram) {
    struct Amd64Program *result = amd64_program_new();
    result->function = compile_function(irProgram->function);
    return result;
}

static struct Amd64Function *compile_function(struct IrFunction *irFunction) {
    struct Amd64Function *function = amd64_function_new(irFunction->name);
    for (int ix=0; ix<irFunction->body.list_count; ix++) {
        struct IrInstruction *irInstruction = irFunction->body.items[ix];
        compile_instruction(function, irInstruction);
    }
    // Allocate the pseudo registers
    function->stack_allocations = allocate_pseudo_registers(function);
    fixup_stack_to_stack_moves(function);
    return function;
}

static enum INST unary_ir_to_inst(enum IR_UNARY_OP ir) {
    switch (ir) {
        case IR_UNARY_NEGATE:
            return INST_NEG;
        case IR_UNARY_COMPLEMENT:
            return INST_NOT;
    }
}
static enum INST binary_ir_to_inst(enum IR_BINARY_OP ir) {
    switch (ir) {
        case IR_BINARY_ADD:
            return INST_ADD;
        case IR_BINARY_SUBTRACT:
            return INST_SUB;
        case IR_BINARY_MULTIPLY:
            return INST_MULT;
        case IR_BINARY_DIVIDE:
        case IR_BINARY_REMAINDER:
            break;
        case IR_BINARY_OR:
            return INST_OR;
        case IR_BINARY_AND:
            return INST_AND;
        case IR_BINARY_XOR:
            return INST_XOR;
        case IR_BINARY_LSHIFT:
            return INST_SAL;
        case IR_BINARY_RSHIFT:
            return INST_SAR;
    }
    return 0;
}

static int compile_instruction(struct Amd64Function *asmFunction, struct IrInstruction *irInstruction) {
    enum INST amd_inst;
    struct Amd64Instruction *inst;
    struct Amd64Operand src;
    struct Amd64Operand src2;
    struct Amd64Operand dst;
    struct Amd64Operand result_register;
    switch (irInstruction->inst) {
        case IR_OP_RET:
            src = make_operand(irInstruction->a);
            dst = amd64_operand_reg(REG_AX);
            inst = amd64_instruction_new(INST_MOV, src, dst);
            amd64_function_append_instruction(asmFunction, inst);
            inst = amd64_instruction_new(INST_RET, amd64_operand_none, amd64_operand_none);
            amd64_function_append_instruction(asmFunction, inst);
            break;
        case IR_OP_UNARY:
            src = make_operand(irInstruction->a);
            dst = make_operand(irInstruction->b);
            inst = amd64_instruction_new(INST_MOV, src, dst);
            amd64_function_append_instruction(asmFunction, inst);
            amd_inst = unary_ir_to_inst(irInstruction->unary_op);
            inst = amd64_instruction_new(amd_inst, dst, amd64_operand_none);
            amd64_function_append_instruction(asmFunction, inst);
            break;
        case IR_OP_BINARY:
            switch (irInstruction->binary_op) {
                case IR_BINARY_DIVIDE:
                    result_register = amd64_operand_reg(REG_AX);
                    goto idiv_mod_common;
                case IR_BINARY_REMAINDER:
                    result_register = amd64_operand_reg(REG_DX);
                idiv_mod_common:
                    src = make_operand(irInstruction->a);
                    src2 = make_operand(irInstruction->b);
                    dst = make_operand(irInstruction->c);
                    inst = amd64_instruction_new(INST_MOV, src, amd64_operand_reg(REG_AX));
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new(INST_CDQ, amd64_operand_none, amd64_operand_none);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new(INST_IDIV, src2, amd64_operand_none);
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new(INST_MOV, result_register, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
                case IR_BINARY_LSHIFT:
                    amd_inst = INST_SAL;
                    goto shift_common;
                case IR_BINARY_RSHIFT:
                    amd_inst = INST_SAR;
                shift_common:
                    src = make_operand(irInstruction->a);
                    src2 = make_operand(irInstruction->b);
                    dst = make_operand(irInstruction->c);
                    inst = amd64_instruction_new(INST_MOV, src, amd64_operand_reg(REG_AX));
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new(amd_inst, src2, amd64_operand_reg(REG_AX));
                    amd64_function_append_instruction(asmFunction, inst);
                    inst = amd64_instruction_new(INST_MOV, amd64_operand_reg(REG_AX), dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
                default:
                    src = make_operand(irInstruction->a);
                    src2 = make_operand(irInstruction->b);
                    dst = make_operand(irInstruction->c);
                    inst = amd64_instruction_new(INST_MOV, src, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    amd_inst = binary_ir_to_inst(irInstruction->binary_op);
                    inst = amd64_instruction_new(amd_inst, src2, dst);
                    amd64_function_append_instruction(asmFunction, inst);
                    break;
            }
            break;
    }
    return 1;
}

static struct Amd64Operand make_operand(struct IrValue* value) {
    struct Amd64Operand operand;
    switch (value->type) {
        case IR_VAL_CONST_INT:
            operand = amd64_operand_imm_literal(value->text);
            break;
        case IR_VAL_ID:
            operand = amd64_operand_pseudo(value->text);
            break;
    }
    return operand;
}

static void fixup_stack_to_stack_moves(struct Amd64Function* function) {
    for (int i = 0; i < function->instructions.list_count; ++i) {
        struct Amd64Instruction* inst = function->instructions.items[i];
        if (IS_DEST_IS_REG(inst->instruction) && inst->dst.operand_type != OPERAND_REGISTER) {
            // The stack address that we must flow through a scratch register.
            struct Amd64Operand dst = inst->dst;
            inst->dst = amd64_operand_reg(REG_R11);
            // Load the scratch register before the mult instruction
            struct Amd64Instruction *fixup = amd64_instruction_new(INST_MOV, dst, amd64_operand_reg(REG_R11));
            Amd64Instruction_list_insert(&function->instructions, fixup, i);
            // Save the scratch register after the mult instruction
            fixup = amd64_instruction_new(INST_MOV, amd64_operand_reg(REG_R11), dst);
            Amd64Instruction_list_insert(&function->instructions, fixup, i+2);
            // Fix up loop index to skip the instruction we've already fixed. Two instructions inserted.
            i+=2;
        } else if (inst->src.operand_type == OPERAND_STACK &&
            inst->dst.operand_type == OPERAND_STACK) {
            // Fix instructions that can't use two stack locations.
            if (IS_NO_2_REG(inst->instruction)) {
                // Look for "add -4(%rbp),-8(%rbp)" and use a scratch register.
                // Change the instruction to use the scratch register.
                struct Amd64Operand src = inst->src;
                inst->src = amd64_operand_reg(REG_R10);
                // Load the scratch register before the instruction.
                struct Amd64Instruction *fixup = amd64_instruction_new(INST_MOV, src, amd64_operand_reg(REG_R10));
                Amd64Instruction_list_insert(&function->instructions, fixup, i);
                // Fix up loop index to skip the instruction we've already fixed. One instruction inserted.
                i+=1;
            }
        } else if (inst->instruction == INST_IDIV) {
            if (inst->src.operand_type != OPERAND_REGISTER) {
                struct Amd64Operand src = inst->src;
                inst->src = amd64_operand_reg(REG_R10);
                // Load the scratch register before the instruction.
                struct Amd64Instruction *fixup = amd64_instruction_new(INST_MOV, src, amd64_operand_reg(REG_R10));
                Amd64Instruction_list_insert(&function->instructions, fixup, i);
                i+=1;
            }
        } else if (IS_SHIFT_CONST_OR_CL(inst->instruction)) {
            if (! (inst->src.operand_type == OPERAND_IMM_LIT ||
                    (inst->src.operand_type==OPERAND_REGISTER && inst->src.reg==REG_CL)) ) {
                // if the src operand isn't a constant, and isn't CX, use CX.
                struct Amd64Operand src = inst->src;
                inst->src = amd64_operand_reg(REG_CL);
                struct Amd64Instruction *fixup = amd64_instruction_new(INST_MOV, src, amd64_operand_reg(REG_CX));
                Amd64Instruction_list_insert(&function->instructions, fixup, i);
                i+=1;
            }
        }
    }
}

static int allocate_pseudo_registers(struct Amd64Function* function) {
    struct set_of_pseudo_register pseudo_registers;
    set_of_pseudo_register_init(&pseudo_registers, 10);
    int bytes_allocated = 0;
    for (int i=0; i<function->instructions.list_count; ++i) {
        struct Amd64Instruction* inst = function->instructions.items[i];
        if (inst->src.operand_type == OPERAND_PSEUDO) {
            bytes_allocated += fixup_pseudo_register(&pseudo_registers, &inst->src, bytes_allocated);
        }
        if (inst->dst.operand_type == OPERAND_PSEUDO) {
            bytes_allocated += fixup_pseudo_register(&pseudo_registers, &inst->dst, bytes_allocated);
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