//
// Created by Bill Evans on 12/5/24.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "symtab.h"
#include "../utils/utils.h"
#include "../utils/startup.h"



struct symtab_item {
    enum SYMTAB_TYPE type;
    const char* source_name;
    const char* mapped_name;
};
unsigned long symtab_item_hash(struct symtab_item item) {
    return hash_str(item.source_name) + item.type;
}
int symtab_item_cmp(struct symtab_item l, struct symtab_item r) {
    if (l.type < r.type) return -1;
    else if (l.type > r.type) return 1;
    return strcmp(l.source_name, r.source_name);
}
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
void symtab_item_free(struct symtab_item item) {
#pragma clang diagnostic pop
    // Don't do anything because the struct doesn't own the strings.
}
struct symtab_item symtab_item_dup(struct symtab_item item) {return item;}
int symtab_item_is_null(struct symtab_item item) {
    return item.source_name == NULL;
}
SET_OF_ITEM_DECL(symtab_item, struct symtab_item)
struct set_of_symtab_item_helpers symtab_item_helpers = {
        .hash = symtab_item_hash,
        .cmp = symtab_item_cmp,
        .dup = symtab_item_dup,
        .free = symtab_item_free,
        .null = {},
        .is_null = symtab_item_is_null
};
SET_OF_ITEM_DEFN(symtab_item, struct symtab_item, symtab_item_helpers)

// A scoped symbol table. 'prev' points to any containing scope.
struct symbol_table {
    struct symbol_table* prev;
    struct set_of_symtab_item symbols;
};
struct symbol_table* symbol_table_new(struct symbol_table* prev) {
    struct symbol_table* table = malloc(sizeof(struct symbol_table));
    table->prev = prev;
    set_of_symtab_item_init(&table->symbols, 5);
    return table;
}
void symbol_table_free(struct symbol_table* table) {
    set_of_symtab_item_free(&table->symbols);
    free(table);
}

// The complete, segmented symbol table, as a linked list. Each prev-> is a containing scope.
struct symbol_table* symbol_table = NULL;
struct symbol_table* function_symbol_table = NULL;

// This holds long-lifetime strings, for the mapped variable names, like "a.0".
struct set_of_str mapped_vars;

void symtab_init() {
    symbol_table = symbol_table_new(NULL);
    set_of_str_init(&mapped_vars, 101);
}

static const char* tag_for(enum SYMTAB_TYPE type) {
    if (type == SYMTAB_VAR)
        return "var";
    else if (type == SYMTAB_LABEL)
        return "label";
    else
        return "??";
}

static struct set_of_symtab_item* symtab_for(enum SYMTAB_TYPE type) {
    if (type == SYMTAB_VAR) {
        assert(symbol_table != NULL);
        return &symbol_table->symbols;
    } else if (type == SYMTAB_LABEL) {
        assert(function_symbol_table != NULL);
        return &function_symbol_table->symbols;
    } else
        assert("Unknown symbol type" && 0);
}

const char* add_symbol(enum SYMTAB_TYPE type, const char* source_name) {
    const char* tag = tag_for(type);
    struct set_of_symtab_item* table = symtab_for(type);
    // The key for find()
    struct symtab_item item = {
            .type = type,
            .source_name = source_name,
    };
    // Result, if found.
    struct symtab_item found = set_of_symtab_item_find(table, item);
    if (!symtab_item_is_null(found)) {
        // Was found; duplicate declaration.
        fprintf(stderr, "Duplicate %s: \"%s\"\n", tag, source_name);
        exit(1);
    }
    // name_buf points to a shared buffer after this call.
    const char* name_buf = uniquify_name("%.100s.%d", source_name);
    // Add to global string pool. Returns the long-lifetime copy.
    name_buf = set_of_str_insert(&mapped_vars, name_buf);
    if (traceResolution) {
        printf("assigning %s for %s %s\n", name_buf, tag, source_name);
    }
    // save the mapping.
    item.mapped_name = name_buf;
    set_of_symtab_item_insert(table, item);
    // return the uniquified name
    return name_buf;
}
const char* resolve_symbol(enum SYMTAB_TYPE type, const char* source_name) {
//    const char* tag = tag_for(type);
    struct symbol_table* table = symbol_table;
    struct set_of_symtab_item* symbols = symtab_for(type);
    // The key for find()
    struct symtab_item item = {
            .type = type,
            .source_name = source_name,
    };
    // Look in local scope, then walk global-wards if not found.
    do {
        // Result, if found.
        struct symtab_item found = set_of_symtab_item_find(symbols, item);
        if (!symtab_item_is_null(found)) {
            // Found it; return mapped name.
            return found.mapped_name;
        }
        // Not found, look in containing scope.
        table = table->prev;
        if (table) {
            symbols = &table->symbols;
        }
    } while (table != NULL);
    return NULL;
}

void push_symtab_context(int is_function_context) {
    printf("push_symtab_context: %s function context\n", is_function_context?"":"not ");
    symbol_table = symbol_table_new(symbol_table);
    if (is_function_context) {
        function_symbol_table = symbol_table;
    }
}

void pop_symtab_context(void) {
    printf("pop_symtab_context\n");
    struct symbol_table* old = symbol_table;
    symbol_table = old->prev;
    symbol_table_free(old);
    if (old == function_symbol_table) {
        function_symbol_table = NULL;
    }
}

int next_uniquifier(void) {
    static int counter = 0;
    return counter++;
}

const char* uniquify_name(const char* fmt, const char* name) {
    static char name_buf[120];
    sprintf(name_buf, fmt, name, next_uniquifier());
    return name_buf;
}


