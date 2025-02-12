//
// Created by Bill Evans on 8/27/24.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "utils.h"

unsigned long hash_str(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;
    } /* hash * 33 + c */

    return hash;
}

int long_is_zero(long l) {
    return l == 0;
}

void fail(const char* msg) {
    fprintf(stderr, "Fail: %s\n", msg);
    exit(1);
}
void failf(const char* fmt, ...) {
    fputs("Fail: ", stderr);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    exit(1);
}

int next_uniquifier(void) {
    static int counter = 0;
    return counter++;
}

long identity(long l) { return l; }

///////////////////////////////////////////////////////////////////////////////
//
// Set implementation for "strings".
//
#define NAME set_of_str
#define TYPE const char *
#include "set_of_item.tmpl"
#undef NAME
#undef TYPE
int set_of_str_is_null(const char* item) { return item == NULL;}
struct set_of_str_helpers set_of_str_helpers = {
        .hash = hash_str,
        .cmp = strcmp,
        .dup =(const char *(*)(const char *)) strdup,
        .delete = (void (*)(const char *)) free,
        .is_null = set_of_str_is_null,
};


///////////////////////////////////////////////////////////////////////////////
//
// Set implementation for int.
//
#define NAME set_of_int
#define TYPE int
#include "set_of_item.tmpl"
#undef NAME
#undef TYPE

int intcmp(int l, int r) { return r - l; }
void no_delete(int x) {;}
struct set_of_int_helpers set_of_int_helpers = {
        .hash = (unsigned long (*)(int)) identity,
        .cmp = intcmp,
        .dup = (int (*)(int)) identity,
        .delete = no_delete,
        .null = 0,
        .is_null = (int (*)(int)) long_is_zero,
};

#define NAME list_of_int
#define TYPE int
struct list_of_int_helpers list_of_int_helpers = {
        .delete = no_delete,
        .null = 0,
};
#include "./list_of_item.tmpl"
