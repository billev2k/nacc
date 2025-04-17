//
// Created by Bill Evans on 3/4/25.
//

#include "inc/constant.h"

/*
 * Implementation of generic "mk_const_xyz(xyz val)" functions.
 */

#define X(NAME,name,TYPE) struct Constant mk_const_##name(TYPE val) {   \
struct Constant c = { CONST_##NAME };                                       \
    c.kind = CONST_##NAME;                                                  \
    c.name##_value = val;                                                   \
    return c;}
CONSTANT_LIST
#undef X
