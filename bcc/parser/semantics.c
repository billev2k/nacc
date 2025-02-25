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
#include "idtable.h"
#include "symtable.h"
#include "../utils/startup.h"

struct LoopLabelContext {
    struct CStatement *enclosing_switch;
    int enclosing_break_id;
    int enclosing_continue_id;
};


static void analyze_function(struct CFuncDecl *function);
static void resolve_file_scope_vardecl(struct CDeclaration *decl);
static void resolve_expression(struct CExpression *exp);
static void resolve_forinit(struct CForInit *for_init);

static void resolve_block(const struct CBlock *block);
static void resolve_statement(const struct CStatement *statement);
static void resolve_goto(const struct CStatement *statement);
static void resolve_vardecl(struct CVarDecl *vardecl);
static void resolve_funcdecl(struct CFuncDecl *function);
static void label_block_loops(const struct CBlock *block, struct LoopLabelContext context);
static void label_statement_loops(struct CStatement *statement, struct LoopLabelContext context);

static void typecheck_program(const struct CProgram *program);
static void typecheck_function(struct CFuncDecl *function);
static void typecheck_file_scope_vardecl(struct CVarDecl *vardecl);
static void typecheck_block(struct CBlock* block);
static void typecheck_statement(const struct CStatement *statement);
static void typecheck_block_scope_vardecl(struct CVarDecl *vardecl);
static void typecheck_expression(struct CExpression *exp);

void semantic_analysis(const struct CProgram *program) {
    symtab_init();
    for (int ix=0; ix<program->declarations.num_items; ix++) {
        struct CDeclaration *decl = program->declarations.items[ix];
        switch (decl->decl_kind) {
            case FUNC_DECL:
                analyze_function(decl->func);
                break;
            case VAR_DECL:
                resolve_file_scope_vardecl(decl);
                break;
        }
    }
    typecheck_program(program);
}

static void resolve_file_scope_vardecl(struct CDeclaration *decl) {
    if (!decl) return;
    struct CVarDecl *vardecl = decl->var;
    vardecl->var.name =  add_identifier(IDENTIFIER_ID, vardecl->var.source_name, 1 /*has_linkage*/);
    // File scope variables can only have constant initializers, so we don't need to resolve any initializer.
    //if (vardecl->initializer) {
    //    resolve_expression(vardecl->initializer);
    //}
}

static void analyze_function(struct CFuncDecl *function) {
    if (!function) return;
    resolve_funcdecl(function);
    if (function->body) {
        label_block_loops(function->body, (struct LoopLabelContext) {0});
    }
}

static void resolve_block(const struct CBlock *block) {
    for (int ix = 0; ix < block->items.num_items; ix++) {
        struct CBlockItem *bi = block->items.items[ix];
        switch( bi->kind ) {
            case AST_BI_STATEMENT:
                resolve_statement(bi->statement);
                break;
            case AST_BI_DECLARATION:
                switch (bi->declaration->decl_kind) {
                    case FUNC_DECL:
                        if (bi->declaration->func->body) {
                            failf("Nested function definitions are not supported: %s\n", bi->declaration->func->name);
                        }
                        resolve_funcdecl(bi->declaration->func);
                        if (bi->declaration->func->storage_class == SC_STATIC) {
                            failf("Block-scope static functions are not supported: %s\n", bi->declaration->func->name);
                        }
                        break;
                    case VAR_DECL:
                        resolve_vardecl(bi->declaration->var);
                        break;
                }
                break;
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
}

static const char *resolve_label(const char *source_name) {
    return resolve_identifier(IDENTIFIER_LABEL, source_name, NULL);
}

static const char *resolve_var(const char *source_name) {
    return resolve_identifier(IDENTIFIER_ID, source_name, NULL);
}

static void resolve_expression(struct CExpression *exp) {
    if (!exp) return;
    const char *mapped_name;
    switch (exp->kind) {
        case AST_EXP_CONST:
            break;
        case AST_EXP_UNOP:
            resolve_expression(exp->unary.operand);
            break;
        case AST_EXP_BINOP:
            resolve_expression(exp->binop.left);
            resolve_expression(exp->binop.right);
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
            resolve_expression(exp->assign.src);
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
            resolve_expression(exp->conditional.left_exp);
            resolve_expression(exp->conditional.middle_exp);
            resolve_expression(exp->conditional.right_exp);
            break;
        case AST_EXP_FUNCTION_CALL:
            mapped_name = resolve_var(exp->function_call.func.source_name);
            if (!mapped_name) {
                printf("Error: function %s has not been declared\n", exp->function_call.func.name);
                exit(1);
            }
            exp->function_call.func.name = mapped_name;
            for (int ix = 0; ix < exp->function_call.args.num_items; ix++) {
                resolve_expression(exp->function_call.args.items[ix]);
            }
            break;
    }
}

static void resolve_funcdecl(struct CFuncDecl *function) {
    // Doesn't decorate, just makes sure it doesn't collide with a non-extern.
    add_identifier(IDENTIFIER_ID, function->name, 1 /*has_linkage*/);

    if (function->body) {
        // Any parameters are in the same scope as function block declarations.
        push_id_context(1);
        for (int ix = 0; ix < function->params.num_items; ix++) {
            struct CIdentifier *var = &function->params.items[ix];
            var->name =  add_identifier(IDENTIFIER_ID, var->source_name, 0 /*has_linkage*/);
        }
        resolve_block(function->body);
        pop_id_context();
    }
}

static void resolve_vardecl(struct CVarDecl *vardecl) {
    if (!vardecl) return;
    bool has_linkage = false;
    bool current_scope = false;
    if (lookup_identifier(IDENTIFIER_ID, vardecl->var.source_name, &has_linkage, &current_scope) != NULL) {
        if (current_scope && !(has_linkage && vardecl->storage_class==SC_EXTERN)) {
            failf("Error: variable \"%s\" has conflicting local declarations.", vardecl->var.source_name);
        }
    }
    vardecl->var.name =  add_identifier(IDENTIFIER_ID, vardecl->var.source_name, vardecl->storage_class==SC_EXTERN);
    // Update the initializer, if there is one.
    if (vardecl->initializer) {
        resolve_expression(vardecl->initializer);
    }
}

static void resolve_forinit(struct CForInit *for_init) {
    if (!for_init) return;
    if (for_init->kind == FOR_INIT_DECL) {
        assert(for_init->declaration->decl_kind == VAR_DECL);
        resolve_vardecl(for_init->declaration->var);
        if (for_init->declaration->var->storage_class != SC_NONE) {
            failf("Error: unexpected storage class for for-init declaration: %s", for_init->declaration->var->var.name);
        }
    } else {
        resolve_expression(for_init->expression);
    }
}

static void resolve_statement(const struct CStatement *statement) {
    if (!statement) return;
    if (c_statement_has_labels(statement)) {
        int num_labels = c_statement_num_labels(statement);
        struct CLabel *labels = c_statement_get_labels(statement);
        for (int i=0; i<num_labels; ++i) {
            if (labels[i].kind == LABEL_DECL) {
                struct CIdentifier *var = &labels[i].label;
                var->name =  add_identifier(IDENTIFIER_LABEL, var->source_name, 0 /*has_linkage*/);
            }
        }
    }
    switch (statement->kind) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            resolve_expression(statement->expression);
            break;
        case STMT_EXP:
            resolve_expression(statement->expression);
            break;
        case STMT_NULL:
            break;
        case STMT_IF:
            resolve_expression(statement->if_statement.condition);
            resolve_statement(statement->if_statement.then_statement);
            resolve_statement(statement->if_statement.else_statement);
            break;
        case STMT_GOTO:
            // Not handled here; separate pass, after all labels in function have been found.
            break;
        case STMT_COMPOUND:
            push_id_context(0);
            resolve_block(statement->compound);
            pop_id_context();
            break;
        case STMT_BREAK:
        case STMT_CONTINUE:
            break;
        case STMT_FOR:
            push_id_context(0);
            resolve_forinit(statement->for_statement.init);
            resolve_expression(statement->for_statement.condition);
            resolve_expression(statement->for_statement.post);
            resolve_statement(statement->for_statement.body);
            pop_id_context();
            break;
        case STMT_SWITCH:
            resolve_expression(statement->switch_statement.expression);
            resolve_statement(statement->switch_statement.body);
            break;
        case STMT_WHILE:
        case STMT_DOWHILE:
            resolve_expression(statement->while_or_do_statement.condition);
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

void typecheck_program(const struct CProgram *program) {
    for (int ix=0; ix<program->declarations.num_items; ix++) {
        struct CDeclaration *decl = program->declarations.items[ix];
        switch (decl->decl_kind) {
            case FUNC_DECL:
                typecheck_function(decl->func);
                break;
            case VAR_DECL:
                typecheck_file_scope_vardecl(decl->var);
                break;
        }

    }
}
void typecheck_block(struct CBlock* block) {
    for (int ix = 0; ix < block->items.num_items; ix++) {
        struct CBlockItem *bi = block->items.items[ix];
        switch (bi->kind) {
            case AST_BI_STATEMENT:
                typecheck_statement(bi->statement);
                break;
            case AST_BI_DECLARATION:
                switch (bi->declaration->decl_kind) {
                    case FUNC_DECL:
                        typecheck_function(bi->declaration->func);
                        break;
                    case VAR_DECL:
                        typecheck_block_scope_vardecl(bi->declaration->var);
                        break;
                }
                break;
        }
    }
}
void typecheck_expression(struct CExpression *exp) {
    if (!exp) return;
    struct Symbol symbol;
    switch (exp->kind) {
        case AST_EXP_FUNCTION_CALL:
            if (find_symbol(exp->var, &symbol) == SYMTAB_OK) {
                if (!(symbol.attrs & SYMBOL_FUNC)) {
                    failf("Non function used as function: %s", exp->var.name);
                }
                if (symbol.num_params != exp->function_call.args.num_items) {
                    failf("Function %s called with %d arguments, but %d expected",
                           exp->var.name, exp->function_call.args.num_items, symbol.num_params);
                }
                for (int ix = 0; ix < exp->function_call.args.num_items; ix++) {
                    typecheck_expression(exp->function_call.args.items[ix]);
                }
            } else {
                failf("(Internal) Function not found in symbol table: %s", exp->var.name);
            }
            break;
        case AST_EXP_VAR:
            if (find_symbol(exp->var, &symbol) == SYMTAB_OK) {
                if (!SYMBOL_ATTR_IS_VAR(symbol.attrs)) {
                    failf("Non variable used as variable: %s", exp->var.name);
                }
            } else {
                failf("(Internal) Variable not found in symbol table: %s", exp->var.name);
            }
            break;
        case AST_EXP_ASSIGNMENT:
            typecheck_expression(exp->assign.dst);
            typecheck_expression(exp->assign.src);
            break;
        case AST_EXP_BINOP:
            typecheck_expression(exp->binop.left);
            typecheck_expression(exp->binop.right);
            break;
        case AST_EXP_CONDITIONAL:
            typecheck_expression(exp->conditional.left_exp);
            typecheck_expression(exp->conditional.middle_exp);
            typecheck_expression(exp->conditional.right_exp);
            break;
        case AST_EXP_CONST:
            // We only have integer constants at this time, so nothing to check.
            break;
        case AST_EXP_INCREMENT:
            typecheck_expression(exp->increment.operand);
            break;
        case AST_EXP_UNOP:
            typecheck_expression(exp->unary.operand);
            break;
    }
}
void typecheck_forinit(struct CForInit *for_init) {
    if (!for_init) return;
    switch (for_init->kind) {
        case FOR_INIT_DECL:
            typecheck_block_scope_vardecl(for_init->declaration->var);
            break;
        case FOR_INIT_EXPR:
            typecheck_expression(for_init->expression);
            break;
    }
}
void typecheck_function(struct CFuncDecl *function) {
    if (!function) return;
    int num_params = function->params.num_items;
    int has_body = function->body != NULL;
    int func_defined = 0;
    int global = function->storage_class != SC_STATIC;

    struct CIdentifier function_id = {function->name, function->name};
    struct Symbol symbol;
    if (find_symbol(function_id, &symbol) == SYMTAB_OK) {
        if (SYMBOL_ATTR_IS_VAR(symbol.attrs)) {
            failf("Incompatible function/var declarations: %s", function->name);
        }
        func_defined = SYMBOL_IS_DEFINED(symbol.attrs);
        if (func_defined && has_body) {
            failf("Function %s already defined", function->name);
        }
        if (num_params != symbol.num_params) {
            failf("Incompatible function declarations for %s; %d vs %d params", function->name, num_params, symbol.num_params);
        }
        if (SYMBOL_IS_GLOBAL(symbol.attrs) && function->storage_class == SC_STATIC) {
            failf("Static function declaration follows non-static: %s", function->name);
        }
        global = SYMBOL_IS_GLOBAL(symbol.attrs);
    }
    enum SYMBOL_ATTRS func_attrs = SYMBOL_GLOBAL_IF(global) |
                                   SYMBOL_DEFINED_IF(func_defined || has_body);
    symbol = symbol_new_func(function_id, function->params.num_items, func_attrs);
    upsert_symbol(symbol);
    if (has_body) {
        // Add decorated param names to symbol table. They've been uniquified, so no collisions.
        for (int ix = 0; ix < function->params.num_items; ix++) {
            symbol = symbol_new_local_var(function->params.items[ix]);
            add_symbol(symbol);
        }
        typecheck_block(function->body);
    }
}
void typecheck_file_scope_vardecl(struct CVarDecl *vardecl) {
    if (!vardecl) return;
    // File scope variables have static lifetime. Any initial_value must be a constant expression.
    enum SYMBOL_ATTRS initializer_attrs = SYMBOL_NONE;
    int has_initializer = vardecl->initializer != NULL;
    int initial_value = 0;
    if (has_initializer) {
        if (c_expression_is_const(vardecl->initializer)) {
            initial_value = vardecl->initializer->literal.int_val;
            initializer_attrs = SYMBOL_STATIC_INITIALIZED;
        } else {
            failf("Initializer for file scope variable %s is not a constant expression", vardecl->var.name);
        }
    } else if (vardecl->storage_class == SC_EXTERN) {
        initializer_attrs = SYMBOL_STATIC_NO_INIT;
    } else {
        initializer_attrs = SYMBOL_STATIC_TENTATIVE;
    }
    int global = vardecl->storage_class != SC_STATIC;

    struct Symbol symbol;
    if (find_symbol(vardecl->var, &symbol) == SYMTAB_OK) {
        if (!SYMBOL_ATTR_IS_VAR(symbol.attrs)) {
            failf("Function redeclared as a variable: %s", vardecl->var.name);
        }

        if (vardecl->storage_class == SC_EXTERN) {
            // If now declared extern, use prior linkage.
            global = SYMBOL_IS_GLOBAL(symbol.attrs);
        } else if (SYMBOL_IS_GLOBAL(symbol.attrs) != global) {
            // was-global == is-global ?
            failf("Incompatible variable linkage for %s", vardecl->var.name);
        }

        if (symbol.attrs & SYMBOL_STATIC_INITIALIZED) {
            if (initializer_attrs == SYMBOL_STATIC_INITIALIZED) {
                failf("Conflicting file scope variable definitions for %s", vardecl->var.name);
            } else {
                initializer_attrs = SYMBOL_STATIC_INITIALIZED;
                initial_value = symbol.int_val;
            }
        } else if (initializer_attrs != SYMBOL_STATIC_INITIALIZED && symbol.attrs & SYMBOL_STATIC_TENTATIVE) {
            initializer_attrs = SYMBOL_STATIC_TENTATIVE;
        }
    }
    enum SYMBOL_ATTRS attrs = SYMBOL_GLOBAL_IF(global) | initializer_attrs;
    symbol = symbol_new_static_var(vardecl->var, attrs, initial_value);
    upsert_symbol(symbol);
}

void typecheck_statement(const struct CStatement *statement) {
    if (!statement) return;
    switch (statement->kind) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            typecheck_expression(statement->expression);
            break;
        case STMT_EXP:
            typecheck_expression(statement->expression);
            break;
        case STMT_COMPOUND:
            typecheck_block(statement->compound);
            break;
        case STMT_WHILE:
        case STMT_DOWHILE:
            typecheck_expression(statement->while_or_do_statement.condition);
            typecheck_statement(statement->while_or_do_statement.body);
            break;
        case STMT_FOR:
            typecheck_forinit(statement->for_statement.init);
            typecheck_expression(statement->for_statement.condition);
            typecheck_expression(statement->for_statement.post);
            typecheck_statement(statement->for_statement.body);
            break;
        case STMT_IF:
            typecheck_expression(statement->if_statement.condition);
            typecheck_statement(statement->if_statement.then_statement);
            typecheck_statement(statement->if_statement.else_statement);
            break;
        case STMT_SWITCH:
            typecheck_expression(statement->switch_statement.expression);
            typecheck_statement(statement->switch_statement.body);
            break;
        case STMT_BREAK:
        case STMT_CONTINUE:
        case STMT_GOTO:
        case STMT_NULL:
            break;
    }
}
void typecheck_block_scope_vardecl(struct CVarDecl *vardecl) {
    if (!vardecl) return;
    struct Symbol symbol;
    if (vardecl->storage_class == SC_EXTERN) {
        if (vardecl->initializer) {
            failf("Initializer on local extern variable deckaration: %s", vardecl->var.name);
        }
        if (find_symbol(vardecl->var, &symbol) == SYMTAB_OK) {
            if (!SYMBOL_ATTR_IS_VAR(symbol.attrs)) {
                failf("Function redeclared as a variable: %s", vardecl->var.name);
            }
        } else {
            symbol = symbol_new_static_var(vardecl->var, SYMBOL_STATIC_NO_INIT|SYMBOL_GLOBAL, 0);
            add_symbol(symbol);
        }

    } else if (vardecl->storage_class == SC_STATIC) {
        int initial_value = 0;
        if (c_expression_is_const(vardecl->initializer)) {
            initial_value = vardecl->initializer->literal.int_val;
        } else if (vardecl->initializer) {
            failf("Initializer for static variable %s is not a constant expression", vardecl->var.name);
        }
        symbol = symbol_new_static_var(vardecl->var, SYMBOL_STATIC_INITIALIZED, initial_value);
        add_symbol(symbol);

    } else {
        symbol = symbol_new_local_var(vardecl->var);
        add_symbol(symbol);
        if (vardecl->initializer) {
            typecheck_expression(vardecl->initializer);
        }
    }
}


#pragma clang diagnostic pop
