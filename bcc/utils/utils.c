//
// Created by Bill Evans on 8/27/24.
//

#include <stdlib.h>
#include <string.h>
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

///////////////////////////////////////////////////////////////////////////////
//
// Set implementation for "strings".
//


struct set_of_str_helpers set_of_str_helpers = {
        hash_str,
        strcmp,
        (const char *(*)(const char *)) strdup,
        (void (*)(const char *)) free
};
#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
SET_OF_ITEM_DEFN(str, const char *, set_of_str_helpers)
#pragma clang diagnostics pop

/**
 * Notes on set_of_T_remove:
 *
 * These definitions will be used in this discussion:                                                   
 * "Natural slot" is the slot in which an item would appear if there were no collisions.                
 * "Top block", the largest contiguous set of filled slots immediately following a                      
 *    given item's natural slot, not including any that may have wrapped around to slot 0.              
 *    Set ix_top to the first index _after_ the set of filled slots. This may be past the               
 *    end of the array.                                                                                 
 * "Bottom block", the largest contiguous set of filled slots immediately preceding a                  
 *    given item's natural slot, not including any that may have wrapped around to slot 0.              
 *    Set ix_bottom to the first element in the bottom block. This may be equal to the                  
 *    index being cleared, the current location of the item being deleted.                              
 * - Either the following or the preceding block may wrap around the array of slots; not both.         
 *    If the preceding block wraps around to the top of the array, that is an "underflow               
 *    block". If the following block wraps around to the bottom of the array, that is an                
 *    "overflow block".                                                                                 
 * - Because we grow the array when the load factor reaches ~0.75, empty slots are guaranteed.          
 *    There may sometimes be an underflow block, or sometimes may be an overflow block, but             
 *    it is not possible that there will be both.                                                       
 *                                                                                                      
 * The general process is as follows:                                                                   
 * 1) Find the item in the array. If not found, done. Otherwise, set ix_free to the location            
 *    of the item in the array.                                                                         
 * 2) Set ix_top, ix_bottom, ix_overflow, and ix_underflow appropriately.                               
 * 3) Delete the item, clear slot ix_free.                                                              
 * 4) Iterate the top block: for (int ix=ix_free+1; ix<ix_top; ++ix)                                    
 * 5)   For each ix, get the "natural slot" for the item in that slot. Call that ix_item.               
 * 6)   If (ix_item <= ix_free || ix_item > ix_underflow), move the item at ix_item to ix_free,         
 *      clear the slot at ix_item, and set ix_free = ix_item.                                           
 * 7)   Continue                                                                                        
 * 8) Iterate the overflow block: for (int ix=0; ix<ix_overflow; ++ix)                                  
 * 9)   For each ix, get the "natural slot" for the item in that slot. Call that ix_item.               
 * 10)  If (ix_free < ix_overflow ? (ix_item <= ix_free || ix_item > ix_overflow)                       
 *                                : (ix_item <= ix_free && ix_item >= ix_bottom) ),                     
 *        move the item at ix_item to ix_free, clear the slot at ix_item, and set ix_free = ix_item.    
 * 11)  Continue                                                                                        
 *                                                                                                      
 * Given this population (with much omitted):                                                           
 *     Slot: |  0|  1|  2|  3|  4|  5| ... | 44| 45| 46| 47| 48| 49| ... | 94| 95| 96| 97| 98| 99|      
 *           +---+---+---+---+---+---+-...-+---+---+---+---+---+---+-...-+---+---+---+---+---+---+      
 * Contents: |  0|197|  1|  3|100|   | ... |   | 45|146|145| 46|   | ... |   | 95|195| 97|295|199|      
 *                                                                                                      
 *                                                                                                      
 * Consider deleting item 45, natural slot 45, in slot 45.                                              
 * - ix_top = 49, ix_bottom = 45, ix_underflow = 100, ix_overflow = 0                                   
 * 1) Clear slot 45. set ix_free=45                                                                     
 * 2) Examine slot 46, ix_item = 46, not <= ix_free, don't move.                                        
 * 3) Examine slot 47, ix_item = 45, <= ix_free, move. Set ix_free=47.                                  
 * 4) Examine slot 48, ix_item = 46, <= ix_free, move. Set ix_free=48.                                  
 * 5) Finished.                                                                                         
 *                                                                                                      
 * Now consider deleting item 0, natural slot 0, in slot 0.                                             
 * - ix_top = 5, ix_bottom = 0, ix_underflow = 95, ix_overflow = 0                                      
 * 1) Clear slot 0. Set ix_free=0.                                                                      
 * 2) Examine slot 1, ix_item = 99, not <= ix_free, but in underflow block, move.                       
 *     Set ix_free=1.                                                                                   
 * 3) Examine slot 2, ix_item = 1, <= ix_free, move. Set ix_free=2.                                     
 * 4) Examine slot 3, ix_item = 3, not <= ix_free, don't move.                                          
 * 5) Examine slot 4, ix_item = 0, <= ix_free, move. Set ix_free=4.                                     
 * 6) Finished.                                                                                         
 *                                                                                                      
 * Finally, consider deleting item 195 (natural slot 95, in slot 96).                                   
 * - ix_top = 100, ix_bottom = 95, ix_underflow = 100, ix_overflow = 5                                  
 * 1) Clear slot 96. Set ix_free=96.                                                                    
 * 2) Examine slot 97, ix_item = 97, not <= ix_free, don't move.                                        
 * 3) Examine slot 98, ix_item = 95, <= ix_free, move. Set ix_free=98.                                  
 * 4) Examine slot 99, ix_item = 99, not <= ix_free, don't move.                                        
 * -- process overflow block --                                                                         
 * 5) Examine slot 0, ix_item = 0, FALSE, don't move.                                                   
 * 6) Examine slot 1, ix_item = 97, TRUE, move, set ix_free=1.                                          
 * 7) Examine slot 2, ix_item = 1, TRUE, move, set ix_free=2;                                           
 * 8) Examine slot 3, ix_item = 3, FALSE, don't move.                                                   
 * 9) Examine slot 4, ix_item = 0, TRUE, move, set ix_free=4.                                           
 *                                                                                                      
 */                                                                                                                                                                                                                                                                                                                                                             
   


