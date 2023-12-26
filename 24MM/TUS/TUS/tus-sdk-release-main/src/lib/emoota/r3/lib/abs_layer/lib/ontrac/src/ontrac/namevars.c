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
 * @file ontrac/namevars.c
 * @date Mar 31, 2020
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {HELP ME}
 */

#include <ontrac/ontrac/abq_str.h>
#include <ontrac/ontrac/namevars.h>

#ifndef MAX_NAMEVARS_INSTANCES
#define MAX_NAMEVARS_INSTANCES (32U)
#endif /* MAX_NAMEVARS_INSTANCES */

static void namevars_delete(cvar_t old_namevars);

DEFINE_CLASS(namevars_class, namevars_t, MAX_NAMEVARS_INSTANCES,
        NULL, NULL, NULL, NULL, namevars_delete);

namevars_t *namevars_create( void ) {
    namevars_t *rvalue = CREATE_BASE_INSTANCE(namevars_class, namevars_t);
    if (NULL != rvalue) {
        *rvalue = (namevars_t) {
            .array=NULL,
            .count=0U,
            .capacity=0U,
        };
    }
    return rvalue;
}

namevars_t* namevars_resolve(cvar_t item) {
    namevars_t * retval = NULL;
    CLASS_RESOLVE(namevars_class, namevars_t, retval, item);
    return retval;
}

static void namevars_delete(cvar_t old_namevars) {
    namevars_t * namevars = namevars_resolve(old_namevars);
    VITAL_NOT_NULL(namevars);
    VITAL_IS_OK(namevars_clear(namevars));
}

static abq_namevar_t* namevars_realloc(abq_namevar_t* old_stack,
        uint16_t *capacity, uint16_t increment) {
    // Return old-array with unmodified capacity if we are unable to realloc
    abq_namevar_t* retval = old_stack;
    if (0U >= increment) {
            abq_status_set(ERANGE, false);
    } else if (NULL == capacity) {
        // TODO: attempt to lookup capacity based on mem_usage_t of old_stack
        abq_status_set(EFAULT, false);
    } else {
        mem_usage_t *mem_usage = NULL;
        static const size_t abq_namevar_size
                = ABQ_ALIGN_SIZE( sizeof(abq_namevar_t), abq_word_t);
        byte_t* new_stack = abq_calloc_ex(((size_t)*capacity + (size_t)increment), abq_namevar_size, &mem_usage);
        if (NULL == new_stack) {
            retval = old_stack;
            abq_status_set(ENOMEM, false);
        } else {
            VITAL_NOT_NULL(mem_usage);
            if (NULL != old_stack) {
                // Copy existing stack data onto the new stack
                memcpy_ltr(new_stack, old_stack, (size_t) *capacity * abq_namevar_size);
                // Release the old stack
                abq_free(old_stack);
            } else {
                ABQ_VITAL(0U == *capacity);
            }
            *capacity = (uint16_t) (mem_usage->increment / abq_namevar_size);
            bytes_copy(&retval, &new_stack, sizeof(cvar_t));
        }
    }
    return retval;
}

static err_t namevars_acquisition(namevars_t *namevars, uint16_t index, cstr_t name, cvar_t value) {
    err_t retval = EXIT_SUCCESS;
    if(namevars->capacity == namevars->count){
        // Must attempt to grow the stack if we are at capacity
        namevars->array = namevars_realloc(namevars->array,
                &namevars->capacity, (namevars->capacity < 4U) ? 4U : namevars->capacity);
        if(namevars->capacity <= namevars->count) {
            // Failed to upgrade stack capacity
            retval = abq_status_take(ENOMEM);
        }
    }
    if(EXIT_SUCCESS != retval) {
        // Return error code as is
    } else {
        // Vital that index is within a sane range (0 to count)
        ABQ_VITAL(index <= namevars->count);
        // Capacity for new element was previously ensured
        //  but won't kill us to verify conditions are as expected
        ABQ_VITAL(namevars->count < namevars->capacity);
        abq_namevar_t namevar = (abq_namevar_t) {
            .name = str_coerce(name, namevars),
            .value = value
        };
        if (NULL == namevar.name) {
            retval = abq_status_take(EINVAL);
        } else {
            if (NULL != value) {
                // Must be an ontrac classified object if not NULL
                VITAL_IS_OK(obj_reserve(value, namevars));
            }
            // And copy data into internal stack of namevar_t
            ABQ_INSERT_AT(namevars->array, abq_namevar_t, namevars->count,
                    namevars->capacity, namevar, index);
        }
    }
    return retval;
}

err_t namevars_append(namevars_t *namevars, cstr_t name, cvar_t value) {
    err_t retval = EXIT_SUCCESS;
    LOCK_CLASS(namevars_class);
    if((NULL == namevars) || (NULL == name)) {
        retval = EFAULT;
    } else {
        retval = namevars_acquisition(namevars, namevars->count, name, value);
    }
    UNLOCK_CLASS(namevars_class);
    return retval;
}

err_t namevars_addstr(namevars_t *namevars, cstr_t name, cstr_t value) {
    err_t retval = EXIT_SUCCESS;
    cstr_t coerced = str_coerce(value, NULL);
    if((NULL == coerced) && (NULL != value)) {
        retval = abq_status_take(EINVAL);
    } else {
        retval = namevars_append(namevars, name, coerced);
    }
    (void)obj_release_self(coerced);
    return retval;
}

err_t namevars_insert(namevars_t *namevars, uint16_t index, cstr_t name, cvar_t value) {
    err_t retval = EXIT_SUCCESS;
    LOCK_CLASS(namevars_class);
    if ((NULL == namevars) || (NULL == name)) {
        retval = EFAULT;
    } else if(namevars->count < index) {
        retval = ERANGE;
    } else {
        retval = namevars_acquisition(namevars, index, name, value);
    }
    UNLOCK_CLASS(namevars_class);
    return retval;
}

cvar_t namevars_take(namevars_t *namevars, cstr_t name, cvar_t ref) {
    cvar_t retval = NULL;
    LOCK_CLASS(namevars_class);
    if ((NULL == namevars) || (NULL == name)) {
        abq_status_set(EFAULT, false);
    } else {
        uint16_t match_index = 0U;
        for(match_index = 0U; match_index < namevars->count; match_index++) {
            if(UTF8_EQUALS(name, namevars->array[match_index].name)) {
                break; // Found a match
            }
        }
        if(match_index >= namevars->count) {
            abq_status_set(NOT_FOUND, false);
        } else {
            // Release reference to the name value pair
            EXPECT_IS_OK(obj_release(namevars->array[match_index].name, namevars));
            namevars->array[match_index].name = NULL;
            if(NULL != namevars->array[match_index].value){
                retval = namevars->array[match_index].value;
                VITAL_IS_OK(obj_reserve(retval, ref));
                VITAL_IS_OK(obj_release(retval, namevars));
            }
            // And remove it from the array of abq_namevar_t
            ABQ_REMOVE_AT(namevars->array, abq_namevar_t,
                    namevars->count, namevars->capacity, match_index);
            // Finally, release array if empty
            if (0U == namevars->count) {
                abq_free(namevars->array);
                namevars->array = NULL;
                namevars->capacity = 0U;
            }
        }
    }
    UNLOCK_CLASS(namevars_class);
    return retval;
}

static void namevars_relinquish(namevars_t *namevars, uint16_t index) {
    ABQ_VITAL(index < namevars->count);
    // Release reference to the name value pair
    EXPECT_IS_OK(obj_release(namevars->array[index].name, namevars));
    namevars->array[index].name = NULL;
    if (NULL != namevars->array[index].value) {
        EXPECT_IS_OK(obj_release(namevars->array[index].value, namevars));
        namevars->array[index].value = NULL;
    }
    // And remove it from the array of abq_namevar_t
    ABQ_REMOVE_AT(namevars->array, abq_namevar_t,
            namevars->count, namevars->capacity, index);
    // Finally, release array if empty
    if (0U == namevars->count) {
        abq_free(namevars->array);
        namevars->array = NULL;
        namevars->capacity = 0U;
    }
}

err_t namevars_clear(namevars_t *namevars) {
    LOCK_CLASS(namevars_class);
    err_t rvalue=CHECK_NULL(namevars);
    if (EXIT_SUCCESS == rvalue) {
        while(0U != namevars->count) {
            namevars_relinquish(namevars, (namevars->count-1U));
        }
        ABQ_VITAL(0U == namevars->capacity);
        VITAL_IS_NULL(namevars->array);
    }
    UNLOCK_CLASS(namevars_class);
    return rvalue;
}
