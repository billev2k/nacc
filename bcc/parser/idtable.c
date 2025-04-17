/**
 * Created by Bill Evans on 12/5/24.
 *
 * The "id table" is used in the AST handling to track identifiers (functions, variables, labels) that
 * are declared or referenced in the C source. Non-global symbols are assigned a unique variation, decorated
 * with the function name for convenience, and with a unique number, like:
 *      int foo(int x) { ...
 *      'x' => 'foo.x.1'
 * which is a legal identifier in assembly, but not a legal name in C, avoiding the possibility of collisions.
 *
 * Both the original (source) name and the uniquified, decorated name are saved, making it possible to query
 * "What is the uniquified name for this source name, in the current scope?" The semantic analysis uses
 * push_id_context() and pop_id_context() as it enters and leaves lexical scopes. Symbol lookup starts in
 * the current scope, and examines successive enclosing scopes until the symbol is found, or the search
 * is not found in the global scope.
 *
 * Labels are scoped differently in two ways. First, there are no global labels; all labels are within a
 * function definition. Then the entire function is the scope of the label. Therefore, only a single table
 * is needed for label resolution. When semantic analysis calls push_id_context() upon starting the analysis
 * of a function, the new context is remembered, and labels are added to and looked up in that context.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "idtable.h"
#include "symtable.h"
#include "inc/utils.h"
#include "inc/set_of.h"
#include "../utils/startup.h"

/**
 * Entries in the id table.
 * kind:        ID or LABEL
 * has_linkage: function, file-scope variable, or extern variable.
 * source_name: the name of the variable, function, function parameter, or label, as given in the source
 * mapped_name: the uniquified name of a local variable, parameter, or label
 */
struct identifier_item {
    enum IDENTIFIER_KIND kind;
    bool has_linkage;
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
void identifier_item_delete(struct identifier_item item) {
#pragma clang diagnostic pop
    // Don't do anything because the struct doesn't own the strings.
}
struct identifier_item identifier_item_dup(struct identifier_item item) {
    return item;
}
int identifier_item_is_null(struct identifier_item item) {
    return item.source_name == NULL;
}

/*
 * Declare a set of identifier_item, keyed by source_name and kind.
 */
SET_OF_ITEM_DECL(set_of_identifier_item, struct identifier_item)
SET_OF_ITEM_DEFN(set_of_identifier_item, struct identifier_item)
struct set_of_identifier_item_helpers set_of_identifier_item_helpers = {
        .hash = identifier_item_hash,
        .cmp = identifier_item_cmp,
        .dup = identifier_item_dup,
        .delete = identifier_item_delete,
        .null = {},
        .is_null = identifier_item_is_null
};

/*
 * A scoped identifier table. 'prev' points to any containing scope. 'push_id_context' allocates
 * a new one of these, and links it to the previous one.
 */
struct identifier_table {
    struct identifier_table* prev;
    struct set_of_identifier_item ids;
};
// Allocate and initialize an identifier table.
static struct identifier_table* identifier_table_new(struct identifier_table* prev) {
    struct identifier_table* table = malloc(sizeof(struct identifier_table));
    table->prev = prev;
    set_of_identifier_item_init(&table->ids, 5);
    return table;
}
// Clean up and free an identifier table.
void identifier_table_delete(struct identifier_table* table) {
    set_of_identifier_item_delete(&table->ids);
    free(table);
}

// The complete, segmented symbol table, as a linked list. Each prev-> is a containing scope.
struct identifier_table* identifier_table = NULL;
struct identifier_table* function_identifier_table = NULL;

// This holds long-lifetime strings, for the mapped (uniquified) variable names, like "a.0".
struct set_of_str mapped_vars;

/**
 * Initialize the identifier table.
 */
void idtable_init() {
    identifier_table = identifier_table_new(NULL);
    set_of_str_init(&mapped_vars, 101);
}

static const char* tag_for(enum IDENTIFIER_KIND kind) {
    if (kind == IDENTIFIER_ID)
        return "var";
    else if (kind == IDENTIFIER_LABEL)
        return "label";
    else
        assert("Unknown identifier kind" && 0);
}

static struct set_of_identifier_item* id_set_for(enum IDENTIFIER_KIND kind) {
    if (kind == IDENTIFIER_ID) {
        assert(identifier_table != NULL);
        return &identifier_table->ids;
    } else if (kind == IDENTIFIER_LABEL) {
        assert(function_identifier_table != NULL);
        return &function_identifier_table->ids;
    } else
        assert("Unknown identifier kind" && 0);
}

/**
 * Adds an identifier to the identifier table. Only checks for duplicates in the current id scope (function
 * for labels, block for other ids).
 *
 * @param kind ID or LABEL
 * @param source_name name of the identifier in the source
 * @param has_linkage does the identifier have linkage (see identifier_item definition)
 * @return The uniquified name. It is a syntax error if the item already exists without linkage. If the
 *      item exists with linkage, the existing name is returned.
 */
const char *add_identifier(enum IDENTIFIER_KIND kind, const char *source_name, bool has_linkage) {
    const char* tag = tag_for(kind);
    struct set_of_identifier_item* table = id_set_for(kind);
    // The key for find()
    struct identifier_item item = {
            .kind = kind,
            .has_linkage = (bool)has_linkage,
            .source_name = source_name,
    };
    struct identifier_item found = {0};
    int was_found = set_of_identifier_item_find(table, item, &found);
    if (was_found) {
        if (has_linkage && found.has_linkage) {
            // Duplicate declaration of extern symbol is OK.
            return found.mapped_name;
        }
        // Was found; duplicate declaration.
        failf("Duplicate %s: \"%s\"\n", tag, source_name);
    }
    const char* name_buf;
    if (has_linkage) {
        // Don't decorate externs; they're named the same everywhere.
        name_buf = source_name;
    } else {
        // name_buf points to a shared buffer after this call.
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

const char *lookup_identifier(enum IDENTIFIER_KIND kind, const char *source_name, bool *pHas_linkage, bool *pCurrent_scope) {
    struct identifier_table* table = identifier_table;
    struct set_of_identifier_item* id_set = id_set_for(kind);
    // The key for find()
    struct identifier_item item = {
            .kind = kind,
            .source_name = source_name,
    };
    // Look in local scope, then walk global-wards if not found.
    do {
        // Result, if found.
        struct identifier_item found;
        int was_found = set_of_identifier_item_find(id_set, item, &found);
        if (was_found) {
            // Found it; return mapped name.
            if (pHas_linkage) {
                *pHas_linkage = (int)found.has_linkage;
            }
            if (pCurrent_scope) {
                *pCurrent_scope = table == identifier_table;
            }
            return found.mapped_name;
        }
        // Not found, look in containing scope.
        table = table->prev;
        if (table) {
            id_set = &table->ids;
        }
    } while (table != NULL);
    return NULL;

}

/**
 * Called by the semantic analysis when beginning a new scope (ie, a function, a compound statement, a for statement).
 * A new symbol table segment is allocated and linked to the previous segment.
 *
 * If the new context is for a function, the new context is saved so that labels can be added and looked up in the
 * scope of the entire function.
 *
 * @param is_function_context If the new context is for a function, should be true, otherwise false.
 */
void push_id_context(int is_function_context) {
    printf("push_id_context: %s function context\n", is_function_context?"":"not ");
    identifier_table = identifier_table_new(identifier_table);
    if (is_function_context) {
        function_identifier_table = identifier_table;
    }
}

/**
 * Called by the semantic analysis when processing of a scope is complete, and the context is no longer needed.
 * If the context being popped is the function-scope context, semantic analysis is done with the function, and
 * the function-scope context is no longer needed.
 */
void pop_id_context(void) {
    printf("pop_id_context\n");
    struct identifier_table* old = identifier_table;
    identifier_table = old->prev;
    identifier_table_delete(old);
    if (old == function_identifier_table) {
        function_identifier_table = NULL;
    }
}

const char* uniquify_name(const char* fmt, const char* name) {
    static char name_buf[120];
    sprintf(name_buf, fmt, name, next_uniquifier());
    return name_buf;
}


