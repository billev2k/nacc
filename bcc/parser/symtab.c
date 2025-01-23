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



struct identifier_item {
    enum IDENTIFIER_KIND kind;
    enum SYMTAB_FLAGS flags;
    const char* source_name;
    const char* mapped_name;
};
unsigned long identifier_item_hash(struct identifier_item item) {
    return hash_str(item.source_name) + item.kind;
}
int identifier_item_cmp(struct identifier_item l, struct identifier_item r) {
    if (l.kind < r.kind) return -1;
    else if (l.kind > r.kind) return 1;
    return strcmp(l.source_name, r.source_name);
}
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
void identifier_item_free(struct identifier_item item) {
#pragma clang diagnostic pop
    // Don't do anything because the struct doesn't own the strings.
}
struct identifier_item identifier_item_dup(struct identifier_item item) {return item;}
int identifier_item_is_null(struct identifier_item item) {
    return item.source_name == NULL;
}
#define NAME set_of_identifier_item
#define TYPE struct identifier_item
#include "../utils/set_of_item.h"
#include "../utils/set_of_item.tmpl"
struct set_of_identifier_item_helpers set_of_identifier_item_helpers = {
        .hash = identifier_item_hash,
        .cmp = identifier_item_cmp,
        .dup = identifier_item_dup,
        .free = identifier_item_free,
        .null = {},
        .is_null = identifier_item_is_null
};
#undef NAME
#undef TYPE

// A scoped symbol table. 'prev' points to any containing scope.
struct symbol_table {
    struct symbol_table* prev;
    struct set_of_identifier_item symbols;
};
struct symbol_table* symbol_table_new(struct symbol_table* prev) {
    struct symbol_table* table = malloc(sizeof(struct symbol_table));
    table->prev = prev;
    set_of_identifier_item_init(&table->symbols, 5);
    return table;
}
void symbol_table_free(struct symbol_table* table) {
    set_of_identifier_item_free(&table->symbols);
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

static const char* tag_for(enum IDENTIFIER_KIND kind) {
    if (kind == IDENTIFIER_ID)
        return "var";
    else if (kind == IDENTIFIER_LABEL)
        return "label";
    else
        return "??";
}

static struct set_of_identifier_item* symtab_for(enum IDENTIFIER_KIND kind) {
    if (kind == IDENTIFIER_ID) {
        assert(symbol_table != NULL);
        return &symbol_table->symbols;
    } else if (kind == IDENTIFIER_LABEL) {
        assert(function_symbol_table != NULL);
        return &function_symbol_table->symbols;
    } else
        assert("Unknown symbol kind" && 0);
}

const char *add_identifier(enum IDENTIFIER_KIND kind, const char *source_name, enum SYMTAB_FLAGS flags) {
    const char* tag = tag_for(kind);
    struct set_of_identifier_item* table = symtab_for(kind);
    // The key for find()
    struct identifier_item item = {
            .kind = kind,
            .flags = flags,
            .source_name = source_name,
    };
    struct identifier_item found = {0};
    int was_found = set_of_identifier_item_find(table, item, &found);
    if (was_found) {
        if ((flags & SYMTAB_EXTERN) && (found.flags & SYMTAB_EXTERN)) {
            // Duplicate declaration of extern symbol is OK.
            return found.mapped_name;
        }
        // Was found; duplicate vardecl.
        fprintf(stderr, "Duplicate %s: \"%s\"\n", tag, source_name);
        exit(1);
    }
    // name_buf points to a shared buffer after this call.
    const char* name_buf;
    if (flags & SYMTAB_EXTERN) {
        // Don't decorate externs
        name_buf = source_name;
    } else {
        name_buf = uniquify_name("%.100s.%d", source_name);
        // Add to global string pool. Returns the long-lifetime copy.
        name_buf = set_of_str_insert(&mapped_vars, name_buf);
        if (traceResolution) {
            printf("assigning %s for %s %s\n", name_buf, tag, source_name);
        }
    }
    // save the mapping.
    item.mapped_name = name_buf;
    set_of_identifier_item_insert(table, item);
    // return the uniquified name
    return name_buf;
}
const char *resolve_identifier(enum IDENTIFIER_KIND kind, const char *source_name, enum SYMTAB_FLAGS *pFlags) {
//    const char* tag = tag_for(kind);
    struct symbol_table* table = symbol_table;
    struct set_of_identifier_item* symbols = symtab_for(kind);
    // The key for find()
    struct identifier_item item = {
            .kind = kind,
            .source_name = source_name,
    };
    // Look in local scope, then walk global-wards if not found.
    do {
        // Result, if found.
        struct identifier_item found;
        int was_found = set_of_identifier_item_find(symbols, item, &found);
        if (was_found) {
            // Found it; return mapped name.
            if (pFlags) {
                *pFlags = found.flags;
            }
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

void push_id_context(int is_function_context) {
    printf("push_id_context: %s function context\n", is_function_context?"":"not ");
    symbol_table = symbol_table_new(symbol_table);
    if (is_function_context) {
        function_symbol_table = symbol_table;
    }
}

void pop_id_context(void) {
    printf("pop_id_context\n");
    struct symbol_table* old = symbol_table;
    symbol_table = old->prev;
    symbol_table_free(old);
    if (old == function_symbol_table) {
        function_symbol_table = NULL;
    }
}

const char* uniquify_name(const char* fmt, const char* name) {
    static char name_buf[120];
    sprintf(name_buf, fmt, name, next_uniquifier());
    return name_buf;
}


