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

struct LoopLabelContext {
    struct CStatement *enclosing_switch;
    int enclosing_break_id;
    int enclosing_continue_id;
};


static void analyze_function(const struct CFunction *function);

static void resolve_block(const struct CBlock *block);

static void resolve_statement(const struct CStatement *statement);

static void resolve_goto(const struct CStatement *statement);

static void resolve_vardecl(struct CVarDecl *vardecl);

static void label_block_loops(const struct CBlock *block, struct LoopLabelContext context);

static void label_statement_loops(struct CStatement *statement, struct LoopLabelContext context);

void semantic_analysis(const struct CProgram *program) {
    symtab_init();
    struct CFunction** functions = program->functions.items;
    for (int ix=0; ix<program->functions.num_items; ix++) {
        analyze_function(program->functions.items[ix]);
    }
}

static void analyze_function(const struct CFunction *function) {
    if (!function || !function->body) return;
    resolve_block(function->body);
    label_block_loops(function->body, (struct LoopLabelContext){0});
}

static void resolve_block(const struct CBlock *block) {
    push_symtab_context(block->is_function_block);
    for (int ix = 0; ix < block->items.num_items; ix++) {
        struct CBlockItem *bi = block->items.items[ix];
        if (bi->kind == AST_BI_STATEMENT) {
            resolve_statement(bi->statement);
        } else {
            resolve_vardecl(bi->vardecl);
        }
    }
    if (block->is_function_block) {
        for (int ix = 0; ix < block->items.num_items; ix++) {
            struct CBlockItem *bi = block->items.items[ix];
            if (bi->kind == AST_BI_STATEMENT) {
                resolve_goto(bi->statement);
            }
        }
    }
    pop_symtab_context();
}

static const char *resolve_label(const char *source_name) {
    return resolve_symbol(SYMTAB_LABEL, source_name);
}

static const char *resolve_var(const char *source_name) {
    return resolve_symbol(SYMTAB_VAR, source_name);
}

static void resolve_exp(struct CExpression *exp) {
    if (!exp) return;
    const char *mapped_name;
    switch (exp->kind) {
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
            if (exp->assign.dst->kind != AST_EXP_VAR) {
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
            if (exp->increment.operand->kind != AST_EXP_VAR) {
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

static void uniquify_thing(struct CIdentifier *var, enum SYMTAB_KIND kind) {
    const char *uniquified = add_symbol(kind, var->source_name);
    var->name = uniquified;
}

static void uniquify_variable(struct CIdentifier *var) {
    uniquify_thing(var, SYMTAB_VAR);
}

static void uniquify_label(struct CIdentifier *var) {
    uniquify_thing(var, SYMTAB_LABEL);
}

static void resolve_vardecl(struct CVarDecl *vardecl) {
    if (!vardecl) return;
    uniquify_variable(&vardecl->var);
    // Update the initializer, if there is one.
    if (vardecl->initializer) {
        resolve_exp(vardecl->initializer);
    }
}

static void resolve_for_init(struct CForInit *for_init) {
    if (!for_init) return;
    if (for_init->kind == FOR_INIT_DECL) {
        resolve_vardecl(for_init->vardecl);
    } else {
        resolve_exp(for_init->expression);
    }
}

static void resolve_statement(const struct CStatement *statement) {
    if (!statement) return;
    if (c_statement_has_labels(statement)) {
        int num_labels = c_statement_num_labels(statement);
        struct CLabel *labels = c_statement_get_labels(statement);
        for (int i=0; i<num_labels; ++i) {
            if (labels[i].kind == LABEL_DECL) {
                uniquify_label(&labels[i].label);
            }
        }
    }
    switch (statement->kind) {
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
            push_symtab_context(0);
            resolve_for_init(statement->for_statement.init);
            resolve_exp(statement->for_statement.condition);
            resolve_exp(statement->for_statement.post);
            resolve_statement(statement->for_statement.body);
            pop_symtab_context();
            break;
        case STMT_SWITCH:
            resolve_exp(statement->switch_statement.expression);
            resolve_statement(statement->switch_statement.body);
            break;
        case STMT_WHILE:
        case STMT_DOWHILE:
            resolve_exp(statement->while_or_do_statement.condition);
            resolve_statement(statement->while_or_do_statement.body);
            break;
    }
}

static void resolve_goto(const struct CStatement *statement) {
    const char *mapped_name;
    switch (statement->kind) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
        case STMT_EXP:
        case STMT_BREAK:
        case STMT_CONTINUE:
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
                printf("Resolving label \"%s\" as \"%s\"\n", statement->goto_statement.label->var.source_name,
                       mapped_name);
            }
            break;
        case STMT_COMPOUND:
            for (int ix = 0; ix < statement->compound->items.num_items; ix++) {
                struct CBlockItem *bi = statement->compound->items.items[ix];
                if (bi->kind == AST_BI_STATEMENT) {
                    resolve_goto(bi->statement);
                }
            }
            break;
        case STMT_WHILE:
        case STMT_DOWHILE:
            resolve_goto(statement->while_or_do_statement.body);
            break;
        case STMT_FOR:
            resolve_goto(statement->for_statement.body);
            break;
        case STMT_SWITCH:
            resolve_goto(statement->switch_statement.body);
            break;
    }
}

static void label_block_loops(const struct CBlock *block, struct LoopLabelContext context) {
    for (int ix = 0; ix < block->items.num_items; ix++) {
        struct CBlockItem *bi = block->items.items[ix];
        if (bi->kind == AST_BI_STATEMENT) {
            label_statement_loops(bi->statement, context);
        }
    }
}

static void label_statement_loops(struct CStatement *statement, struct LoopLabelContext context) {
    if (!statement) return;
    int flow_id;
    struct LoopLabelContext new_context = context;
    struct CStatement *body;

    switch (statement->kind) {
        case STMT_BREAK:
            if (!context.enclosing_break_id) {
                printf("Error: break statement outside of loop\n");
                exit(1);
            }
            c_statement_set_flow_id(statement, context.enclosing_break_id);
            break;
        case STMT_CONTINUE:
            if (!context.enclosing_continue_id) {
                printf("Error: continue statement outside of loop\n");
                exit(1);
            }
            c_statement_set_flow_id(statement, context.enclosing_continue_id);
            break;

        case STMT_WHILE:
        case STMT_DOWHILE:
            body = statement->while_or_do_statement.body;
        label_loop_body:
            flow_id = next_uniquifier();
            c_statement_set_flow_id(statement, flow_id);
            new_context.enclosing_break_id = flow_id;
            new_context.enclosing_continue_id = flow_id;
            label_statement_loops(body, new_context);
            break;
        case STMT_FOR:
            body = statement->for_statement.body;
            goto label_loop_body;

        case STMT_SWITCH:
            flow_id = next_uniquifier();
            c_statement_set_flow_id(statement, flow_id);
            new_context.enclosing_break_id = flow_id;
            new_context.enclosing_switch = statement;
            label_statement_loops(statement->switch_statement.body, new_context);
            break;

        case STMT_IF:
            label_statement_loops(statement->if_statement.then_statement, context);
            label_statement_loops(statement->if_statement.else_statement, context);
            break;
        case STMT_COMPOUND:
            label_block_loops(statement->compound, context);
            break;

        case STMT_GOTO:
        case STMT_EXP:
        case STMT_NULL:
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            break;
    }

    if (c_statement_has_labels(statement)) {
        int num_labels = c_statement_num_labels(statement);
        struct CLabel *labels = c_statement_get_labels(statement);
        for (int i=0; i<num_labels; ++i) {
            if (labels[i].kind == LABEL_DEFAULT) {
                if (!context.enclosing_switch) {
                    printf("Error: 'default:' label outside of switch\n");
                    exit(1);
                }
                if (c_statement_set_switch_has_default(context.enclosing_switch) == AST_DUPLICATE) {
                    printf("Error: 'default:' label already used in switch\n");
                    exit(1);
                }
                labels[i].switch_flow_id = context.enclosing_switch->flow_id;
            } else if (labels[i].kind == LABEL_CASE) {
                if (!context.enclosing_switch) {
                    printf("Error: 'case X:' label outside of switch\n");
                    exit(1);
                }
                if (c_statement_register_switch_case(context.enclosing_switch, atoi(labels[i].case_value)) == AST_DUPLICATE) {
                    printf("Error: 'case X:' label '%s' already used in switch\n", labels[i].case_value);
                    exit(1);
                }
                labels[i].switch_flow_id = context.enclosing_switch->flow_id;
            } else if (labels[i].kind == LABEL_DECL) {
                // LABEL_DECL is handled completely in IR generation.
            }
        }
    }
}

#pragma clang diagnostic pop
