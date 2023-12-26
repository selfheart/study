//#line 2 "ontrac/mem_usage.c"
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
 * @file ontrac/mem_usage.c
 * @date Feb 27, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/ontrac/mem_usage.h>
#include <ontrac/unicode/utf8_utils.h>
#include <ontrac/ontrac/abq_class.h>

size_t total_registered_memory = 0U;
// shared memory is memory anyone can access via abq_malloc and abq_free
static LLIST_HEAD(mem_usage_t, shared_mem_usage_sorted_bysize);
// private memory must be allocated with malloc from and should be freed using abq_free_into
static LLIST_HEAD(mem_usage_t, private_mem_usage_sorted_bysize);
static LLIST_HEAD(mem_usage_t, empty_mem_usages);

static byte_t abq_ctx_buffer[ABQ_BUFFER_SIZE]; /* parasoft-suppress MISRAC2012-RULE_1_1-a-2 MISRAC2012-RULE_1_1-b-2 "Dedicated context buffer, greater than 32767 / 65535 bytes in a single object" */
abq_ctx_t abq_ctx = {
    .buffer = &abq_ctx_buffer[0],
    .buf_size = (size_t) sizeof(abq_ctx_buffer)
};

static bool_t mem_usage_contains(const mem_usage_t *mem_usage, cvoidc_ptr ptr) {
    bool_t retval = false;
    if ((NULL != mem_usage) && (NULL != ptr)) {
        retval = abq_memory_match(mem_usage->partition,
                (uintptr_t) mem_usage->range, ptr);
    }
    return retval;
}

uint16_t mem_usage_index_of(const mem_usage_t *mem_usage, cvoidc_ptr ptr) {
    uint16_t retval = (uint16_t) UINT16_MAX;
    if ((NULL != mem_usage) && (NULL != ptr)) {
        uintptr_t tempvalue = ptr2addr(ptr);
        uintptr_t range_start = ptr2addr(mem_usage->partition);
        if (range_start <= tempvalue) {
            tempvalue -= range_start;
            if (tempvalue < (uintptr_t)mem_usage->range) {
                tempvalue /= (uintptr_t) mem_usage->increment;
                retval = (uint16_t)tempvalue;
            }
        }
    }
    return retval;
}

static byte_t* mem_usage_get(const mem_usage_t *mem_usage, uint16_t chunk_index) {
    ABQ_VITAL(chunk_index < mem_usage->total);
    ABQ_VITAL(uint64_bitmap_is_on(mem_usage->bitmap, (size_t)chunk_index));
    // Convert from chunk index to byte index within the partition
    size_t byte_index = ABQ_ALIGN_SIZE((size_t)chunk_index * (size_t)mem_usage->increment, abq_word_t);
    ABQ_VITAL((byte_index + (size_t)mem_usage->increment) <= (size_t) mem_usage->range);
    return &mem_usage->partition[byte_index];
}

cvar_t mem_usage_resolve(const mem_usage_t *mem_usage, cvar_t item) {
    cvar_t retval = NULL;
    if ((NULL != mem_usage) && (NULL != item)) {
        uint16_t index = mem_usage_index_of(mem_usage, item);
        if (index >= mem_usage->total) {
            ABQ_ERROR_MSG(mem_usage->name);
        } else if(mem_usage_is_free(mem_usage, index)) {
            // Item has been free'd, return NULL
        } else {
            // Item checks out, return pointer to start of data-chuck
            retval = (cvar_t) mem_usage_get(mem_usage, index);
        }                                                       \
    }
    return retval;
}

byte_t* mem_usage_iter(const mem_usage_t *mem_usage, const byte_t *prev) {
    byte_t* retval = NULL;
    size_t index = 0U;
    if (NULL != prev) {
        index = (size_t) mem_usage_index_of(mem_usage, prev);
        index++;
    }
    if (index >= (size_t)mem_usage->total) {
        // Invalid parameter
    } else {
        // find the next allocated index
        index = uint64_bitmap_next_on(mem_usage->bitmap,
                index, (size_t) mem_usage->total);
        if ((index < (size_t)mem_usage->total) && (index < (size_t)UINT16_MAX)) {
            retval = mem_usage_get(mem_usage, (uint16_t)index);
        }
    }
    return retval;
}

static void mem_usage_warning (int32_t line, const mem_usage_t *mem_usage, cstr_t msg) {
    byte_t buffer[B512_SIZE];
    ABQ_ENCODER(encoder, &ascii_codec, buffer, sizeof(buffer));
    VITAL_NOT_NULL(mem_usage);
    (void) abq_encode_ascii(&encoder, mem_usage->name, -1);
    (void) abq_encode_ascii(&encoder,  ": ", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0219. This string does not contain sensitive information."
    (void) abq_encode_ascii(&encoder,  msg, -1);
    abq_log_msg(ABQ_WARN_LEVEL, __FILENAME__, line, buffer); // parasoft-suppress CERT_C-MSC41-a-1 "c0220. This string does not contain sensitive information."
}

void register_mem_usage(mem_usage_t* mem_usage, bool_t shared) {
    // vital that the usage is not already configured
    VITAL_IS_NULL(mem_usage->mutex);
    mem_usage->mutex = ontrac_mutex_get();

    // Mem-usage consists of three components, the mem_usage_t struct, the backing array of data, and a bitmap used to track allocated chunks
    total_registered_memory += sizeof(mem_usage_t);
    total_registered_memory += mem_usage->range;
    total_registered_memory += (sizeof(uint64_t) * UINT64_BITMAP_LENGTH(mem_usage->total));
#if !defined(NDEBUG)
    if (0U != mem_usage->total) {
        VITAL_NOT_NULL(mem_usage->bitmap);
        FATAL_IF(mem_usage_contains(mem_usage, mem_usage));
        for (size_t i=0UL; i < (size_t)mem_usage->total; i++) {
            FATAL_IF(uint64_bitmap_is_on(mem_usage->bitmap, i));
        }
        // verify that blocks to not cross boundaries
        LLIST_FOREACH(shared_mem_usage_sorted_bysize, mem_usage_t, other, next) {
            FATAL_IF(mem_usage_contains(other,  mem_usage));
            FATAL_IF(mem_usage_contains(other, &mem_usage->partition[0]));
            FATAL_IF(mem_usage_contains(other, &mem_usage->bitmap[0]));
        }
    }
#endif /* DEBUG build */

    // Link mem_usage into the global linked list
    if (0U == mem_usage->total) {
        LLIST_PUSH(empty_mem_usages, mem_usage, next);
    } else if (shared) {
        if (NULL == shared_mem_usage_sorted_bysize) {
                LLIST_PUSH(shared_mem_usage_sorted_bysize, mem_usage, next);
            } else if (mem_usage->increment < shared_mem_usage_sorted_bysize->increment) {
                LLIST_PUSH(shared_mem_usage_sorted_bysize, mem_usage, next);
            } else {
                // continue to iterate until chunk size is larger then next in list
                mem_usage_t *preceeding = shared_mem_usage_sorted_bysize;
                while ((NULL != preceeding->next)
                        && (mem_usage->increment > preceeding->next->increment)) {
                    preceeding = preceeding->next;
                }
                // and then insert chunk into linked-list at the given location
                LLIST_PUSH(preceeding->next, mem_usage, next);
            }
    } else if (NULL == private_mem_usage_sorted_bysize) {
        LLIST_PUSH(private_mem_usage_sorted_bysize, mem_usage, next);
    } else if (mem_usage->increment
            < private_mem_usage_sorted_bysize->increment) {
        LLIST_PUSH(private_mem_usage_sorted_bysize, mem_usage, next);
    } else {
        // continue to iterate until chunk size is larger then next in list
        mem_usage_t *preceeding = private_mem_usage_sorted_bysize;
        while ((NULL != preceeding->next)
                && (mem_usage->increment > preceeding->next->increment)) {
            preceeding = preceeding->next;
        }
        // and then insert chunk into linked-list at the given location
        LLIST_PUSH(preceeding->next, mem_usage, next);
    }
}

void mem_usage_lock_mutex(mem_usage_t *mem_usage) {
    if (NULL == mem_usage->mutex) {
        register_mem_usage(mem_usage, false);
    }
    (void) mem_usage->mutex->lock(mem_usage->mutex->mutex);
}

void mem_usage_unlock_mutex(mem_usage_t *mem_usage) {
    if (NULL == mem_usage->mutex) {
        register_mem_usage(mem_usage, false);
    }
    (void) mem_usage->mutex->unlock(mem_usage->mutex->mutex);
}

static abq_mutex_t* nio_mutex = NULL;
void abq_context_lock( void ){
    if(NULL == nio_mutex) {
        nio_mutex = ontrac_mutex_get();
    }
    EXPECT_IS_OK(nio_mutex->lock(nio_mutex->mutex));
}

void abq_context_unlock( void ){
    if (NULL != nio_mutex) {
        VITAL_IS_OK(nio_mutex->unlock(nio_mutex->mutex));
    }
}

#ifdef ENABLE_DEFAULT_MEMORY_USAGE
static bool_t defaults_registered = false;
// Pre-allocated byte-arrays "chunks"
// only called if the ENABLE_DEFAULT_MEMORY_USAGE macro is set

#ifdef MAX_LINK_VAR_COUNT
// Either 8 or 16 bytes depending on 32 or 64 bit architecture
DEFINE_STATIC_MEM_USAGE(links_usage, sizeof(link_t), MAX_LINK_VAR_COUNT);
#endif /* MAX_LINK_VAR_COUNT */
#ifdef DEFAULT_B16_COUNT
DEFINE_STATIC_MEM_USAGE(default_b16,    B16_SIZE,    DEFAULT_B16_COUNT);
#endif /* DEFAULT_B16_COUNT */
#ifdef DEFAULT_B32_COUNT
DEFINE_STATIC_MEM_USAGE(default_b32,    B32_SIZE,    DEFAULT_B32_COUNT);
#endif /* DEFAULT_B32_COUNT */
#ifdef DEFAULT_B48_COUNT
DEFINE_STATIC_MEM_USAGE(default_b48,    B48_SIZE,    DEFAULT_B48_COUNT);
#endif /* DEFAULT_B48_COUNT */
#ifdef DEFAULT_B64_COUNT
DEFINE_STATIC_MEM_USAGE(default_b64,    B64_SIZE,    DEFAULT_B64_COUNT);
#endif /* DEFAULT_B64_COUNT */
#ifdef DEFAULT_B80_COUNT
DEFINE_STATIC_MEM_USAGE(default_b80,    B80_SIZE,    DEFAULT_B80_COUNT);
#endif /* DEFAULT_B80_COUNT */
#ifdef DEFAULT_B96_COUNT
DEFINE_STATIC_MEM_USAGE(default_b96,    B96_SIZE,    DEFAULT_B96_COUNT);
#endif /* DEFAULT_B96_COUNT */
#ifdef DEFAULT_B112_COUNT
DEFINE_STATIC_MEM_USAGE(default_b112,   B112_SIZE,   DEFAULT_B112_COUNT);
#endif /* DEFAULT_B112_COUNT */
#ifdef DEFAULT_B128_COUNT
DEFINE_STATIC_MEM_USAGE(default_b128,   B128_SIZE,   DEFAULT_B128_COUNT);
#endif /* DEFAULT_B128_COUNT */
#ifdef DEFAULT_B192_COUNT
DEFINE_STATIC_MEM_USAGE(default_b192,   B192_SIZE,   DEFAULT_B192_COUNT);
#endif /* DEFAULT_B192_COUNT */
#ifdef DEFAULT_B256_COUNT
DEFINE_STATIC_MEM_USAGE(default_b256,   B256_SIZE,   DEFAULT_B256_COUNT);
#endif /* DEFAULT_B256_COUNT */
#ifdef DEFAULT_B512_COUNT
DEFINE_STATIC_MEM_USAGE(default_b512,   B512_SIZE,   DEFAULT_B512_COUNT);
#endif /* DEFAULT_B512_COUNT */
#ifdef DEFAULT_B1024_COUNT
DEFINE_STATIC_MEM_USAGE(default_b1024,  B1024_SIZE,  DEFAULT_B1024_COUNT);
#endif /* DEFAULT_B1024_COUNT */
#ifdef DEFAULT_B2048_COUNT
DEFINE_STATIC_MEM_USAGE(default_b2048,  B2048_SIZE,  DEFAULT_B2048_COUNT);
#endif /* DEFAULT_B2048_COUNT */
#ifdef DEFAULT_B4096_COUNT
DEFINE_STATIC_MEM_USAGE(default_b4096,  B4096_SIZE,  DEFAULT_B4096_COUNT);
#endif /* DEFAULT_B4096_COUNT */
#ifdef DEFAULT_B8192_COUNT
DEFINE_STATIC_MEM_USAGE(default_b8192,  B8192_SIZE,  DEFAULT_B8192_COUNT);
#endif /* DEFAULT_B8192_COUNT */
#ifdef DEFAULT_B16384_COUNT
DEFINE_STATIC_MEM_USAGE(default_b16384, B16384_SIZE, DEFAULT_B16384_COUNT);
#endif /* DEFAULT_B16384_COUNT */
#ifdef DEFAULT_B32768_COUNT
DEFINE_STATIC_MEM_USAGE(default_b32768, B32768_SIZE, DEFAULT_B32768_COUNT);
#endif /* DEFAULT_B32768_COUNT */
#ifdef DEFAULT_B65536_COUNT
DEFINE_STATIC_MEM_USAGE(default_b65536, B65536_SIZE, DEFAULT_B65536_COUNT);
#endif /* DEFAULT_B65536_COUNT */

static void register_default_mem_usages( void ) {
    abq_context_lock();
    if(0 == defaults_registered) {
        // Prior to registering memory
        //  Do some quick system mem_align sanity checks
        ABQ_VITAL(sizeof(abq_ctx_buffer) >= B1024_SIZE);
        // Initialize all bytes within test range
        bytes_set(abq_ctx_buffer, '\0', B1024_SIZE);
        // First target it WORD aligned with source buffer
        byte_t* word_aligned_target = &abq_ctx_buffer[B128_SIZE];
        // Second target is UINT16 aligned with source buffer,
        //  bit is NOT WORD aligned with source buffer
        byte_t* uint16_aligned_target = &abq_ctx_buffer[B256_SIZE+2U];
        // Third target is un-alignable with source buffer
        byte_t* unaligned_target = &abq_ctx_buffer[B512_SIZE+1U];
        byte_t datum = '0';
        for(size_t pos=1U; pos <= (2U*sizeof(abq_word_t)); pos++) {
            datum++;
            // Test bytes set for each unaligned byte position
            bytes_set(&abq_ctx_buffer[pos], (cbyte_t)datum, (B64_SIZE-pos));
            size_t addr1 = (size_t) ptr2addr(&abq_ctx_buffer[pos]);
            size_t align1 = ABQ_ALIGN_SIZE(addr1, abq_word_t);
            ABQ_VITAL(addr1 <=align1);
            ABQ_VITAL((align1 - addr1) < sizeof(abq_word_t));
            ABQ_VITAL(0UL == (align1 % sizeof(abq_word_t)));
            size_t addr2 = (size_t) ptr2addr(&word_aligned_target[pos]);
            size_t align2 = ABQ_ALIGN_SIZE(addr2, abq_word_t);
            ABQ_VITAL(addr2 <= align2);
            ABQ_VITAL((align2 - addr2) < sizeof(abq_word_t));
            VITAL_VALUE((align1 - addr1) , (align2 - addr2));
            ABQ_VITAL(0UL == (align2 % sizeof(abq_word_t)));
            // Test bytes copy for each unaligned byte position
            bytes_copy(&word_aligned_target[pos], &abq_ctx_buffer[pos], (B64_SIZE-pos));
            bytes_copy(&uint16_aligned_target[pos], &abq_ctx_buffer[pos], (B64_SIZE-pos));
            bytes_copy(&unaligned_target[pos], &abq_ctx_buffer[pos], (B64_SIZE-pos));
            // Sinity checks against the WORD aligned buffer
            align1 = ABQ_ALIGN_LAST_OFFSET(&abq_ctx_buffer[pos], abq_word_t);
            ABQ_VITAL(align1 < (abq_word_t) sizeof(abq_word_t));
            align2 = ABQ_ALIGN_NEXT_OFFSET(&word_aligned_target[pos], abq_word_t);
            ABQ_VITAL(align2 < (abq_word_t) sizeof(abq_word_t));
            ABQ_VITAL(0U == ((align1 + align2) % (abq_word_t)sizeof(abq_word_t)));
        }
        // Check data was set & copied as expected
        for(size_t idx = 0U; idx <= B64_SIZE; idx++){
            if ((0U == idx) || (B64_SIZE == idx)) {
                // If boundary position, check that byte is still set to initial value
                VITAL_VALUE( '\0', abq_ctx_buffer[idx]);
                VITAL_VALUE( '\0', word_aligned_target[idx]);
                VITAL_VALUE( '\0', uint16_aligned_target[idx]);
                VITAL_VALUE( '\0', unaligned_target[idx]);
            }else if(idx <= (2U*sizeof(abq_word_t))) {
                VITAL_VALUE( '0'+(byte_t)idx, abq_ctx_buffer[idx]);
                VITAL_VALUE( '0'+(byte_t)idx, word_aligned_target[idx]);
                VITAL_VALUE( '0'+(byte_t)idx, uint16_aligned_target[idx]);
                VITAL_VALUE( '0'+(byte_t)idx, unaligned_target[idx]);
            } else {
                // Check that value matches final datum written
                VITAL_VALUE( datum, abq_ctx_buffer[idx]);
                VITAL_VALUE( datum, word_aligned_target[idx]);
                VITAL_VALUE( datum, uint16_aligned_target[idx]);
                VITAL_VALUE( datum, unaligned_target[idx]);
            }
        }
#ifdef MAX_LINK_VAR_COUNT
        register_mem_usage(&links_usage, true);
#endif /* MAX_LINK_VAR_COUNT */
#ifdef DEFAULT_B16_COUNT
        register_mem_usage(&default_b16, true);
#endif /* DEFAULT_B16_COUNT */
#ifdef DEFAULT_B32_COUNT
        register_mem_usage(&default_b32, true);
#endif /* DEFAULT_B32_COUNT */
#ifdef DEFAULT_B48_COUNT
        register_mem_usage(&default_b48, true);
#endif /* DEFAULT_B48_COUNT */
#ifdef DEFAULT_B64_COUNT
        register_mem_usage(&default_b64, true);
#endif /* DEFAULT_B64_COUNT */
#ifdef DEFAULT_B80_COUNT
        register_mem_usage(&default_b80, true);
#endif /* DEFAULT_B80_COUNT */
#ifdef DEFAULT_B96_COUNT
        register_mem_usage(&default_b96, true);
#endif /* DEFAULT_B96_COUNT */
#ifdef DEFAULT_B112_COUNT
        register_mem_usage(&default_b112, true);
#endif /* DEFAULT_B112_COUNT */
#ifdef DEFAULT_B128_COUNT
        register_mem_usage(&default_b128, true);
#endif /* DEFAULT_B128_COUNT */
#ifdef DEFAULT_B192_COUNT
        register_mem_usage(&default_b192, true);
#endif /* DEFAULT_B192_COUNT */
#ifdef DEFAULT_B256_COUNT
        register_mem_usage(&default_b256, true);
#endif /* DEFAULT_B256_COUNT */
#ifdef DEFAULT_B512_COUNT
        register_mem_usage(&default_b512, true);
#endif /* DEFAULT_B512_COUNT */
#ifdef DEFAULT_B1024_COUNT
        register_mem_usage(&default_b1024, true);
#endif /* DEFAULT_B1024_COUNT */
#ifdef DEFAULT_B2048_COUNT
        register_mem_usage(&default_b2048, true);
#endif /* DEFAULT_B2048_COUNT */
#ifdef DEFAULT_B4096_COUNT
        register_mem_usage(&default_b4096, true);
#endif /* DEFAULT_B4096_COUNT */
#ifdef DEFAULT_B8192_COUNT
        register_mem_usage(&default_b8192, true);
#endif /* DEFAULT_B8192_COUNT */
#ifdef DEFAULT_B16384_COUNT
        register_mem_usage(&default_b16384, true);
#endif /* DEFAULT_B16384_COUNT */
#ifdef DEFAULT_B32768_COUNT
        register_mem_usage(&default_b32768, true);
#endif /* DEFAULT_B32768_COUNT */
#ifdef DEFAULT_B65536_COUNT
        register_mem_usage(&default_b65536, true);
#endif /* DEFAULT_B65536_COUNT */
        defaults_registered = 1;
    }
    abq_context_unlock();
}
#endif

// attempts to match ptr to a used chunk pointer stored in a mem_usage_t
mem_usage_t *lookup_mem_usage(cvoidc_ptr ptr) {
    mem_usage_t *rvalue = NULL;
    LLIST_FOREACH(shared_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
        if (mem_usage_contains(mem_usage, ptr)){
            rvalue = mem_usage;
            break;
        }
    }
    if(NULL == rvalue) {
        LLIST_FOREACH(private_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
            if (mem_usage_contains(mem_usage, ptr)){
                rvalue = mem_usage;
                break;
            }
        }
    }
    return rvalue;
}

static byte_t* mem_usage_pop(mem_usage_t *mem_usage) {
    byte_t* result=NULL;
    VITAL_NOT_NULL(mem_usage);
    EXPECT_IS_OK(mem_usage->mutex->lock(mem_usage->mutex->mutex));
    if (mem_usage->usage < mem_usage->total) {
        size_t index = uint64_bitmap_next_off(mem_usage->bitmap,
                0UL, (size_t) mem_usage->total);
        ABQ_VITAL(index < (size_t) mem_usage->total);
        FATAL_IF(uint64_bitmap_is_on(mem_usage->bitmap, index));
        uint64_bitmap_set_on(mem_usage->bitmap, index);
        result = mem_usage_get(mem_usage, (uint16_t)index);
        // Increment counter used for tracking max usage
        mem_usage->usage += 1U;
        if(mem_usage->usage > mem_usage->peak) {
            mem_usage->peak = mem_usage->usage;
        }
    } else if (mem_usage->peak <= mem_usage->total) { // only log and out-of-usage once per usage
        mem_usage->peak += 1U;
        ABQ_WARN_MSG_X(mem_usage->name, mem_usage->peak);
        mem_usage_warning(__LINE__, mem_usage, "All-Blocks-In-Use!"); // parasoft-suppress CERT_C-MSC41-a-1 "c0221. This string does not contain sensitive information."
#if !defined(NDEBUG)
        if (NULL == mem_usage->next) {
            // If this is the largest mem_usage_t dump information
            (void) objs_dump_refs( mem_usage );
        }
#endif /* !defined(NDEBUG) */
    } else {
        // Already reported the error
    }
    EXPECT_IS_OK(mem_usage->mutex->unlock(mem_usage->mutex->mutex));
    return result;
}

static void mem_usage_push(mem_usage_t *mem_usage, cvoidc_ptr ptr) {
    EXPECT_IS_OK(mem_usage->mutex->lock(mem_usage->mutex->mutex));
    ABQ_VITAL(0U != mem_usage->usage);
    uint16_t index = mem_usage_index_of(mem_usage, ptr);
    ABQ_VITAL(index < mem_usage->total);
    if (false == uint64_bitmap_is_on(mem_usage->bitmap, index)) {
        ABQ_ERROR_MSG_Z("Twice-Free'd pointer: ", mem_usage->name);
    } else {
#if !defined(NDEBUG)
        byte_t *chunk = mem_usage_get(mem_usage, index);
        if (ptr != (cvoidc_ptr) chunk) {
            ABQ_ERROR_MSG_Z("Mis-aligned pointer: ", mem_usage->name);
            // Free'ing anyway (Alternative would be panic?)
        }
        // "Zero" out memory when it is free'd (DEBUG only)
        bytes_set(chunk, (byte_t) 0xff, mem_usage->increment);
#endif /* !defined(NDEBUG) */
        uint64_bitmap_set_off(mem_usage->bitmap, index);
        mem_usage->usage -= 1U;
    }
    EXPECT_IS_OK(mem_usage->mutex->unlock(mem_usage->mutex->mutex));
}

byte_t* abq_malloc_ex(size_t size, mem_usage_t **usage) {
#ifdef ENABLE_DEFAULT_MEMORY_USAGE
    if (false == defaults_registered) {
        register_default_mem_usages();
    }
#endif
    byte_t* result=NULL;
    LLIST_FOREACH(shared_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
        if (size <= mem_usage->increment) {
            result = mem_usage_pop(mem_usage);
            if (NULL != result) {
                if (NULL != usage) {
                    *usage = mem_usage;
                }
                break;
            }
        }
    }
    // If result is still NULL, might want to consider a call to the system implementation of malloc in a production build
    return result;
}

byte_t* malloc_from(mem_usage_t *mem_usage) {
    if (NULL == mem_usage->mutex) {
        // must register memory so that 'abq_free' can find the mem_usage_t later
        // more importantly, we must construct the linked-lists
        register_mem_usage(mem_usage, false);
    }
    return mem_usage_pop(mem_usage);
}

byte_t* abq_calloc_ex(size_t nitems, size_t size,  mem_usage_t **usage) {
    byte_t* retval = abq_malloc_ex(nitems * size, usage);
    // NULL check is done within the 'bytes_set' function
    (void) bytes_set(retval, '\0', nitems * size);
    return retval;
}

void abq_free_into(cvoidc_ptr ptr, mem_usage_t * mem_usage) {
    // the first byte commonly contains a typecheck or class_ptr
    //  zero it out so that future checks fail
    if(NULL == ptr) {
        // log error ?
    } else {
        mem_usage_t * usage = NULL;
        if (NULL == mem_usage) {
            usage = lookup_mem_usage(ptr);
        } else if(mem_usage_contains(mem_usage, ptr)) {
            // Expected, no need to do additional work
            usage = mem_usage;
        } else {
            // If mem_usage was tapped out, data may have been initialized from shared mem_usage
            usage = lookup_mem_usage(ptr);
        }
        if (NULL == usage) {
            // Attempt to free unmanaged memory
            abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0222. This string does not contain sensitive information."
                      EFAULT, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0223. This string does not contain sensitive information."
        } else {
            mem_usage_push(usage, ptr);
        }
    }
}

void abq_free(cvoidc_ptr ptr){
    if (NULL == ptr) {
        // don't free NULL pointer
    } else {
        mem_usage_t *mem_usage = lookup_mem_usage(ptr);
        if(NULL != mem_usage){
            mem_usage_push(mem_usage, ptr);
        }else{
            // TODO: log error
        }
    }
}

static void mem_usage_counts(cstr_t tag, mem_usage_t* mem_usage,
        int32_t *cur_usage, int32_t *peak_usage, int32_t *total_registered, bool_t log_details,
        bool_t reset_peak) {
    VITAL_NOT_NULL(tag);
    VITAL_NOT_NULL(mem_usage);
    VITAL_NOT_NULL(cur_usage);
    VITAL_NOT_NULL(peak_usage);
    VITAL_NOT_NULL(total_registered);
    byte_t buffer[B256_SIZE];
    ABQ_ENCODER(encoder, &ascii_codec, buffer, sizeof(buffer));

    // size per instance is total size of the instance
    size_t padding = mem_usage->range
            - ((size_t)mem_usage->increment * (size_t)mem_usage->total);
    size_t bitmap_size = sizeof(uint64_t) * UINT64_BITMAP_LENGTH(mem_usage->total);
    size_t base_size = sizeof(mem_usage_t) + bitmap_size + padding;

    size_t size_per = mem_usage->increment; // + sizeof(var_t)

    size_t used = base_size
            + (size_per * (size_t) mem_usage->usage);
    size_t peak = base_size
            + (size_per * (size_t)mem_usage->peak);
    size_t total = base_size
            + (size_per * (size_t)mem_usage->total);

    *cur_usage += (int32_t) used;
    *peak_usage += (int32_t) peak;
    *total_registered += (int32_t) total;

    if (log_details) {
        number_t peak_percent_used = 100.0;
        if ( 0U < mem_usage->total ) {
            peak_percent_used = (number_t)
                    (100.0 * (((number_t)mem_usage->peak) / ((number_t)mem_usage->total)));
        } else {
            // Empty mem_usage_t
        }

        (void) abq_encode_ascii(&encoder, "[mem_usage]", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0224. This string does not contain sensitive information."
        (void) abq_encode_ascii(&encoder, tag, -1);
        (void) abq_encode_ascii(&encoder, " %:", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0225. This string does not contain sensitive information."
        (void)abq_encode_left_padded_int(&encoder,
                (int64_t) peak_percent_used, DECIMAL_RADIX, ' ', (size_t) 3);
        (void) abq_encode_ascii(&encoder, ", count:", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0226. This string does not contain sensitive information."
        (void)abq_encode_left_padded_int(&encoder,
                (int64_t) mem_usage->total, DECIMAL_RADIX, ' ', (size_t) 4);
        (void) abq_encode_ascii(&encoder, ", peak:", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0227. This string does not contain sensitive information."
        (void)abq_encode_left_padded_int(&encoder,
                (int64_t) mem_usage->peak, DECIMAL_RADIX, ' ', (size_t) 4);
        (void) abq_encode_ascii(&encoder, ", used:", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0228. This string does not contain sensitive information."
        (void)abq_encode_left_padded_int(&encoder,
                (int64_t) mem_usage->usage, DECIMAL_RADIX, ' ', (size_t) 4);
        (void) abq_encode_ascii(&encoder, ", size_per:", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0229. This string does not contain sensitive information."
        (void)abq_encode_left_padded_int(&encoder,
                (int64_t) size_per, DECIMAL_RADIX, ' ', (size_t) 5);
        (void) abq_encode_ascii(&encoder, ", cost:", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0230. This string does not contain sensitive information."
        (void)abq_encode_left_padded_int(&encoder,
                (int64_t) total, DECIMAL_RADIX, ' ', (size_t) 7);
        (void) abq_encode_ascii(&encoder, ", spent:", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0231. This string does not contain sensitive information."
        (void)abq_encode_left_padded_int(&encoder,
                (int64_t) peak, DECIMAL_RADIX, ' ', (size_t) 7);
        (void) abq_encode_ascii(&encoder, ", name:", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0232. This string does not contain sensitive information."
        (void) abq_encode_ascii(&encoder, mem_usage->name, -1);
        ABQ_INFO_MSG(buffer);
    }
    if (reset_peak) {
        mem_usage->peak = mem_usage->usage;
    }
}

void mem_usage_stats_ex(int32_t *total_mem_registered,
                        int32_t *current_mem_used, int32_t *peak_mem_used,
                        bool_t log_details, bool_t reset_peak)
{
    int32_t cur_usage = 0;
    int32_t max_usage = 0;
    int32_t total_registered = 0;
    LLIST_FOREACH(empty_mem_usages, mem_usage_t, mem_usage, next) {
        mem_usage_counts("[ empty ]", mem_usage, &cur_usage, &max_usage, &total_registered, // parasoft-suppress CERT_C-MSC41-a-1 "c0233. This string does not contain sensitive information."
                log_details, reset_peak);
    }
    LLIST_FOREACH(private_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
        mem_usage_counts("[private]", mem_usage, &cur_usage, &max_usage, &total_registered, // parasoft-suppress CERT_C-MSC41-a-1 "c0234. This string does not contain sensitive information."
                log_details, reset_peak);
    }
    LLIST_FOREACH (shared_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
        mem_usage_counts("[shared]", mem_usage, &cur_usage, &max_usage, &total_registered, // parasoft-suppress CERT_C-MSC41-a-1 "c0235. This string does not contain sensitive information."
                log_details, reset_peak);
    }
    // Write results to caller's storage areas if supplied
    if (NULL != total_mem_registered) {
        *total_mem_registered = total_registered;
    }
    if (NULL != current_mem_used) {
        *current_mem_used = cur_usage;
    }
    if (NULL != peak_mem_used) {
        *peak_mem_used = max_usage;
    }
    ABQ_VITAL(total_registered == total_registered_memory);
}

void mem_usage_stats(int32_t* total_mem_registered,
        int32_t* current_mem_used, int32_t* peak_mem_used, bool_t log_details) {
    mem_usage_stats_ex(total_mem_registered, current_mem_used, peak_mem_used, log_details, false);
}


void abq_mem_check_all_chunks_freed(void) {
    bool_t all_clear = true;
    LLIST_FOREACH(shared_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
        if (0U != mem_usage->usage) {
            ABQ_ERROR_MSG_X(mem_usage->name, mem_usage->usage);
            all_clear = false;
            debugpoint();
        }
    }
    LLIST_FOREACH(private_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
        if (0U != mem_usage->usage) {
            ABQ_ERROR_MSG_X(mem_usage->name, mem_usage->usage);
            all_clear = false;
            debugpoint();
        }
    }
    ABQ_VITAL(all_clear);
}

void abq_mem_usage_free_all(void) {
    LLIST_FOREACH(shared_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
        for(byte_t *chunk = mem_usage_iter(mem_usage, NULL);
                NULL != chunk;
                chunk = mem_usage_iter(mem_usage, chunk)) {
            mem_usage_push(mem_usage, chunk);
        }
        ABQ_VITAL(0UL == mem_usage->usage);
    }
    LLIST_FOREACH(private_mem_usage_sorted_bysize, mem_usage_t, mem_usage, next) {
        for(byte_t *chunk = mem_usage_iter(mem_usage, NULL);
                NULL != chunk;
                chunk = mem_usage_iter(mem_usage, chunk)) {
            mem_usage_push(mem_usage, chunk);
        }
        ABQ_VITAL(0UL == mem_usage->usage);
    }
}
