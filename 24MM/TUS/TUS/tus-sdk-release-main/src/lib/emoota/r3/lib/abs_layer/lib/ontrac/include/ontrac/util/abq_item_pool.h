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
 * @file util/abq_item_pool.h
 * @date Jun 26, 2018
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief a Memory-Pool set of MACROs for working with an array of a specified item type filled with pre-allocated blocks of memory
 *  the pool keeps track of which blocks have been allocated out and which are free for future consumption.
 *  User must provide a item-type reference so that the macro's can build an array items with memory pre-aligned by the compiler.
 *
 *  - Not thread safe -
 */

#ifndef HELLO_C_ABQ_ITEM_POOL_H
#define HELLO_C_ABQ_ITEM_POOL_H

#include <ontrac/util/abq_stacks.h>
#ifndef ABQ_VITAL
#include <assert.h>
#define ABQ_VITAL assert
#endif /* ABQ_VITAL */

#include <ontrac/ontrac/mem_usage.h>

/** Defines and initializes an item-pool for the given type, amount and name */
#define STATIC_ITEM_POOL(type_, amount_, name_)  DEFINE_STATIC_MEM_USAGE(name_, sizeof(type_), amount_)


#define ITEM_POOL_RESOLVE(head_ptr, type_, item_, var_) do { \
    cvar_t chunk = mem_usage_resolve(head_ptr, var_);       \
    bytes_copy(&(item_), &chunk, sizeof(cvar_t));           \
} while(0)

#define TYPE_ITER(type_) type_ ## _iter
#define DEFINE_TYPE_ITER(type_)                                 \
type_* TYPE_ITER(type_)(mem_usage_t *head_ptr, type_ *prev) {   \
    type_* retval = NULL;                                       \
    byte_t *chunk = mem_usage_iter(head_ptr, ptr2raw(prev));    \
    if (NULL != chunk) {                                        \
        bytes_copy(&retval, &chunk, sizeof(cvar_t));            \
    }                                                           \
    return retval;                                              \
}

/** Allocates the next unused chunk of data within the item-pool for a given head_ptr, type of elements, and pointer for storing the result */
#define ITEM_POOL_ALLOC(head_ptr, type, item_ptr) do {          \
    byte_t *chunk = malloc_from(head_ptr);                      \
    if (NULL != chunk) {                                        \
        bytes_copy(&(item_ptr), &chunk, sizeof(cvar_t));        \
    }                                                           \
} while(0)

/** Frees a previously allocated item back into the pool for a future reallocations */
#define ITEM_POOL_FREE(head_ptr, type, item_ptr) (void) abq_free_into((cvoidc_ptr)item_ptr, head_ptr);

#define ITEM_POOL_LOOP(head_ptr, type, item_ptr)                \
        for ((item_ptr) = TYPE_ITER(type)((head_ptr), NULL);    \
                (NULL != (item_ptr));                           \
                (item_ptr) = TYPE_ITER(type)((head_ptr), (item_ptr)))

#endif //HELLO_C_ABQ_ITEM_POOL_H
