//
// Created by Bill Evans on 1/23/25.
//

#ifndef BCC_SYMTABLE_H
#define BCC_SYMTABLE_H

#include <stdbool.h>
#include "ast.h"

enum SYMTAB_RESULT {
    SYMTAB_OK,
    SYMTAB_DUPLICATE,
    SYMTAB_NOTFOUND,
};

enum SYMBOL_ATTRS {
    SYMBOL_NONE                 = 0x00,             // No attrs
    SYMBOL_FUNC                 = 0x01,
    SYMBOL_LOCAL_VAR            = 0x02,
    SYMBOL_STATIC_TENTATIVE     = 0x04,
    SYMBOL_STATIC_INITIALIZED   = 0x08,
    SYMBOL_STATIC_NO_INIT       = 0x10,

    SYMBOL_STATIC_MASK          = SYMBOL_STATIC_TENTATIVE|SYMBOL_STATIC_INITIALIZED|SYMBOL_STATIC_NO_INIT,
    SYMBOL_VAR_MASK             = SYMBOL_LOCAL_VAR|SYMBOL_STATIC_MASK,

    SYMBOL_GLOBAL               = 0x20,
    SYMBOL_DEFINED              = 0x40,
};

#define SYMBOL_ATTR_IS_VAR(attr) ((attr & SYMBOL_VAR_MASK) != 0)
#define SYMBOL_IS_GLOBAL(attr) ((attr & SYMBOL_GLOBAL) == SYMBOL_GLOBAL)
#define SYMBOL_IS_DEFINED(attr) ((attr & SYMBOL_DEFINED) == SYMBOL_DEFINED)

#define VAL_IF_PRED(val,pred) ((pred)?(val):0)
#define SYMBOL_GLOBAL_IF(pred) VAL_IF_PRED(SYMBOL_GLOBAL,pred)
#define SYMBOL_DEFINED_IF(pred) VAL_IF_PRED(SYMBOL_DEFINED,pred)

struct Symbol {
    struct CIdentifier identifier;
    enum SYMBOL_ATTRS attrs;
    union {
        int int_val;
        struct {
            int num_params;
            // param types
        };
    };
};

extern struct Symbol symbol_new_static_var(struct CIdentifier id, enum SYMBOL_ATTRS attrs, int int_val);
extern struct Symbol symbol_new_local_var(struct CIdentifier id);
extern struct Symbol symbol_new_func(struct CIdentifier id, int num_params, enum SYMBOL_ATTRS attrs);
extern void symbol_delete(struct Symbol symbol);

extern enum SYMTAB_RESULT add_symbol(struct Symbol symbol);
extern enum SYMTAB_RESULT find_symbol(struct CIdentifier id, struct Symbol* found);
extern enum SYMTAB_RESULT upsert_symbol(struct Symbol symbol);

extern void symtab_init(void);

#endif //BCC_SYMTABLE_H
