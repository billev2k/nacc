//
// Created by Bill Evans on 1/15/25.
//

// This file can be included multiple times within a single compiland.
//#ifndef BCC_LIST_OF_ITEM_H
//#define BCC_LIST_OF_ITEM_H

#ifndef NAME
#error "NAME must be defined, eg 'list_of_str'"
#endif
#ifndef TYPE
#error "TYPE must be defined, eg 'const char*'"
#endif

#if defined(W_SUFFIX_X)
#undef W_SUFFIX_X
#endif
#if defined(W_SUFFIX)
#undef W_SUFFIX
#endif
#if defined(HELPERS)
#undef HELPERS
#endif
#if defined(MEMBER)
#undef MEMBER
#endif
#if defined(STRUCT)
#undef STRUCT
#endif

#define W_SUFFIX_X(n,suff) n##_##suff
#define W_SUFFIX(n,suff) W_SUFFIX_X(n,suff)

#define STRUCT NAME
#define HELPERS W_SUFFIX(NAME,helpers)
#define MEMBER(m) W_SUFFIX(NAME,m)

struct HELPERS {                                                                           
    void (*free)(TYPE item);                                                                                
    TYPE null;                                                                                              
};                                                                                                          
struct STRUCT {                                                                                     
    struct HELPERS helpers;                                                                
    TYPE* items;                                                                                            
    int num_items;                                                                                          
    int max_num_items;                                                                                      
};                                                                                                          
extern void MEMBER(init)(struct STRUCT *list, int init_size);                              
extern void MEMBER(append)(struct STRUCT* list, TYPE new_item);                            
extern void MEMBER(insert)(struct STRUCT* list, TYPE new_item, int atIx);                  
extern void MEMBER(clear)(struct STRUCT* list);                                            
extern void MEMBER(free)(struct STRUCT* list);                                             
extern int MEMBER(is_empty)(struct STRUCT* list);                                          

//#endif //BCC_LIST_OF_ITEM_H
