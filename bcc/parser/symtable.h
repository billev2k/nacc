//
// Created by Bill Evans on 1/23/25.
//

#ifndef BCC_SYMTABLE_H
#define BCC_SYMTABLE_H
#include "ast.h"

enum SYMTAB_RESULT {
    SYMTAB_OK,
    SYMTAB_DUPLICATE,
    SYMTAB_NOTFOUND,
};

enum SYMTAB_FLAGS {
    SYMTAB_NONE      = 0x00,            // No flags
    SYMTAB_EXTERN    = 0x01,            // Symbol has external linkage

    SYMTAB_VAR       = 0x10,            // A variable
    SYMTAB_FUNC      = 0x20,            // A function
    SYMTAB_FUNC_DEFINED = 0x40,         // A function with a body

    // intrinsic types
//    SYMTAB_CHAR      = 0x20,
//    SYMTAB_UCHAR     = 0x21,
//    SYMTAB_SCHAR     = 0x22,
//    SYMTAB_SHORT     = 0x23,
//    SYMTAB_USHORT    = 0x24,
    SYMTAB_INT       = 0x25,
//    SYMTAB_UINT      = 0x26,
//    SYMTAB_LONG      = 0x27,
//    SYMTAB_ULONG     = 0x28,
//    SYMTAB_LLONG     = 0x29,
//    SYMTAB_ULLONG    = 0x2A,
//
//    SYMTAB_FLOAT     = 0x2B,
//    SYMTAB_DOUBLE    = 0x2C,
//    SYMTAB_LDOUBLE   = 0x2D,
};

struct Symbol {
    struct CIdentifier identifier;
    enum SYMTAB_FLAGS flags;
    union {
        struct {
            int num_params;
        };
    };
};
extern struct Symbol symbol_new_var(struct CIdentifier id, enum SYMTAB_FLAGS flags);
extern struct Symbol symbol_new_func(struct CIdentifier id, int num_params, enum SYMTAB_FLAGS flags);
extern void symbol_delete(struct Symbol symbol);

extern enum SYMTAB_RESULT add_symbol(struct Symbol symbol);
extern enum SYMTAB_RESULT find_symbol(struct CIdentifier id, struct Symbol* found);
extern enum SYMTAB_RESULT upsert_symbol(struct Symbol symbol);

extern void symtab_init(void);

#endif //BCC_SYMTABLE_H
