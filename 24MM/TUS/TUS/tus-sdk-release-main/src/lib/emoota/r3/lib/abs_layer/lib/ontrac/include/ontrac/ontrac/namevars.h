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
 * @file ontrac/name_var_pairs.h
 * @date Mar 31, 2020
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {HELP ME}
 */

#ifndef LIB_ONTRAC_INCLUDE_ONTRAC_ONTRAC_NAMEVARS_H_
#define LIB_ONTRAC_INCLUDE_ONTRAC_ONTRAC_NAMEVARS_H_

#include <ontrac/util/abq_stacks.h>
#include <ontrac/ontrac/abq_class.h>

typedef struct for_namevar_t abq_namevar_t;
struct for_namevar_t {
    cstr_t name;
    cvar_t value;
};

typedef struct for_abq_namevar_stack_t namevars_t;
struct for_abq_namevar_stack_t {
    /** pointers to items stored in the stack */
    abq_namevar_t *array;
    /** number of items currently stored in the stack */
    uint16_t count;
    /** maximum number of items which can be stored in current stack without resizing */
    uint16_t capacity;
};

/** class_t for namevars_t instances */
extern const class_t namevars_class;

/**
 * @brief allocated and initializes a new instance of namevars_t
 *
 * @return a pointer to new instance of namevars_t or NULL on failure
 */
extern namevars_t *namevars_create( void );

/**
 * @brief resolves an instance of namevars_class to the namevars_t*
 *
 * @param pointer to item to be resolved
 * @return resolved namevars_t* or NULL on failure
 */
extern namevars_t* namevars_resolve(cvar_t item);

/**
 * @brief appends the given name/value pair to the end of a namevars_t
 *
 * @param namevars: pointer to the namevars_t to add the item to
 * @param name: the name for the namevar_t to add to the namevars_t
 * @param value: the value of the namevar_t to add to the namevars_t
 * @return 0 on success, else an error code
 */
extern err_t namevars_append(namevars_t *namevars, cstr_t name, cvar_t value);

/**
 * @brief same as namevars_append except is internalizes value as with string_class prior to adding it to the collection
 *
 * @param namevars: pointer to the namevars_t to add the item to
 * @param name: the name for the namevar_t to add to the namevars_t
 * @param value: the value of the namevar_t to add to the namevars_t
 * @return 0 on success, else an error code
 */
extern err_t namevars_addstr(namevars_t *namevars, cstr_t name, cstr_t value);
/**
 * @brief attempts to insert the supplied item at the given index within the namevars_t
 *  ENOSYS will be returned if the stack is marked as sorted
 *
 * @param namevars: the stack to add the item to
 * @param index: the index within the stack to which the item should be placed
 * @param name: the name for the namevar_t to add to the namevars_t
 * @param value: the value of the namevar_t to add to the namevars_t
 * @return  0 on success, else an error code
 */
extern err_t namevars_insert(namevars_t *namevars, uint16_t index, cstr_t name, cvar_t value);
/**
 * @brief removes the first found occurrence of abq_namevar_t with matching name from the collection, transferring value's ref to the provided ref
 *
 * @param namevars: pointer to the namevars_t instance to be modified
 * @param name: name of the property to be removed
 * @param ref: new anchor to be used to reference child, NULL for self-reference
 * @return the value of the removed property if successful, NULL if not found
 */
extern cvar_t namevars_take(namevars_t *namevars, cstr_t name, cvar_t ref);
/**
 * @brief removes all items from the stack
 *
 * @param namevars: reference to a namevars_t to be cleared
 * @return 0 on success, otherwise an error code
 */
extern err_t namevars_clear(namevars_t *namevars);

#endif /* LIB_ONTRAC_INCLUDE_ONTRAC_ONTRAC_NAMEVARS_H_ */
