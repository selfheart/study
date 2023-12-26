/*
 * var_stack.c
 *
 *  Created on: May 6, 2019
 *      Author: mvogel
 */

#include <ontrac/ontrac/var_stack.h>
#include <ontrac/ontrac/abq_str.h>
#include <ontrac/util/abq_alist.h>

#ifndef MAX_VSTACK_INSTANCES
#define MAX_VSTACK_INSTANCES (5)
#endif /* MAX_VSTACK_INSTANCES */

static void vstack_delete(cvar_t old_stack);

DEFINE_CLASS(vstack_class, vstack_t, MAX_VSTACK_INSTANCES,
        NULL, NULL, NULL, NULL, vstack_delete);

vstack_t *vstack_create(class_ptr class_of_items) {
    vstack_t *rvalue = CREATE_BASE_INSTANCE(vstack_class, vstack_t);
    if (NULL != rvalue) {
        *rvalue = (vstack_t) {
            // if we were passed a stack of 'stack_of_*' instead of the real class ...
                .class_of_items = (NULL == class_of_items) ? NULL : class_of_items->class_ref,
                .array=NULL,
                .count=0,
                .capacity=0,
                .dynamic=true,
                .sorted=false
        };
    }
    return rvalue;
}

vstack_t* vstack_resolve(cvar_t item) {
    vstack_t * retval = NULL;
    CLASS_RESOLVE(vstack_class, vstack_t, retval, item);
    return retval;
}

static void vstack_delete(cvar_t old_stack) {
    vstack_t * vstack = vstack_resolve(old_stack);
    VITAL_NOT_NULL(vstack);
    VITAL_IS_OK(vstack_clear(vstack));
}

bool_t vstack_is_empty(const vstack_t *vstack) {
    bool_t rvalue = true;
    if((NULL != vstack) && (0UL != vstack->count)) {
        rvalue = false;
    }
    return rvalue;
}

int32_t vstack_size(const vstack_t *vstack) {
    int32_t rvalue = -1;
    if(NULL == vstack){
        (void)abq_status_set(EFAULT, false);
        rvalue = -1;
    }else{
        rvalue = (int32_t) vstack->count;
    }
    return rvalue;
}

static err_t vstack_reserve(vstack_t *vstack, cvar_t *item) {
    err_t retval = EXIT_SUCCESS;
    if ((NULL == vstack) || (NULL == item)) {
        retval = EFAULT;
    } else {
        if ((uint16_t) (vstack->count + 1U) <= vstack->capacity) {
            // vstack can handle additial item as is
            retval = EXIT_SUCCESS;
        } else if (vstack->dynamic) {
            // Grow stack to ensure capacity
            vstack->array = stack_realloc(vstack->array,
                    &vstack->capacity, (vstack->capacity < 4U) ? 4U : vstack->capacity);
            // Check if we were able to make room for the new item
            if ((uint16_t) (vstack->count + 1U) <= vstack->capacity) {
                // OK
            }else{
                retval = abq_status_take(ERANGE);
            }
        } else {
            // over-filled the capacity of a static stack
            retval = EOVERFLOW;
        }
        if ((EXIT_SUCCESS == retval) && (NULL != *item)) {
            cvar_t internalized  = classify(*item, vstack->class_of_items, (cvar_t)vstack);
            if (internalized == NULL) {
                retval = abq_status_take(EINVAL);
            } else {
                *item = internalized;
            }
        }
    }
    return retval;
}

static inline compare_function_t vstack_cmp_fctn(const vstack_t *vstack) {
    compare_function_t retval = identity_compare;
    if((NULL != vstack->class_of_items)
            && (NULL != vstack->class_of_items->compare)){
        retval = vstack->class_of_items->compare;
    }
    return retval;
}

err_t vstack_add(vstack_t *vstack, cvar_t item) {
    LOCK_CLASS(vstack_class);
    err_t retval = vstack_reserve(vstack, &item);
    if (EXIT_SUCCESS == retval) {
        if (vstack->sorted) {
            uint16_t index = vstack->count;
            compare_function_t cmp_fctn = vstack_cmp_fctn(vstack);
            ABQ_INSERT_SORTED(vstack->array, cvar_t, vstack->count,
                    vstack->capacity, index, cmp_fctn, item);
        } else {
            // Append to top-of-stack
            ABQ_INSERT_AT(vstack->array, cvar_t, vstack->count,
                    vstack->capacity, item, vstack->count);
        }
        if (EXIT_SUCCESS != retval) {
            // release our reference on failure
            (void) obj_release(item, (cvar_t)vstack);
        }
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}


err_t vstack_insert(vstack_t *vstack, uint16_t index, cvar_t item) {
    LOCK_CLASS(vstack_class);
    cvar_t stack_item = item;
    err_t retval = vstack_reserve(vstack, &stack_item);
    if (EXIT_SUCCESS == retval) {
        if (vstack->sorted) {
            retval = ENOSYS;
        } else {
            ABQ_INSERT_AT(vstack->array, cvar_t, vstack->count,
                    vstack->capacity, stack_item, index);
        }
        if (EXIT_SUCCESS != retval) {
            // release our reference on failure
            (void) obj_release(stack_item, (cvar_t)vstack);
        }
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

cvar_t vstack_get(const vstack_t *vstack, uint16_t index) {
    LOCK_CLASS(vstack_class);
    cvar_t retval = NULL;
    if(NULL == vstack) {
        (void) abq_status_set(EFAULT, false);
    }else if(index >= vstack->count){
        (void) abq_status_set(ERANGE, false);
    }else{
        retval = vstack->array[index];
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

cvar_t vstack_first(const vstack_t *vstack) {
    LOCK_CLASS(vstack_class);
    cvar_t retval = NULL;
    if (NULL == vstack) {
        (void) abq_status_set(EFAULT, false);
    } else if(0U == vstack->count) {
        // No error, return NULL
    } else {
        retval = vstack->array[0];
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

cvar_t vstack_last(const vstack_t *vstack) {
    LOCK_CLASS(vstack_class);
    cvar_t retval = NULL;
    if (NULL == vstack) {
        (void) abq_status_set(EFAULT, false);
    } else if(0U == vstack->count) {
        // No error, return NULL
    } else {
        retval = vstack->array[vstack->count-1U];
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

cvar_t vstack_match(const vstack_t *vstack, cvar_t item) {
    cvar_t retval = NULL;
    LOCK_CLASS(vstack_class);
    if (vstack_is_empty(vstack)) {
        retval = NULL;
    } else {
        int32_t index_of = vstack_index_of(vstack, item, 0U);
        if(0 <= index_of) {
            retval = vstack->array[index_of];
        }
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

bool_t vstack_contains(const vstack_t *vstack, cvar_t item) {
    bool_t retval = false;
    LOCK_CLASS(vstack_class);
    if (vstack_is_empty(vstack)) {
        retval = false;
    } else if (0 > vstack_index_of(vstack, item, 0U)) {
        retval = false;
    } else {
        retval = true;
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

int32_t vstack_index_of(const vstack_t *vstack,
        cvar_t item, uint16_t from_index) {
    int32_t retval = -1;
    LOCK_CLASS(vstack_class);
    if (NULL == vstack) {
        (void) abq_status_set(EFAULT, false);
    } else if(from_index >= vstack->count){
        (void) abq_status_set(ERANGE, false);
    } else {
        uint16_t index = (uint16_t) UINT16_MAX;
        compare_function_t cmp_fctn = vstack_cmp_fctn(vstack);
        if (vstack->sorted) {
            ABQ_SEEK_BINARY(vstack->array, cvar_t,
                    vstack->count, index, cmp_fctn, item);
        } else {
            cvar_t match = NULL;
            ABQ_SEEK_NEXT(vstack->array, cvar_t, vstack->count,
                    from_index, index, match, (0 == cmp_fctn(match, item)));
        }
        if(index >= vstack->count){
            retval = -1;
        }else{
            retval = (int32_t) index;
        }
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

err_t vstack_remove(vstack_t *vstack, cvar_t item) {
    err_t retval = EXIT_SUCCESS;
    LOCK_CLASS(vstack_class);
    if (NULL == vstack) {
        retval = EFAULT;
    } else if (NULL == vstack->array) {
        retval = EALREADY;
    } else {
        uint16_t index = vstack->capacity;
        cvar_t match = NULL;
        compare_function_t cmp_fctn = vstack_cmp_fctn(vstack);
        ABQ_SEEK_LAST(vstack->array, var_t,
                vstack->count, index, match, (0 == cmp_fctn(match, item)));
        if (index >= vstack->count) {
            retval = abq_status_take(EALREADY);
        } else {
            // Release reference to the item
            EXPECT_IS_OK(obj_release(vstack->array[index], vstack));
            // And remove it from the array
            ABQ_REMOVE_AT(vstack->array, cvar_t,
                    vstack->count, vstack->capacity, index);
            // Finally, release array if empty
            if ((0U == vstack->count) && (vstack->dynamic)) {
                abq_free(vstack->array);
                vstack->array = NULL;
                vstack->capacity = 0U;
            }
        }
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

cvar_t vstack_pop(vstack_t *vstack, cvar_t ref) {
    cvar_t retval = NULL;
    LOCK_CLASS(vstack_class);
    if (vstack_is_empty(vstack)) {
        // No items to pop from stack
    } else {
        // Return value will be the last item on the stack
        retval = vstack->array[vstack->count-1U];
        // Add new reference to the item
        (void) obj_reserve(retval, ref);
        // Release stack reference to the item
        EXPECT_IS_OK(obj_release(retval, vstack));
        // Release array if empty
        ABQ_REMOVE_AT(vstack->array, cvar_t,
                vstack->count, vstack->capacity, (vstack->count-1U));
        // Finally, release array if empty
        if ((0U == vstack->count) && (vstack->dynamic)) {
            abq_free(vstack->array);
            vstack->array = NULL;
            vstack->capacity = 0U;
        }
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

cvar_t vstack_pop_at(vstack_t *vstack, cvar_t ref, uint16_t index) {
    cvar_t retval = NULL;
    LOCK_CLASS(vstack_class);
    if (NULL == vstack) {
        abq_status_set(EFAULT, false);
    } else if(index >= vstack->count) {
        abq_status_set(ERANGE, false);
    } else {
        // Return value will be the last item on the stack
        retval = vstack->array[index];
        // Add new reference to the item
        (void) obj_reserve(retval, ref);
        // Release stack reference to the item
        EXPECT_IS_OK(obj_release(retval, vstack));
        // And remove it from the array
        ABQ_REMOVE_AT(vstack->array, cvar_t,
                vstack->count, vstack->capacity, index);
        // Finally, release array if empty
        if ((0U == vstack->count) && (vstack->dynamic)) {
            abq_free(vstack->array);
            vstack->array = NULL;
            vstack->capacity = 0U;
        }
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

err_t vstack_clear(vstack_t *vstack) {
    LOCK_CLASS(vstack_class);
    err_t rvalue=CHECK_NULL(vstack);
    if (EXIT_SUCCESS == rvalue) {
        for(uint16_t index=0U; index < vstack->count; index+= 1U){
            EXPECT_IS_OK(obj_release(vstack->array[index], vstack));
        }
        vstack->count = 0U;
        if (vstack->dynamic) {
            abq_free(vstack->array);
            vstack->array = NULL;
            vstack->capacity = 0U;
        }
    }
    UNLOCK_CLASS(vstack_class);
    return rvalue;
}

err_t vstack_set_sorted(vstack_t *vstack, bool_t sorted) {
    err_t retval = CHECK_NULL(vstack);
    LOCK_CLASS(vstack_class);
    if (EXIT_SUCCESS != retval){
        // do nothing
    }else if(NULL == vstack->class_of_items){
        retval = ENODATA;
    }else if (NULL == vstack->class_of_items->compare){
        retval = ENOSYS;
    }else if(sorted == vstack->sorted){
        retval = EALREADY;
    }else {
        if (sorted) {
            compare_function_t cmp_fctn = vstack_cmp_fctn(vstack);
            ABQ_BUBBLE_SORT(vstack->array, cvar_t, vstack->count, cmp_fctn);
        }
        vstack->sorted = sorted;
    }
    UNLOCK_CLASS(vstack_class);
    return retval;
}

int8_t vstack_compare_vstack(const vstack_t *left, const vstack_t *right) {
    LOCK_CLASS(vstack_class);
    int8_t rvalue = 0;
    if(NULL == left){
        rvalue = (int8_t) ((NULL == right) ? 0 : 1);
    }else if(NULL == right){
        rvalue = -1;
    }else{
        compare_function_t comp_fctn = var_compare;
        if (NULL != left->class_of_items) {
            if (NULL != right->class_of_items) {
                rvalue = class_class.compare((cvar_t) left->class_of_items,
                        (cvar_t) right->class_of_items);
            }
            if (NULL != left->class_of_items->compare) {
                comp_fctn = left->class_of_items->compare;
            }
        } else if ((NULL != right->class_of_items)
                && (NULL != right->class_of_items->compare)) {
            comp_fctn = right->class_of_items->compare;
        } else {
            // leave comp_fctn as var_compare
        }
        uint16_t left_index = 0;
        uint16_t right_index = 0;
        while (0 == rvalue) {
            if (left->count == left_index) {
                if (right->count == right_index) {
                    ABQ_VITAL(0 == rvalue);
                    break; // end of stack, everything matched
                } else {
                    // left stack has more items then right stack
                    rvalue = 1;
                }
            } else if (right->count == right_index) {
                // right stack has more items then left stack
                rvalue = -1;
            } else {
                rvalue = comp_fctn(left->array[left_index],
                        right->array[right_index]);
                // iterate to the next index and compare those values
                left_index += 1U;
                right_index += 1U;
            }
        };
    }
    UNLOCK_CLASS(vstack_class);
    return rvalue;
}

vstack_t *vstack_from_array(class_ptr item_type, cvar_t *array, uint16_t size) {
    vstack_t *retval = NULL;
    if (NULL == array) {
        abq_status_set(EFAULT, false);
    }else{
        retval = vstack_create(item_type);
        if (NULL == retval) {
            abq_status_set(ENOMEM, false);
        } else {
            ABQ_VITAL(retval->dynamic);
            VITAL_IS_NULL(retval->array);
            retval->array = stack_realloc(retval->array,
                    &retval->capacity, size);
            if (NULL == retval->array) {
                abq_status_set(ENOMEM, false);
                (void) obj_release_self(retval);
                retval = NULL;
            } else {
                for (uint16_t index=0;index < size; index+=1U) {
                    // Will take care of classifying the items as needed
                    EXPECT_IS_OK(vstack_add(retval, array[index]));
                }
            }
        }
    }
    return retval;
}

cvar_t* stack_realloc(cvar_t* old_stack,
        uint16_t *capacity, uint16_t increment) {
    // Return old-array with unmodified capacity if we are unable to realloc
    cvar_t* retval = old_stack;
    if (0U >= increment) {
            abq_status_set(ERANGE, false);
    } else if (NULL == capacity) {
        // TODO: attempt to lookup capacity based on mem_usage_t of old_stack
        abq_status_set(EFAULT, false);
    } else {
        mem_usage_t *mem_usage = NULL;
        var_t new_stack = abq_calloc_ex(((size_t)*capacity + (size_t)increment),
                (size_t) sizeof(cvar_t), &mem_usage);
        if (NULL == new_stack) {
            retval = old_stack;
            abq_status_set(ENOMEM, false);
        } else {
            VITAL_NOT_NULL(mem_usage);
            if (NULL != old_stack) {
                // Copy existing stack data onto the new stack
                memcpy_ltr(new_stack, old_stack, (size_t) *capacity * (size_t) sizeof(cvar_t));
                // Release the old stack
                abq_free(old_stack);
            } else {
                ABQ_VITAL(0U == *capacity);
            }
            *capacity = (uint16_t) (mem_usage->increment / sizeof(cvar_t));
            retval = ptr2ptrptr(new_stack);
        }
    }
    return retval;
}
