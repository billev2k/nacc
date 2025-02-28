#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 8/28/24.
//

#include <stdlib.h>
#include <string.h>
#include <printf.h>

#include "ast.h"

#include <assert.h>

#include "../utils/startup.h"

//region list and set definitions
//region struct CIdentifier
void c_variable_free(struct CIdentifier var) {}
struct list_of_CIdentifier_helpers list_of_CIdentifier_helpers = {
        .free = c_variable_free,
        .null = {0},
};
#define NAME list_of_CIdentifier
#define TYPE struct CIdentifier
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE
//endregion struct CIdentifier

void c_label_free(struct CLabel var) {}
struct list_of_CLabel_helpers list_of_CLabel_helpers = {
        .free = c_label_free,
        .null = {0},
};
#define NAME list_of_CLabel
#define TYPE struct CLabel
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

struct list_of_CExpression_helpers list_of_CExpression_helpers = { .free = c_expression_free };
#define NAME list_of_CExpression
#define TYPE struct CExpression*
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

struct list_of_CBlockItem_helpers list_of_CBlockItem_helpers = { .free = c_block_item_free };
#define NAME list_of_CBlockItem
#define TYPE struct CBlockItem*
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE

struct list_of_CFunction_helpers list_of_CFunction_helpers = { .free = c_function_free };
#define NAME list_of_CFunction
#define TYPE struct CFunction*
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE
//endregion list and set definitions

//region global data definitions (kind names, precedence tables, etc.)
const char * const ASM_CONST_TYPE_NAMES[] = {
#define X(a,b) b 
    AST_CONST_LIST__
#undef X
};

const char * const AST_BINARY_NAMES[] = {
#define X(a,b,c,d) b
    AST_BINARY_LIST__
#undef X
};
const int AST_BINARY_PRECEDENCE[] = {
#define X(a,b,c,d) c
    AST_BINARY_LIST__
#undef X
};
const int AST_TO_IR_BINARY[] = {
#define X(a,b,c,d) d
    AST_BINARY_LIST__
#undef X
};

const char * const AST_UNARY_NAMES[] = {
#define X(a,b) b
        AST_UNARY_LIST__
#undef X
};
enum IR_UNARY_OP const AST_TO_IR_UNARY[] = {
#define X(a,b) IR_UNARY_##a
        AST_UNARY_LIST__
#undef X
};
//endregion global data

//region CExpression
static struct CExpression* c_expression_new(enum AST_EXP_KIND kind) {
    struct CExpression* expression = malloc(sizeof(struct CExpression));
    expression->kind = kind;
    return expression;
}
struct CExpression* c_expression_new_assign(struct CExpression* src, struct CExpression* dst) {
    struct CExpression* expression = c_expression_new(AST_EXP_ASSIGNMENT);
    expression->assign.dst = dst;
    expression->assign.src = src;
    return expression;
}
struct CExpression* c_expression_new_binop(enum AST_BINARY_OP op, struct CExpression* left, struct CExpression* right) {
    struct CExpression* expression = c_expression_new(AST_EXP_BINOP);
    expression->binop.op = op;
    expression->binop.left = left;
    expression->binop.right = right;
    return expression;
}
struct CExpression* c_expression_new_conditional(struct CExpression* left_exp, struct CExpression* middle_exp, struct CExpression* right_exp) {
    struct CExpression* expression = c_expression_new(AST_EXP_CONDITIONAL);
    expression->conditional.left_exp = left_exp;
    expression->conditional.middle_exp = middle_exp;
    expression->conditional.right_exp = right_exp;
    return expression;
}
struct CExpression* c_expression_new_const(enum AST_CONST_TYPE const_type, int int_val) {
    struct CExpression* expression = c_expression_new(AST_EXP_CONST);
    expression->literal.type = const_type;
    expression->literal.int_val = int_val;
    return expression;
}
struct CExpression* c_expression_new_function_call(struct CIdentifier func) {
    struct CExpression* expression = c_expression_new(AST_EXP_FUNCTION_CALL);
    expression->function_call.func = func;
    list_of_CExpression_init(&expression->function_call.args, 7);
    return expression;
}
struct CExpression* c_expression_new_increment(enum AST_INCREMENT_OP op, struct CExpression* operand) {
    struct CExpression* expression = c_expression_new(AST_EXP_INCREMENT);
    expression->increment.op = op;
    expression->increment.operand = operand;
    return expression;
}
struct CExpression* c_expression_new_unop(enum AST_UNARY_OP op, struct CExpression* operand) {
    struct CExpression* expression = c_expression_new(AST_EXP_UNOP);
    expression->unary.op = op;
    expression->unary.operand = operand;
    return expression;
}
struct CExpression* c_expression_new_var(const char* name) {
    struct CExpression* expression = c_expression_new(AST_EXP_VAR);
    expression->var.name = name;
    expression->var.source_name = name;
    return expression;
}
void c_expression_function_call_add_arg(struct CExpression* call_expr, struct CExpression* arg) {
    if (call_expr->kind != AST_EXP_FUNCTION_CALL) {
        fprintf(stderr, "Error: c_expression_function_call_add_arg called on non-function call expression\n");
        exit(1);
    }
    list_of_CExpression_append(&call_expr->function_call.args, arg);
}
struct CExpression* c_expression_clone(const struct CExpression* expression) {
    struct CExpression* clone = c_expression_new(expression->kind);
    switch (expression->kind) {
        case AST_EXP_BINOP:
            clone->binop.op = expression->binop.op;
            if (expression->binop.left) clone->binop.left = c_expression_clone(expression->binop.left);
            if (expression->binop.right) clone->binop.right = c_expression_clone(expression->binop.right);
            break;
        case AST_EXP_CONST:
            clone->literal.type = expression->literal.type;
            clone->literal.int_val = expression->literal.int_val;
            break;
        case AST_EXP_UNOP:
            clone->unary.op = expression->unary.op;
            if (expression->unary.operand) clone->unary.operand = c_expression_clone(expression->unary.operand);
            break;
        case AST_EXP_VAR:
            clone->var.name = expression->var.name;
            clone->var.source_name = expression->var.source_name;
            break;
        case AST_EXP_ASSIGNMENT:
            if (expression->assign.src) clone->assign.src = c_expression_clone(expression->assign.src);
            if (expression->assign.dst) clone->assign.dst = c_expression_clone(expression->assign.dst);
            break;
        case AST_EXP_INCREMENT:
            clone->increment.op = expression->increment.op;
            clone->increment.operand = c_expression_clone(expression->increment.operand);
            break;
        case AST_EXP_CONDITIONAL:
            clone->conditional.left_exp = c_expression_clone(expression->conditional.left_exp);
            clone->conditional.middle_exp = c_expression_clone(expression->conditional.middle_exp);
            clone->conditional.right_exp = c_expression_clone(expression->conditional.right_exp);
            break;
        case AST_EXP_FUNCTION_CALL:
            clone->function_call.func = expression->function_call.func;
            list_of_CExpression_init(&clone->function_call.args, expression->function_call.args.num_items);
            for (int i = 0; i < expression->function_call.args.num_items; i++) {
                list_of_CExpression_append(&clone->function_call.args, c_expression_clone(expression->function_call.args.items[i]));
            }
            break;
    }
    return clone;
}
void c_expression_free(struct CExpression *expression) {
    if (!expression) return;
    if (traceAstMem) printf("Free expression @ %p\n", expression);
    switch (expression->kind) {
        case AST_EXP_BINOP:
            if (expression->binop.left) c_expression_free(expression->binop.left);
            if (expression->binop.right) c_expression_free(expression->binop.right);
            break;
        case AST_EXP_CONST:
            // Nothing to do; strings owned by global string table.
            break;
        case AST_EXP_UNOP:
            if (expression->unary.operand) c_expression_free(expression->unary.operand);
            break;
        case AST_EXP_VAR:
            break;
        case AST_EXP_ASSIGNMENT:
            if (expression->assign.src) c_expression_free(expression->assign.src);
            if (expression->assign.dst) c_expression_free(expression->assign.dst);
            break;
        case AST_EXP_INCREMENT:
            c_expression_free(expression->increment.operand);
            break;
        case AST_EXP_CONDITIONAL:
            c_expression_free(expression->conditional.left_exp);
            c_expression_free(expression->conditional.middle_exp);
            c_expression_free(expression->conditional.right_exp);
            break;
        case AST_EXP_FUNCTION_CALL:
            list_of_CExpression_free(&expression->function_call.args);
            break;
    }
    free(expression);
}
//endregion CExpression

//region CBlock
struct CBlock* c_block_new(int is_function) {
    struct CBlock* result = malloc(sizeof(struct CBlock));
    list_of_CBlockItem_init(&result->items, 101);
    result->is_function_block = is_function;
    return result;
}
void c_block_append_item(struct CBlock* block, struct CBlockItem* item) {
    list_of_CBlockItem_append(&block->items, item);
}
void c_block_free(struct CBlock *block) {
    list_of_CBlockItem_free(&block->items);
    free(block);
}
//endregion

//region CStatement
static struct CStatement* c_statement_new(enum AST_STMT kind) {
    struct CStatement* statement = malloc(sizeof(struct CStatement));
    statement->kind = kind;
    return statement;
}
struct CStatement* c_statement_new_break(void) {
    struct CStatement* statement = c_statement_new(STMT_BREAK);
    return statement;
}
struct CStatement* c_statement_new_compound(struct CBlock* block) {
    struct CStatement* statement = c_statement_new(STMT_COMPOUND);
    statement->compound = block;
    return statement;
}
struct CStatement* c_statement_new_continue(void) {
    struct CStatement* statement = c_statement_new(STMT_CONTINUE);
    return statement;
}
struct CStatement* c_statement_new_do(struct CStatement* body, struct CExpression* condition) {
    struct CStatement* statement = c_statement_new(STMT_DOWHILE);
    statement->while_or_do_statement.body = body;
    statement->while_or_do_statement.condition = condition;
    return statement;
}
struct CStatement* c_statement_new_exp(struct CExpression* expression) {
    struct CStatement* statement = c_statement_new(STMT_EXP);
    statement->expression = expression;
    return statement;
}
struct CStatement* c_statement_new_for(struct CForInit* init, struct CExpression* condition, struct CExpression* post, struct CStatement* body) {
    struct CStatement* statement = c_statement_new(STMT_FOR);
    statement->for_statement.init = init;
    statement->for_statement.condition = condition;
    statement->for_statement.post = post;
    statement->for_statement.body = body;
    return statement;
}
struct CStatement *c_statement_new_if(struct CExpression *condition, struct CStatement *then_statement,
                                      struct CStatement *else_statement) {
    struct CStatement* statement = c_statement_new(STMT_IF);
    statement->if_statement.condition = condition;
    statement->if_statement.then_statement = then_statement;
    statement->if_statement.else_statement = else_statement;
    return statement;
}        
struct CStatement* c_statement_new_goto(struct CExpression* label) {
    struct CStatement* statement = c_statement_new(STMT_GOTO);
    statement->goto_statement.label = label;
    return statement;
}
struct CStatement* c_statement_new_null(void) {
    struct CStatement* statement = c_statement_new(STMT_NULL);
    return statement;
}
struct CStatement* c_statement_new_return(struct CExpression* expression) {
    struct CStatement* statement = c_statement_new(STMT_RETURN);
    statement->expression = expression;
    return statement;
}
struct CStatement* c_statement_new_switch(struct CExpression* expression, struct CStatement* body) {
    struct CStatement* statement = c_statement_new(STMT_SWITCH);
    statement->switch_statement.expression = expression;
    statement->switch_statement.body = body;
    return statement;
}
struct CStatement* c_statement_new_while(struct CExpression* condition, struct CStatement *body) {
    struct CStatement* statement = c_statement_new(STMT_WHILE);
    statement->while_or_do_statement.condition = condition;
    statement->while_or_do_statement.body = body;
    return statement;
}

int c_statement_has_labels(const struct CStatement * statement) {
    return statement->labels != NULL;
}
void c_statement_add_labels(struct CStatement *statement, struct list_of_CLabel newLabels) {
    if (statement->labels == NULL) {
        statement->labels = malloc(sizeof(struct list_of_CLabel));
        list_of_CLabel_init(statement->labels, newLabels.num_items);
    }
    for (int i = 0; i < newLabels.num_items; i++) {
        list_of_CLabel_append(statement->labels, newLabels.items[i]);
    }
}
int c_statement_num_labels(const struct CStatement* statement) {
    return statement->labels ? statement->labels->num_items : 0;
}
struct CLabel  * c_statement_get_labels(const struct CStatement * statement) {
    static struct CLabel empty = {0};
    if (statement->labels == NULL) return &empty;
    return statement->labels->items;
}
enum AST_RESULT c_statement_set_flow_id(struct CStatement * statement, int flow_id) {
    if (statement->flow_id != 0) return AST_DUPLICATE;
    statement->flow_id = flow_id;
    return AST_OK;
}
enum AST_RESULT c_statement_set_switch_has_default(struct CStatement *statement) {
    if (statement->switch_statement.has_default) return AST_DUPLICATE;
    statement->switch_statement.has_default = 1;
    return AST_OK;
}
enum AST_RESULT c_statement_register_switch_case(struct CStatement *statement, int case_value) {
    if (statement->switch_statement.case_labels == NULL) {
        statement->switch_statement.case_labels = malloc(sizeof(struct list_of_int));
        list_of_int_init(statement->switch_statement.case_labels, 31);
    } else {
        for (int ix = 0; ix < statement->switch_statement.case_labels->num_items; ix++) {
            if (statement->switch_statement.case_labels->items[ix] == case_value) { return AST_DUPLICATE; }
        }
    }
    list_of_int_append(statement->switch_statement.case_labels, case_value);
    return AST_OK;
}

void c_statement_free(struct CStatement *statement) {
    if (!statement) return;
    switch (statement->kind) {
        case STMT_BREAK:
            // Nothing to free.
            break;
        case STMT_COMPOUND:
            list_of_CBlockItem_free(&statement->compound->items);
            break;
        case STMT_CONTINUE:
            // Nothing to free.
            break;
        case STMT_DOWHILE:
            c_expression_free(statement->while_or_do_statement.condition);
            c_statement_free(statement->while_or_do_statement.body);
            break;
        case STMT_EXP:
            c_expression_free(statement->expression);
            break;
        case STMT_FOR:
            c_for_init_free(statement->for_statement.init);
            c_expression_free(statement->for_statement.condition);
            c_expression_free(statement->for_statement.post);
            c_statement_free(statement->for_statement.body);
            break;
        case STMT_GOTO:
            c_expression_free(statement->goto_statement.label);
            break;
        case STMT_IF:
            c_expression_free(statement->if_statement.condition);
            c_statement_free(statement->if_statement.then_statement);
            if (statement->if_statement.else_statement) {
                c_statement_free(statement->if_statement.else_statement);
            }
            break;
        case STMT_NULL:
            // No-op
            break;
        case STMT_SWITCH:
            c_expression_free(statement->switch_statement.expression);
            c_statement_free(statement->switch_statement.body);
            if (statement->switch_statement.case_labels) {
                list_of_int_free(statement->switch_statement.case_labels);
                free(statement->switch_statement.case_labels);
            }
            break;
        case STMT_WHILE:
            c_expression_free(statement->while_or_do_statement.condition);
            c_statement_free(statement->while_or_do_statement.body);
            break;
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            c_expression_free(statement->expression);
            break;
    }
    if (statement->labels) {
        list_of_CLabel_free(statement->labels);
        free(statement->labels);
    }
    free(statement);
}
//endregion CStatement

//region struct CVarDecl
struct CVarDecl* c_vardecl_new(const char* identifier) {
    struct CVarDecl* result = malloc(sizeof(struct CVarDecl));
    result->var.name = identifier;
    result->var.source_name = identifier;
    return result;
}
struct CVarDecl* c_vardecl_new_init(const char* identifier, struct CExpression* initializer) {
    struct CVarDecl* result = c_vardecl_new(identifier);
    result->initializer = initializer;
    return result;
}
void c_vardecl_free(struct CVarDecl* vardecl) {
    if (!vardecl) return;
    if (vardecl->initializer) {
        c_expression_free(vardecl->initializer);
    }
}
//endregion struct CVarDecl

//region struct CForInit
struct CForInit* c_for_init_new(enum FOR_INIT_KIND kind) {
    struct CForInit* result = malloc(sizeof(struct CForInit));
    result->kind = kind;
    return result;
}
struct CForInit* c_for_init_new_vardecl(struct CVarDecl* vardecl)  {
    struct CForInit* result = c_for_init_new(FOR_INIT_DECL);
    result->vardecl = vardecl;
    return result;
}
struct CForInit* c_for_init_new_expression(struct CExpression* expression)  {
    struct CForInit* result = c_for_init_new(FOR_INIT_EXPR);
    result->expression = expression;
    return result;
}
void c_for_init_free(struct CForInit* for_init)  {
    if (!for_init) return;
    if (for_init->kind == FOR_INIT_DECL) {
        c_vardecl_free(for_init->vardecl);
    } else if (for_init->kind == FOR_INIT_EXPR) {
        c_expression_free(for_init->expression);
    }
}
//endregion

//region struct CBlockItem
struct CBlockItem* c_block_item_new_var_decl(struct CVarDecl* vardecl) {
    struct CBlockItem* result = malloc(sizeof(struct CBlockItem));
    result->kind = AST_BI_VAR_DECL;
    result->vardecl = vardecl;
    return result;
}
struct CBlockItem* c_block_item_new_func_decl(struct CFunction* funcdecl) {
    struct CBlockItem* result = malloc(sizeof(struct CBlockItem));
    result->kind = AST_BI_FUNC_DECL;
    result->funcdecl = funcdecl;
    return result;
}
struct CBlockItem* c_block_item_new_stmt(struct CStatement* statement) {
    struct CBlockItem* result = malloc(sizeof(struct CBlockItem));
    result->kind = AST_BI_STATEMENT;
    result->statement = statement;
    return result;
}
void c_block_item_free(struct CBlockItem* blockItem) {
    if (blockItem == NULL) return;
    switch (blockItem->kind) {
        case AST_BI_STATEMENT:
            c_statement_free(blockItem->statement);
            break;
        case AST_BI_VAR_DECL:
            c_vardecl_free(blockItem->vardecl);
            break;
        case AST_BI_FUNC_DECL:
            c_function_free(blockItem->funcdecl);
            break;
    }
    free(blockItem);
}
//endregion CBlockItem

//region struct CFunction
struct CFunction *c_function_new(const char *name) {
    struct CFunction* result = malloc(sizeof(struct CFunction));
    result->name = name;
    result->body = NULL;
    list_of_CIdentifier_init(&result->params, 7);
    return result;
}
enum AST_RESULT c_function_add_param(struct CFunction* function, const char* param_name) {
    struct CIdentifier* params = function->params.items;
    for (int i = 0; i < function->params.num_items; i++) {
        if (strcmp(params[i].name, param_name) == 0) {
            return AST_DUPLICATE;
        }
    }
    struct CIdentifier param = {param_name, param_name};
    list_of_CIdentifier_append(&function->params, param);
    return AST_OK;
}
enum AST_RESULT c_function_add_body(struct CFunction* function, struct CBlock* body) {
    if (function->body) return AST_DUPLICATE;
    function->body = body;
    return AST_OK;
}
void c_function_free(struct CFunction *function) {
    if (!function) return;
    // Don't free 'name'; owned by global name table.
    if (function->body) {
        c_block_free(function->body);
    }
    list_of_CIdentifier_free(&function->params);
    free(function);
}
//endregion CFunction

//region CProgram
struct CProgram* c_program_new(void) {
    struct CProgram* result = malloc(sizeof(struct CProgram));
    list_of_CFunction_init(&result->functions, 3);
    return result;
}
enum AST_RESULT c_program_add_func(struct CProgram* program, struct CFunction* function) {
    list_of_CFunction_append(&program->functions, function);
    return AST_OK;
}
void c_program_free(struct CProgram *program) {
    if (!program) return;
    list_of_CFunction_free(&program->functions);
    free(program);
}
//endregion CProgram


#pragma clang diagnostic pop