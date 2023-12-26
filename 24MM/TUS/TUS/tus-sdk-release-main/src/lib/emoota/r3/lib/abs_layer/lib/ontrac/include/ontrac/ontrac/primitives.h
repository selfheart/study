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
 * @file ontrac/primitives.h
 * @date Mar 30, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief classes used for serializing to and from JSON primitives (NULL, bool, & Numbers)
 *
 */

#ifndef SPIL_NIO_PRIMITIVES_H
#define SPIL_NIO_PRIMITIVES_H

#include <ontrac/ontrac/abq_class.h>
#include <ontrac/ontrac/abq_str.h>
/** class_of(NULL) should return this class_t */
extern const class_t null_class;
/** a class_t for classified bool_ptr */
extern const class_t bool_class;
/** a class_t for classified number_ptr */
extern const class_t number_class;
extern const class_t list_of_number_class;

/** the interned value of a true bool_t */
extern const bool_ptr true_ptr;
/** the interned value of a false bool_t */
extern const bool_ptr false_ptr;
/**
 * @brief intern a boolean value to either true_ptr or false_ptr but as a generic cvar_t
 *
 * @param value: a boolean value to intern
 * @return either true_ptr or false_ptr based on the conditional value passed in
 */
extern cvar_t bool_var(bool_t value);
/**
 * @brief intern a boolean value to either true_ptr or false_ptr
 *
 * @param value: a boolean value to intern
 * @return either true_ptr or false_ptr based on the conditional value passed in
 */
extern bool_ptr bool_intern(bool_t value);
/**
 * @brief coerce an item to a bool_ptr if able
 *
 * @param item: a classified instance to be resolved to a bool, either true_ptr, fals_ptr, "true", "false" or a number_t (0.0 will resolve to false_ptr)
 * @return true_ptr for values which equate to true, false_ptr for values that equate to false, or NULL for unknown values
 */
extern bool_t* bool_resolve(cvar_t item);

static inline bool_t bool_value(cvar_t item, bool_t if_null){
    bool_t* ptr = bool_resolve(item);
    return (NULL == ptr) ? if_null : ptr[0];
}
/**
 * @brief creates a classified self referencing number_ptr
 *
 * @param value: the number used to initialize data of newly allocated number_t
 * @return a pointer to a newly initialized instance of number_t
 */
extern number_ptr number_create(const cnumber_t value);
/**
 * @breif resolves a classified number back to raw value, returns abq_nan on failute
 *
 * @param number: classified number_ptr to be resolved for a number
 * @return value of the number pointed to by number pointer on success
 * @return abq_nan on failure to resolve number
 */
extern number_t number_resolve(cvar_t number);
/**
 * @brief coerce an item to a number_ptr instance with a given anchor (ref)
 *
 * @param source: Source item which needs to be coerced into a number_ptr instance, unclassified items will be assumed to be a pointer to a number_t
 * @param ref: Some reference identifier to the object (often a pointer to parent object)
 * @return An instance of number_ptr representing the source data, or NULL on failure to coerce
 */
extern number_ptr number_coerce(cvar_t source, cvar_t ref);
/**
 * @brief compares two number pointers to determine equivalence / sort order
 *
 * @param left the first instance to compare
 * @param right the second instance to compare
 * @return the value 0 if left instance has the same value as the right instance
 *          a value less then 0 if left instance has higher value then right instance
 *          a value greater then 0 if left instance has smaller value then right instance
 */
extern int8_t number_compare(cvar_t left, cvar_t right);
/**
 * @brief compares two number_t instances to determine equivalence / sort order
 *
 * @param left the first instance to compare
 * @param right the second instance to compare
 * @return the value 0 if left instance has the same value as the right instance
 *          a value less then 0 if left instance has higher value then right instance
 *          a value greater then 0 if left instance has smaller value then right instance
 *
 * @todo account for NaN, Inf and the like
 */
extern int8_t number_compare_number(const number_t left, const number_t right);

/** This is a read-only list only exposed solely for the benefits of class.c */
extern LLIST_HEAD_DEF(obj_t, primative_obj_list);
#endif //SPIL_NIO_PRIMITIVES_H
