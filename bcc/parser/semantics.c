#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
//
// Created by Bill Evans on 11/19/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ir/ir.h"
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
struct set_of_var_map var_map;
struct set_of_str mapped_vars;

int next_uniquifier(void) {
    static int counter = 0;
    return counter++;
}

const char* make_unique(const char* fmt, const char* context) {
    static char name_buf[120];
    sprintf(name_buf, fmt, context, next_uniquifier());
    return name_buf;
}

static void analyze_function(struct CFunction* function);
static void resolve_statement(struct CStatement* statement);
static void resolve_declaration(struct CDeclaration* decl);

void semantic_analysis(struct CProgram* program) {
    set_of_var_map_init(&var_map, VAR_MAP_INITIAL_SIZE);
    set_of_str_init(&mapped_vars, 101);
    analyze_function(program->function);
}

static void analyze_function(struct CFunction* function) {
    for (int ix=0; ix<function->body.num_items; ix++) {
        struct CBlockItem* bi = function->body.items[ix];
        if (bi->type == AST_BI_STATEMENT) {
            resolve_statement(bi->statement);
        } else {
            resolve_declaration(bi->declaration);
        }
    }

}

static const char* resolve_var(const char* source_name) {
    // The key for find()
    struct var_map_item item = {
            .source_name = source_name,
    };
    // Result, if found.
    struct var_map_item found;
    found = set_of_var_map_find(&var_map, item);
    // mapped_name will be NULL if not found.
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
                printf("Error: %s has not been declared\n", exp->assign.dst->var.source_name);
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
    }
}

static void resolve_declaration(struct CDeclaration* decl) {
    // The key for find()
    struct var_map_item item = {
        .source_name = decl->source_name,
    };
    // Result, if found.
    struct var_map_item found;
    found = set_of_var_map_find(&var_map, item);
    if (!var_map_is_null(found)) {
        // Was found; duplicate declaration.
        fprintf(stderr, "Duplicate declaration for %s\n", decl->source_name);
        exit(1);
    }
    const char* name_buf = make_unique("%.100s.%d", decl->name);
    // Add to global string pool. Returns the long-lifetime copy.
    name_buf = set_of_str_insert(&mapped_vars, name_buf);
    // Update the declaration.
    if (traceResolution) {
        printf("assigning %s for %s\n", name_buf, decl->name);
    }
    decl->name = name_buf;
    // save the mapping.
    item.mapped_name = name_buf;
    set_of_var_map_insert(&var_map, item);
    // Update the initializer, if there is one.
    if (decl->initializer) {
        resolve_exp(decl->initializer);
    }
}

static void resolve_statement(struct CStatement* statement) {
    switch (statement->type) {
        case STMT_RETURN:
            if (statement->expression)
                resolve_exp(statement->expression);
            break;
        case STMT_EXP:
            resolve_exp(statement->expression);
            break;
        case STMT_NULL:
            break;
    }
}

#pragma clang diagnostic pop