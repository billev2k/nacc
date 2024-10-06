//
// Created by Bill Evans on 9/25/24.
//

#include <stdio.h>
#include "ast2ir.h"

static struct IrFunction *compile_function(struct CFunction *cFunction);
static void compile_statement(struct CStatement *statement, struct IrFunction *function);
static struct IrValue *make_temporary(struct IrFunction *function);

struct IrProgram *ast2ir(struct CProgram *cProgram) {
    struct IrProgram *program = ir_program_new();
    program->function = compile_function(cProgram->function);
    return program;
}

struct IrFunction *compile_function(struct CFunction *cFunction) {
    struct IrFunction *function = ir_function_new(cFunction->name);
    compile_statement(cFunction->statement, function);
    return function;
}

struct IrValue *compile_expression(struct CExpression *cExpression, struct IrFunction *irFunction);

void compile_statement(struct CStatement *statement, struct IrFunction *function) {
    struct IrValue *src;
    struct IrInstruction *inst;

    switch (statement->type) {
        case STMT_RETURN:
            src = compile_expression(statement->expression, function);
            inst = ir_instruction_new_unary(IR_OP_RET, src, NULL);
            ir_function_append_instruction(function, inst);
            break;
    }
}

/**
 * Emits IR instructions to evaluate the given expression. Returns an IrValue* containing the location
 * of where the computed result is stored.
 * @param cExpression An AST expression to be evaluated.
 * @param irFunction The IrFunction in which the expression appears, and which will receive the instructions
 *      to evaluate the expression.
 * @return An IrValue * with the location of where the computed value is stored.
 */
struct IrValue *compile_expression(struct CExpression *cExpression, struct IrFunction *irFunction) { // NOLINT(*-no-recursion)
    struct IrValue *src;
    struct IrValue *dst;
    struct IrInstruction *inst;
    enum IR_OP op;
    switch (cExpression->type) {
        case EXP_CONST_INT:
            return IrValue_new(IR_VAL_CONST_INT, cExpression->value);
        case EXP_NEGATE:
            op = IR_OP_NEGATE;
            goto emit_unary_op;
        case EXP_COMPLEMENT:
            op = IR_OP_COMPLEMENT;
        emit_unary_op:
            src = compile_expression(cExpression->exp, irFunction);
            dst = make_temporary(irFunction);
            inst = ir_instruction_new_unary(op, src, dst);
            ir_function_append_instruction(irFunction, inst);
            return dst;
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// Temporary variables. Temporary variables have unique, non-c names, and
// global, permanent scope.
//
// TODO: Maybe we can delete a function's temporaries after the function if
//      completely compiled.
//
void tmp_vars_init(void);
const char *tmp_vars_insert(const char *str);
struct IrValue * make_temporary(struct IrFunction *function) {
    char name_buf[120];
    static int counter = 0;
    if (counter == 0) tmp_vars_init();
    sprintf(name_buf, "%.100s.tmp.%d", function->name, ++counter);
    const char *tmp_name = tmp_vars_insert(name_buf);
    struct IrValue *result = IrValue_new(IR_VAL_ID, tmp_name);
    return result;
}

struct set_of_str tmp_vars;
/**
 * Initialize the set of token_text strings.
 */
void tmp_vars_init(void) {
    set_of_str_init(&tmp_vars, 101);
}
const char * tmp_vars_insert(const char *str) {
    return set_of_str_insert(&tmp_vars, str);
}

