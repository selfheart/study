/****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2020 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file ontrac/var_stack.h
 * @date May 6, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {HELP ME}
 */

#ifndef ONTRAC_INCLUDE_ONTRAC_ONTRAC_VAR_STACK_H_
#define ONTRAC_INCLUDE_ONTRAC_ONTRAC_VAR_STACK_H_

#include <ontrac/util/abq_stacks.h>

static inline bool_t abq_stack_iter(cvar_t *array, uint16_t size,
        cvar_t *item, uint16_t *counter) {
    bool_t retval = false;
    if (NULL != item) {
        if((NULL == array) || (NULL == counter)){
            *item = NULL;
        }else if(*counter >= size){
            *item = NULL;
        }else{
            *item = array[*counter];
            *counter += 1U;
            retval = true;
        }
    }
    return retval;
}
/**
 * a vstack loop that accesses each cvar_t in the list
 * The condition check is a little tricky and likely shouldn't be done this way,
 *   if link_var is not NULL, we wish to set variable to link_var->item and continue looping
 *   else when link_var is NULL, we wish to set variable to NULL and stop looping
 */
#define VSTACK_LOOP(vstack, type, variable)  \
        for(uint16_t counter = 0U; (NULL != (vstack)) &&    \
        (abq_stack_iter((vstack)->array, (vstack)->count, \
                ptr2ptrptr((type*)&(variable), &counter))) && (NULL != (variable)); )

typedef struct for_abq_stack vstack_t;
#include <ontrac/ontrac/abq_class.h>
struct for_abq_stack {
    /** vstack_t can limit items in the stack by specifying a class_ptr */
    class_ptr class_of_items;
    /** pointers to items stored in the stack */
    cvar_t *array;
    /** number of items currently stored in the stack */
    uint16_t count;
    /** maximum number of items which can be stored in current stack without resizing */
    uint16_t capacity;
    /** Should the stack allow for dynamic growns of the stack when capacity is exceeded ? */
    bool_t dynamic;
    /** Is this a sorted stack ? */
    bool_t sorted;
};

/** class_t for vstack_t instances */
extern const class_t vstack_class;
/**
 * @brief allocated and initializes a new instance of vstack_t
 *
 * @param item_class: limits items in the stack to instances of the specified class
 * @return a pointer to new instance of vstack_t or NULL on failure
 */
extern vstack_t *vstack_create(class_ptr item_class);
/**
 * @brief resolves an instance of vstack_class to the vstack_t*
 *
 * @param pointer to item to be resolved
 * @return resolved vstack_t* or NULL on failure
 */
extern vstack_t* vstack_resolve(cvar_t item);
/**
 * @brief check if a given stack has any items within it
 *
 * @param vstack: pointer to the vstack_t to check
 * @return false if vstack_t is valid and has a count of 0, true otherwise
 */
extern bool_t vstack_is_empty(const vstack_t *vstack);
/**
 *@brief lookup the number of items within a stack
 *
 * @param vstack: pointer to the vstack_t to check
 * @return vstack->count if a valid vstack_t, -1 on bad pointer
 */
extern int32_t vstack_size(const vstack_t *vstack);
/**
 * @brief adds the given item to the vstack_t
 * for an unsorted stack appends it to the end
 * but a sorted stack will insert it a the weighted location
 * determined by class comparison function.
 *
 * @param vstack: pointer to the vstack_t to add the item to
 * @param item: the item to add to the vstack_t
 * @return 0 on success, else an error code
 */
extern err_t vstack_add(vstack_t *vstack, cvar_t item);
/**
 * @brief attempts to insert the supplied item at the given index within the vstack_t
 *  ENOSYS will be returned if the stack is marked as sorted
 *
 * @param vstack: the stack to add the item to
 * @param index: the index within the stack to which the item should be placed
 * @param item: the item to be added to the stack
 * @return  0 on success, else an error code
 */
extern err_t vstack_insert(vstack_t *vstack, uint16_t index, cvar_t item);
/**
 * @brief loads the item at the given index
 *
 * @param vstack: the stack containing the item
 * @param index: the index at which to get item from
 * @return the item at index if all goes well, NULL otherwise
 */
extern cvar_t vstack_get(const vstack_t *vstack, uint16_t index);
/**
 * @brief gets the first item of stack
 *
 * @param vstack: the stack to get item from
 * @return the first item of the given stack, or NULL if none
 */
extern cvar_t vstack_first(const vstack_t *vstack);
/**
 * @brief gets the last item of stack
 *
 * @param vstack: the stack to get item from
 * @return the last item of the given stack, or NULL if none
 */
extern cvar_t vstack_last(const vstack_t *vstack);
/**
 * @brief returns the first item that matches given item using vstack->class_of_items->compare function
 * will set abq_status to ENOSYS and return NULL if sorted if false
 *
 * @param vstack: the stack to get match the item within
 * @param item: the item to match within the stack
 * @return the instance matching item within the stack if found, NULL otherwise
 */
extern cvar_t vstack_match(const vstack_t *vstack, cvar_t item);
/**
 * @brief checks if the given item is contained in the stack
 *
 * @param vstack: reference to a vstack_t that may contain item
 * @param item: the item that may exist within the stack
 * @return true if item was found within the vstack, false otherwise
 */
extern bool_t vstack_contains(const vstack_t *vstack, cvar_t item);
/**
 * @brief looks for index of the given item within the provided stack
 *
 * @param vstack: pointer to a vstack_t that might contain
 * @param item: the item to be searched for within the given stack
 * @param from_index: only looks for items at or after this index
 * @return index of next matching instance of item within the stack, or -1 if not found
 */
extern int32_t vstack_index_of(const vstack_t *vstack, cvar_t item, uint16_t from_index);
/**
 * @brief removes the specified item from the vstack_t
 *
 * @param vstack: reference to a vstack_t containing item
 * @param item: the item to be removed from the stack
 * @return 0 on success, otherwise an error code
 */
extern err_t vstack_remove(vstack_t *vstack, cvar_t item);
/**
 * @brief removes the last item of stack, but first adds ref as an anchor
 *
 * @param vstack: the stack to get item from
 * @param ref: new anchor to be used to reference child, NULL for self-reference
 * @return the first item of the given stack, or NULL if none
 */
extern cvar_t vstack_pop(vstack_t *vstack, cvar_t ref);
/**
 * @brief removes item at the specified index from the vstack_t, but first adds ref as an anchor
 *
 * @param vstack: reference to a vstack_t containing item
 * @param ref: new anchor to be used to reference child, NULL for self-reference
 * @param index: the index we wish to remove from the stack
 * @return the item that was removed from the stack
 */
extern cvar_t vstack_pop_at(vstack_t *vstack, cvar_t ref, uint16_t index) ;
/**
 * @brief removes all items from the stack
 *
 * @param vstack: reference to a vstack_t to be cleared
 * @return 0 on success, otherwise an error code
 */
extern err_t vstack_clear(vstack_t *vstack);
/**
 * @brief sets the stack to a sorted/unsorted state, sorts the stack if set to true
 *  only applicable for stacks with a non-NULL class_of_items->compare method
 *
 * @param vstack: reference to a vstack_t to be sorted / released
 * @param sorted: true if the stack should sort itself and maintain the sorted state as new values are added
 * @return 0 on success, otherwise an error code
 */
extern err_t vstack_set_sorted(vstack_t *vstack, bool_t sorted);
/**
 * @brief compared items in a stack sequentially until it finds one which differs
 *  items are compared using there respective class.compare functions if available, else a identity compare
 *
 * @param left: first stack to be compared
 * @param right: second stack to be compared
 * @return the value 0 if left instance has the same value as the right instance
 *          a value less then 0 if left instance has higher value then right instance
 *          a value greater then 0 if left instance has smaller value then right instance
 */
extern int8_t vstack_compare_vstack(const vstack_t *left, const vstack_t *right);
/**
 * @brief utility method to create a vstack_t from a array of 'size' items
 *
 * @param item_class: the class of items within the array, used to initialize the vstack_t
 * @param array: an array of items to be added to the stack
 * @param size: number of items in the array that should be added to the vstack_t
 * @return pointer to initialized vstack_t, or NULL on failure
 */
extern vstack_t *vstack_from_array(class_ptr item_class, cvar_t *array, uint16_t size);
/**
 * @brief reallocates stack by an increment amount & free old_stack on success
 *
 * @param old_stack: Previous stack to copy onto new stack, NULL for empty stack
 * @param capacity: pointer to stack's capacity, indicates old_stack's capacity when passed in, and updated on success
 * @param increment: number of additional entries to add to the stack
 * @return new stack on success or old_stack on failure.
 */
extern cvar_t* stack_realloc(cvar_t* old_stack,
        uint16_t *capacity, uint16_t increment);

#endif /* ONTRAC_INCLUDE_ONTRAC_ONTRAC_VAR_STACK_H_ */
