//
// Created by Bill Evans on 8/27/24.
//

#ifndef BCC_UTILS_H
#define BCC_UTILS_H

#include <memory.h>

extern unsigned long hash_str(const char *str);
extern int long_is_zero(long l);

extern void fail(const char* msg);
extern void failf(const char* fmt, ...);

extern int next_uniquifier(void);

#define NAME set_of_str
#define TYPE const char*
#include "set_of_item.h"
#undef NAME
#undef TYPE

#define NAME set_of_int
#define TYPE int
#include "set_of_item.h"
#undef NAME
#undef TYPE

#define NAME list_of_int
#define TYPE int
#include "list_of_item.h"
#undef NAME
#undef TYPE

#endif //BCC_UTILS_H
