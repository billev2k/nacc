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

struct list_of_CBlockItem_helpers cBlockItemHelpers = {
    .free = c_block_item_free
};
LIST_OF_ITEM_DEFN(CBlockItem, struct CBlockItem*, cBlockItemHelpers)

const char * const ASM_CONST_TYPE_NAMES[] = {
#define X(a,b) b 
    AST_CONST_LIST__
#undef X
};

const char * const AST_BINARY_NAMES[] = {
#define X(a,b,c) b
    AST_BINARY_LIST__
#undef X
};
const int AST_BINARY_PRECEDENCE[] = {
#define X(a,b,c) c
    AST_BINARY_LIST__
#undef X
};
const int AST_TO_IR_BINARY[] = {
#define X(a,b,c) IR_BINARY_##a 
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
    struct CExpression* expression = (struct CExpression*)malloc(sizeof(struct CExpression));
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
    expression->unary.op = op;
    expression->unary.operand = operand;
    return expression;
}
struct CExpression* c_expression_clone(struct CExpression* expression) {
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
    }
    free(expression);
}
//endregion CExpression

//region CStatement
static struct CStatement* c_statement_new(enum AST_STMT type) {
    struct CStatement* result = (struct CStatement*)malloc(sizeof(struct CStatement));
    result->type = type;
    return result;
}
struct CStatement* c_statement_new_return(struct CExpression* expression) {
    struct CStatement* result = c_statement_new(STMT_RETURN);
    result->expression = expression;
    return result;
}
struct CStatement* c_statement_new_exp(struct CExpression* expression) {
    struct CStatement* result = c_statement_new(STMT_EXP);
    result->expression = expression;
    return result;
}
struct CStatement* c_statement_new_null(void) {
    struct CStatement* result = c_statement_new(STMT_NULL);
    return result;
}
void c_statement_free(struct CStatement *statement) {
    if (!statement) return;
    if (statement->expression) c_expression_free(statement->expression);
    free(statement);
}
//endregion CStatement

//region CDeclaration
struct CDeclaration* c_declaration_new(const char* identifier) {
    struct CDeclaration* result = (struct CDeclaration*)malloc(sizeof(struct CDeclaration));
    result->name = identifier;
    result->source_name = identifier;
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

//region CBlockItem
struct CBlockItem* c_block_item_new_decl(struct CDeclaration* declaration) {
    struct CBlockItem* result = (struct CBlockItem*)malloc(sizeof(struct CBlockItem));
    result->type = AST_BI_DECLARATION;
    result->declaration = declaration;
    return result;
}
struct CBlockItem* c_block_item_new_stmt(struct CStatement* statement) {
    struct CBlockItem* result = (struct CBlockItem*)malloc(sizeof(struct CBlockItem));
    result->type = AST_BI_STATEMENT;
    result->statement = statement;
    return result;
}
void c_block_item_free(struct CBlockItem* blockItem) {
    if (blockItem->type == AST_BI_STATEMENT) {
        c_statement_free(blockItem->statement);
    } else {
        c_declaration_free(blockItem->declaration);
    }
}
void CBlockItem_free(struct CBlockItem* blockItem) {
    c_block_item_free(blockItem);
}
//endregion CBlockItem

//region CFunction
struct CFunction* c_function_new(const char* name) {
    struct CFunction* result = (struct CFunction*)malloc(sizeof(struct CFunction));
    result->name = name;
    list_of_CBlockItem_init(&result->body, 10);
    return result;
}

void c_function_append_block_item(struct CFunction* function, struct CBlockItem* item) {
    list_of_CBlockItem_append(&function->body, item);
}

void c_function_free(struct CFunction *function) {
    if (!function) return;
    // Don't free 'name'; owned by global name table.
    list_of_CBlockItem_free(&function->body);
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