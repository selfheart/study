//#line 2 "ontrac/abq_class.c"
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
 * @file ontrac/abq_class.c
 * @date Mar 27, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/ontrac/abq_class.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/ontrac/traverser.h>
#include <ontrac/ontrac/var_list.h>

#ifdef HAVE_AQSPIL_H
#if !defined(NDEBUG)
#include <spil_os/aqSpilImpl.h>
#endif /* !defined(NDEBUG) */
#endif

static int8_t class_compare(cvar_t left, cvar_t right);
EMPTY_CLASS(class_class, class_t, NULL, NULL, class_compare,
        NULL, global_property_getter, global_property_setter, NULL, &class_class);

#ifndef EX_OBJ_VAR_COUNT
#define EX_OBJ_VAR_COUNT (32U)
#endif /* EX_OBJ_VAR_COUNT */

STATIC_ITEM_POOL(obj_t, EX_OBJ_VAR_COUNT, ex_obj_usage);

// a "hash-map" instance of all active obj_t instances
//  hash is computed using the address of obj->member
//  so that a lookup using the member instead of obj is possible
static LLIST_HEAD_DEF(obj_t, obj_hash_map)[CLASS_HASH_CAPACITY]={0};
/** objects that are actively being deleted */
static LLIST_HEAD_DEF(obj_t, recycle_bin) = NULL;

#if !defined(NDEBUG)
cvar_t item_to_debug = (cvar_t) NULL;
#endif /* !defined(NDEBUG) */

size_t obj_address_hash(const void *ptr) {
    uintptr_t address = ptr2addr(ptr);
#ifndef __SIZEOF_POINTER__
    address ^= (address >> 32U);
#elif __SIZEOF_POINTER__ == 8
    address ^= (address >> 32U);
#else /* 32-bit arithmetic */
    // Assume 32 bits or less
#endif /* 32-bit arithmetic */
    // first fold it in "half"
    address ^= (address >> 16U);
    // then fold the twice-fold bits into themselves
    address ^= (address >> 8U);
    // then return the significant composite bits
    static const size_t class_hash_bitmask = CLASS_HASH_CAPACITY - 1U;
    return (class_hash_bitmask & (size_t) address);
}

int8_t identity_compare(cvar_t left, cvar_t right) {
    uintptr_t first = ptr2addr(left);
    uintptr_t second = ptr2addr(right);
    int8_t rvalue = 0;
    if (first > second) {
        rvalue = 1;
    } else if (first < second) {
        rvalue = -1;
    } else {
        rvalue = 0;
    }
    return rvalue;
}

int8_t var_compare(cvar_t left, cvar_t right) {
    int8_t retval = 0;
    class_ptr first = class_of(left);
    class_ptr second = class_of(right);
    if((NULL != first) && (NULL != second)) {
        if (first == second) {
            if(NULL != first->compare) {
                retval = first->compare(left, right);
            } else {
                retval = identity_compare(left, right);
            }
        } else {
            retval = utf8_compare_exact(first->class_name, second->class_name, -1);
            FATAL_IF(0 == retval);
        }
    } else if((NULL != first) && (NULL != first->compare)) {
        retval = first->compare(left, right);
    } else if((NULL != second) && (NULL != second->compare)) {
        retval = second->compare(left, right);
    } else {
        retval = identity_compare(left, right);
    }
    return retval;
}

static obj_t *obj_hashmap_get(cvar_t item) {
    obj_t *rvalue=NULL;
    size_t bin = obj_address_hash(item);
    LLIST_HEAD_DEF(obj_t, objs_in_bin) = obj_hash_map[bin];
    LLIST_FIND(objs_in_bin, obj_t, rvalue, next, (rvalue->member == item));
    if (NULL == rvalue) {
        // object wasn't found in list of active objects, try the recycle_bin
        LLIST_FIND(recycle_bin, obj_t, rvalue, next, (rvalue->member == item));
    }
    return rvalue;
}

obj_t *obj_lookup(cvar_t item) {
    obj_t *rvalue=NULL;
    // Quick pre-search of the three primitive objects (NULL, TruePtr, and FalsePtr)
    LLIST_FIND(primative_obj_list, obj_t, rvalue, next, (rvalue->member == item));
    if(NULL == rvalue) {
        // Expected behavior is to find active objects in the obj_hash_map
        rvalue = obj_hashmap_get(item);
    }
    return rvalue;
}

void item_field_set(cvar_t item,
        cvar_t* field, cvar_t new_value) {
    VITAL_NOT_NULL(item);
    VITAL_NOT_NULL(field);
    if (new_value != *field) {
        // Overwrite the old value with new one
        cvar_t old_value = *field;
        *field = new_value;
        // And update the references as appropriate
        (void) obj_reserve(new_value, item);
        (void) obj_release(old_value, item);
    }
}

void item_field_take(cvar_t item,
        cvar_t* field, cvar_t new_value) {
    VITAL_NOT_NULL(item);
    VITAL_NOT_NULL(field);
    // Overwrite the old value with new one
    cvar_t old_value = *field;
    *field = new_value;
    // And update the references as appropriate
    (void) obj_takeover(new_value, item);
    (void) obj_release(old_value, item);
}

static err_t class_typecheck(class_ptr meta) {
    err_t rvalue = EXIT_SUCCESS;
    // ugly, consider use of magic number instead
    if (NULL == meta) {
        rvalue = EFAULT;
    } else if(&class_class != meta->magic) {
        rvalue = EINVAL;
    } else {
        if ((NULL != meta->mem_usage)
                && (NULL == meta->mem_usage->mutex)) {
            register_mem_usage(meta->mem_usage, false); // this is to init the class_class mutex
        }
        rvalue = EXIT_SUCCESS;
    }
    return rvalue;
}

class_ptr class_of(cvar_t item) {
    class_ptr retval = NULL;
    obj_t *obj = NULL;
    // Quick pre-search of the three primitive objects (NULL, TruePtr, and FalsePtr)
    LLIST_FIND(primative_obj_list, obj_t, obj, next, (obj->member == item));
    if (NULL != obj) {
        // Primitive item
        retval = obj->meta;
    } else {
        obj = obj_hashmap_get(item);
        if (NULL == obj) {
            // Unclassified item
        } else {
            retval = obj->meta;
        }
    }
    return retval;
}
err_t class_check(class_ptr meta, cvar_t item) {
    err_t retval = class_typecheck(meta);
    if (EXIT_SUCCESS == retval) {
        if (&class_class == meta) {
            retval = ENOSYS; // unsupported
        } else {
            class_ptr real_meta = class_realize(meta);
            obj_t *obj = obj_lookup(item);
            class_ptr obj_meta = (NULL==obj) ? NULL : obj->meta;
            if (obj_meta != real_meta) {
                if (NULL == item) {
                    retval = EFAULT;
                } else {
                    retval = EINVAL;
                }
            }
        }
    }
    return retval;
}

#if !defined(NDEBUG)
static cvar_t* cvars_realloc(cvar_t* old_stack,
        uint16_t *capacity, uint16_t increment) {
    mem_usage_t *mem_usage = NULL;
    // Return old-array with unmodified capacity if we are unable to realloc
    cvar_t* retval = old_stack;
    ABQ_VITAL(0U != increment);
    VITAL_NOT_NULL(capacity);
    var_t new_stack = abq_calloc_ex(((size_t)*capacity + (size_t)increment),
            (size_t) sizeof(cvar_t), &mem_usage);
    if (NULL == new_stack) {
        abq_status_set(ENOMEM, false);
    } else {
        VITAL_NOT_NULL(mem_usage);
        if (NULL != old_stack) {
            // Copy existing stack data onto the new stack
            memcpy_ltr(new_stack, old_stack, (size_t) *capacity * (size_t)sizeof(var_t));
            // Release the old stack
            abq_free(old_stack);
        } else {
            ABQ_VITAL(0U == *capacity);
        }
        retval = ptr2ptrptr(new_stack);
        *capacity = (uint16_t) (mem_usage->increment / sizeof(cvar_t));
    }
    return retval;
}

#endif /* !defined(NDEBUG) */

static err_t abq_encode_item_info(abq_encoder_t *encoder, cvar_t item) {
    err_t retval = EXIT_SUCCESS;
    obj_t *obj = obj_lookup(item);
    mem_usage_t *mem_usage = lookup_mem_usage(item);
    if((NULL == obj) || (NULL == obj->meta)) {
        if (NULL != mem_usage) {
            retval = abq_encode_ascii(encoder, mem_usage->name, -1);
        } else {
            // TODO: lookup intel from mem_map or debug symbols as able
            //  but as long as address information is available, should be
            //  able to manually lookup that info if needed
            (void) abq_encode_ascii(encoder, "0x", 2); // parasoft-suppress CERT_C-MSC41-a-1 "c0203. This string does not contain sensitive information."
            retval = abq_encode_uint(encoder, ptr2addr(item), HEX_RADIX);
        }
    }else if (&string_class == obj->meta) {
        (void) abq_encode_char(encoder, '\"');
        (void) abq_encode_ascii(encoder, str_resolve(obj->member), -1);
        retval = abq_encode_char(encoder, '\"');
    } else if(&number_class == obj->meta) {
        retval = abq_encode_number(encoder, number_resolve(obj->member));
    } else if(&null_class == obj->meta) {
        retval = abq_encode_ascii(encoder, abq_null_str, -1);
    } else if(&bool_class == obj->meta) {
        if (obj->member == (cvar_t)true_ptr) {
            retval = abq_encode_ascii(encoder, abq_true_str, -1);
        } else {
            ABQ_VITAL(obj->member == (cvar_t)false_ptr);
            retval = abq_encode_ascii(encoder, abq_false_str, -1);
        }
    } else {
        retval = abq_encode_ascii(encoder, obj->meta->class_name, -1);
    }
    if (NULL != mem_usage) {
        (void) abq_encode_char(encoder, '[');
        uint16_t usage_index = mem_usage_index_of(mem_usage, item);
        ABQ_VITAL(usage_index < mem_usage->total);
        (void) abq_encode_int(encoder, (int64_t)usage_index, DECIMAL_RADIX);
        (void) abq_encode_char(encoder, ']');
        (void) abq_encode_char(encoder, ' ');
    }
    return retval;
}

static void print_change_in_obj(const obj_t* obj, cstr_t desc, cvar_t ref) {
    byte_t buffer[B1024_SIZE];
    ABQ_ENCODER(encoder, &ascii_codec, buffer, sizeof(buffer));
    EXPECT_IS_OK(abq_encode_item_info(&encoder, obj->member));
#if !defined(NDEBUG)
    for(uint16_t index = 0U; index < obj->refs_len; index++) {
        if (0U == index) {
            (void) abq_encode_ascii(&encoder, "refs: ", 6); // parasoft-suppress CERT_C-MSC41-a-1 "c0204. This string does not contain sensitive information."
        } else {
            (void) abq_encode_ascii(&encoder, ", ", 2); // parasoft-suppress CERT_C-MSC41-a-1 "c0205. This string does not contain sensitive information."
        }
        EXPECT_IS_OK(abq_encode_item_info(&encoder, obj->refs[index]));
    }
#endif /* !defined(NDEBUG) */
    (void) abq_encode_ascii(&encoder, " => ", 4); // parasoft-suppress CERT_C-MSC41-a-1 "c0206. This string does not contain sensitive information."
    (void) abq_encode_ascii(&encoder, desc, -1);
    if(NULL != ref) {
        EXPECT_IS_OK(abq_encode_item_info(&encoder, ref));
    }
    ABQ_DEBUG_MSG(buffer);
}

static bool_t obj_initialize(obj_t *obj,
        class_ptr meta, cvar_t member, cvar_t ref) {
    // USDM-12332: don't check the recycle_bin unless we have exclusive access
    abq_context_lock();
    ABQ_VITAL(LLIST_IS_EMPTY(recycle_bin, next));
    VITAL_IS_OK(class_typecheck(meta));
    ABQ_VITAL(invalid_delete != meta->on_delete);
    size_t bin = obj_address_hash(member);
#if !defined(NDEBUG)
    LLIST_FOREACH(obj_hash_map[bin], obj_t, existing, next) {
        ABQ_VITAL(existing != obj);
    }
#endif /* !defined(NDEBUG) */
    *obj = (obj_t) {
            .meta = meta,
#if !defined(NDEBUG)
            .refs=NULL,
            .refs_len=0U,
            .refs_max=0U,
#else /* NDEBUG */
            .ref_count=0U,
#endif /* NDEBUG */
            .member=member,
            .next=NULL
    };
    bool_t retval = true;
#if !defined(NDEBUG)
    // Grow the stack as needed to fit new references
    obj->refs = cvars_realloc(obj->refs, &obj->refs_max, 2U);
    // Fatal if we were unable to allocate memory
    if (obj->refs_len >= obj->refs_max) {
        abq_free_into(obj, obj->meta->mem_usage);
        // Free the stack of refs
        obj->refs_max = 0U;
        abq_free(obj->refs);
        obj->refs = NULL;
        abq_free_into(obj, meta->mem_usage);
        retval = false;
    } else{
        cvar_t anchor = (NULL==ref) ? member : ref;
        ABQ_INSERT_AT(obj->refs, cvar_t,
                obj->refs_len, obj->refs_max, anchor, obj->refs_len);
        // Add object to global lookup table
        LLIST_PUSH(obj_hash_map[bin], obj, next);
        // ABQ_INFO_MSG_P("initialized: ", anchor)
    }
#else /* NDEBUG */
    // Record the initial reference
    obj->ref_count += 1U;
    // Add object to global lookup table
    LLIST_PUSH(obj_hash_map[bin], obj, next);
#endif /* NDEBUG */
    abq_context_unlock();
    return retval;
}

static const size_t aligned_obj_size
    = ABQ_ALIGN_SIZE(sizeof(obj_t), abq_word_t);

cvar_t class_new_instance(class_ptr meta, cvar_t ref) {
    byte_t* retval = NULL;
    obj_t* obj = NULL;
    byte_t *buffer = malloc_from(meta->mem_usage);
    if (NULL == buffer) {
        // private pool of memory has completely died up, so we
        //  must now attempt allocating memory from shared memory
        buffer = abq_malloc_ex(meta->mem_usage->increment, NULL);
    }
    (void) bytes_copy(&obj, &buffer, sizeof(cvar_t));
    if (NULL != obj) {
        retval = &buffer[aligned_obj_size];
        if (obj_initialize(obj, meta, retval, ref)) {
            // Good to go, return a clean instance
            bytes_set(retval, '\0', (meta->mem_usage->increment - aligned_obj_size));
        } else {
            // freed in obj_initialize
            retval = NULL;
        }
    }
    return retval;
}

byte_t* new_instance_flex(class_ptr meta, size_t size) {
    byte_t* retval = NULL;
    obj_t* obj = NULL;
    byte_t *buffer = abq_malloc_ex(aligned_obj_size + size, NULL);
    (void) bytes_copy(&obj, &buffer, sizeof(cvar_t));
    if (NULL != obj) {
        retval = &buffer[aligned_obj_size];
        if (obj_initialize(obj, meta, retval, NULL)) {
            // Good to go, return a clean instance
            bytes_set(retval, '\0', size);
        } else {
            // freed in obj_initialize
            retval = NULL;
        }
    }
    return retval;
}

err_t obj_reserve(cvar_t item, cvar_t ref) {
    err_t retval = EXIT_SUCCESS;
    if (NULL != item) {
        abq_context_lock();
        obj_t *obj = obj_hashmap_get(item);
        if (NULL == obj) {
            // check if it is a primitive
            LLIST_FIND(primative_obj_list, obj_t, obj, next,
                    (obj->member == item));
            if (NULL == obj) {
                retval = EINVAL;
            }
        } else if (&class_class == obj->meta) {
            // classes should forever be unreferenced
        } else {
#if !defined(NDEBUG)
            ABQ_VITAL(0U != obj->refs_len);
            // Check that the refs stack has room for another reference
            if (obj->refs_len >= obj->refs_max) {
                // Grow the stack as needed to fit new references
                obj->refs = cvars_realloc(obj->refs, &obj->refs_max, 2U);
                // Fatal if we were unable to allocate memory
                FATAL_IF(obj->refs_len >= obj->refs_max);
            }
            cvar_t anchor = (NULL==ref) ? item : ref;
            if (item_to_debug == item) {
                print_change_in_obj(obj, "reserve: ", anchor); // parasoft-suppress CERT_C-MSC41-a-1 "c0207. This string does not contain sensitive information."
            }
            ABQ_INSERT_AT(obj->refs, cvar_t,
                    obj->refs_len, obj->refs_max, anchor, obj->refs_len);
#else /* NDEBUG */
            ABQ_VITAL(0U != obj->ref_count);
            obj->ref_count += 1U;
#endif /* NDEBUG */
            retval = EXIT_SUCCESS;
        }
        abq_context_unlock();
    }
    return retval;
}
static err_t obj_delete(obj_t* obj) {
#if !defined(NDEBUG)
    VITAL_VALUE(0U, obj->refs_len);
    if (item_to_debug == obj->member) {
        print_change_in_obj(obj, "delete", NULL); // parasoft-suppress CERT_C-MSC41-a-1 "c0208. This string does not contain sensitive information."
        item_to_debug = NULL;
    }
#else /* NDEBUG */
    VITAL_VALUE(0U, obj->ref_count);
#endif /* NDEBUG */

    obj_t *match = NULL;
    // Remove the obj_t from the obj_hash_map
    size_t bin = obj_address_hash(obj->member);
    LLIST_FIND_AND_REMOVE(obj_hash_map[bin],
            obj_t, match, next, (match == obj));
    if (NULL == match) {
        // twice deleted obj_t ?
        LLIST_FIND(recycle_bin,
                obj_t, match, next, (match == obj));
        VITAL_NOT_NULL(match);
    } else {
        // And add it to the recycle_bin for the duration of on_delete callback
        LLIST_PUSH(recycle_bin, obj, next);
        if (NULL != obj->meta->on_delete) {
            (void) obj->meta->on_delete(obj->member);
        }
        LLIST_POP(recycle_bin, match, next);
        ABQ_VITAL(match == obj);

#if !defined(NDEBUG)
        // Free the stack of refs
        obj->refs_max = 0U;
        abq_free(obj->refs);
        obj->refs = NULL;
#else /* NDEBUG */
#endif /* NDEBUG */
        abq_free_into(obj, obj->meta->mem_usage);
    }
    return EXIT_SUCCESS;
}

err_t obj_release(cvar_t item, cvar_t ref) {
    err_t rvalue = EXIT_SUCCESS;
    if (NULL != item) {
        abq_context_lock();
        obj_t *obj = obj_hashmap_get(item);
        if (NULL == obj) {
            // check if it is a primitive
            LLIST_FIND(primative_obj_list, obj_t, obj, next,
                    (obj->member == item));
            if (NULL == obj) {
                rvalue = EINVAL;
            }
        } else {
#if !defined(NDEBUG)
            VITAL_NOT_NULL(obj->refs);
            uint16_t index_of = obj->refs_max;
            cvar_t match = NULL;
            cvar_t anchor = (NULL == ref) ? item : ref;
            if (item_to_debug == item) {
                print_change_in_obj(obj, "release: ", anchor); // parasoft-suppress CERT_C-MSC41-a-1 "c0209. This string does not contain sensitive information."
            }
            ABQ_SEEK_LAST(obj->refs, cvar_t,
                    obj->refs_len, index_of, match, (match == anchor));
            if(index_of >= obj->refs_len) {
                if (0U == obj->refs_len) {
                    // Might happen during a failed unit-test
                    //  when using abq_free_all_objs_and_mem
                    rvalue = ENOLINK;
                } else {
                    print_change_in_obj(obj, "invalid: ", anchor); // parasoft-suppress CERT_C-MSC41-a-1 "c0210. This string does not contain sensitive information."
                    abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0211. This string does not contain sensitive information."
                              ENOLINK, "unref'd"); // parasoft-suppress CERT_C-MSC41-a-1 "c0212. This string does not contain sensitive information."
                }
            } else {
                ABQ_REMOVE_AT(obj->refs, cvar_t, obj->refs_len, obj->refs_max, index_of);
                if(0U != obj->refs_len) {
                    // Still has more refs
                } else {
                    // and delete the obj in question
                    VITAL_IS_OK(obj_delete(obj));
                }
            }
#else /* NDEBUG */
            if(0U == obj->ref_count){
                abq_fatal(__FILENAME__, __LINE__, ENOLINK, "zero refs");
            }else if(1 == obj->ref_count){
                obj->ref_count = 0U;
                // and delete the obj in question
                VITAL_IS_OK(obj_delete(obj));
            }else{
                obj->ref_count -= 1U;
            }
#endif /* NDEBUG */
        }
        abq_context_unlock();
    }
    return rvalue;
}

err_t obj_takeover(cvar_t item, cvar_t ref) {
    err_t retval = EXIT_SUCCESS;
    if(NULL == item) {
        retval = EFAULT;
    } else {
#if !defined(NDEBUG)
        abq_context_lock();
        obj_t *obj = obj_hashmap_get(item);
        if (NULL == obj) {
            retval = EINVAL;
            ABQ_DUMP_ERROR(retval, "unclassified");
        } else {
            uint16_t index = obj->refs_max;
            cvar_t match = NULL;
            ABQ_SEEK_NEXT(obj->refs, cvar_t,
                    obj->refs_len, 0U, index, match, (match == item));
            FATAL_IF(index >= obj->refs_len);
            // Replace the self-reference with ref
            if (item_to_debug == item) {
                print_change_in_obj(obj, "takeover: ", ref); // parasoft-suppress CERT_C-MSC41-a-1 "c0213. This string does not contain sensitive information."
            }
            if (NULL == ref) {
                // Self reference
                obj->refs[index] = (cvar_t)item;
            } else {
                obj->refs[index] = (cvar_t)ref;
            }
        }
        abq_context_unlock();
#endif /* !defined(NDEBUG)  */
    }
    return retval;
}

cvar_t item_of_class(cvar_t item, class_ptr meta, cvar_t ref) {
    cvar_t retval = NULL;
    if (NULL == meta) {
        //simply reserve the reference if able
        (void) obj_reserve(item, ref);
        retval = item;
    } else if(NULL == item) {
        // Return NULL for NULL items
    }else if(EXIT_SUCCESS != class_typecheck(meta)){
        (void) abq_status_set(class_typecheck(meta), false);
    }else{
        abq_context_lock();
        class_ptr real_meta = class_realize(meta);
        obj_t *obj = obj_lookup(item);
        if(NULL != obj){
            if (real_meta == obj->meta) {
                // the item was already classified with meta
                VITAL_IS_OK(obj_reserve(item, ref));
                retval = obj->member;
            } else {
                // the item was already classified as something else
                (void) abq_status_set(EINVAL, false);
                // Invoking build_class_instance results in recursive call stacks
            }
        } else {
            // classify the object
            ITEM_POOL_ALLOC(&ex_obj_usage, obj_t, obj);
            if(NULL != obj) {
                if (obj_initialize(obj, real_meta, item, ref)) {
                    // Good to go
                    retval = item;
                } else {
                    ITEM_POOL_FREE(&ex_obj_usage, obj_t, obj);
                    obj = NULL;
                }
            }else{
                (void) abq_status_set(ENOMEM, false);
            }
        }
        abq_context_unlock();
    }
    return retval;
}

cvar_t classify(cvar_t item, class_ptr meta, cvar_t ref) {
    cvar_t rvalue = NULL;
    if (NULL != item) {
        obj_t *obj = obj_hashmap_get(item);
        if (NULL == meta) {
            //simply reserve the reference if able
            (void) obj_reserve(item, ref);
            rvalue = item;
        } else if(EXIT_SUCCESS != class_typecheck(meta)) {
            (void) abq_status_set(class_typecheck(meta), false);
        } else {
            class_ptr real_meta = class_realize(meta);
            if((NULL != obj) && (obj->meta == real_meta)) {
                //simply reserve the reference if able
                (void) obj_reserve(item, ref);
                rvalue = item;
            }else{
                if(NULL != real_meta->coerce){
                    rvalue = real_meta->coerce(item, ref);
                }
                if(NULL == rvalue){
                    rvalue = item_of_class(item, meta, ref);
                }
            }
        }
    }
    return rvalue;
}

cvar_t abq_coerce(cvar_t item, class_ptr meta, cvar_t ref) {
    cvar_t retval = NULL;
    if (NULL == meta) {
        //simply reserve the reference if able
        (void) obj_reserve(item, ref);
        retval = item;
    } else if(EXIT_SUCCESS != class_typecheck(meta)) {
        (void) abq_status_set(class_typecheck(meta), false);
    } else {
        class_ptr real_meta = class_realize(meta);
        if (NULL != real_meta->coerce) {
        	retval = real_meta->coerce(item, ref);
        } else {
            obj_t *obj = obj_hashmap_get(item);
        	if ((NULL != obj) && (obj->meta == real_meta)) {
        		//simply reserve the reference if able
        		(void) obj_reserve(item, ref);
        		retval = item;
        	} else {
        		retval = item_of_class(item, meta, ref);
        	}
        }
    }
    return retval;
}

static int8_t class_compare(cvar_t left, cvar_t right) {
    int8_t retval = 0;
    class_ptr meta1 = NULL;
    class_ptr meta2 = NULL;
    (void) bytes_copy(&meta1, &left, sizeof(cvar_t));
    (void) bytes_copy(&meta2, &right, sizeof(cvar_t));
    if(EXIT_SUCCESS == class_typecheck(meta1)) {
        if(EXIT_SUCCESS == class_typecheck(meta2)) {
            retval = utf8_compare_exact(meta1->class_name, meta2->class_name, -1);
        }else{
            retval = -1;
        }
    } else if(EXIT_SUCCESS == class_typecheck(meta2)) {
        retval = 1;
    } else {
        retval = 0;
    }
    return retval;
}

cvar_t list_of_class_create(class_ptr meta, cvar_t ref) {
    vlist_t* retval = NULL;
    if (NULL != meta) {
        // verify that we were passes a class
        VITAL_IS_OK(class_typecheck(meta));
        // verify that we were passes 'list_of_*)a class
        retval = vlist_create(meta->class_ref);
        if((NULL != ref) && (NULL != retval)) {
            (void) obj_takeover(retval, ref);
        }
    }
    return (cvar_t) retval;
}
void invalid_delete(cvar_t old_item) {
    VITAL_NOT_NULL(old_item);
    ABQ_FATAL_STATUS(EPERM);
}

bool_t class_is_primitive(class_ptr meta) {
    bool_t retval = false;
    if (NULL == meta) {
        ABQ_ERROR_MSG(abq_null_str);
    } else if(list_of_class_create == meta->create) {
        // list_of classes are not "primitive"
    } else if ((&null_class == meta) || (&bool_class == meta)
            || (&number_class == meta) || (&string_class == meta)
            || (&invalid_delete == meta->on_delete)) {
        // or (&byte_buffer_class == meta) ?
        retval = true;
    } else {
        // Not a "primitive"
    }
    return retval;
}

static void abq_log_item_info(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, cvar_t item) {
    byte_t buffer[B256_SIZE];
    ABQ_ENCODER(encoder, &ascii_codec, buffer, sizeof(buffer));
    (void) abq_encode_item_info(&encoder, item);
    abq_log_msg_y(level, tag, line, msg, buffer, (int32_t)encoder.pos);
}

void class_resolve_item(class_ptr meta,
        cvar_t *item_ptr, cvar_t variable) {
    VITAL_NOT_NULL(meta);
    VITAL_NOT_NULL(item_ptr);
    if (NULL == variable) {
        *item_ptr = NULL;
    } else {
        class_ptr item_meta
            = class_of(variable);
        if(item_meta != meta)  {
        	if (NULL != item_meta) {
				abq_log_item_info(ABQ_ERROR_LEVEL,
						__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0214. This string does not contain sensitive information."
						CLASS_NAME(meta), variable);
        	}
            *item_ptr = NULL;
        } else {
            *item_ptr = variable;
        }
    }
}

#if !defined(NDEBUG)
static void print_obj_info(cvar_t item, size_t depth) {
    byte_t buffer[B128_SIZE];
    ABQ_ENCODER(encoder, &ascii_codec, buffer, sizeof(buffer));
    (void) abq_encode_char(&encoder, '[');
    (void) abq_encode_int(&encoder, (int64_t)depth, DECIMAL_RADIX);
    (void) abq_encode_char(&encoder, ']');
    for(size_t index = 0U; index <= depth; index++) {
        (void) abq_encode_ascii(&encoder, "  ", 2); // parasoft-suppress CERT_C-MSC41-a-1 "c0215. This string does not contain sensitive information."
    }
    abq_log_item_info(ABQ_INFO_LEVEL,
            __FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0216. This string does not contain sensitive information."
            (cstr_t)buffer, item);
}

typedef ALIST_HEAD_DEF(cvar_t) refalist_t;
static cvar_t hard_refs_stack[B1024_SIZE];
static refalist_t hard_refs = ALIST_HEAD_INIT(cvar_t, hard_refs_stack);
static cvar_t self_refs_stack[B1024_SIZE];
static refalist_t self_refs = ALIST_HEAD_INIT(cvar_t, self_refs_stack);
static cvar_t looped_refs_stack[B1024_SIZE];
static refalist_t looped_refs = ALIST_HEAD_INIT(cvar_t, looped_refs_stack);

static bool_t refalist_contains(const refalist_t* alist, cvar_t ref) {
    bool_t retval = false;
    for(int32_t index=0; index<ALIST_SIZE(alist); index++) {
        if(ref == alist->array[index]) {
            retval = true;
            break;
        }
    }
    return retval;
}

static bool_t linked_set_add_if_missing(refalist_t *alist, const cvar_t ref){
    bool_t added_to_set = false;
    if(refalist_contains(alist, ref)) {
        // Match found, not missing
    }else {
        ALIST_PUSH(alist, cvar_t, ref);
        added_to_set = true;
    }
    return added_to_set;
}
typedef struct {obj_t *obj; uint16_t index;} refitem_t;
typedef ALIST_HEAD_DEF(refitem_t) refstack_t;

static bool_t refstack_contains(const refstack_t* stack, const obj_t *obj) {
    bool_t retval = false;
    for(int32_t index=0; index < ALIST_SIZE(stack); index++) {
        if(obj == stack->array[index].obj) {
            retval = true;
            break;
        }
    }
    return retval;
}

static bool_t find_obj_anchors(obj_t *obj, const mem_usage_t* mem_usage) {
    refitem_t frame = (refitem_t) {.obj=obj, .index=0U};
    bool_t found_anchor = false;
    refitem_t refstack[B256_SIZE];
    refstack_t children = ALIST_HEAD_INIT(refitem_t, refstack);
    if (NULL == frame.obj) {
        ABQ_ERROR_MSG(abq_null_str);
    } else {
        while(frame.index < frame.obj->refs_len) {
            ABQ_VITAL(NULL != frame.obj->refs);
            cvar_t ref = frame.obj->refs[frame.index];
            obj_t* parent = obj_lookup(ref);
            size_t index = 0U;
            frame.index+=1U;
            if(parent == (cvar_t) frame.obj){
                if(((linked_set_add_if_missing(&self_refs, ref)) || (!found_anchor))
                        && (NULL != mem_usage)) {
                    print_obj_info(ref, 0U);
                    for(index = 1U; index <= ALIST_SIZE(&children); index++){
                        print_obj_info(refstack[ALIST_SIZE(&children)-index].obj->member, index);
                    }
                }
                found_anchor = true;
            } else if(NULL == parent) {
                if(((linked_set_add_if_missing(&hard_refs, ref)) || (!found_anchor))
                    && (NULL != mem_usage)) {
                    print_obj_info(ref, 0U);
                    print_obj_info(frame.obj->member, 1U);
                    for(index = 1U; index <= ALIST_SIZE(&children); index++){
                        print_obj_info(refstack[ALIST_SIZE(&children)-index].obj->member, (1U+index));
                    }
                }
                found_anchor = true;
            } else if(refstack_contains(&children, parent)) {
                if(((linked_set_add_if_missing(&looped_refs, ref)) || (!found_anchor))
                        && (NULL != mem_usage)) {
                    print_obj_info(ref, 0U);
                    print_obj_info(frame.obj->member, 1U);
                    for(index = 1U; index <= ALIST_SIZE(&children); index++){
                        print_obj_info(refstack[ALIST_SIZE(&children)-index].obj->member, (1U+index));
                    }
                }
                found_anchor = true;
            } else {
                ALIST_PUSH(&children, refitem_t, frame);
                frame.obj = parent;
                frame.index = 0U;
            }
            if((frame.index >= frame.obj->refs_len) && (0U != ALIST_SIZE(&children))) {
                ALIST_POP(&children, refitem_t, frame);
            }
        }
        ABQ_VITAL(found_anchor);
    }
    return found_anchor;
}

static void fill_obj_anchors(void) {
    // Reset anchors
    ALIST_SIZE(&hard_refs) = 0U;
    ALIST_SIZE(&self_refs) = 0U;
    ALIST_SIZE(&looped_refs) = 0U;
    for(size_t bin=0U; bin < ARRAY_COUNT(obj_t*, obj_hash_map); bin++) {
        LLIST_FOREACH(obj_hash_map[bin], obj_t, obj, next) {
            if(NULL != obj->refs) {
                // Walk the tree looking for parent references
                ABQ_VITAL(find_obj_anchors(obj, NULL));
            }
        }
    }
}

#define DEFAULT_OBJ_PRINT_DEPTH (3)
static bool_t obj_dump_children(cvar_t anchor, const mem_usage_t* mem_usage){
	bool_t has_children = false;
    uint16_t ref_idx = 0;
    cvar_t match = NULL;
    cvar_t ref = anchor;

    cvar_t refstack[B256_SIZE];
    refalist_t ancestors = ALIST_HEAD_INIT(cvar_t, refstack);

    size_t bin = 0U; // obj_address_hash(item)
    obj_t *obj = obj_lookup(anchor);

    if (NULL == mem_usage) {
        print_obj_info(anchor, 0U);
    }
    obj_t *child = LLIST_FIRST(obj_hash_map[bin]);
    while(bin < ARRAY_COUNT(obj_t*, obj_hash_map)) {
        while(NULL != child){
            if((NULL != child->member) && (child != obj)) {
                ABQ_SEEK_LAST(child->refs, cvar_t,child->refs_len, ref_idx, match, (match == ref));
                if(ref_idx < child->refs_len) {
                	has_children = true;
                    if(refalist_contains(&ancestors, child->member)) {
                        // looped reference, continue
                    } else {
                        // Found a child, pop it onto the stack
                        ALIST_PUSH(&ancestors, cvar_t, ref);
                        obj = child;
                        ref = obj->member;
                        VITAL_VALUE(obj_address_hash(ref), bin);
                        bin = 0U; // Restart fresh with new stack frame

                        if (NULL != mem_usage) {
                            // If debugging a specific mem_usage_t, match instances of that mem_usage_t
                            if (lookup_mem_usage(ref) == mem_usage) {
                                // print the current stack when matches are found
                                for (size_t depth=0U; depth < ALIST_SIZE(&ancestors); depth++) {
                                    print_obj_info(ancestors.array[depth], depth);
                                }
                                print_obj_info(ref, ALIST_SIZE(&ancestors));
                            }
                        } else if(ALIST_SIZE(&ancestors) < DEFAULT_OBJ_PRINT_DEPTH) {
                            print_obj_info(ref, ALIST_SIZE(&ancestors));
                        } else {
                            // don't print excessive information
                        }
                    }
                }
                if(obj == child) {
                    // If we have started a new frame, restart loop
                    child = LLIST_FIRST(obj_hash_map[bin]);
                }else{
                    child = LLIST_NEXT(child, next);
                }
            }else{
                child = LLIST_NEXT(child, next);
            }
        }
        bin++;
        if (bin >= ARRAY_COUNT(obj_t*, obj_hash_map)) {
            if(0U != ALIST_SIZE(&ancestors)) {
                // Completed this node but still have ancestors, restore state and iterate to next
                ABQ_VITAL(obj->member == ref);
                child = LLIST_NEXT(obj, next);
                bin = obj_address_hash(ref);
                ALIST_POP(&ancestors, cvar_t, ref);
                obj = obj_lookup(ref);
                if (NULL == obj) {
                    VITAL_VALUE(0U, ancestors.item_count);
                } else {
                    ABQ_VITAL(obj->member == ref);
                }
            }else{
                break; // All done
            }
        } else {
            child = LLIST_FIRST(obj_hash_map[bin]);
        }
    }
    return has_children;
}
static obj_t* obj_resolve(cvar_t item){
    obj_t *retval = NULL;
    for(size_t bin= 0UL; bin < ARRAY_COUNT(obj_t*, obj_hash_map); bin++) {
        retval = LLIST_FIRST(obj_hash_map[bin]);
        while(NULL != retval){
            if((item == (cvar_t)retval) || (item == (cvar_t)retval->member)){
                break;
            }
            retval = LLIST_NEXT(retval, next);
        }
        if(NULL != retval) {
            break;
        }
    }
    return retval;
}

bool_t ref_dump_children(cvar_t ref) {
	return obj_dump_children(ref, NULL);
}

bool_t objs_dump_refs(const mem_usage_t* mem_usage ) {
    bool_t all_clear = true;
    if((NULL != mem_usage) && (0 <= utf8_index_of(mem_usage->name, -1, "_mu", -1))) { // parasoft-suppress CERT_C-MSC41-a-1 "c0217. This string does not contain sensitive information."
        // Expect it was defined within DEFINE_CLASS
        ABQ_VITAL(mem_usage->usage <= mem_usage->total);
        for(byte_t *chunk = mem_usage_iter(mem_usage, NULL);
                NULL != chunk;
                chunk = mem_usage_iter(mem_usage, chunk)) {
            obj_t *obj = obj_resolve(chunk);
            ABQ_VITAL(find_obj_anchors(obj, mem_usage));
            all_clear = false;
            debugpoint();
        }
    } else {
        cvar_t ref = NULL;
        fill_obj_anchors();
        ABQ_INFO_MSG("hard_refs:");
        ALIST_LOOP(&hard_refs, cvar_t, ref){
            all_clear = false;
            debugpoint();
            (void) obj_dump_children(ref, mem_usage);
        }
        ABQ_INFO_MSG("self_refs:");
        ALIST_LOOP(&self_refs, cvar_t, ref){
            all_clear = false;
            debugpoint();
            (void) obj_dump_children(ref, mem_usage);
        }
        ABQ_INFO_MSG("looped_refs:");
        ALIST_LOOP(&looped_refs, cvar_t, ref){
            all_clear = false;
            debugpoint();
            (void) obj_dump_children(ref, mem_usage);
        }
    }
    return all_clear;
}

static void abq_release_child(cvar_t ref) {
    LLIST_HEAD_DEF(obj_t, objs_in_bin) = NULL;
    obj_t* obj = NULL;
    cvar_t  match = NULL;
    uint16_t ref_idx = (uint16_t) UINT16_MAX;
    for(size_t bin= 0UL; bin < ARRAY_COUNT(obj_t*, obj_hash_map); bin++) {
        objs_in_bin = obj_hash_map[bin];
        LLIST_LOOP(objs_in_bin, obj, next) {
            // Seek out the hard-ref
            ABQ_SEEK_LAST(obj->refs, cvar_t,obj->refs_len, ref_idx, match, (match == ref));
            if (ref_idx < obj->refs_len) {
                // found it, remove the reference
                ABQ_REMOVE_AT(obj->refs, cvar_t, obj->refs_len, obj->refs_max, ref_idx);
                if(0U != obj->refs_len) {
                    // Still has more refs
                } else {
                    // and delete the obj in question
                    VITAL_IS_OK(obj_delete(obj));
                    // objs_in_bin has been modified, break out of current cycle
                    break;
                }
            }
        }
    }
}
static class_ptr locked_class_stack[B256_SIZE];
ALIST_HEAD_TYPE(class_ptr) locked_classes
        = ALIST_HEAD_INIT(class_ptr, locked_class_stack);

#endif /* !defined(NDEBUG) */

void check_objs_and_mem_is_free(void) {
#if !defined(NDEBUG)
    bool_t all_clear = objs_dump_refs( NULL );
    class_ptr locked_class = NULL;
    ALIST_LOOP (&locked_classes, class_ptr, locked_class) {
        ABQ_ERROR_MSG(locked_class->class_name);
        all_clear = false;
        debugpoint();
    }
#else /* NDEBUG */
    bool_t all_clear = true;
    for(size_t bin=0U; bin < CLASS_HASH_CAPACITY; bin++) {
        LLIST_FOREACH(obj_hash_map[bin], obj_t, obj, next) {
            if(0U < obj->ref_count) {
                all_clear = false; // break here ?
                ABQ_ERROR_MSG_X(obj->meta->class_name, obj->ref_count);
                if (NULL != obj->meta->on_delete) {
                    (void) obj->meta->on_delete(obj->member);
                }
            }
        }
    }
#endif /* NDEBUG */
    // Check unclassified memory as well
    abq_mem_check_all_chunks_freed();
    if (!abq_status_is_ok()) {
        // error status is set
    	ABQ_DUMP_ERROR(abq_status_pop(), "Uncleared status");
        // all_clear = false;
    }

#if !defined(NDEBUG)
    // Print out & Reset memory usage metrics
    int32_t total_mem_registered = 0;
    int32_t current_mem_used = 0;
    int32_t peak_mem_used = 0;
    mem_usage_stats(&total_mem_registered, &current_mem_used, &peak_mem_used, false);
    byte_t summery[B256_SIZE] = {0};
    ABQ_ENCODER(encoder, &ascii_codec, summery, sizeof(summery));
    (void) abq_encode_number(&encoder, (number_t) peak_mem_used);
    (void) abq_encode_char(&encoder, '/');
    (void) abq_encode_number(&encoder, (number_t) total_mem_registered);
    (void) abq_encode_ascii(&encoder, ", => ", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0218. This string does not contain sensitive information."
    (void) abq_encode_number(&encoder, ((100.0 * (number_t) peak_mem_used) / (number_t) total_mem_registered));
    (void) abq_encode_char(&encoder, '%');
    ABQ_DEBUG_MSG_Y("peak usage: ", summery, encoder.pos);
#endif /* !defined(NDEBUG) */

    // Abort if memory resources where still in use
    ABQ_VITAL(all_clear);
}

void abq_free_all_objs_and_mem(void) {
#if !defined(NDEBUG)
    cvar_t ref = NULL;
    do {
        fill_obj_anchors();
        if(0U != ALIST_SIZE(&hard_refs)) {
            ALIST_LOOP(&hard_refs, cvar_t, ref) {
                // "Simply" loop over all obj_t and release the hard refs
                abq_release_child(ref);
            }
        } else if(0U != ALIST_SIZE(&self_refs)){
            ALIST_LOOP(&self_refs, cvar_t, ref){
                // Release the self reference
                (void) obj_release_self(ref);
            }
        } else {
            ALIST_LOOP(&looped_refs, cvar_t, ref){
                // treat looped refs like hard refs and hope they release each other
                abq_release_child(ref);
            }
        }
    }while((0U != ALIST_SIZE(&hard_refs))
            || (0U != ALIST_SIZE(&self_refs))
            || (0U != ALIST_SIZE(&looped_refs)));
    class_ptr locked_class = NULL;
    while (0U != ALIST_SIZE(&locked_classes)) {
        ALIST_POP(&locked_classes, class_ptr, locked_class);
        VITAL_NOT_NULL(locked_class);
    }
#else /* NDEBUG */
    // Release all resources after a fatal for a "clean" start
    for (size_t bin=0UL; bin < CLASS_HASH_CAPACITY; bin++) {
        obj_t* obj = LLIST_FIRST(obj_hash_map[bin]);
        while (NULL != obj) {
            if (&class_class != obj->meta) {
            // TODO find & delete each anchor first
                obj->ref_count = 0U;
                VITAL_IS_OK(obj_delete(obj));
                // Restart bin after each item is deleted
                obj = LLIST_FIRST(obj_hash_map[bin]);
            } else {
                obj = LLIST_NEXT(obj, next);
            }
        }
    }
#endif /* !defined(NDEBUG) */
    abq_mutex_t* mutex = ontrac_mutex_get( );
    // releases active locks as able
    (void) ontrac_reg_global_mutex(mutex->mutex,
            mutex->lock, mutex->unlock);
    // Free up unclassified memory
    abq_mem_usage_free_all();
    // Reset peak usage counts
    mem_usage_stats_ex(NULL, NULL, NULL, false, true);
    // Clear "is_aborting" status after everything has been reset
    abq_status_set(EXIT_SUCCESS, true);
}
