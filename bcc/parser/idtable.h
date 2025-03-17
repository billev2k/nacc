//
// Created by Bill Evans on 12/5/24.
//

#ifndef BCC_IDTABLE_H
#define BCC_IDTABLE_H

#include "ast.h"
#include "symtable.h"

enum IDENTIFIER_KIND {
    IDENTIFIER_ID,
    IDENTIFIER_LABEL,
};

extern void idtable_init(void);

extern const char *add_identifier(enum IDENTIFIER_KIND kind, const char *source_name, bool has_linkage);
extern const char *lookup_identifier(enum IDENTIFIER_KIND kind, const char *source_name, bool *pHas_linkage, bool *pCurrent_scope);

extern void push_id_context(int is_function_context);
extern void pop_id_context(void);

/**
 * Make a unique name by combining a valid symbol name with a '.' and a unique number.
 *
 * @param fmt The format to use to create the unique symbol. Should contain a "%s",
 *          a "%d", and a '.'. May also contain fixed text, eg.: "%s.tmp.%d".
 * @param context A string to help make the resulting string human readable (and understandable).
 * @return A pointer to the generated string. NOTE: the generated string is in a global buffer,
*           and will ony be valid until the next call to uniquify_name().
*/
extern const char* uniquify_name(const char* fmt, const char* context);

#endif //BCC_IDTABLE_H
