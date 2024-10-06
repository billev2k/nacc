//
// Created by Bill Evans on 10/7/24.
//

#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include "utils.h"

struct set_of_str str_set;

void string_generator(const int max_len, void (*handler)(const char *string)) {
    static const char *letters = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
    char buf[max_len + 2];

    for (int is = 0; is < 25; ++is) {
        for (int il = 1; il <= max_len; ++il) {
            for (int ic = 0; ic < il; ++ic) {
                buf[ic] = letters[is + ic];
            }
            buf[il] = '\0';
            handler(buf);
        }
    }
}

void str_set_inserter(const char *string) {
    set_of_str_insert(&str_set, string);
}

void str_set_checker(const char *string) {
    const char *result = set_of_str_find(&str_set, string);
    if (result == NULL) {
        printf("Expected but not found: %s\n", string);
    } else if (strcmp(string, result) != 0) {
        printf("Found but not expected value: %s -> %s\n", string, result);
    }
    set_of_str_remove(&str_set, string);
    result = set_of_str_find(&str_set, string);
    if (result != NULL) {
        printf("Unexpectedly found after remove: %s\n", string);
    }
}

void set_of_str_tests() {
    set_of_str_init(&str_set, 10);

    const int max_len = 25;
    string_generator(max_len, str_set_inserter);
    string_generator(max_len, str_set_checker);

    const char *result = set_of_str_find(&str_set, "abc");
    if (result == 0) {
        printf("Expected to find 'abc'' in str set\n");
    }

    printf("Size: %d, items: %d, collisions: %d\n",
           str_set.max_num_items, str_set.num_items, str_set.collisions);
}

SET_OF_ITEM_DECL(int, int)

unsigned long set_of_int_hash(int i) { return i; }

int set_of_int_cmp(int l, int r) { return r - l; }

void set_of_int_no_free(__attribute__((unused)) int x) {}

struct set_of_int_helpers set_of_int_helpers = {
        set_of_int_hash,
        set_of_int_cmp,
        set_of_int_no_dup,
        set_of_int_no_free
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
SET_OF_ITEM_IMPL(int, int, set_of_int_helpers)
#pragma clang diagnostic pop

struct set_of_int int_set;

void set_of_int_tests() {
    set_of_int_init(&int_set, 10);

    struct set_of_int *set = &int_set;
//    for (int i=-15; i<15; i++) printf("x: %d, x%%: %d, x%%+: %d, SET_OF_ITEM_MOD(x): %d\n", i, i%set->max_num_items, (i%set->max_num_items+set->max_num_items), SET_OF_ITEM_MOD(i));

    set_of_int_insert(set, 18);
    set_of_int_insert(set, 9);
    set_of_int_insert(set, 28);
    set_of_int_insert(set, 38);
    set_of_int_insert(set, 19);

    set_of_int_remove(set, 18);


    int result = set_of_int_find(set, 38);
    if (result == 0) {
        printf("Expected to find '38' in int set\n");
    }
}

int unit_tests(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    set_of_str_tests();
    set_of_int_tests();
    return 0;
}
