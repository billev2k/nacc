//
// Created by Bill Evans on 3/4/25.
//

#ifndef BCC_CONSTANT_H
#define BCC_CONSTANT_H

#define CONSTANT_LIST \
X(INT, int, int)                    \
X(LONG, long, long)                  \
X(STRING, string, const char *)
// X(ULONG, unsigned long)...

enum CONSTANT_KIND {
#define X(NAME, name, TYPE) CONST_##NAME,
    CONSTANT_LIST
#undef X
};

struct Constant {
    enum CONSTANT_KIND kind;
    union {
#define X(NAME,name,TYPE) TYPE name##_value;
        CONSTANT_LIST
#undef X
    };
};

#define X(NAME,name,TYPE) extern struct Constant mk_const_##name(TYPE val);
CONSTANT_LIST
#undef X

#endif //BCC_CONSTANT_H
