//
// Created by Bill Evans on 9/23/24.
//

#include <stdlib.h>
#include <string.h>
#include "ir.h"

const char * const IR_UNARY_NAMES[] = {
#define X(a,b) b
    IR_UNARY_OP_LIST__
#undef X            
};
const char * const IR_BINARY_NAMES[] = {
#define X(a,b) b
        IR_BINARY_OP_LIST__
#undef X
};

struct list_of_IrInstruction_helpers list_of_IrInstruction_helpers = {
    .free = IrInstruction_free
};
#define NAME list_of_IrInstruction
#define TYPE struct IrInstruction*
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

struct IrProgram * ir_program_new() {
    struct IrProgram * program = (struct IrProgram*)malloc(sizeof(struct IrProgram));
    return program;
}

/**
 * Releases the memory associated with an IrProgram.
 * @param program the IrProgram to be freed.
 */
void IrProgram_free(struct IrProgram *program) {
    if (program->function) IrFunction_free(program->function);
    free(program);
}

struct IrFunction * ir_function_new(const char *name) {
    struct IrFunction *function = (struct IrFunction*)malloc(sizeof(struct IrFunction));
    function->name = name;
    list_of_IrInstruction_init(&function->body, 10);
    return function;
}

/**
 * Releases the memory associated with an IrFunction.
 * @param function the IrFunction to be freed.
 */
void IrFunction_free(struct IrFunction *function) {
    list_of_IrInstruction_free(&function->body);
    free(function);
}
void ir_function_append_instruction(struct IrFunction *function, struct IrInstruction *instruction) {
    list_of_IrInstruction_append(&function->body, instruction);
}

static struct IrInstruction* ir_instruction_new(enum IR_OP inst) {
    struct IrInstruction *instruction = (struct IrInstruction*)malloc(sizeof(struct IrInstruction));
    instruction->inst = inst;
    return instruction;
}

struct IrInstruction* ir_instruction_new_var(struct IrValue value) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_VAR);
    instruction->var.value = value;
    return instruction;
}

struct IrInstruction* ir_instruction_new_ret(struct IrValue value) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_RET);
    instruction->ret.value = value;
    return instruction;
}

struct IrInstruction *ir_instruction_new_unary(enum IR_UNARY_OP op, struct IrValue src, struct IrValue dst) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_UNARY);
    instruction->unary.op = op;
    instruction->unary.src = src;
    instruction->unary.dst = dst;
    return instruction;
}

struct IrInstruction *ir_instruction_new_binary(enum IR_BINARY_OP op, struct IrValue src1, struct IrValue src2, struct IrValue dst) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_BINARY);
    instruction->binary.op = op;
    instruction->binary.src1 = src1;
    instruction->binary.src2 = src2;
    instruction->binary.dst = dst;
    return instruction;
}
struct IrInstruction* ir_instruction_new_copy(struct IrValue src, struct IrValue dst) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_COPY);
    instruction->copy.src = src;
    instruction->copy.dst = dst;
    return instruction;
}
struct IrInstruction* ir_instruction_new_jump(struct IrValue target) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_JUMP);
    instruction->jump.target = target;
    return instruction;
}
struct IrInstruction* ir_instruction_new_jumpz(struct IrValue value, struct IrValue target) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_JUMP_ZERO);
    instruction->cjump.value = value;
    instruction->cjump.target = target;
    return instruction;
}
struct IrInstruction* ir_instruction_new_jumpnz(struct IrValue value, struct IrValue target) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_JUMP_NZERO);
    instruction->cjump.value = value;
    instruction->cjump.target = target;
    return instruction;
}
struct IrInstruction* ir_instruction_new_label(struct IrValue label) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_LABEL);
    instruction->label.label = label;
    return instruction;
}

/**
 * Releases the memory associated with an IrInstruction.
 * @param instruction the IrInstruction to be freed.
 */
void IrInstruction_free(struct IrInstruction *instruction) {
    free(instruction);
}

/**
 * Initialize a new IrValue.
 * @param valType The type of value, a member of IR_VAL.
 * @param valText The text representation; a variable name, register name, or constant literal (which
 *      must have a lifetime greater than the IrValue).
 * @return The new IrValue.
 */
struct IrValue ir_value_new(enum IR_VAL valType, const char *valText) {
    struct IrValue result = {.type = valType, .text = valText};
    return result;
}

struct IrValue ir_value_new_id(const char* id) {
    return ir_value_new(IR_VAL_ID, id);
}
struct IrValue ir_value_new_const(const char* text) {
    return ir_value_new(IR_VAL_CONST_INT, text);
}
