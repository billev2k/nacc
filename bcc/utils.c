//
// Created by Bill Evans on 8/27/24.
//

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