//
// Created by Bill Evans on 1/23/25.
//

#include "symtable.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "idtable.h"
#include "inc/utils.h"
#include "../utils/startup.h"

LIST_OF_ITEM_DECL(list_of_symbol, struct Symbol)
struct list_of_symbol_helpers list_of_symbol_helpers = {
        .delete = symbol_delete,
        .null = {},
};
LIST_OF_ITEM_DEFN(list_of_symbol,struct Symbol)


struct list_of_symbol symbol_table;

void symtab_init() {
    list_of_symbol_init(&symbol_table, 1023);
    idtable_init();
}

struct Symbol* get_symbol(int ix) {
    if (ix < 0 || ix >= symbol_table.num_items) {
        return NULL;
    }
    return &symbol_table.items[ix];
}
int get_num_symbols() {
    return symbol_table.num_items;
}



struct Symbol symbol_new_static_var(struct CIdentifier id, enum SYMBOL_ATTRS attrs, int int_val) {
    // No symbol flags other than static flags and global flag
    assert((attrs & ~(SYMBOL_STATIC_MASK | SYMBOL_GLOBAL)) == 0);
    struct Symbol symbol = {
            .identifier = id,
            .attrs = attrs,
            .int_val = int_val,
    };
    return symbol;
}
struct Symbol symbol_new_local_var(struct CIdentifier id) {
    struct Symbol symbol = {
            .identifier = id,
            .attrs = SYMBOL_LOCAL_VAR,
    };
    return symbol;
}
struct Symbol symbol_new_func(struct CIdentifier id, int num_params, enum SYMBOL_ATTRS attrs) {
    // Only FUNC, GLOBAL, and/or DEFINED allowed.
    assert((attrs & ~(SYMBOL_FUNC|SYMBOL_GLOBAL|SYMBOL_DEFINED)) == 0);
    struct Symbol symbol = {
            .identifier = id,
            .attrs = attrs | SYMBOL_FUNC,
            .num_params = num_params,
    };
    return symbol;
}
void symbol_delete(struct Symbol symbol) {
    // no-op
}

static struct Symbol* find_internal(const char* name) {
    // TODO: Implement a hash lookup
    for (int ix=0; ix<symbol_table.num_items; ix++) {
        struct Symbol* symbol = &symbol_table.items[ix];
        if (strcmp(symbol->identifier.name, name) == 0) {
            return symbol;
        }
    }
    return NULL;
}

enum SYMTAB_RESULT add_symbol(struct Symbol symbol) {
    if (find_internal(symbol.identifier.name)) {
        return SYMTAB_DUPLICATE;
    }
    list_of_symbol_append(&symbol_table, symbol);
    return SYMTAB_OK;;
}
enum SYMTAB_RESULT find_symbol(struct CIdentifier id, struct Symbol* pResult) {
    struct Symbol* found = find_internal(id.name);
    if (found) *pResult = *found;
    return found ? SYMTAB_OK : SYMTAB_NOTFOUND;
}
enum SYMTAB_RESULT find_symbol_by_name(const char *name, struct Symbol* pResult) {
    struct Symbol* found = find_internal(name);
    if (found) *pResult = *found;
    return found ? SYMTAB_OK : SYMTAB_NOTFOUND;
}

enum SYMTAB_RESULT upsert_symbol(struct Symbol symbol) {
    struct Symbol* found = find_internal(symbol.identifier.name);
    if (found) {
        *found = symbol;
        return SYMTAB_OK;
    }
    return add_symbol(symbol);
}

