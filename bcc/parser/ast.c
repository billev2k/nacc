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

void c_label_free(struct CLabel var) {}
struct list_of_CLabel_helpers c_label_helpers = {
        .free = c_label_free,
        .null = {0},
};
LIST_OF_ITEM_DEFN(CLabel, struct CLabel, c_label_helpers)

struct list_of_CBlockItem_helpers c_blockitem_helpers = {
    .free = c_block_item_free,
    .null = NULL,
};
LIST_OF_ITEM_DEFN(CBlockItem, struct CBlockItem*, c_blockitem_helpers)

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

//region CExpression
static struct CExpression* c_expression_new(enum AST_EXP_TYPE type) {
    struct CExpression* expression = malloc(sizeof(struct CExpression));
    expression->type = type;
    return expression;
}
struct CExpression* c_expression_new_unop(enum AST_UNARY_OP op, struct CExpression* operand) {
    struct CExpression* expression = c_expression_new(AST_EXP_UNOP);
    expression->unary.op = op;
    expression->unary.operand = operand;
    return expression;
}
struct CExpression* c_expression_new_binop(enum AST_BINARY_OP op, struct CExpression* left, struct CExpression* right) {
    struct CExpression* expression = c_expression_new(AST_EXP_BINOP);
    expression->binary.op = op;
    expression->binary.left = left;
    expression->binary.right = right;
    return expression;
}
struct CExpression* c_expression_new_const(enum AST_CONST_TYPE const_type, const char *value) {
    struct CExpression* expression = c_expression_new(AST_EXP_CONST);
    expression->literal.type = const_type;
    expression->literal.value = value;
    return expression;
}
struct CExpression* c_expression_new_var(const char* name) {
    struct CExpression* expression = c_expression_new(AST_EXP_VAR);
    expression->var.name = name;
    expression->var.source_name = name;
    return expression;
}
struct CExpression* c_expression_new_assign(struct CExpression* src, struct CExpression* dst) {
    struct CExpression* expression = c_expression_new(AST_EXP_ASSIGNMENT);
    expression->assign.dst = dst;
    expression->assign.src = src;
    return expression;
}
struct CExpression* c_expression_new_increment(enum AST_INCREMENT_OP op, struct CExpression* operand) {
    struct CExpression* expression = c_expression_new(AST_EXP_INCREMENT);
    expression->increment.op = op;
    expression->increment.operand = operand;
    return expression;
}
struct CExpression* c_expression_new_conditional(struct CExpression* left_exp, struct CExpression* middle_exp, struct CExpression* right_exp) {
    struct CExpression* expression = c_expression_new(AST_EXP_CONDITIONAL);
    expression->conditional.left_exp = left_exp;
    expression->conditional.middle_exp = middle_exp;
    expression->conditional.right_exp = right_exp;
    return expression;
}
struct CExpression* c_expression_clone(const struct CExpression* expression) {
    struct CExpression* clone = c_expression_new(expression->type);
    switch (expression->type) {
        case AST_EXP_BINOP:
            clone->binary.op = expression->binary.op;
            if (expression->binary.left) clone->binary.left = c_expression_clone(expression->binary.left);
            if (expression->binary.right) clone->binary.right = c_expression_clone(expression->binary.right);
            break;
        case AST_EXP_CONST:
            clone->literal.type = expression->literal.type;
            clone->literal.value = expression->literal.value;
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
    }
    return clone;
}
void c_expression_free(struct CExpression *expression) {
    if (!expression) return;
    if (traceAstMem) printf("Free expression @ %p\n", expression);
    switch (expression->type) {
        case AST_EXP_BINOP:
            if (expression->binary.left) c_expression_free(expression->binary.left);
            if (expression->binary.right) c_expression_free(expression->binary.right);
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
static struct CStatement* c_statement_new(enum AST_STMT type) {
    struct CStatement* statement = malloc(sizeof(struct CStatement));
    statement->type = type;
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
void c_statement_add_labels(struct CStatement *statement, struct CLabel* pLabels) {
    if (statement->labels == NULL) {
        statement->labels = malloc(sizeof(struct list_of_CLabel));
        list_of_CLabel_init(statement->labels, 3);
    }
    while (pLabels->label.source_name != NULL) {
        list_of_CLabel_append(statement->labels, *pLabels++);
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

void c_statement_free(struct CStatement *statement) {
    if (!statement) return;
    switch (statement->type) {
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

//region CDeclaration
struct CDeclaration* c_declaration_new(const char* identifier) {
    struct CDeclaration* result = malloc(sizeof(struct CDeclaration));
    result->var.name = identifier;
    result->var.source_name = identifier;
    return result;
}
struct CDeclaration* c_declaration_new_init(const char* identifier, struct CExpression* initializer) {
    struct CDeclaration* result = c_declaration_new(identifier);
    result->initializer = initializer;
    return result;
}
void c_declaration_free(struct CDeclaration* declaration) {
    if (!declaration) return;
    if (declaration->initializer) {
        c_expression_free(declaration->initializer);
    }
}
//endregion CDeclaration

//region CForInit
struct CForInit* c_for_init_new(enum FOR_INIT_TYPE type) {
    struct CForInit* result = malloc(sizeof(struct CForInit));
    result->type = type;
    return result;
}
struct CForInit* c_for_init_new_declaration(struct CDeclaration* declaration)  {
    struct CForInit* result = c_for_init_new(FOR_INIT_DECL);
    result->declaration = declaration;
    return result;
}
struct CForInit* c_for_init_new_expression(struct CExpression* expression)  {
    struct CForInit* result = c_for_init_new(FOR_INIT_EXPR);
    result->expression = expression;
    return result;
}
void c_for_init_free(struct CForInit* for_init)  {
    if (!for_init) return;
    if (for_init->type == FOR_INIT_DECL) {
        c_declaration_free(for_init->declaration);
    } else if (for_init->type == FOR_INIT_EXPR) {
        c_expression_free(for_init->expression);
    }
}
//endregion

//region CBlockItem
struct CBlockItem* c_block_item_new_decl(struct CDeclaration* declaration) {
    struct CBlockItem* result = malloc(sizeof(struct CBlockItem));
    result->type = AST_BI_DECLARATION;
    result->declaration = declaration;
    return result;
}
struct CBlockItem* c_block_item_new_stmt(struct CStatement* statement) {
    struct CBlockItem* result = malloc(sizeof(struct CBlockItem));
    result->type = AST_BI_STATEMENT;
    result->statement = statement;
    return result;
}
void c_block_item_free(struct CBlockItem* blockItem) {
    if (blockItem == NULL) return;
    if (blockItem->type == AST_BI_STATEMENT) {
        c_statement_free(blockItem->statement);
    } else {
        c_declaration_free(blockItem->declaration);
    }
    free(blockItem);
}
void CBlockItem_free(struct CBlockItem* blockItem) {
    c_block_item_free(blockItem);
}
//endregion CBlockItem

//region CFunction
struct CFunction* c_function_new(const char* name, struct CBlock* block) {
    struct CFunction* result = malloc(sizeof(struct CFunction));
    result->name = name;
    result->block = block;
    return result;
}

void c_function_append_block_item(const struct CFunction* function, struct CBlockItem* item) {
    list_of_CBlockItem_append(&function->block->items, item);
}

void c_function_free(struct CFunction *function) {
    if (!function) return;
    // Don't free 'name'; owned by global name table.
    c_block_free(function->block);
    free(function);
}
//endregion CFunction

//region CProgram
void c_program_free(struct CProgram *program) {
    if (!program) return;
    if (program->function) c_function_free(program->function);
    free(program);
}
//endregion CProgram


#pragma clang diagnostic pop