//
// Created by Bill Evans on 1/23/25.
//

#include "symtable.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "idtable.h"
#include "../utils/utils.h"
#include "../utils/startup.h"

#define NAME list_of_symbol
#define TYPE struct Symbol
#include "../utils/list_of_item.h"
struct list_of_symbol_helpers list_of_symbol_helpers = {
        .delete = symbol_delete,
        .null = {},
};
#include "../utils/list_of_item.tmpl"
#undef NAME
#undef TYPE


struct list_of_symbol symbol_table;

void symtab_init() {
    list_of_symbol_init(&symbol_table, 1023);
    idtable_init();
}


struct Symbol symbol_new_var(struct CIdentifier id, enum SYMTAB_FLAGS flags) {
    struct Symbol symbol = {
            .identifier = id,
            .flags = flags,
    };
    return symbol;
}
struct Symbol symbol_new_func(struct CIdentifier id, int num_params, enum SYMTAB_FLAGS flags) {
    struct Symbol symbol = {
            .identifier = id,
            .flags = flags,
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

enum SYMTAB_RESULT upsert_symbol(struct Symbol symbol) {
    struct Symbol* found = find_internal(symbol.identifier.name);
    if (found) {
        *found = symbol;
        return SYMTAB_OK;
    }
    return add_symbol(symbol);
}

