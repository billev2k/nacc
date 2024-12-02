#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 11/19/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "semantics.h"
#include "../utils/startup.h"

//region struct var_map
#define VAR_MAP_INITIAL_SIZE 101
struct var_map_item {
    const char* source_name;
    const char* mapped_name;
};
unsigned long var_map_hash(struct var_map_item item) {
    return hash_str(item.source_name);
}
int var_map_cmp(struct var_map_item l, struct var_map_item r) {
    return strcmp(l.source_name, r.source_name);
}
void var_map_free(struct var_map_item item) {
    // Don't do anything because the struct doesn't own the strings.
}
struct var_map_item var_map_dup(struct var_map_item item) {return item;}
int var_map_is_null(struct var_map_item item) {
    return item.source_name == NULL;
}
SET_OF_ITEM_DECL(var_map, struct var_map_item)
struct set_of_var_map_helpers var_map_helpers = {
    .hash = var_map_hash,
    .cmp = var_map_cmp,
    .dup = var_map_dup,
    .free = var_map_free,
    .null = {},
    .is_null = var_map_is_null
};
SET_OF_ITEM_DEFN(var_map, struct var_map_item, var_map_helpers)
//endregion
// This holds a map of declared name to uniquified name, like "a"=>"a.0".
// Will require work when we support multiple scopes.
struct set_of_var_map var_map;
// Same, but for labels.
struct set_of_var_map label_map;
// This holds long-lifetime strings, for the mapped variable names, like "a.0".
struct set_of_str mapped_vars;

int next_uniquifier(void) {
    static int counter = 0;
    return counter++;
}

const char* make_unique(const char* fmt, const char* context, char tag) {
    static char name_buf[120];
    sprintf(name_buf, fmt, context, next_uniquifier());
    return name_buf;
}

static void analyze_function(const struct CFunction* function);
static void resolve_statement(const struct CStatement* statement);
static void resolve_goto(const struct CStatement* statement);
static void resolve_declaration(struct CDeclaration* decl);

void semantic_analysis(const struct CProgram* program) {
    set_of_var_map_init(&var_map, VAR_MAP_INITIAL_SIZE);
    set_of_var_map_init(&label_map, VAR_MAP_INITIAL_SIZE);
    set_of_str_init(&mapped_vars, 101);
    analyze_function(program->function);
}

static void analyze_function(const struct CFunction* function) {
    struct CStatement* label_without_statement = NULL;
    for (int ix=0; ix<function->body.num_items; ix++) {
        struct CBlockItem* bi = function->body.items[ix];
        if (bi->type == AST_BI_STATEMENT) {
            resolve_statement(bi->statement);
            if(bi->statement->type == STMT_LABEL)
                label_without_statement = bi->statement;
            else if (bi->statement->type != STMT_AUTO_RETURN)
                label_without_statement = NULL;
        } else {
            if (label_without_statement) {
                printf("Error: label \"%s\" has no subsequent statement\n", label_without_statement->label_statement.label->var.source_name);
                exit(1);
            }
            resolve_declaration(bi->declaration);
        }
    }
    if (label_without_statement) {
        printf("Error: label \"%s\" has no subsequent statement\n", label_without_statement->label_statement.label->var.source_name);
        exit(1);
    }
    // Handle GOTO after the first pass through statements. Lets us find all the labels first.
    for (int ix=0; ix<function->body.num_items; ix++) {
        struct CBlockItem* bi = function->body.items[ix];
        if (bi->type == AST_BI_STATEMENT) {
            resolve_goto(bi->statement);
        }
    }
}

static const char* resolve_label(const char* source_name) {
    // The key for find()
    const struct var_map_item item = {
        .source_name = source_name,
};
    // Result, if found, will be NULL if not found.
    const struct var_map_item found = set_of_var_map_find(&label_map, item);
    return found.mapped_name;
}

static const char* resolve_var(const char* source_name) {
    // The key for find()
    const struct var_map_item item = {
            .source_name = source_name,
    };
    // Result, if found, will be NULL if not found.
    const struct var_map_item found = set_of_var_map_find(&var_map, item);
    return found.mapped_name;
}

static void resolve_exp(struct CExpression* exp) {
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

static void uniquify_thing(struct CVariable* var, struct set_of_var_map* map, char* tag) {
    // The key for find()
    struct var_map_item item = {
        .source_name = var->source_name,
    };
    // Result, if found.
    struct var_map_item found = set_of_var_map_find(map, item);
    if (!var_map_is_null(found)) {
        // Was found; duplicate declaration.
        fprintf(stderr, "Duplicate %s: \"%s\"\n", tag, var->source_name);
        exit(1);
    }
    const char* name_buf = make_unique("%.100s.%d", var->name, *tag);
    // Add to global string pool. Returns the long-lifetime copy.
    name_buf = set_of_str_insert(&mapped_vars, name_buf);
    // Update the declaration.
    if (traceResolution) {
        printf("assigning %s for %s %s\n", name_buf, tag, var->name);
    }
    var->name = name_buf;
    // save the mapping.
    item.mapped_name = name_buf;
    set_of_var_map_insert(map, item);
  
}
static void uniquify_variable(struct CVariable* var) {
    uniquify_thing(var, &var_map, "variable");
}
static void uniquify_label(struct CVariable* var) {
    uniquify_thing(var, &label_map, "label");
}

static void resolve_declaration(struct CDeclaration* decl) {
    uniquify_variable(&decl->var);
    // Update the initializer, if there is one.
    if (decl->initializer) {
        resolve_exp(decl->initializer);
    }
}

static void resolve_statement(const struct CStatement* statement) {
    switch (statement->type) {
        case STMT_RETURN:
        case STMT_AUTO_RETURN:
            if (statement->expression)
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
            if (statement->if_statement.else_statement) {
                resolve_statement(statement->if_statement.else_statement);
            }
            break;
        case STMT_GOTO:
            // Not handled here; separate pass, after all labels in function have been found.
            break;
        case STMT_LABEL:
            uniquify_label(&statement->label_statement.label->var);
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
        case STMT_LABEL:
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
            statement->label_statement.label->var.name = mapped_name;
            if (traceResolution) {
                printf("Resolving label \"%s\" as \"%s\"\n", statement->goto_statement.label->var.name, mapped_name);
            }
        break;
    }
}

#pragma clang diagnostic pop