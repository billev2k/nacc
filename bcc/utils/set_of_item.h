//
// Created by Bill Evans on 1/14/25.
//

// This file can be included multiple times within a single compiland.
//#ifndef BCC_SET_OF_DECL_H
//#define BCC_SET_OF_DECL_H

//#define NAME set_of_str
//#define TYPE const char*

#ifndef NAME
#error "NAME must be defined, eg 'set_of_str'"
#endif
#ifndef TYPE
#error "TYPE must be defined, eg 'char*'"
#endif

#define W_SUFFIX_X(n,suff) n##_##suff
#define W_SUFFIX(n,suff) W_SUFFIX_X(n,suff)

#define STRUCT NAME
#define HELPERS W_SUFFIX(NAME,helpers)
#define MEMBER(m) W_SUFFIX(NAME,m)

struct HELPERS {
    unsigned long (*hash)(TYPE item);
    int (*cmp)(TYPE left, TYPE right);
    TYPE (*dup)(TYPE);
    void (*delete)(TYPE);
    int (*is_null)(TYPE item);
    TYPE null;
};
struct STRUCT {
    struct HELPERS v_helpers;
    TYPE *items;
    unsigned int num_items;
    unsigned int max_num_items;
    unsigned int collisions;
    TYPE null_item;
    int is_null_item_set;
};
extern TYPE MEMBER(no_dup)(TYPE value);
extern struct MEMBER(helpers) HELPERS;
extern void MEMBER(init)(struct STRUCT *set, int init_size);
extern TYPE MEMBER(insert)(struct STRUCT *set, TYPE newItem);
extern void MEMBER(remove)(struct STRUCT *set, TYPE oldItem);
extern int MEMBER(find)(struct STRUCT *set, TYPE item, TYPE* found_item);
extern void MEMBER(delete)(struct STRUCT *set);

//#endif //BCC_SET_OF_DECL_H
