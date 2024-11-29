//
// Created by Bill Evans on 8/27/24.
//

#ifndef BCC_UTILS_H
#define BCC_UTILS_H

extern unsigned long hash_str(const char *str);
extern int long_is_zero(long l);

//region LIST_OF_ITEM declaration and definition
#define LIST_OF_ITEM_DECL(NAME, TYPE)                                                                       \
struct list_of_##NAME##_helpers {                                                                           \
    void (*free)(TYPE item);                                                                                \
};                                                                                                          \
struct list_of_##NAME {                                                                                     \
    struct list_of_##NAME##_helpers helpers;                                                                \
    TYPE* items;                                                                                            \
    int num_items;                                                                                          \
    int max_num_items;                                                                                      \
};                                                                                                          \
extern void list_of_##NAME##_append(struct list_of_##NAME* list, TYPE new_item);                            \
extern void list_of_##NAME##_insert(struct list_of_##NAME* list, TYPE new_item, int atIx);                  \
extern void list_of_##NAME##_free(struct list_of_##NAME* list);                                             \

#define LIST_OF_ITEM_DEFN(NAME, TYPE, HELPERS)                                                              \
void list_of_##NAME##_init(struct list_of_##NAME *list, int init_size) {                                    \
    list->helpers = HELPERS;                                                                                \
    list->num_items = 0;                                                                                    \
    list->max_num_items = init_size;                                                                        \
    list->items = (TYPE *)malloc(list->max_num_items * sizeof(TYPE));                                       \
    memset(list->items, 0, list->max_num_items * sizeof(TYPE));                                             \
}                                                                                                           \
void list_of_##NAME##_grow(struct list_of_##NAME* list) {                                                   \
    list->max_num_items *= 2;                                                                               \
    TYPE* new_list = malloc(list->max_num_items * sizeof(TYPE));                                            \
    for (int i=0; i<list->num_items; ++i) {                                                                 \
        new_list[i] = list->items[i];                                                                       \
    };                                                                                                      \
    free(list->items);                                                                                      \
    list->items = new_list;                                                                                 \
}                                                                                                           \
void list_of_##NAME##_append(struct list_of_##NAME* list, TYPE new_item) {                                  \
    if (list->num_items == list->max_num_items) {                                                           \
        list_of_##NAME##_grow(list);                                                                        \
    }                                                                                                       \
    list->items[(list->num_items)++] = new_item;                                                            \
}                                                                                                           \
void list_of_##NAME##_insert(struct list_of_##NAME* list, TYPE new_item, int atIx) {                        \
    if (list->num_items == list->max_num_items) {                                                           \
        list_of_##NAME##_grow(list);                                                                        \
    }                                                                                                       \
    for (int i=list->num_items++; i>atIx; --i) {                                                            \
        list->items[i] = list->items[i-1];                                                                  \
    }                                                                                                       \
    list->items[atIx] = new_item;                                                                           \
}                                                                                                           \
void list_of_##NAME##_free(struct list_of_##NAME* list) {                                                   \
    for (int i=0; i<list->num_items; ++i) {                                                                 \
        list->helpers.free(list->items[i]);                                                                 \
    }                                                                                                       \
    free(list->items);                                                                                      \
}
//endregion

//region SET_OF_ITEM declaration and definition
#define SET_OF_ITEM_DECL(NAME, TYPE)                                                                        \
struct set_of_##NAME##_helpers {                                                                            \
    unsigned long (*hash)(TYPE item);                                                                       \
    int (*cmp)(TYPE left, TYPE right);                                                                      \
    TYPE (*dup)(TYPE);                                                                                      \
    void (*free)(TYPE);                                                                                     \
    int (*is_null)(TYPE item);                                                                              \
    TYPE null;                                                                                              \
};                                                                                                          \
struct set_of_##NAME {                                                                                      \
    struct set_of_##NAME##_helpers v_helpers;                                                               \
    TYPE *items;                                                                                            \
    unsigned int num_items;                                                                                 \
    unsigned int max_num_items;                                                                             \
    unsigned int collisions;                                                                                \
};                                                                                                          \
extern TYPE set_of_##NAME##_no_dup(TYPE value);                                                             \
extern struct set_of_##NAME##_helpers set_of_##NAME##_helpers;                                              \
extern void set_of_##NAME##_init(struct set_of_##NAME *set, int init_size);                                 \
extern TYPE set_of_##NAME##_insert(struct set_of_##NAME *set, TYPE newItem);                                \
extern void set_of_##NAME##_remove(struct set_of_##NAME *set, TYPE oldItem);                                \
extern TYPE set_of_##NAME##_find(struct set_of_##NAME *set, TYPE item);                                     \
extern void set_of_##NAME##_free(struct set_of_##NAME *set);                                                

#define SET_OF_ITEM_DEFN(NAME, TYPE, HELPERS)                                                               \
void set_of_##NAME##_init(struct set_of_##NAME *set, int init_size) {                                       \
    set->v_helpers = HELPERS;                                                                               \
    set->collisions = set->num_items = 0;                                                                   \
    set->max_num_items = init_size;                                                                         \
    set->items = (TYPE *)malloc(set->max_num_items * sizeof(TYPE));                                         \
    memset(set->items, 0, set->max_num_items * sizeof(TYPE));                                               \
}                                                                                                           \
static int set_of_##NAME##_mod(struct set_of_##NAME* set, int x) {                                          \
    return (((x)%set->max_num_items + set->max_num_items) % set->max_num_items);                            \
}                                                                                                           \
TYPE set_of_##NAME##_no_dup(TYPE value) {                                                                   \
    return value;                                                                                           \
}                                                                                                           \
void set_of_##NAME##_grow(struct set_of_##NAME *set) {                                                      \
    /* New larger set to which to move the old set's contents.  */                                          \
    struct set_of_##NAME newSet;                                                                            \
    set_of_##NAME##_init(&newSet, set->max_num_items*2);                                                    \
    /* Reuse existing values; don't copy as new ones. */                                                    \
    newSet.v_helpers.dup = set_of_##NAME##_no_dup;                                                          \
    /* For every active slot in the old set, insert it into the new set. */                                 \
    for (int ix=0; ix<set->max_num_items; ++ix) {                                                           \
        TYPE val = set->items[ix];                                                                          \
        if (!set->v_helpers.is_null(val)) set_of_##NAME##_insert(&newSet, val);                                                      \
    }                                                                                                       \
    /* Clean up old set's memory. */                                                                        \
    free(set->items);                                                                                       \
    /* And replace with the new set. */                                                                     \
    set->items = newSet.items;                                                                              \
    set->collisions = newSet.collisions;                                                                    \
    set->max_num_items = newSet.max_num_items;                                                              \
}                                                                                                           \
TYPE set_of_##NAME##_insert(struct set_of_##NAME *set, TYPE newItem) {                                      \
    if (set->collisions > set->max_num_items/3 || set->num_items > set->max_num_items*3/4) {                \
        set_of_##NAME##_grow(set);                                                                          \
    }                                                                                                       \
    unsigned long h = set->v_helpers.hash(newItem);                                                         \
    int ix = (int)(h % set->max_num_items);                                                                 \
    int isCollision = 0;                                                                                    \
    /* Search until we find a match or there's nothing else to inspect.  */                                 \
    while (!set->v_helpers.is_null(set->items[ix]) && set->v_helpers.cmp(newItem, set->items[ix])!=0) {                           \
        if (++ix >= set->max_num_items) ix=0;                                                               \
        isCollision = 1;                                                                                    \
    }                                                                                                       \
    /* ix either indexes an existing match or an empty slot. If not a match, add it. */                     \
    if (set->v_helpers.is_null(set->items[ix])) {                                                                              \
        set->items[ix] = set->v_helpers.dup(newItem);                                                       \
        set->num_items++;                                                                                   \
        set->collisions += isCollision;                                                                     \
    }                                                                                                       \
    return set->items[ix];                                                                                  \
}                                                                                                           \
void set_of_##NAME##_remove(struct set_of_##NAME *set, TYPE oldItem) {                                      \
    /* Find the item to be deleted */                                                                       \
    unsigned long h = set->v_helpers.hash(oldItem);                                                         \
    int ix_free = (int) (h % set->max_num_items);                                                           \
    int cmp = 1;                                                                                            \
    while (set->v_helpers.is_null(set->items[ix_free]) && (cmp = set->v_helpers.cmp(oldItem, set->items[ix_free]))) {               \
        if (++ix_free >= set->max_num_items)                                                                \
            ix_free = 0;                                                                                    \
    }                                                                                                       \
    /* Was the oldItem in the set? */                                                                       \
    if (cmp != 0) return;  /* No, done. */                                                                  \
                                                                                                            \
    /* Yes. Free the oldItem and fix up any affected collisions. */                                         \
    /* First, find ix_underflow, ix_bottom, ix_top, ix_overflow */                                          \
    int ix_underflow = ix_free;                                                                             \
    while (!set->v_helpers.is_null(set->items[set_of_##NAME##_mod(set, ix_underflow - 1)])) --ix_underflow;                          \
    int ix_bottom = ix_underflow >= 0 ? ix_underflow : 0;                                                   \
    ix_underflow = ix_underflow < 0 ? set_of_##NAME##_mod(set, ix_underflow) : set->max_num_items;          \
                                                                                                            \
    int ix_overflow = ix_free;                                                                              \
    while (!set->v_helpers.is_null(set->items[set_of_##NAME##_mod(set, ix_overflow)])) ++ix_overflow;                                \
    int ix_top = ix_overflow <= set->max_num_items ? ix_overflow : set->max_num_items;                      \
    ix_overflow = ix_overflow <= set->max_num_items ? 0 : set_of_##NAME##_mod(set, ix_overflow);            \
                                                                                                            \
    /* Free the oldItem */                                                                                  \
    set->v_helpers.free(set->items[ix_free]);                                                               \
    set->items[ix_free] = set->v_helpers.null;                                                                                \
                                                                                                            \
    /* Handle the "top block" */                                                                            \
    for (int ix = ix_free + 1; ix < ix_top; ++ix) {                                                         \
        int ix_item = (int) (set->v_helpers.hash(set->items[ix]) % set->max_num_items);                     \
        int do_move = ix_item <= ix_free || ix_item > ix_underflow;                                         \
        if (do_move) {                                                                                      \
            set->items[ix_free] = set->items[ix];                                                           \
            set->items[ix] = set->v_helpers.null;                                                                             \
            ix_free = ix;                                                                                   \
        }                                                                                                   \
    }                                                                                                       \
    /* Handle any "overflow block" */                                                                       \
    for (int ix = 0; ix < ix_overflow; ++ix) {                                                              \
        int ix_item = (int) (set->v_helpers.hash(set->items[ix]) % set->max_num_items);                     \
        int do_move = ix_free < ix_overflow ? (ix_item <= ix_free || ix_item > ix_overflow)                 \
                                            : (ix_item <= ix_free && ix_item >= ix_bottom);                 \
        if (do_move) {                                                                                      \
            set->items[ix_free] = set->items[ix];                                                           \
            set->items[ix] = set->v_helpers.null;                                                                             \
            ix_free = ix;                                                                                   \
        }                                                                                                   \
    }                                                                                                       \
}                                                                                                           \
TYPE set_of_##NAME##_find(struct set_of_##NAME *set, TYPE item) {                                           \
    unsigned long h = set->v_helpers.hash(item);                                                            \
    int ix = (int)(h % set->max_num_items);                                                                 \
    int cmp=1; /* init to 'not equal'  */                                                                   \
    /* Search until we find a match or there's nothing else to inspect.  */                                 \
    while (!set->v_helpers.is_null(set->items[ix]) && (cmp=set->v_helpers.cmp(item, set->items[ix]))) {                              \
        if (++ix >= set->max_num_items) ix=0;                                                               \
    }                                                                                                       \
    /* return item if we found a match. */                                                                  \
    return cmp==0 ? set->items[ix] : set->v_helpers.null;                                                                     \
}                                                                                                           \
void set_of_##NAME##_free(struct set_of_##NAME *set) {                                                      \
    for (int i=0; i<set->max_num_items; ++i) {                                                              \
        if (!set->v_helpers.is_null(set->items[i])) {                                                                                \
            set->v_helpers.free(set->items[i]);                                                             \
        }                                                                                                   \
    }                                                                                                       \
    free(set->items);                                                                                       \
}
//endregion

// Declare a set_of_str. Implementation in utils.c.
SET_OF_ITEM_DECL(str, const char *)

#endif //BCC_UTILS_H
