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
 * @file sorted_set.c
 * @date Mar 13, 2018
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/ontrac/sorted_set.h>
#include <ontrac/ontrac/var_stack.h>

cvar_t sorted_set_intern(sorted_set_t *sorted, cvar_t immutable) {
    cvar_t retval = immutable; // Default to input item
    abq_context_lock();
    if (NULL == sorted->stack) {
        // Create the stack as to fit new reference
        sorted->stack = stack_realloc(sorted->stack,
                &sorted->capacity, 4U);
        if (NULL != sorted->stack) {
            ABQ_INSERT_AT(sorted->stack, cvar_t,
                    sorted->count, sorted->capacity, immutable, (uint16_t)0U);
            retval = sorted->stack[0];
        }
    } else {
        uint16_t index = (uint16_t) UINT16_MAX;
        ABQ_SEEK_BINARY(sorted->stack, cvar_t,
                sorted->count, index, sorted->cmp_fctn, immutable);
        if (index >= sorted->count) {
            if (sorted->count >= sorted->capacity) {
                // Grow the stack as needed to fit new references
                sorted->stack = stack_realloc(sorted->stack,
                        &sorted->capacity, sorted->capacity);
            }
            if (sorted->count >= sorted->capacity) {
                ABQ_WARN_STATUS(abq_status_peek(), "Failure to intern");
            } else {
                // new item not yet found in list
                //  convert from insert index (-1 * (1+index))
                // index = ~(index) Parasoft hates me
                index = (uint16_t) (((uint16_t)UINT16_MAX) - index);
                ABQ_INSERT_AT(sorted->stack, cvar_t,
                        sorted->count, sorted->capacity, immutable, (uint16_t)index);
                retval = sorted->stack[index];
            }
        } else {
            retval = sorted->stack[index];
        }
    }
    abq_context_unlock();
    return retval;
}

cvar_t sorted_set_lookup(const sorted_set_t *sorted, cvar_t immutable) {
    cvar_t retval = NULL;
    abq_context_lock();
    if ((NULL != sorted->stack) && (0U != sorted->count)) {
        uint16_t index; // Initialized in ABQ_SEEK_BINARY
        ABQ_SEEK_BINARY(sorted->stack, cvar_t,
                sorted->count, index, sorted->cmp_fctn, immutable);
        if (index >= sorted->count) {
            // Not found
        } else {
            retval = sorted->stack[index];
        }
    }
    abq_context_unlock();
    return retval;
}

err_t sorted_set_remove (sorted_set_t *sorted, cvar_t immutable) {
    // default to a Not-Found err code
    err_t retval = EALREADY;
    if((NULL == sorted) || (NULL == immutable)){
        retval = EFAULT;
    }else if(NULL == sorted->stack){
        retval = EALREADY; // stack is empty
    }else{
        abq_context_lock();
        // While ABQ_SEEK_BINARY with cmp_fntc may be more efficient,
        //  we need to lookup the exact instance of the item based on pointer
        uint16_t index = sorted->capacity;
        cvar_t match = NULL;
        ABQ_SEEK_LAST(sorted->stack, cvar_t,
                sorted->count, index, match, (match == immutable));
        if (index >=  sorted->count) {
            retval = EALREADY;
        } else {
            ABQ_REMOVE_AT(sorted->stack, cvar_t, sorted->count, sorted->capacity, index);
            // If the sorted-set is empty, release all allocated resources
            if (0U == sorted->count) {
                sorted->capacity = 0U;
                abq_free(sorted->stack);
                sorted->stack = NULL;
            }
        }
        abq_context_unlock();
    }
    return retval;
}


