#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 9/2/24.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "amd64.h"
#include "ir2amd64.h"
#include "../parser/symtable.h"
#include "inc/set_of.h"

struct pseudo_register {
    const char *name;
    int offset;
};
SET_OF_ITEM_DECL(set_of_pseudo_register, struct pseudo_register)
SET_OF_ITEM_DEFN(set_of_pseudo_register, struct pseudo_register)
unsigned long pseudo_register_hash(struct pseudo_register pl) {
    return hash_str(pl.name);
}
int pseudo_register_cmp(struct pseudo_register l, struct pseudo_register r) {
    return strcmp(l.name, r.name);
}
struct pseudo_register pseudo_register_dup(struct pseudo_register pl) {
    return pl;
}
void pseudo_register_delete(struct pseudo_register pl) {
    ; // no-op
}
int pseudo_register_is_null(struct pseudo_register pl) {
    return pl.name == NULL;
}
struct set_of_pseudo_register_helpers set_of_pseudo_register_helpers = {
        .hash=pseudo_register_hash,
        .cmp=pseudo_register_cmp,
        .dup=pseudo_register_dup,
        .delete=pseudo_register_delete,
        .is_null=pseudo_register_is_null,
        .null={0}
};

static enum REGISTER param_registers[] = {REG_DI, REG_SI, REG_DX, REG_CX, REG_R8, REG_R9};

static struct Amd64Function *convert_function(struct IrFunction *irFunction);
static int convert_instruction(struct Amd64Function *asmFunction, struct IrInstruction *irInstruction);
static struct Amd64Operand make_operand(struct IrValue value);
static void fixup_stack_accesses(struct Amd64Function* function);
static int allocate_pseudo_registers(struct Amd64Function* function);
static int fixup_pseudo_register(struct set_of_pseudo_register* locations, struct Amd64Operand* operand, int previously_allocated);

struct Amd64Operand zero = {
        .operand_kind = OPERAND_IMM_INT,
        .int_val = 0
};
struct Amd64Operand one = {
        .operand_kind = OPERAND_IMM_INT,
        .int_val = 1
};

struct Amd64Program *ir2amd64(struct IrProgram* irProgram) {
    struct Amd64Program *program = amd64_program_new();
    for (int ix=0; ix<irProgram->top_level.num_items; ix++) {
        struct IrTopLevel *top_level = irProgram->top_level.items[ix];
        switch (top_level->kind) {
            case IR_FUNCTION:
                ;
                struct Amd64Function* function = convert_function(top_level->function);
                amd64_program_add_function(program, function);
                break;
            case IR_STATIC_VAR:
                ;
                struct Amd64StaticVar *static_var = amd64_static_var_new(top_level->static_var->name, top_level->static_var->global, top_level->static_var->init_value);
                amd64_program_add_static_var(program, static_var);
                break;
        }
    }
    return program;
}

/**
 * Copy params from ABI specified locations to more convenient (but less performant)
 * stack locations.
 * @param function AMD64 being emitted
 * @param irFunction TACKY being compiled
 */
static void copy_function_params(struct Amd64Function *function, struct IrFunction *irFunction) {
    // Param registers: DI, SI, DX, CX, R8, R9
    int num_parameters = irFunction->params.num_items;
    for (int ix=0; ix<6 && ix<num_parameters; ix++) {
        struct Amd64Operand src = amd64_operand_reg(param_registers[ix]);
        struct Amd64Operand dst = amd64_operand_pseudo(irFunction->params.items[ix].text);
        struct Amd64Instruction *inst = amd64_instruction_new_mov(src, dst);
        amd64_function_append_instruction(function, inst);
    }
    for (int ix=6; ix<num_parameters; ix++) {
        struct Amd64Operand src = amd64_operand_stack(16 + (ix-6)*8);
        struct Amd64Operand dst = amd64_operand_pseudo(irFunction->params.items[ix].text);
        struct Amd64Instruction *inst = amd64_instruction_new_mov(src, dst);
        amd64_function_append_instruction(function, inst);
    }
}
static struct Amd64Function *convert_function(struct IrFunction *irFunction) {
    struct Amd64Function *function = amd64_function_new(irFunction->name, irFunction->global);
    copy_function_params(function, irFunction);
    
    amd64_function_append_instruction(function, amd64_instruction_new_comment("end of function prolog"));
    for (int ix=0; ix<irFunction->body.num_items; ix++) {
        struct IrInstruction *irInstruction = irFunction->body.items[ix];
        convert_instruction(function, irInstruction);
    }
    // Allocate space on the stack for the pseudo registers (locals and temporaries)
    function->stack_allocations = allocate_pseudo_registers(function);
    if (function->stack_allocations != 0) {
        // Keep stack aligned on 16-byte boundaries.
        if (function->stack_allocations % 16 != 0) function->stack_allocations += 16 - (function->stack_allocations % 16);
        struct Amd64Instruction* stack = amd64_instruction_new_alloc_stack(function->stack_allocations);
        list_of_Amd64Instruction_insert(&function->instructions, stack, 0);
    }

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

static void convert_function_call(struct Amd64Function *asmFunction, struct IrInstruction *irInstruction) {
    // DI, SI, DX, CX, R8, R9, push[n], push[n-1], push[n-2], ...
    // Where does any result go?
    struct Amd64Operand result = make_operand(irInstruction->funcall.dst);
    // What function to call?
    struct Amd64Operand target = amd64_operand_func(irInstruction->funcall.func_name.text);
    struct Amd64Instruction* inst;
    // How many register args, stack args? Pad stack to 16 bytes (per Sys V ABI)
    int num_register_args = (irInstruction->funcall.args.num_items > 6) ? 6 : irInstruction->funcall.args.num_items;
    int num_stack_args = (irInstruction->funcall.args.num_items <= 6) ? 0 : (irInstruction->funcall.args.num_items - 6);
    int stack_padding = (num_stack_args & 1) ? 8 : 0;
    if (stack_padding) {
        struct Amd64Instruction* stack = amd64_instruction_new_alloc_stack(stack_padding);
        amd64_function_append_instruction(asmFunction, stack);
    }
    // register args
    for (int ix=0; ix<num_register_args; ix++) {
        struct Amd64Operand dst = amd64_operand_reg(param_registers[ix]);
        struct Amd64Operand src = make_operand(irInstruction->funcall.args.items[ix]);
        inst = amd64_instruction_new_mov(src, dst);
        amd64_function_append_instruction(asmFunction, inst);
    }
    // stack args
    for (int ix=irInstruction->funcall.args.num_items-1; ix>=6; ix--) {
        struct Amd64Operand src = make_operand(irInstruction->funcall.args.items[ix]);
        if (src.operand_kind == OPERAND_REGISTER || src.operand_kind == OPERAND_IMM_INT) {
            inst = amd64_instruction_new_push(src);
        } else {
            inst = amd64_instruction_new_mov(src, amd64_operand_reg(REG_AX));
            amd64_function_append_instruction(asmFunction, inst);
            inst = amd64_instruction_new_push(amd64_operand_reg(REG_AX));
        }
        amd64_function_append_instruction(asmFunction, inst);
    }
    // Call target
    inst = amd64_instruction_new_call(target);
    amd64_function_append_instruction(asmFunction, inst);
    // adjust stack pointer
    int bytes_to_remove = stack_padding + 8 * num_stack_args;
    if (bytes_to_remove) {
        inst = amd64_instruction_new_dealloc_stack(bytes_to_remove);
        amd64_function_append_instruction(asmFunction, inst);
    }
    // retrieve return value
    inst = amd64_instruction_new_mov(amd64_operand_reg(REG_AX), result);
    amd64_function_append_instruction(asmFunction, inst);
}

static int convert_instruction(struct Amd64Function *asmFunction, struct IrInstruction *irInstruction) {
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
                    // Cmp(Imm(0), operand1)
                    // Mov(Imm(0), operand2)
                    // SetCC(E, operand2)
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
        case IR_OP_JUMP_EQ:
            cc = CC_EQ;
            src2 = make_operand(irInstruction->cjump.comparand);
            src = make_operand(irInstruction->cjump.value);
            dst = make_operand(irInstruction->cjump.target);
            inst = amd64_instruction_new_cmp(src2, src);
            amd64_function_append_instruction(asmFunction, inst);
            inst = amd64_instruction_new_jmpcc(cc, dst);
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
        case IR_OP_COMMENT:
            inst = amd64_instruction_new_comment(irInstruction->comment.text);
            amd64_function_append_instruction(asmFunction, inst);
        case IR_OP_VAR:
            // no code for this.
            break;
        case IR_OP_FUNCALL:
            convert_function_call(asmFunction, irInstruction);
            break;
    }
    return 1;
}

static struct Amd64Operand make_operand(struct IrValue value) {
    struct Amd64Operand operand = {0};
    switch (value.kind) {
        case IR_VAL_CONST:
            operand = amd64_operand_imm_int(value.const_value.int_value);
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
    return inst->instruction == INST_BINARY && inst->binary_op == BINARY_OP_MULTIPLY;
}
static int is_shift(struct Amd64Instruction* inst) {
    return inst->instruction == INST_BINARY &&
            (inst->binary_op == BINARY_OP_LSHIFT || inst->binary_op == BINARY_OP_RSHIFT);
}

static void fixup_stack_accesses(struct Amd64Function* function) {
    for (int i = 0; i < function->instructions.num_items; ++i) {
        struct Amd64Instruction* inst = function->instructions.items[i];
        
        if (is_multiply(inst) && inst->operand2.operand_kind != OPERAND_REGISTER) {
            // The stack address that we must flow through a scratch register.
            struct Amd64Operand operand2 = inst->operand2;
            inst->operand2 = amd64_operand_reg(REG_R11);
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
            if (inst->operand1.operand_kind != OPERAND_REGISTER) {
                struct Amd64Operand operand = inst->operand1;
                inst->operand1 = amd64_operand_reg(REG_R10);
                // Load the scratch register before the instruction.
                struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand, amd64_operand_reg(REG_R10));
                list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
                i+=1;
            }
        }
        else if (is_shift(inst)) {
            if (! (inst->operand1.operand_kind == OPERAND_IMM_INT ||
                   (inst->operand1.operand_kind == OPERAND_REGISTER && inst->operand1.reg == REG_CX)) ) {
                // if the operand1 operand1 isn't a constant, and isn't CX, use CX.
                struct Amd64Operand operand1 = inst->operand1;
                inst->operand1 = amd64_operand_reg(REG_CX);
                struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand1, amd64_operand_reg(REG_CX));
                list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
                i+=1;
            }
        }
        else if ((inst->instruction == INST_BINARY || inst->instruction == INST_MOV) &&
                OPERAND_IS_MEMORY(inst->operand1.operand_kind) &&
                OPERAND_IS_MEMORY(inst->operand2.operand_kind) ) {
            // Fix instructions that can't use two stack locations.
            // Look for "add -4(%rbp),-8(%rbp)" and use a scratch register.
            // Change the instruction to use the scratch register.
            struct Amd64Operand operand1 = inst->operand1;
            inst->operand1 = amd64_operand_reg(REG_R10);
            // Load the scratch register before the instruction.
            struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand1, amd64_operand_reg(REG_R10));
            list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
            // Fix up loop index to skip the instruction we've already fixed. One instruction inserted.
            i += 1;
        } else if (inst->instruction == INST_CMP) {
            if (OPERAND_IS_MEMORY(inst->operand1.operand_kind) &&
                OPERAND_IS_MEMORY(inst->operand2.operand_kind)) {
                struct Amd64Operand operand1 = inst->operand1;
                inst->operand1 = amd64_operand_reg(REG_R10);
                // Load the scratch register before the instruction.
                struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand1, amd64_operand_reg(REG_R10));
                list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
                // Fix up loop index to skip the instruction we've already fixed. One instruction inserted.
                i += 1;
            } else if (inst->operand2.operand_kind == OPERAND_IMM_INT) {
                // The second operand1 of a cmp instruction can't be a literal. Load literals into R11
                struct Amd64Operand operand2 = inst->operand2;
                inst->operand2 = amd64_operand_reg(REG_R11);
                // Load the scratch register before the instruction.
                struct Amd64Instruction *fixup = amd64_instruction_new_mov(operand2, amd64_operand_reg(REG_R11));
                list_of_Amd64Instruction_insert(&function->instructions, fixup, i);
                // Fix up loop index to skip the instruction we've already fixed. One instruction inserted.
                i += 1;
            }
        }
    }
}

/**
 * Allocate the pseudo registers for one function. A pseudo register is a stack location used to hold a
 * value, which may be a compiler-generated temporary variable, or may be an explicitly declared local variable.
 *
 * These will be Amd64Operand structs with an operand_kind == OPERAND_PSEUDO.
 *
 * The pseudo registers for a function are tracked by uniquified name. The first time a name is seen,
 * space is reserved for it. The offset of that reservation is stored in the Amd64Operand record.
 *
 * Later, when we need to load from or store to the variable, the offset is used to address the correct stack location.
 *
 * @param function The function for which to allocate pseudo registers.
 * @return The number of stack bytes allocated.
 */
static int allocate_pseudo_registers(struct Amd64Function* function) {
    // Map of {uniquified-name: offset}
    struct set_of_pseudo_register pseudo_registers;
    set_of_pseudo_register_init(&pseudo_registers, 10);
    int bytes_allocated = 0;
    for (int i=0; i<function->instructions.num_items; ++i) {
        struct Amd64Instruction* inst = function->instructions.items[i];
        if (inst->instruction != INST_ALLOC_STACK) {
            if (inst->operand1.operand_kind == OPERAND_PSEUDO) {
                bytes_allocated += fixup_pseudo_register(&pseudo_registers, &inst->operand1, bytes_allocated);
            }
            if (opcode_num_operands[inst->opcode] > 1 && inst->operand2.operand_kind == OPERAND_PSEUDO) {
                bytes_allocated += fixup_pseudo_register(&pseudo_registers, &inst->operand2, bytes_allocated);
            }
        }
    }
    set_of_pseudo_register_delete(&pseudo_registers);
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
    int allocation = 0;
    struct pseudo_register pseudo_register_lookup_key = {.name = operand->name};
    struct pseudo_register pseudo_register_found;
    bool was_found = set_of_pseudo_register_find(locations, pseudo_register_lookup_key, &pseudo_register_found);
    bool is_static = false;

    // If the space for this pseudo hasn't already been allocation, do so now.
    if (!was_found) {
        // Maybe this is a static variable. Look up in symbol table.
        struct Symbol symbol;
        if (find_symbol_by_name(operand->name, &symbol) == SYMTAB_OK) {
            is_static = SYMBOL_IS_STATIC_VAR(symbol.attrs);
        }
        if (!is_static) {
            allocation = 4; // when we have other sizes of stack variables, this will need to change.
            pseudo_register_lookup_key.offset = -(previously_allocated + allocation);
            pseudo_register_found = set_of_pseudo_register_insert(locations, pseudo_register_lookup_key);
        }
    }
    if (is_static) {
        operand->operand_kind = OPERAND_DATA;
        // name stays the same, of course.
    } else {
        operand->operand_kind = OPERAND_STACK;
        operand->offset = pseudo_register_found.offset;
    }
    return allocation;
}


#pragma clang diagnostic pop