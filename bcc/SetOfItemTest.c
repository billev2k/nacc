//
// Created by Bill Evans on 10/7/24.
//

#include <string.h>
#include <stdlib.h>
#include <printf.h>

#include "inc/utils.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

int unit_tests(__attribute__((unused)) int argc, __attribute__((unused)) char **argv);
int main(int argc, char **argv) {
    return unit_tests(argc, argv);
}

struct set_of_str str_set;

const int num_positions = 25;
const int max_len = 25;
char protected_str[] = "abc";

void string_generator(const int max_len, void (*handler)(const char *string)) {
    static const char *letters = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
    char buf[max_len + 2];

    for (int is = 0; is < num_positions; ++is) {
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
    const char *found;
    int result = set_of_str_find(&str_set, string, &found);
    if (!result) {
        printf("Expected but not found: %s\n", string);
    } else if (strcmp(string, found) != 0) {
        printf("Found but not expected int_value: %s -> %s\n", string, found);
    }
    if (strcmp(string, protected_str) != 0) {
        set_of_str_remove(&str_set, string);
        result = set_of_str_find(&str_set, string, &found);
        if (result) {
            printf("Unexpectedly found after remove: %s\n", string);
        }
    }
}

void set_of_str_tests() {
    set_of_str_init(&str_set, 10);

    string_generator(max_len, str_set_inserter);
    string_generator(max_len, str_set_checker);

    const char *found;
    int result = set_of_str_find(&str_set, "abc", &found);
    if (!result) {
        printf("Expected to find 'abc'' in str set\n");
    }

    printf("Size: %d, items: %d, collisions: %d\n",
           str_set.max_num_items, str_set.num_items, str_set.collisions);
}



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

    set_of_int_insert(set, 0);
    int zero;
    int result = set_of_int_find(set, 0, &zero);
    printf("result: %d, '1' expected.\n", result);
    set_of_int_remove(set, 0);
    int result2 = set_of_int_find(set, 0, &zero);
    printf("result: %d, '0' expected.\n", result2);


    int found;
    result = set_of_int_find(set, 38, &found);
    if (result == 0) {
        printf("Expected to find '38' in int set\n");
    }
}

int unit_tests(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    set_of_str_tests();
    set_of_int_tests();
    return 0;
}

#pragma clang diagnostic pop