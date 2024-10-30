//
// Created by Bill Evans on 9/23/24.
//

#include <stdlib.h>
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

DYN_LIST_OF_P_IMPL(IrInstruction)

struct IrProgram * ir_program_new() {
    struct IrProgram * program = (struct IrProgram*)malloc(sizeof(struct IrProgram));
    return program;
}

/**
 * Releases the memory associated with an IrProgram.
 * @param value the IrProgram to be freed.
 */
void IrProgram_free(struct IrProgram *program) {
    if (program->function) IrFunction_free(program->function);
    free(program);
}

struct IrFunction * ir_function_new(const char *name) {
    struct IrFunction *function = (struct IrFunction*)malloc(sizeof(struct IrFunction));
    function->name = name;
    return function;
}

/**
 * Releases the memory associated with an IrFunction.
 * @param value the IrFunction to be freed.
 */
void IrFunction_free(struct IrFunction *function) {
    IrInstruction_list_free(&function->body);
    free(function);
}
void ir_function_append_instruction(struct IrFunction *function, struct IrInstruction *instruction) {
    IrInstruction_list_append(&function->body, instruction);
}

struct IrInstruction *ir_instruction_new_nonary(enum IR_OP inst, struct IrValue *src) {
    struct IrInstruction *instruction = (struct IrInstruction*)malloc(sizeof(struct IrInstruction));
    instruction->inst = inst;
    instruction->a = src;
    return instruction;
}

struct IrInstruction *ir_instruction_new_unary(enum IR_UNARY_OP op, struct IrValue *src, struct IrValue *dst) {
    struct IrInstruction *instruction = (struct IrInstruction*)malloc(sizeof(struct IrInstruction));
    instruction->inst = IR_OP_UNARY;
    instruction->unary_op = op;
    instruction->a = src;
    instruction->b = dst;
    return instruction;
}

struct IrInstruction *ir_instruction_new_binary(enum IR_BINARY_OP op, struct IrValue *src1, struct IrValue *src2, struct IrValue *dst) {
    struct IrInstruction *instruction = (struct IrInstruction*)malloc(sizeof(struct IrInstruction));
    instruction->inst = IR_OP_BINARY;
    instruction->binary_op = op;
    instruction->a = src1;
    instruction->b = src2;
    instruction->c = dst;
    return instruction;
}

/**
 * Releases the memory associated with an IrInstruction.
 * @param value the IrInstruction to be freed.
 */
void IrInstruction_free(struct IrInstruction *instruction) {
    // TODO: FIX LEAK!
    //    if (instruction->a) ir_value_free(instruction->a);
//    if (instruction->b) ir_value_free(instruction->b);
//    if (instruction->c) ir_value_free(instruction->c);
    free(instruction);
}

/**
 * Allocate and initialize a new IrValue.
 * @param valType The type of value, a member of IR_VAL.
 * @param valText The text representation; a variable name, register name, or constant literal (which
 *      must have a lifetime greater than the IrValue).
 * @return A pointer to the new IrValue.
 */
struct IrValue * ir_value_new(enum IR_VAL valType, const char *valText) {
    struct IrValue * result = (struct IrValue *)malloc(sizeof(struct IrValue));
    result->type = valType;
    result->text = valText;
    return result;
}

/**
 * Releases the memory associated with an IrValue.
 * @param value the IrValue to be freed.
 */
void ir_value_free(struct IrValue *value) {
    free(value);
}

