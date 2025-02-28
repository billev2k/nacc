//
// Created by Bill Evans on 9/25/24.
//

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "ast.h"
#include "ast2ir.h"
#include "idtable.h"

static void tmp_vars_init(void);

static void convert_symbols_to_ir(struct IrProgram *program);
static struct IrFunction *compile_function(const struct CFuncDecl *cFunction);
static void compile_block(const struct list_of_CBlockItem *block, struct IrFunction *irFunction);
static void compile_vardecl(const struct CDeclaration *declaration, struct IrFunction *function);

static void compile_statement(const struct CStatement *statement, struct IrFunction *function);

struct IrValue compile_expression(struct CExpression *cExpression, struct IrFunction *irFunction);

static struct IrValue make_temporary(const struct IrFunction *function);

static void make_conditional_labels(const struct IrFunction *function, struct IrValue *t, struct IrValue *f,
                                    struct IrValue *e);
static void make_loop_labels (const struct IrFunction *function, int flow_id, struct IrValue *s, struct IrValue *b, struct IrValue *c);
static void make_case_label(const struct IrFunction *function, int flow_id, int case_id, struct IrValue *label);
static void make_default_label(const struct IrFunction *function, int flow_id, struct IrValue *label);

struct IrProgram *ast2ir(const struct CProgram *cProgram) {
    tmp_vars_init();
    struct IrProgram *program = ir_program_new();
    struct IrFunction* function;
    for (int ix = 0; ix < cProgram->declarations.num_items; ix++) {
        struct CDeclaration* decl = cProgram->declarations.items[ix];
        switch (decl->decl_kind) {
            case FUNC_DECL:
                function = compile_function(decl->func);
                if (!function) continue;
                ir_program_add_function(program, function);
                break;
            case VAR_DECL:
                // static & extern (both top-level & block) vars are compiled from the
                // symbol table.
                break;
        }
    }
    convert_symbols_to_ir(program);
    return program;
}

void convert_symbols_to_ir(struct IrProgram *program) {
    struct Symbol *pSymbol;
    for (int ix=0; ix<get_num_symbols(); ix++) {
        pSymbol = get_symbol(ix);
        if (SYMBOL_IS_STATIC_VAR(pSymbol->attrs)) {
            struct IrConstant init_value = {.kind = CONST_INT, .int_val = 0};
            struct IrStaticVar *irStaticVar;
            switch (pSymbol->attrs & SYMBOL_STATIC_MASK) {
                case SYMBOL_STATIC_INITIALIZED:
                    init_value.int_val = pSymbol->int_val;
                    irStaticVar = ir_static_var_new(pSymbol->identifier.name,SYMBOL_IS_GLOBAL(pSymbol->attrs), init_value);
                    ir_program_add_static_var(program, irStaticVar);
                    break;
                case SYMBOL_STATIC_TENTATIVE:
                    init_value.int_val = 0;
                    irStaticVar = ir_static_var_new(pSymbol->identifier.name,SYMBOL_IS_GLOBAL(pSymbol->attrs), init_value);
                    ir_program_add_static_var(program, irStaticVar);
                    break;
                case SYMBOL_STATIC_NO_INIT:
                    break;
            }
        }
    }

}

static void compile_block(const struct list_of_CBlockItem *block, struct IrFunction *irFunction) {
    for (int ix = 0; ix < block->num_items; ix++) {
        struct CBlockItem *bi = block->items[ix];
        switch (bi->kind) {
            case AST_BI_STATEMENT:
                compile_statement(bi->statement, irFunction);
                break;
            case AST_BI_DECLARATION:
                switch (bi->declaration->decl_kind) {
                    case FUNC_DECL:
                        compile_function(bi->declaration->func);
                        break;
                    case VAR_DECL:
                        compile_vardecl(bi->declaration, irFunction);
                        break;
                }
                break;
        }
    }
}

struct IrFunction *compile_function(const struct CFuncDecl *cFunction) {
    // If only declaration, no body and nothing to compile.
    if (!cFunction->body) return NULL;
    bool global = false;
    struct Symbol symbol;
    if (find_symbol_by_name(cFunction->name, &symbol) != SYMTAB_OK) {
        failf("Function '%s' is not defined.", cFunction->name);
    }
    global = SYMBOL_IS_GLOBAL(symbol.attrs);
    struct IrFunction *function = ir_function_new(cFunction->name, global);
    for (int ix = 0; ix < cFunction->params.num_items; ix++) {
        IrFunction_add_param(function, cFunction->params.items[ix].name);
    }
    compile_block(&cFunction->body->items, function);

    // Add return instruction, in case the source didn't include one.
    struct IrValue zero = ir_value_new_int(0);
    struct IrInstruction* inst = ir_instruction_new_ret(zero);
    ir_function_append_instruction(function, inst);

    return function;
}

void compile_vardecl(const struct CDeclaration *declaration, struct IrFunction *function) {
    if (declaration->decl_kind != VAR_DECL) {
        failf("Expected a variable declaration, got a function declaration instead.");
    }
    struct CVarDecl *vardecl = (struct CVarDecl *)declaration->var;
    if (vardecl->storage_class == SC_EXTERN || vardecl->storage_class == SC_STATIC) {
        return; // extern and static handled later.
    }
    struct IrValue var = ir_value_new_id(vardecl->var.name);
    struct IrInstruction *inst = ir_instruction_new_var(var);
    ir_function_append_instruction(function, inst);
    if (vardecl->initializer) {
        struct IrValue initializer = compile_expression(vardecl->initializer, function);
        inst = ir_instruction_new_copy(initializer, var);
        ir_function_append_instruction(function, inst);
    }
}

static void compile_do_while(const struct CStatement *do_statement, struct IrFunction *function) {
    struct IrValue start_label;
    struct IrValue break_label;
    struct IrValue continue_label;
    make_loop_labels(function, do_statement->flow_id, &start_label, &break_label, &continue_label);

    // emit start label
    struct IrInstruction *inst = ir_instruction_new_label(start_label);
    ir_function_append_instruction(function, inst);
    // emit body
    compile_statement(do_statement->while_or_do_statement.body, function);
    // emit continue label
    inst = ir_instruction_new_label(continue_label);
    ir_function_append_instruction(function, inst);
    // emit condition and jnz
    struct IrValue condition = compile_expression(do_statement->while_or_do_statement.condition, function);
    inst = ir_instruction_new_jumpnz(condition, start_label);
    ir_function_append_instruction(function, inst);
    // emit break label
    inst = ir_instruction_new_label(break_label);
    ir_function_append_instruction(function, inst);
}

static void compile_while(const struct CStatement *while_statement, struct IrFunction *function) {
    struct IrValue break_label;
    struct IrValue continue_label;
    make_loop_labels(function, while_statement->flow_id, NULL, &break_label, &continue_label);

    // emit continue label(also the start label)
    struct IrInstruction *inst = ir_instruction_new_label(continue_label);
    ir_function_append_instruction(function, inst);
    // emit condition and jz
    struct IrValue condition = compile_expression(while_statement->while_or_do_statement.condition, function);
    inst = ir_instruction_new_jumpz(condition, break_label);
    ir_function_append_instruction(function, inst);
    // emit body
    compile_statement(while_statement->while_or_do_statement.body, function);
    // jump back to start, ie, continue
    inst = ir_instruction_new_jump(continue_label);
    ir_function_append_instruction(function, inst);
    // emit break label
    inst = ir_instruction_new_label(break_label);
    ir_function_append_instruction(function, inst);
}

static void compile_for(const struct CStatement *for_statement, struct IrFunction *function) {
    struct IrValue start_label;
    struct IrValue break_label;
    struct IrValue continue_label;
    make_loop_labels(function, for_statement->flow_id, &start_label, &break_label, &continue_label);

    // emit init
    if (for_statement->for_statement.init) {
        if (for_statement->for_statement.init->kind == FOR_INIT_EXPR) {
            compile_expression(for_statement->for_statement.init->expression, function);
        } else if (for_statement->for_statement.init->kind == FOR_INIT_DECL) {
            compile_vardecl(for_statement->for_statement.init->declaration, function);
        } else {
            assert("Unknown kind in 'for init'" && 0);
        }
    }
    // emit start label
    struct IrInstruction *inst = ir_instruction_new_label(start_label);
    ir_function_append_instruction(function, inst);
    // emit condition and "jz break", if present
    if (for_statement->for_statement.condition) {
        struct IrValue condition = compile_expression(for_statement->for_statement.condition, function);
        inst = ir_instruction_new_jumpz(condition, break_label);
        ir_function_append_instruction(function, inst);
    }
    // body
    compile_statement(for_statement->for_statement.body, function);
    // emit continue label
    inst = ir_instruction_new_label(continue_label);
    ir_function_append_instruction(function, inst);
    // post, if present
    if (for_statement->for_statement.post) {
        compile_expression(for_statement->for_statement.post, function);
    }
    inst = ir_instruction_new_jump(start_label);
    ir_function_append_instruction(function, inst);
    // emit break label
    inst = ir_instruction_new_label(break_label);
    ir_function_append_instruction(function, inst);
}

static void compile_switch(const struct CStatement *switch_statement, struct IrFunction *function) {
    struct IrInstruction* inst;
    struct IrValue break_label;
    struct IrValue label;
    make_loop_labels(function, switch_statement->flow_id, NULL, &break_label, NULL);

    // emit code to compute condition
    struct IrValue condition = compile_expression(switch_statement->switch_statement.expression, function);
    if (switch_statement->switch_statement.case_labels) {
        // Branch based on condition.
        int num_cases = switch_statement->switch_statement.case_labels->num_items;
        int *case_labels = switch_statement->switch_statement.case_labels->items;
        for (int i = 0; i < num_cases; ++i) {
            make_case_label(function, switch_statement->flow_id, case_labels[i], &label);
            inst = ir_instruction_new_jumpeq(condition, ir_value_new_int(case_labels[i]), label);
            ir_function_append_instruction(function, inst);
        }
    }
    if (switch_statement->switch_statement.has_default) {
        make_default_label(function, switch_statement->flow_id, &label);
    } else {
        label = break_label;
    }
    inst = ir_instruction_new_jump(label);
    ir_function_append_instruction(function, inst);

    // emit body
    compile_statement(switch_statement->switch_statement.body, function);
    // emit break label (which is also the "default" label, if there's no statement labelled "default:"
    inst = ir_instruction_new_label(break_label);
    ir_function_append_instruction(function, inst);
}

static void compile_labels(const struct CStatement *statement, struct IrFunction *function) {
    if (!c_statement_has_labels(statement)) return;
    struct IrInstruction *inst;
    struct IrValue label;
    int num_labels = c_statement_num_labels(statement);
    struct CLabel *labels = c_statement_get_labels(statement);
    for (int i=0; i<num_labels; ++i) {
        if (labels[i].kind == LABEL_DEFAULT) {
            make_default_label(function, labels[i].switch_flow_id, &label);
            inst = ir_instruction_new_label(label);
            ir_function_append_instruction(function, inst);
        } else if (labels[i].kind == LABEL_CASE) {
            int case_value = c_expression_get_const_value(labels[i].expr);
            make_case_label(function, labels[i].switch_flow_id, case_value, &label);
            inst = ir_instruction_new_label(label);
            ir_function_append_instruction(function, inst);
        } else if (labels[i].kind == LABEL_DECL) {
            label = ir_value_new_label(labels[i].identifier.name);
            inst = ir_instruction_new_label(label);
            ir_function_append_instruction(function, inst);
        }
    }
}

void compile_statement(const struct CStatement *statement, struct IrFunction *function) {
    struct IrValue src;
    struct IrValue condition;
    struct IrInstruction *inst;
    struct IrValue label;
    struct IrValue else_label;
    struct IrValue end_label;
    int has_else;

    // Emit any labels that target this statement.
    compile_labels(statement, function);

    switch (statement->kind) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            src = compile_expression(statement->expression, function);
            inst = ir_instruction_new_ret(src);
            ir_function_append_instruction(function, inst);
            break;
        case STMT_EXP:
            // Ignore return value; the IR code to evaluate the expression are emitted.
            compile_expression(statement->expression, function);
            break;
        case STMT_NULL:
            break;
        case STMT_IF:
            has_else = statement->if_statement.else_statement != NULL;
            make_conditional_labels(function, NULL, has_else ? &else_label : NULL, &end_label);
        // Condition
            condition = compile_expression(statement->if_statement.condition, function);
            inst = ir_instruction_new_jumpz(condition, has_else ? else_label : end_label);
            ir_function_append_instruction(function, inst);
        // Then statement
            compile_statement(statement->if_statement.then_statement, function);
            if (has_else) {
                inst = ir_instruction_new_jump(end_label);
                ir_function_append_instruction(function, inst);
                // Else statement
                inst = ir_instruction_new_label(else_label);
                ir_function_append_instruction(function, inst);
                compile_statement(statement->if_statement.else_statement, function);
            }
        // end
            inst = ir_instruction_new_label(end_label);
            ir_function_append_instruction(function, inst);
            break;
        case STMT_GOTO:
            label = ir_value_new_label(statement->goto_statement.label->var.name);
            inst = ir_instruction_new_jump(label);
            ir_function_append_instruction(function, inst);
            break;
        case STMT_COMPOUND:
            compile_block(&statement->compound->items, function);
            break;
        case STMT_BREAK:
            make_loop_labels(function, statement->flow_id, NULL, &label, NULL);
            inst = ir_instruction_new_jump(label);
            ir_function_append_instruction(function, inst);
            break;
        case STMT_CONTINUE:
            make_loop_labels(function, statement->flow_id, NULL, NULL, &label);
            inst = ir_instruction_new_jump(label);
            ir_function_append_instruction(function, inst);
            break;
        case STMT_DOWHILE:
            compile_do_while(statement, function);
            break;
        case STMT_FOR:
            compile_for(statement, function);
            break;
        case STMT_SWITCH:
            compile_switch(statement, function);
            break;
        case STMT_WHILE:
            compile_while(statement, function);
            break;
    }
}

/**
 * Emits IR instructions to evaluate the given expression. Returns an IrValue containing the location
 * of where the computed result is stored.
 * @param cExpression An AST expression to be evaluated.
 * @param irFunction The IrFunction in which the expression appears, and which will receive the instructions
 *      to evaluate the expression.
 * @return An IrValue with the location of where the computed int_val is stored.
 */
struct IrValue compile_expression(struct CExpression *cExpression, struct IrFunction *irFunction) {
    // NOLINT(*-no-recursion)
    struct IrValue src;
    struct IrValue src2;
    struct IrValue dst = {};
    struct IrValue tmp;
    struct IrValue target;
    struct IrValue condition;
    struct IrValue false_label;
    struct IrValue true_label;
    struct IrValue end_label;
    struct IrInstruction *inst;
    enum IR_UNARY_OP unary_op;
    enum IR_BINARY_OP binary_op;
    struct list_of_IrValue arg_list;
    switch (cExpression->kind) {
        case AST_EXP_CONST:
            switch (cExpression->literal.type) {
                case AST_CONST_INT:
                    return ir_value_new_int(cExpression->literal.int_val);
            }
            break;
        case AST_EXP_UNOP:
            unary_op = AST_TO_IR_UNARY[cExpression->unary.op];
            src = compile_expression(cExpression->unary.operand, irFunction);
            dst = make_temporary(irFunction);
            inst = ir_instruction_new_unary(unary_op, src, dst);
            ir_function_append_instruction(irFunction, inst);
            return dst;
        case AST_EXP_BINOP:
            switch (cExpression->binop.op) {
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
                    binary_op = AST_TO_IR_BINARY[cExpression->binop.op];
                    src = compile_expression(cExpression->binop.left, irFunction);
                    src2 = compile_expression(cExpression->binop.right, irFunction);
                    dst = make_temporary(irFunction);
                    inst = ir_instruction_new_binary(binary_op, src, src2, dst);
                    ir_function_append_instruction(irFunction, inst);
                    return dst;
                case AST_BINARY_L_AND:
                    dst = make_temporary(irFunction);
                    make_conditional_labels(irFunction, NULL, &false_label, &end_label);
                // Evaluate left-hand side of && and, if false, jump to false_label.
                    src = compile_expression(cExpression->binop.left, irFunction);
                    inst = ir_instruction_new_jumpz(src, false_label);
                    ir_function_append_instruction(irFunction, inst);
                // Otherwise, evaluate right-hand side of && and, if false, jump to false_label.
                    src2 = compile_expression(cExpression->binop.right, irFunction);
                    inst = ir_instruction_new_jumpz(src2, false_label);
                    ir_function_append_instruction(irFunction, inst);
                // Not false, so result is 1, then jump to end label.
                    inst = ir_instruction_new_copy(ir_value_new_int(1), dst);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_jump(end_label);
                    ir_function_append_instruction(irFunction, inst);
                // False, result is 0
                    inst = ir_instruction_new_label(false_label);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_copy(ir_value_new_int(0), dst);
                    ir_function_append_instruction(irFunction, inst);
                // End label
                    inst = ir_instruction_new_label(end_label);
                    ir_function_append_instruction(irFunction, inst);
                    break;
                case AST_BINARY_L_OR:
                    dst = make_temporary(irFunction);
                    make_conditional_labels(irFunction, &true_label, NULL, &end_label);
                // Evaluate left-hand side of || and, if true, jump to true_label.
                    src = compile_expression(cExpression->binop.left, irFunction);
                    inst = ir_instruction_new_jumpnz(src, true_label);
                    ir_function_append_instruction(irFunction, inst);
                // Otherwise, evaluate right-hand side of || and, if true, jump to true_label.
                    src2 = compile_expression(cExpression->binop.right, irFunction);
                    inst = ir_instruction_new_jumpnz(src2, true_label);
                    ir_function_append_instruction(irFunction, inst);
                // Not true, so result is 0, then jump to end label.
                    inst = ir_instruction_new_copy(ir_value_new_int(0), dst);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_jump(end_label);
                    ir_function_append_instruction(irFunction, inst);
                // True, result is 1
                    inst = ir_instruction_new_label(true_label);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_copy(ir_value_new_int(1), dst);
                    ir_function_append_instruction(irFunction, inst);
                // End label
                    inst = ir_instruction_new_label(end_label);
                    ir_function_append_instruction(irFunction, inst);
                    break;
                case AST_BINARY_ASSIGN:
                // This branch is only here to make the compiler happy.
                case AST_BINARY_QUESTION:
                    // Same.
                    break;
            }
            break;
        case AST_EXP_VAR:
            src = ir_value_new_id(cExpression->var.name);
//            inst = ir_instruction_new_var(src);
//            ir_function_append_instruction(irFunction, inst);
            dst = src;
            break;
        case AST_EXP_ASSIGNMENT:
            src = compile_expression(cExpression->assign.src, irFunction);
            dst = compile_expression(cExpression->assign.dst, irFunction);
            inst = ir_instruction_new_copy(src, dst);
            ir_function_append_instruction(irFunction, inst);
            break;
        case AST_EXP_INCREMENT:
            switch (cExpression->increment.op) {
                case AST_PRE_INCR:
                case AST_PRE_DECR:
                    binary_op = (cExpression->increment.op == AST_PRE_INCR) ? IR_BINARY_ADD : IR_BINARY_SUBTRACT;
                    src = compile_expression(cExpression->increment.operand, irFunction);
                    tmp = make_temporary(irFunction);
                    inst = ir_instruction_new_binary(binary_op, src, ir_value_new_int(1), tmp);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_copy(tmp, src);
                    ir_function_append_instruction(irFunction, inst);
                    dst = src;
                    break;
                case AST_POST_INCR:
                case AST_POST_DECR:
                    binary_op = (cExpression->increment.op == AST_POST_INCR) ? IR_BINARY_ADD : IR_BINARY_SUBTRACT;
                    dst = make_temporary(irFunction);
                    src = compile_expression(cExpression->increment.operand, irFunction);
                    inst = ir_instruction_new_copy(src, dst);
                    ir_function_append_instruction(irFunction, inst);
                    tmp = make_temporary(irFunction);
                    inst = ir_instruction_new_binary(binary_op, src, ir_value_new_int(1), tmp);
                    ir_function_append_instruction(irFunction, inst);
                    inst = ir_instruction_new_copy(tmp, src);
                    ir_function_append_instruction(irFunction, inst);
                    break;
            }
            break;
        case AST_EXP_CONDITIONAL:
            make_conditional_labels(irFunction, NULL, &false_label, &end_label);
            dst = make_temporary(irFunction);
        // Condition ("left") expression
            condition = compile_expression(cExpression->conditional.left_exp, irFunction);
            inst = ir_instruction_new_jumpz(condition, false_label);
            ir_function_append_instruction(irFunction, inst);
        // True ("middle") expression
            tmp = compile_expression(cExpression->conditional.middle_exp, irFunction);
            inst = ir_instruction_new_copy(tmp, dst);
            ir_function_append_instruction(irFunction, inst);
            inst = ir_instruction_new_jump(end_label);
            ir_function_append_instruction(irFunction, inst);
        // False ("right") expression
            inst = ir_instruction_new_label(false_label);
            ir_function_append_instruction(irFunction, inst);
            tmp = compile_expression(cExpression->conditional.right_exp, irFunction);
            inst = ir_instruction_new_copy(tmp, dst);
            ir_function_append_instruction(irFunction, inst);
        // exit
            inst = ir_instruction_new_label(end_label);
            ir_function_append_instruction(irFunction, inst);
            break;
        case AST_EXP_FUNCTION_CALL:
            dst = make_temporary(irFunction);
            target = ir_value_new_id(cExpression->function_call.func.name);
            list_of_IrValue_init(&arg_list, cExpression->function_call.args.num_items);
            for (int i=0; i<cExpression->function_call.args.num_items; ++i) {
                struct CExpression *arg = cExpression->function_call.args.items[i];
                struct IrValue arg_val = compile_expression(arg, irFunction);
                list_of_IrValue_append(&arg_list, arg_val);
            }
            inst = ir_instruction_new_funcall(target, &arg_list, dst);
            ir_function_append_instruction(irFunction, inst);
            list_of_IrValue_delete(&arg_list);
            break;
    }
    return dst;
}

/** //////////////////////////////////////////////////////////////////////////////
//
// Temporary variables. Temporary variables have unique, non-c names, and
// global, permanent scope.
//
// TODO: Maybe we can delete a function's temporaries after the function if
//      completely compiled.
*/
static const char *tmp_vars_insert(const char *str);

static struct IrValue make_temporary(const struct IrFunction *function) {
    const char *name_buf = uniquify_name("%.100s.tmp.%d", function->name);
    const char *tmp_name = tmp_vars_insert(name_buf);
    struct IrValue result = ir_value_new(IR_VAL_ID, tmp_name);
    return result;
}

static void make_unique_label(const char *context, const char *tag, int uniquifier, struct IrValue *label) {
    if (!label) return;
    char name_buf[120];
    sprintf(name_buf, "%.100s.%s.%d", context, tag, uniquifier);
    const char *tmp_name = tmp_vars_insert(name_buf);
    *label = ir_value_new_label(tmp_name);
}

static void make_conditional_labels(const struct IrFunction *function, struct IrValue *t, struct IrValue *f, struct IrValue *e) {
    int uniquifier = next_uniquifier();
    make_unique_label(function->name, "true", uniquifier, t);
    make_unique_label(function->name, "false", uniquifier, f);
    make_unique_label(function->name, "end", uniquifier, e);
}
static void make_loop_labels (const struct IrFunction *function, int flow_id, struct IrValue *s, struct IrValue *b, struct IrValue *c) {
    make_unique_label(function->name, "start", flow_id, s);
    make_unique_label(function->name, "break", flow_id, b);
    make_unique_label(function->name, "continue", flow_id, c);
}
static void make_case_label(const struct IrFunction *function, int flow_id, int case_id, struct IrValue *label) {
    char name_buf[120];
    sprintf(name_buf, "%.100s.switch.%d.case.%d", function->name, flow_id, case_id);
    const char *tmp_name = tmp_vars_insert(name_buf);
    *label = ir_value_new_label(tmp_name);
}
static void make_default_label(const struct IrFunction *function, int flow_id, struct IrValue *label) {
    char name_buf[120];
    sprintf(name_buf, "%.100s.switch.%d.default", function->name, flow_id);
    const char *tmp_name = tmp_vars_insert(name_buf);
    *label = ir_value_new_label(tmp_name);
}

struct set_of_str tmp_vars;
/**
 * Initialize the set of token_text strings.
 */
void tmp_vars_init(void) {
    set_of_str_init(&tmp_vars, 101);
}

const char *tmp_vars_insert(const char *str) {
    return set_of_str_insert(&tmp_vars, str);
}
