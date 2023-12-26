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
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file ontrac/var_list.h
 * @date Mar 22, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief a variable item (cvar_t) linked-list implementation
 * 'vlist' is short for cvar_t-list. A linked-list of cvar_t objects
 *
 * @todo upgrade to a Skip-List
 */


#ifndef SRC_COLLECTIONS_H
#define SRC_COLLECTIONS_H

/** typedef of the vlist_t */
typedef struct for_vlist vlist_t;

#include <ontrac/ontrac/abq_class.h>
#include <ontrac/ontrac/abq_str.h>

/** a vlist foreach macro used to access each link_t in the list */
#define FOREACH_LINK(collection, variable) \
    for(link_t *(variable) = (NULL == (collection)) ? NULL : (collection)->first; \
    NULL != (variable); (variable)=(variable)->next)

/** vlist_t definition */
struct for_vlist {
    /** vlists can limit items in the list by specifying a class_t in the initializer */
    class_ptr class_of_items;
    /** first link in the list */
    link_t * first;
    /** last link in the list */
    link_t * last;
    /** total number of links in the list */
    size_t count;
    /** only settable on lists with a sepecified item_type that has a'compare' function */
    bool_t sorted;
};

/** class_t for vlist_t instances */
extern const class_t vlist_class;
/**
 * @brief allocated and initializes a new instance of vlist_t
 *
 * @param item_class: limites items in the list to instances of the specified class
 * @return a pointer to new instance of vlist_t or NULL on failure
 */
extern vlist_t *vlist_create(class_ptr item_class);
/**
 * @brief resolves an instance of vlist_class to the vlist_t*
 *
 * @param pointer to item to be resolved
 * @return resolved vlist_t* or NULL on failure
 */
extern vlist_t* vlist_resolve(cvar_t item);
/**
 * @brief check if a given list has any items within it
 *
 * @param vlist: pointer to the vlist_t to check
 * @return false if vlist_t is valid and has a count of 0, true otherwise
 */
extern bool_t vlist_is_empty(const vlist_t *vlist);
/**
 *@brief lookup the number of items within a list
 *
 * @param vlist: pointer to the vlist_t to check
 * @return vlist->count if a valid vlist_t, -1 on bad pointer
 */
extern int32_t vlist_size(const vlist_t *vlist);
/**
 * @brief adds the given item to the vlist_t
 * for an unsorted list appends it to the end
 * but a sorted list will insert it a the weighted location
 * determined by class comparison function.
 *
 * @param vlist: pointer to the vlist_t to add the item to
 * @param item: the item to add to the vlist_t
 * @return 0 on success, else an error code
 */
extern err_t vlist_add(vlist_t *vlist, cvar_t item);
/**
 * @brief attempts to insert the supplied item at the given index within the vlist_t
 *  ENOSYS will be returned if the list is marked as sorted
 *
 * @param vlist: the list to add the item to
 * @param index: the index within the list to which the item should be placed
 * @param item: the item to be added to theplatformconfig list
 * @return  0 on success, else an error code
 */
extern err_t vlist_insert(vlist_t *vlist, size_t index, cvar_t item);
/**
 * @brief loads the item at the given index
 *
 * @param vlist: the list containing the item
 * @param index: the index at which to get item from
 * @return the item at index if all goes well, NULL otherwise
 */
extern cvar_t vlist_get(const vlist_t *vlist, size_t index);
/**
 * @brief gets the first item of list
 *
 * @param vlist: the list to get item from
 * @return the first item of the given list, or NULL if none
 */
extern cvar_t vlist_first(const vlist_t *vlist);
/**
 * @brief gets the last item of list
 *
 * @param vlist: the list to get item from
 * @return the last item of the given list, or NULL if none
 */
extern cvar_t vlist_last(const vlist_t *vlist);
/**
 * @brief returns the first item that matches given item using vlist->class_of_items->compare function
 * will set abq_status to ENOSYS and return NULL if sorted if false
 *
 * @param vlist: the list to get match the item within
 * @param item: the item to match within the list
 * @return the instance matching item within the list if found, NULL otherwise
 */
extern cvar_t vlist_match(vlist_t *vlist, cvar_t item);
/**
 * @brief checks if the given item is contained in the list
 *
 * @param vlist: reference to a vlist_t that may contain item
 * @param item: the item that may exist within the list
 * @return true if item was found within the vlist, false otherwise
 */
extern bool_t vlist_contains(vlist_t *vlist, cvar_t item);
/**
 * @brief looks for index of the given item within the provided list
 *
 * @param vlist: pointer to a vlist_t that might contain
 * @param item: the item to be searched for within the given list
 * @param from_index: only looks for items at or after this index
 * @return index of next matching instance of item within the list, or -1 if not found
 */
extern int32_t vlist_index_of(vlist_t *vlist, cvar_t item, size_t from_index);
/**
 * @brief removes the specified item from the vlist_t
 *
 * @param vlist: reference to a vlist_t containing item
 * @param item: the item to be removed from the list
 * @return 0 on success, otherwise an error code
 */
extern err_t vlist_remove(vlist_t *vlist, cvar_t item);
/**
 * @brief removes the first item of list, but first adds ref as an anchor
 *
 * @param vlist: the list to get item from
 * @param ref: new anchor to be used to reference child, NULL for self-reference
 * @return the first item of the given list, or NULL if none
 */
extern cvar_t vlist_pop(vlist_t *vlist, cvar_t ref);
/**
 * @brief removes item at the specified index from the vlist_t, but first adds ref as an anchor
 *
 * @param vlist: reference to a vlist_t containing item
 * @param ref: new anchor to be used to reference child, NULL for self-reference
 * @param index: the inded we wish to remove from the list
 * @return the item that was removed from the list
 */
extern cvar_t vlist_pop_at(vlist_t *vlist, cvar_t ref, size_t index) ;
/**
 * @brief removes all items from the list
 *
 * @param vlist: reference to a vlist_t to be cleared
 * @return 0 on success, otherwise an error code
 */
extern err_t vlist_clear(vlist_t *vlist);
/**
 * @brief sets the list to a sorted/unsorted state, sorts the list if set to true
 *  only applicable for lists with a non-NULL class_of_items->compare method
 *
 * @param vlist: reference to a vlist_t to be sorted / released
 * @param sorted: true if the list should sort itself and maintain the sorted state as new values are added
 * @return 0 on success, otherwise an error code
 */
extern err_t vlist_set_sorted(vlist_t *vlist, bool_t sorted);
/**
 * @brief compared items in a list sequentially until it finds one which differs
 *  items are compared using there respective class.compare functions if available, else a identity compare
 *
 * @param left: first list to be compared
 * @param right: second list to be compared
 * @return the value 0 if left instance has the same value as the right instance
 *          a value less then 0 if left instance has higher value then right instance
 *          a value greater then 0 if left instance has smaller value then right instance
 */
extern int8_t vlist_compare_vlist(const vlist_t *left, const vlist_t *right);
/**
 * @brief utility method to create a vlist_t from a array of 'size' items
 *
 * @param item_class: the class of items within the array, used to initialize the vlist_t
 * @param array: an array of items to be added to the list
 * @param size: number of items in the array that should be added to the vlist_t
 * @return pointer to initialized vlist_t, or NULL on failure
 */
extern vlist_t *vlist_from_array(class_ptr item_class, cvar_t *array, size_t size);

/**
 * @brief return first link_t* in linked-list or NULL if empty
 *
 * @param list: list to get link_t* from
 * @param variable: points to
 * @return pointer to first link_t in list, or NULL if empty
 */
static inline link_t* vlist_begin(vlist_t *list, cvar_t* variable) {
    link_t *retval = NULL;
    VITAL_NOT_NULL(variable);
    if (NULL == list) {
        // Don't iterate over NULL
        *variable = NULL;
    } else {
        uint16_t index = mem_usage_index_of(vlist_class.mem_usage, list);
        if((index < vlist_class.mem_usage->total)
                && (mem_usage_is_free(vlist_class.mem_usage, index))) {
            // Don't iterate over freed lists
            *variable = NULL;
        } else if (NULL != list->first){
            // Unmanaged memory or vlist in use
            retval = list->first;
            *variable = retval->item;
        } else {
            // Empty list
            *variable = NULL;
        }
    }
    return retval;
}

static inline link_t* vlist_next(link_t* prev, cvar_t* variable) {
    link_t *retval = NULL;
    VITAL_NOT_NULL(prev);
    VITAL_NOT_NULL(variable);
    if (NULL != prev->next) {
        retval = prev->next;
        *variable = retval->item;
    } else {
        *variable = NULL;
    }
    return retval;
}

/**
 * a vlist loop that accesses each cvar_t in the list
 * The condition check is a little tricky and likely shouldn't be done this way,
 *   if link_var is not NULL, we wish to set variable to link_var->item and continue looping
 *   else when link_var is NULL, we wish to set variable to NULL and stop looping
 */
#define VLIST_LOOP(vlist, type, variable)  \
    for(link_t *link_var = vlist_begin(vlist, ptr2ptrptr((type*)&(variable))); \
        ((NULL != link_var) && (NULL != variable)); \
        link_var=vlist_next(link_var, ptr2ptrptr((type*)&(variable))))

#endif /* SRC_COLLECTIONS_H */
