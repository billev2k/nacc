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


static void IrValue_delete(struct IrValue val) {}
struct list_of_IrValue_helpers list_of_IrValue_helpers = {
        .delete = IrValue_delete,
        .null = {0},
};
LIST_OF_ITEM_DEFN(list_of_IrValue,struct IrValue)

struct list_of_IrInstruction_helpers list_of_IrInstruction_helpers = {
        .delete = IrInstruction_delete
};
LIST_OF_ITEM_DEFN(list_of_IrInstruction,struct IrInstruction*)

struct list_of_top_level_helpers list_of_top_level_helpers = {
        .delete = ir_top_level_delete,
        .null = {},
};
LIST_OF_ITEM_DEFN(list_of_top_level,struct IrTopLevel*)
#include "inc/constant.h"


struct IrProgram * ir_program_new() {
    struct IrProgram * program = (struct IrProgram*)malloc(sizeof(struct IrProgram));
    list_of_top_level_init(&program->top_level, 10);
    return program;
}

void ir_program_add_function(struct IrProgram* program, struct IrFunction* function) {
    struct IrTopLevel *top_level = ir_top_level_new_function(function);
    list_of_top_level_append(&program->top_level, top_level);
}

void ir_program_add_static_var(struct IrProgram* program, struct IrStaticVar* static_var) {
    struct IrTopLevel *top_level = ir_top_level_new_static_var(static_var);
    list_of_top_level_append(&program->top_level, top_level);
}

/**
 * Releases the memory associated with an IrProgram.
 * @param program the IrProgram to be freed.
 */
void IrProgram_delete(struct IrProgram *program) {
    if (!program) return;
    list_of_top_level_delete(&program->top_level);
    free(program);
}

struct IrFunction *ir_function_new(const char *name, bool global) {
    struct IrFunction *function = (struct IrFunction*)malloc(sizeof(struct IrFunction));
    function->name = name;
    function->global = global;
    list_of_IrValue_init(&function->params, 10);
    list_of_IrInstruction_init(&function->body, 10);
    return function;
}

/**
 * Releases the memory associated with an IrFunction.
 * @param function the IrFunction to be freed.
 */
void IrFunction_delete(struct IrFunction *function) {
    list_of_IrInstruction_delete(&function->body);
    free(function);
}
void IrFunction_add_param(struct IrFunction* function, const char* param_name) {
    list_of_IrValue_append(&function->params, ir_value_new_id(param_name));
}
void ir_function_append_instruction(struct IrFunction *function, struct IrInstruction *instruction) {
    list_of_IrInstruction_append(&function->body, instruction);
}

//region IrStaticVar
struct IrStaticVar *ir_static_var_new(const char *name, bool global, struct Constant init_value) {
    struct IrStaticVar *static_var = (struct IrStaticVar*)malloc(sizeof(struct IrStaticVar));
    static_var->name = name;
    static_var->global = global;
    static_var->init_value = init_value;
    return static_var;
}
void ir_static_var_delete(struct IrStaticVar *static_var) {
    if (!static_var) return;
    free(static_var);
}
//endregion

//region IrTopLevel
struct IrTopLevel* ir_top_level_new_function(struct IrFunction *function) {
    struct IrTopLevel *top_level = (struct IrTopLevel*)malloc(sizeof(struct IrTopLevel));
    top_level->kind = IR_FUNCTION;
    top_level->function = function;
    return top_level;
}
struct IrTopLevel* ir_top_level_new_static_var(struct IrStaticVar *static_var) {
    struct IrTopLevel *top_level = (struct IrTopLevel*)malloc(sizeof(struct IrTopLevel));
    top_level->kind = IR_STATIC_VAR;
    top_level->static_var = static_var;
    return top_level;
}
void ir_top_level_delete(struct IrTopLevel *top_level) {
    switch (top_level->kind) {
        case IR_FUNCTION:
            IrFunction_delete(top_level->function);
            break;
        case IR_STATIC_VAR:
            ir_static_var_delete(top_level->static_var);
            break;
    }
    free(top_level);
}
//endregion

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
void IrInstruction_delete(struct IrInstruction *instruction) {
    switch (instruction->inst) {
        case IR_OP_FUNCALL:
            list_of_IrValue_delete(&instruction->funcall.args);
            break;
        default:
            break;
    }
    free(instruction);
}

struct IrValue ir_value_new_id(const char* id) {
    struct IrValue result = {.kind = IR_VAL_ID, .text = id};
    return result;
}
struct IrValue ir_value_new_label(const char* label_name) {
    struct IrValue result = {.kind = IR_VAL_LABEL, .text = label_name};
    return result;
}
struct IrValue ir_value_new_int(int int_val) {
    struct IrValue result = {.kind = IR_VAL_CONST, .const_value = mk_const_int(int_val)};
    return result;
}
struct IrValue ir_value_new_const(struct Constant const_val) {
    struct IrValue result = {.kind = IR_VAL_CONST, .const_value = const_val};
    return result;
}
