//
// Created by Bill Evans on 8/27/24.
//

#ifndef BCC_UTILS_H
#define BCC_UTILS_H

#include <memory.h>
#include "inc/set_of.h"
#include "inc/list_of.h"

extern unsigned long hash_str(const char *str);
extern int long_is_zero(long l);

extern void fail(const char* msg);
extern void failf(const char* fmt, ...);

extern int next_uniquifier(void);

SET_OF_ITEM_DECL(set_of_str, const char*)

SET_OF_ITEM_DECL(set_of_int, int)

LIST_OF_ITEM_DECL(list_of_int,int)

#endif //BCC_UTILS_H
