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


#define NAME list_of_IrValue
#define TYPE struct IrValue
static void IrValue_free(struct IrValue val) {}
struct list_of_IrValue_helpers list_of_IrValue_helpers = {
        .free = IrValue_free,
        .null = {0},
};
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

#define NAME list_of_IrInstruction
#define TYPE struct IrInstruction*
struct list_of_IrInstruction_helpers list_of_IrInstruction_helpers = {
        .free = IrInstruction_free
};
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

#define NAME list_of_IrFunction
#define TYPE struct IrFunction*
struct list_of_IrFunction_helpers list_of_IrFunction_helpers = {
        .free = IrFunction_free,
        .null = NULL,
};
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

struct IrProgram * ir_program_new() {
    struct IrProgram * program = (struct IrProgram*)malloc(sizeof(struct IrProgram));
    list_of_IrFunction_init(&program->functions, 10);
    return program;
}

void ir_program_add_function(struct IrProgram* program, struct IrFunction* function) {
    list_of_IrFunction_append(&program->functions, function);
}

/**
 * Releases the memory associated with an IrProgram.
 * @param program the IrProgram to be freed.
 */
void IrProgram_free(struct IrProgram *program) {
    if (!program) return;
    list_of_IrFunction_free(&program->functions);
    free(program);
}

struct IrFunction * ir_function_new(const char *name) {
    struct IrFunction *function = (struct IrFunction*)malloc(sizeof(struct IrFunction));
    function->name = name;
    list_of_IrValue_init(&function->params, 10);
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
void IrFunction_add_param(struct IrFunction* function, const char* param_name) {
    list_of_IrValue_append(&function->params, ir_value_new_id(param_name));
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
struct IrInstruction *ir_instruction_new_jumpeq(struct IrValue value, struct IrValue comparand, struct IrValue target) {
struct IrInstruction *instruction = ir_instruction_new(IR_OP_JUMP_EQ);
    instruction->cjump.value = value;
    instruction->cjump.comparand = comparand;
    instruction->cjump.target = target;
    return instruction;
}
struct IrInstruction* ir_instruction_new_jumpz(struct IrValue value, struct IrValue target) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_JUMP_ZERO);
    instruction->cjump.value = value;
    instruction->cjump.comparand = ir_value_new_int(0);
    instruction->cjump.target = target;
    return instruction;
}
struct IrInstruction* ir_instruction_new_jumpnz(struct IrValue value, struct IrValue target) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_JUMP_NZERO);
    instruction->cjump.value = value;
    instruction->cjump.comparand = ir_value_new_int(0);
    instruction->cjump.target = target;
    return instruction;
}
struct IrInstruction* ir_instruction_new_label(struct IrValue label) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_LABEL);
    instruction->label.label = label;
    return instruction;
}
struct IrInstruction* ir_instruction_new_funcall(struct IrValue func_name, struct list_of_IrValue* args, struct IrValue dst) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_FUNCALL);
    instruction->funcall.func_name = func_name;
    list_of_IrValue_init(&instruction->funcall.args, args->num_items);
    for (int i = 0; i < args->num_items; i++) {
        list_of_IrValue_append(&instruction->funcall.args, args->items[i]);
    }
    instruction->funcall.dst = dst;
    return instruction;
}

struct IrInstruction* ir_instruction_new_comment(const char *text) {
    struct IrInstruction *instruction = ir_instruction_new(IR_OP_COMMENT);
    instruction->comment.text = text;
    return instruction;
}
/**
 * Releases the memory associated with an IrInstruction.
 * @param instruction the IrInstruction to be freed.
 */
void IrInstruction_free(struct IrInstruction *instruction) {
    switch (instruction->inst) {
        case IR_OP_FUNCALL:
            list_of_IrValue_free(&instruction->funcall.args);
            break;
        default:
            break;
    }
    free(instruction);
}

/**
 * Initialize a new IrValue.
 * @param valKind The kind of int_val, a member of IR_VAL.
 * @param valText The text representation; a variable name, register name, or constant literal (which
 *      must have a lifetime greater than the IrValue).
 * @return The new IrValue.
 */
struct IrValue ir_value_new(enum IR_VAL valKind, const char *valText) {
    struct IrValue result = {.kind = valKind, .text = valText};
    return result;
}

struct IrValue ir_value_new_id(const char* id) {
    return ir_value_new(IR_VAL_ID, id);
}
struct IrValue ir_value_new_label(const char* label_name) {
    return ir_value_new(IR_VAL_LABEL, label_name);
}
struct IrValue ir_value_new_int(int int_val) {
    struct IrValue result = {.kind = IR_VAL_CONST_INT, .int_val = int_val};
    return result;
}
