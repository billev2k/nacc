//
// Created by Bill Evans on 9/25/24.
//

#include <stdio.h>
#include "../parser/ast.h"
#include "ast2ir.h"

static struct IrFunction *compile_function(struct CFunction *cFunction);
static void compile_statement(struct CStatement *statement, struct IrFunction *function);
static struct IrValue make_temporary(struct IrFunction *function);
static void make_conditional_labels(struct IrFunction *function, struct IrValue* t, struct IrValue* f, struct IrValue* e);

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

struct IrValue compile_expression(struct CExpression *cExpression, struct IrFunction *irFunction);

void compile_statement(struct CStatement *statement, struct IrFunction *function) {
    struct IrValue src;
    struct IrInstruction *inst;

    switch (statement->type) {
        case STMT_RETURN:
            src = compile_expression(statement->expression, function);
            inst = ir_instruction_new_ret(src);
            ir_function_append_instruction(function, inst);
            break;
    }
}

/**
 * Emits IR instructions to evaluate the given expression. Returns an IrValue containing the location
 * of where the computed result is stored.
 * @param cExpression An AST expression to be evaluated.
 * @param irFunction The IrFunction in which the expression appears, and which will receive the instructions
 *      to evaluate the expression.
 * @return An IrValue with the location of where the computed value is stored.
 */
struct IrValue compile_expression(struct CExpression *cExpression, struct IrFunction *irFunction) { // NOLINT(*-no-recursion)
    struct IrValue src;
    struct IrValue src2;
    struct IrValue dst;
    struct IrValue false_label;
    struct IrValue true_label;
    struct IrValue end_label;
    struct IrInstruction *inst;
    enum IR_UNARY_OP unary_op;
    enum IR_BINARY_OP binary_op;
    switch (cExpression->type) {
        case AST_EXP_CONST:
            switch (cExpression->const_type) {
                case AST_CONST_INT:
                    return ir_value_new(IR_VAL_CONST_INT, cExpression->value);
            }
            break;
        case AST_EXP_UNOP:
            unary_op = AST_TO_IR_UNARY[cExpression->unary_op];
            src = compile_expression(cExpression->operand, irFunction);
            dst = make_temporary(irFunction);
            inst = ir_instruction_new_unary(unary_op, src, dst);
            ir_function_append_instruction(irFunction, inst);
            return dst;
        case AST_EXP_BINOP:
            switch (cExpression->binary_op) {
                case AST_BINARY_MULTIPLY:
                case AST_BINARY_DIVIDE:
                case AST_BINARY_REMAINDER:
                case AST_BINARY_ADD:
                case AST_BINARY_SUBTRACT:
                case AST_BINARY_OR:
                case AST_BINARY_AND:
                case AST_BINARY_XOR:
                case AST_BINARY_LSHIFT:
                case AST_BINARY_RSHIFT:
                case AST_BINARY_LT:
                case AST_BINARY_LE:
                case AST_BINARY_GT:
                case AST_BINARY_GE:
                case AST_BINARY_EQ:
                case AST_BINARY_NE:
                    binary_op = AST_TO_IR_BINARY[cExpression->binary_op];
                    src = compile_expression(cExpression->left, irFunction);
                    src2 = compile_expression(cExpression->right, irFunction);
                    dst = make_temporary(irFunction);
                    inst = ir_instruction_new_binary(binary_op, src, src2, dst);
                    ir_function_append_instruction(irFunction, inst);
                    return dst;
                case AST_BINARY_L_AND:
                    dst = make_temporary(irFunction);
                    make_conditional_labels(irFunction, NULL, &false_label, &end_label);
                    // Evaluate left-hand side of && and, if false, jump to false_label.
                    src = compile_expression(cExpression->left, irFunction);
                    inst = ir_instruction_new_jumpz(src, false_label);
                    ir_function_append_instruction(irFunction, inst);
                    // Otherwise, evaluate right-hand side of && and, if false, jump to false_label.
                    src2 = compile_expression(cExpression->right, irFunction);
                    inst = ir_instruction_new_jumpz(src2, false_label);
                    ir_function_append_instruction(irFunction, inst);
                    // Not false, so result is 1, then jump to end label.
                    inst = ir_instruction_new_copy(ir_value_new_const("1"), dst);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_jump(end_label);
                    ir_function_append_instruction(irFunction, inst);
                    // False, result is 0
                    inst = ir_instruction_new_label(false_label);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_copy(ir_value_new_const("0"), dst);
                    ir_function_append_instruction(irFunction, inst);
                    // End label
                    inst = ir_instruction_new_label(end_label);
                    ir_function_append_instruction(irFunction, inst);
                    break;
                case AST_BINARY_L_OR:
                    dst = make_temporary(irFunction);
                    make_conditional_labels(irFunction, &true_label, NULL, &end_label);
                    // Evaluate left-hand side of || and, if true, jump to true_label.
                    src = compile_expression(cExpression->left, irFunction);
                    inst = ir_instruction_new_jumpnz(src, true_label);
                    ir_function_append_instruction(irFunction, inst);
                    // Otherwise, evaluate right-hand side of || and, if true, jump to true_label.
                    src2 = compile_expression(cExpression->right, irFunction);
                    inst = ir_instruction_new_jumpnz(src2, true_label);
                    ir_function_append_instruction(irFunction, inst);
                    // Not true, so result is 0, then jump to end label.
                    inst = ir_instruction_new_copy(ir_value_new_const("0"), dst);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_jump(end_label);
                    ir_function_append_instruction(irFunction, inst);
                    // True, result is 1
                    inst = ir_instruction_new_label(true_label);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_copy(ir_value_new_const("1"), dst);
                    ir_function_append_instruction(irFunction, inst);
                    // End label
                    inst = ir_instruction_new_label(end_label);
                    ir_function_append_instruction(irFunction, inst);
                    break;
            }
    }
    return dst;
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
struct IrValue make_temporary(struct IrFunction *function) {
    char name_buf[120];
    static int counter = 0;
    if (counter == 0) tmp_vars_init();
    sprintf(name_buf, "%.100s.tmp.%d", function->name, ++counter);
    const char *tmp_name = tmp_vars_insert(name_buf);
    struct IrValue result = ir_value_new(IR_VAL_ID, tmp_name);
    return result;
}

static void make_conditional_labels(struct IrFunction *function, struct IrValue* t, struct IrValue* f, struct IrValue* e) {
    char name_buf[120];
    static int counter = 0;
    if (counter == 0) tmp_vars_init();
    ++counter;
    if (t) {
        sprintf(name_buf, "%.100s.true.%d", function->name, counter);
        const char *tmp_name = tmp_vars_insert(name_buf);
        *t = ir_value_new(IR_VAL_LABEL, tmp_name);
    }
    if (f) {
        sprintf(name_buf, "%.100s.false.%d", function->name, counter);
        const char *tmp_name = tmp_vars_insert(name_buf);
        *f = ir_value_new(IR_VAL_LABEL, tmp_name);
    }
    if (e) {
        sprintf(name_buf, "%.100s.end.%d", function->name, counter);
        const char *tmp_name = tmp_vars_insert(name_buf);
        *e = ir_value_new(IR_VAL_LABEL, tmp_name);
    }
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

