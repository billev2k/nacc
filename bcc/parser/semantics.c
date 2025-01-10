#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 11/19/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ast.h"
#include "semantics.h"
#include "symtab.h"
#include "../utils/startup.h"

static void analyze_function(const struct CFunction* function);
static void resolve_block(const struct CBlock* block);
static void resolve_statement(const struct CStatement* statement);
static void resolve_goto(const struct CStatement* statement);
static void resolve_declaration(struct CDeclaration* decl);

void semantic_analysis(const struct CProgram* program) {
    symtab_init();
    analyze_function(program->function);
}

static void analyze_function(const struct CFunction* function) {
    resolve_block(function->block);
}

static void resolve_block(const struct CBlock* block) {
    push_symtab_context(block->is_function_block);
    for (int ix=0; ix<block->items.num_items; ix++) {
        struct CBlockItem* bi = block->items.items[ix];
        if (bi->type == AST_BI_STATEMENT) {
            resolve_statement(bi->statement);
        } else {
            resolve_declaration(bi->declaration);
        }
    }
    if (block->is_function_block) {
        for (int ix=0; ix<block->items.num_items; ix++) {
            struct CBlockItem* bi = block->items.items[ix];
            if (bi->type == AST_BI_STATEMENT) {
                resolve_goto(bi->statement);
            }
        }
    }
    pop_symtab_context();
}

static const char* resolve_label(const char* source_name) {
    return resolve_symbol(SYMTAB_LABEL, source_name);
}

static const char* resolve_var(const char* source_name) {
    return resolve_symbol(SYMTAB_VAR, source_name);
}

static void resolve_exp(struct CExpression* exp) {
    if (!exp) return;
    const char* mapped_name;
    switch (exp->type) {
        case AST_EXP_CONST:
            break;
        case AST_EXP_UNOP:
            resolve_exp(exp->unary.operand);
            break;
        case AST_EXP_BINOP:
            resolve_exp(exp->binary.left);
            resolve_exp(exp->binary.right);
            break;
        case AST_EXP_VAR:
            mapped_name = resolve_var(exp->var.source_name);
            if (!mapped_name) {
                printf("Error: %s has not been declared\n", exp->var.source_name);
                exit(1);
            }
            if (traceResolution) {
                printf("resolving %s as %s\n", exp->var.name, mapped_name);
            }
            exp->var.name = mapped_name;
            break;
        case AST_EXP_ASSIGNMENT:
            if (exp->assign.dst->type != AST_EXP_VAR) {
                printf("Error in assignment; invalid lvalue\n");
                exit(1);
            }
            mapped_name = resolve_var(exp->assign.dst->var.source_name);
            if (!mapped_name) {
                printf("Error: %s has not been declared\n", exp->assign.dst->var.source_name);
                exit(1);
            }
            if (traceResolution) {
                printf("resolving %s as %s\n", exp->assign.dst->var.name, mapped_name);
            }
            exp->assign.dst->var.name = mapped_name;
            resolve_exp(exp->assign.src);
            break;
        case AST_EXP_INCREMENT:
            if (exp->increment.operand->type != AST_EXP_VAR) {
                printf("Error in increment/decrement; invalid lvalue\n");
                exit(1);
            }
            mapped_name = resolve_var(exp->increment.operand->var.source_name);
            if (!mapped_name) {
                printf("Error: %s has not been declared\n", exp->increment.operand->var.source_name);
                exit(1);
            }
            if (traceResolution) {
                printf("resolving %s as %s\n", exp->increment.operand->var.name, mapped_name);
            }
            exp->increment.operand->var.name = mapped_name;
            break;
        case AST_EXP_CONDITIONAL:
            resolve_exp(exp->conditional.left_exp);
            resolve_exp(exp->conditional.middle_exp);
            resolve_exp(exp->conditional.right_exp);
            break;
    }
}

static void uniquify_thing(struct CVariable* var, enum SYMTAB_TYPE type) {
    const char* uniquified = add_symbol(type, var->source_name);
    var->name = uniquified;
}
static void uniquify_variable(struct CVariable* var) {
    uniquify_thing(var, SYMTAB_VAR);
}
static void uniquify_label(struct CVariable* var) {
    uniquify_thing(var, SYMTAB_LABEL);
}

static void resolve_declaration(struct CDeclaration* decl) {
    if (!decl) return;
    uniquify_variable(&decl->var);
    // Update the initializer, if there is one.
    if (decl->initializer) {
        resolve_exp(decl->initializer);
    }
}

static void resolve_for_init(struct CForInit* for_init) {
    if (!for_init) return;
    if (for_init->type == FOR_INIT_DECL) {
        resolve_declaration(for_init->declaration);
    } else {
        resolve_exp(for_init->expression);
    }
}

static void resolve_statement(const struct CStatement* statement) {
    if (!statement) return;
    if (c_statement_has_labels(statement)) {
        struct CLabel * labels = c_statement_get_labels(statement);
        while (labels && labels->label.source_name) {
            uniquify_label(&labels->label);
            ++labels;
        }
    }
    switch (statement->type) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            resolve_exp(statement->expression);
            break;
        case STMT_EXP:
            resolve_exp(statement->expression);
            break;
        case STMT_NULL:
            break;
        case STMT_IF:
            resolve_exp(statement->if_statement.condition);
            resolve_statement(statement->if_statement.then_statement);
            resolve_statement(statement->if_statement.else_statement);
            break;
        case STMT_GOTO:
            // Not handled here; separate pass, after all labels in function have been found.
            break;
        case STMT_COMPOUND:
            resolve_block(statement->compound);
            break;
        case STMT_BREAK:
        case STMT_CONTINUE:
            break;
        case STMT_FOR:
            resolve_for_init(statement->for_statement.init);
            resolve_exp(statement->for_statement.condition);
            resolve_exp(statement->for_statement.post);
            resolve_statement(statement->for_statement.body);
            break;
        case STMT_SWITCH:
            break;
        case STMT_WHILE:
        case STMT_DOWHILE:
            resolve_exp(statement->while_or_do_statement.condition);
            resolve_statement(statement->while_or_do_statement.body);
            break;
    }
}

static void resolve_goto(const struct CStatement* statement) {
    const char *mapped_name;
    switch (statement->type) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
        case STMT_EXP:
        case STMT_NULL:
            break;
        case STMT_IF:
            resolve_goto(statement->if_statement.then_statement);
            if (statement->if_statement.else_statement) {
                resolve_goto(statement->if_statement.else_statement);
            }
            break;
        case STMT_GOTO:
            mapped_name = resolve_label(statement->goto_statement.label->var.source_name);
            if (!mapped_name) {
                printf("Error: label \"%s\" has not been declared\n", statement->goto_statement.label->var.source_name);
                exit(1);
            }
            statement->goto_statement.label->var.name = mapped_name;
            if (traceResolution) {
                printf("Resolving label \"%s\" as \"%s\"\n", statement->goto_statement.label->var.source_name, mapped_name);
            }
        break;
        case STMT_COMPOUND:
            for (int ix=0; ix<statement->compound->items.num_items; ix++) {
                struct CBlockItem* bi = statement->compound->items.items[ix];
                if (bi->type == AST_BI_STATEMENT) {
                    resolve_goto(bi->statement);
                }
            }
            break;
    }
}

#pragma clang diagnostic pop