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
 *  Copyright (c) 2018 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file sorted_set.h
 * @date Mar 13, 2018
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief a variable item (cvar_t) hash-set implementation
 * A linked-list of cvar_t objects with pre-computed hash codes
 * @warning This implementation of sorted_set caches the hash values of instances within it,
 *   The expected used is ONLY for static / immutable instances of various items.
 *   "Immutable" in the context of OTAmatic has it's own unique definition as follows:
 *     The identifiers used for computing hash values are not to be changed, period ...
 *     , however for state-full properties not used in the hash value algorithm, change is allowed.
 * @ref https://en.wikipedia.org/wiki/String_interning
 *
 * @todo upgrade to a Skip-Set
 *
 */

#ifndef ONTRAC_SORTED_SET_H_
#define ONTRAC_SORTED_SET_H_

#include <ontrac/ontrac/abq_class.h>

/** typedef of the sorted_set_t */
typedef struct for_sorted_set sorted_set_t;
/** sorted_set_t definition */
struct for_sorted_set {
    /** function used to confirm equality of an immutable once hash values have been matched */
    compare_function_t cmp_fctn;
    /** stack of items in the set */
    cvar_t *stack;
    /** Number of items stored in the set */
    uint16_t count;
    /** Current capacity of the stack */
    uint16_t capacity;
};

/**
 * @brief looks for an immutable within the sorted_set and returns the previously inserted immutable if found, else it creates an entry for the immutable within the sorted_set and returns the immutable
 *    With this method the sorted_set can be used for object-pooling, narrowing immutable objects down to the first created instance.
 *    By uses a sorted_set to maintain a pool of objects, a program can eliminate multiple instances of the same object in memory and reduce footprint.
 *
 * @warning: The sorted_set does not reserve internal references to immutables it contains,
 *   this way the reference count can go down to zero, triggering the object's delete function, which might then remove the immutable from the sorted_set via sorted_set_remove
 *
 * @warning: All references to the immutable object are transferred to the interned object which is returned if any
 *
 * @param sorted: pointer to a hashed-set of immutables used to track full set of instances within the program
 * @param immutable: the immutable to be interned
 * @return the interned value of the variable within the list, or NULL on error
 */
extern cvar_t sorted_set_intern(sorted_set_t *sorted, cvar_t immutable);
/**
 * @brief looks for an existing match to immutable within the sorted_set similar to sorted_set_intern, but does NOT create a new entry if none are found
 *
 * @param sorted: pointer to a hashed-set of immutables used to track full set of instances within the program
 * @param immutable: the immutable to match values on
 * @return internalize match of the provided immutable, else NULL
 */
extern cvar_t sorted_set_lookup(const sorted_set_t *sorted, cvar_t immutable);

/**
 * @brief remove an immutable from the given list, Only removes the immutable if it is actually contained within the set and does not remove an equivalent immutable if contained within the set.
 *
 * @param sorted: pointer to a hashed-set of immutables used to track full set of instances within the program
 * @param immutable: the immutable to be removed
 * @return 0 on success, EALREADY if not found, else an error code indicating failure
 */
extern err_t sorted_set_remove(sorted_set_t *sorted, cvar_t immutable);

#endif /* ONTRAC_SORTED_SET_H_ */
