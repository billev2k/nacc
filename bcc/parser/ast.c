#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 8/28/24.
//

#include <stdlib.h>
#include <string.h>
#include <printf.h>

#include "ast.h"

#include "../utils/startup.h"

//region list and set definitions
//region struct CIdentifier
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
void c_variable_delete(struct CIdentifier var) {}
#pragma clang diagnostic pop
struct list_of_CIdentifier_helpers list_of_CIdentifier_helpers = { .delete = c_variable_delete, .null = {0} };
#define NAME list_of_CIdentifier
#define TYPE struct CIdentifier
#include "../utils/list_of_item.tmpl"
//endregion struct CIdentifier

void c_label_delete(struct CLabel var) {}
struct list_of_CLabel_helpers list_of_CLabel_helpers = { .delete = c_label_delete, .null = {0} };
#define NAME list_of_CLabel
#define TYPE struct CLabel
#include "../utils/list_of_item.tmpl"

struct list_of_CExpression_helpers list_of_CExpression_helpers = { .delete = c_expression_delete };
#define NAME list_of_CExpression
#define TYPE struct CExpression*
#include "../utils/list_of_item.tmpl"

struct list_of_CBlockItem_helpers list_of_CBlockItem_helpers = { .delete = c_block_item_delete };
#define NAME list_of_CBlockItem
#define TYPE struct CBlockItem*
#include "../utils/list_of_item.tmpl"

struct list_of_CFuncDecl_helpers list_of_CFuncDecl_helpers = { .delete = c_function_delete };
#define NAME list_of_CFuncDecl
#define TYPE struct CFuncDecl*
#include "../utils/list_of_item.tmpl"
//endregion list and set definitions

struct list_of_CDeclaration_helpers list_of_CDeclaration_helpers = { .delete = c_declaration_delete };
#define TYPE struct CDeclaration*
#define NAME list_of_CDeclaration
#include "../utils/list_of_item.tmpl"

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
void c_expression_delete(struct CExpression *expression) {
    if (!expression) return;
    if (traceAstMem) printf("Free expression @ %p\n", expression);
    switch (expression->kind) {
        case AST_EXP_BINOP:
            if (expression->binop.left) c_expression_delete(expression->binop.left);
            if (expression->binop.right) c_expression_delete(expression->binop.right);
            break;
        case AST_EXP_CONST:
            // Nothing to do; strings owned by global string table.
            break;
        case AST_EXP_UNOP:
            if (expression->unary.operand) c_expression_delete(expression->unary.operand);
            break;
        case AST_EXP_VAR:
            break;
        case AST_EXP_ASSIGNMENT:
            if (expression->assign.src) c_expression_delete(expression->assign.src);
            if (expression->assign.dst) c_expression_delete(expression->assign.dst);
            break;
        case AST_EXP_INCREMENT:
            c_expression_delete(expression->increment.operand);
            break;
        case AST_EXP_CONDITIONAL:
            c_expression_delete(expression->conditional.left_exp);
            c_expression_delete(expression->conditional.middle_exp);
            c_expression_delete(expression->conditional.right_exp);
            break;
        case AST_EXP_FUNCTION_CALL:
            list_of_CExpression_delete(&expression->function_call.args);
            break;
    }
    free(expression);
}
//endregion CExpression

struct CDeclaration* c_declaration_new_var(struct CVarDecl* vardecl) {
    struct CDeclaration* declaration = malloc(sizeof(struct CDeclaration));
    declaration->decl_kind = VAR_DECL;
    declaration->var = vardecl;
    return declaration;
}
struct CDeclaration* c_declaration_new_func(struct CFuncDecl* funcdecl) {
    struct CDeclaration* declaration = malloc(sizeof(struct CDeclaration));
    declaration->decl_kind = FUNC_DECL;
    declaration->func = funcdecl;
    return declaration;
}
void c_declaration_delete(struct CDeclaration* declaration) {
    switch (declaration->decl_kind) {
        case FUNC_DECL:
            c_function_delete(declaration->func);
            break;
        case VAR_DECL:
            c_vardecl_delete(declaration->var);
            break;
    }
    free(declaration);
}

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
void c_block_delete(struct CBlock *block) {
    list_of_CBlockItem_delete(&block->items);
    free(block);
}
//endregion

//region CStatement
static struct CStatement* c_statement_new(enum AST_STMT_KIND kind) {
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

void c_statement_delete(struct CStatement *statement) {
    if (!statement) return;
    switch (statement->kind) {
        case STMT_BREAK:
            // Nothing to free.
            break;
        case STMT_COMPOUND:
            list_of_CBlockItem_delete(&statement->compound->items);
            break;
        case STMT_CONTINUE:
            // Nothing to free.
            break;
        case STMT_DOWHILE:
            c_expression_delete(statement->while_or_do_statement.condition);
            c_statement_delete(statement->while_or_do_statement.body);
            break;
        case STMT_EXP:
            c_expression_delete(statement->expression);
            break;
        case STMT_FOR:
            c_for_init_delete(statement->for_statement.init);
            c_expression_delete(statement->for_statement.condition);
            c_expression_delete(statement->for_statement.post);
            c_statement_delete(statement->for_statement.body);
            break;
        case STMT_GOTO:
            c_expression_delete(statement->goto_statement.label);
            break;
        case STMT_IF:
            c_expression_delete(statement->if_statement.condition);
            c_statement_delete(statement->if_statement.then_statement);
            if (statement->if_statement.else_statement) {
                c_statement_delete(statement->if_statement.else_statement);
            }
            break;
        case STMT_NULL:
            // No-op
            break;
        case STMT_SWITCH:
            c_expression_delete(statement->switch_statement.expression);
            c_statement_delete(statement->switch_statement.body);
            if (statement->switch_statement.case_labels) {
                list_of_int_delete(statement->switch_statement.case_labels);
                free(statement->switch_statement.case_labels);
            }
            break;
        case STMT_WHILE:
            c_expression_delete(statement->while_or_do_statement.condition);
            c_statement_delete(statement->while_or_do_statement.body);
            break;
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            c_expression_delete(statement->expression);
            break;
    }
    if (statement->labels) {
        list_of_CLabel_delete(statement->labels);
        free(statement->labels);
    }
    free(statement);
}
//endregion CStatement

//region struct CVarDecl
struct CVarDecl *c_vardecl_new(const char *identifier, enum STORAGE_CLASS storage_class) {
    struct CVarDecl* result = malloc(sizeof(struct CVarDecl));
    result->var.name = identifier;
    result->var.source_name = identifier;
    result->storage_class = storage_class;
    return result;
}
struct CVarDecl *
c_vardecl_new_init(const char *identifier, struct CExpression *initializer, enum STORAGE_CLASS storage_class) {
    struct CVarDecl* result = c_vardecl_new(identifier, storage_class);
    result->initializer = initializer;
    return result;
}
void c_vardecl_delete(struct CVarDecl* vardecl) {
    if (!vardecl) return;
    if (vardecl->initializer) {
        c_expression_delete(vardecl->initializer);
    }
}
//endregion struct CVarDecl

//region struct CForInit
struct CForInit* c_for_init_new(enum FOR_INIT_KIND kind) {
    struct CForInit* result = malloc(sizeof(struct CForInit));
    result->kind = kind;
    return result;
}
struct CForInit* c_for_init_new_vardecl(struct CDeclaration *declaration)  {
    struct CForInit* result = c_for_init_new(FOR_INIT_DECL);
    result->declaration = declaration;
    return result;
}
struct CForInit* c_for_init_new_expression(struct CExpression* expression)  {
    struct CForInit* result = c_for_init_new(FOR_INIT_EXPR);
    result->expression = expression;
    return result;
}
void c_for_init_delete(struct CForInit* for_init)  {
    if (!for_init) return;
    if (for_init->kind == FOR_INIT_DECL) {
        c_declaration_delete(for_init->declaration);
    } else if (for_init->kind == FOR_INIT_EXPR) {
        c_expression_delete(for_init->expression);
    }
}
//endregion

//region struct CBlockItem
extern struct CBlockItem* c_block_item_new_decl(struct CDeclaration* declaration) {
    struct CBlockItem* result = malloc(sizeof(struct CBlockItem));
    result->kind = AST_BI_DECLARATION;
    result->declaration = declaration;
    return result;
}

struct CBlockItem* c_block_item_new_stmt(struct CStatement* statement) {
    struct CBlockItem* result = malloc(sizeof(struct CBlockItem));
    result->kind = AST_BI_STATEMENT;
    result->statement = statement;
    return result;
}
void c_block_item_delete(struct CBlockItem* blockItem) {
    if (blockItem == NULL) return;
    switch (blockItem->kind) {
        case AST_BI_STATEMENT:
            c_statement_delete(blockItem->statement);
            break;
        case AST_BI_DECLARATION:
            c_declaration_delete(blockItem->declaration);
            break;
    }
    free(blockItem);
}
//endregion CBlockItem

//region struct CFuncDecl
struct CFuncDecl* c_function_new(const char *name, enum STORAGE_CLASS storage_class) {
    struct CFuncDecl* result = malloc(sizeof(struct CFuncDecl));
    result->storage_class = storage_class;
    result->name = name;
    result->body = NULL;
    list_of_CIdentifier_init(&result->params, 7);
    return result;
}
enum AST_RESULT c_function_add_param(struct CFuncDecl* function, const char* param_name) {
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
enum AST_RESULT c_function_add_body(struct CFuncDecl* function, struct CBlock* body) {
    if (function->body) return AST_DUPLICATE;
    function->body = body;
    return AST_OK;
}
void c_function_delete(struct CFuncDecl *function) {
    if (!function) return;
    // Don't free 'name'; owned by global name table.
    if (function->body) {
        c_block_delete(function->body);
    }
    list_of_CIdentifier_delete(&function->params);
    free(function);
}
//endregion CFuncDecl

//region CProgram
struct CProgram* c_program_new(void) {
    struct CProgram* result = malloc(sizeof(struct CProgram));
    list_of_CDeclaration_init(&result->declarations, 57);
    return result;
}
extern enum AST_RESULT c_program_add_decl(struct CProgram *program, struct CDeclaration *declaration) {
    list_of_CDeclaration_append(&program->declarations, declaration);
    return AST_OK;
}

void c_program_delete(struct CProgram *program) {
    if (!program) return;
    list_of_CDeclaration_delete(&program->declarations);
    free(program);
}
//endregion CProgram


#pragma clang diagnostic pop