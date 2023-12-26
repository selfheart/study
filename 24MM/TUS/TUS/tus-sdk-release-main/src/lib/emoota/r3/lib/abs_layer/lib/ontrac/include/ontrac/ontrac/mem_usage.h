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
 * @file ontrac/mem_usage.h
 * @date Feb 27, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief A free-list allocator implementation
 *
 * Works by defining a set of static mem_usage_t structures which\n
 *  act a set of memory pools, each with a set number of blocks,\n
 *  with each block having the same pre-determined size.\n
 *  They a linked-list of all free and used blocks assigned to them,\n
 *  and (de)allocate them in/out upon requests.\n
 *
 * More often then not we have one mem_usage_t predefined and private for\n
 *  a particular structure or class, and that structure or class will\n
 *  use 'malloc_from' to grab an instance from a given mem_usage_t\n
 *
 * However it is also useful to have a number of shared mem_usage_t\n
 *  structures of different sizes which can be used with the 'abq_malloc'\n
 *  to get a chunk of memory of a best fit size if the size was not known in advance.\n
 *  This method is of particular importance when working with strings\n
 *  and/or byte-buffers of variable sizes that have an indeterminate life-span.\n
 *
 */

#ifndef SRC_ABQ_MEMORY_H_
#define SRC_ABQ_MEMORY_H_

#include <limits.h>
#include <platform/platformconfig.h>
#include <ontrac/ontrac/ontrac_config.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/util/itsy_bits.h>
#include <ontrac/util/abq_llist.h>
#include <ontrac/util/abq_stacks.h>
#include <ontrac/util/thread_utils.h>

#ifndef WORD_BIT
# ifndef __WORDSIZE
  /* Safest assumption.  */
  typedef uint64_t abq_word_t;
# elif (64 == __WORDSIZE)
  typedef uint64_t abq_word_t;
# elif (32 == __WORDSIZE)
  typedef uint32_t abq_word_t;
# elif (16 == __WORDSIZE)
  typedef uint16_t abq_word_t;
#else
   /* Safest assumption.  */
  typedef uint64_t abq_word_t;
#endif
#elif (64 == WORD_BIT)
 typedef uint64_t abq_word_t;
#elif (32 == WORD_BIT)
 typedef uint32_t abq_word_t;
#elif (16 == WORD_BIT)
 typedef uint16_t abq_word_t;
#else
  /* Safest assumption.  */
 typedef uint64_t abq_word_t;
#endif


typedef struct for_mem_usage mem_usage_t;
typedef void (*mem_usage_init_t) (mem_usage_t* mem_usage);
/**
 * INTERNALLY used to track each track links to pre-allocated chunks of a specific size
 *  exposed in the header file only so that the compiler can determine size
 *  requirements when using the DECLARE_*_MEM_USAGE macros
 */
struct for_mem_usage {
    /** variable name assigned to this memory chunk */
    const char * name;
    /** Used for linking mem_usage_t together in a linked-list sorted by chunk_size */
    LLIST_LINK(struct for_mem_usage, next);
    /** lock used to synchronize recourse access */
    abq_mutex_t* mutex;
    /** Pointer to the head of the allocated chunk array */
    byte_t* partition;
    /** Size (in bytes) of each block within array */
    const size_t increment;
    /** Total span (in bytes) of the entire partition */
    const size_t range;
    /** bitmap recording which chunks are used/free */
    uint64_t* bitmap;
    /** Number of chunks actively in use */
    uint16_t usage;
    /** Peak number of chunks actively used at a given time */
    uint16_t peak;
    /** Total number of chunks available via this mem-usage */
    const uint16_t total;
};

/** total number of bytes of registered mem_usage memory,
 *  memory isn't registered until the associated mem_usage_t is used at least once
 *  does not yet include memory used by the associated class_t if any
 */
extern size_t total_registered_memory;

/**
 * @brief see <stdlib.h>: void *malloc(size_t size);
 *
 * @param size: minimum number of octets needed in the resulting buffer
 * @param usage: (OUT) optional parameter used to record the mem_usage_t from which the block was allocated
 * @return A pointer to a buffer of at least the specified size, or NULL if none are available
 */
extern byte_t* abq_malloc_ex(size_t size, mem_usage_t **usage);

static inline var_t abq_malloc(size_t size) {
    return (var_t) abq_malloc_ex(size, NULL);
}

/**
 * pops the next free chunk of memory from the specified mem_usage_t
 * to return memory back to this specific pool, call 'abq_free'
 * using 'malloc_from' over 'abq_malloc' should be useful
 * in narrowing down memory leaks to specific mem pools.
 * If the mem_usage_t is unregistered, it will register it as
 * a private (unshared) mem_usage_t.
 *
 * @param mem_usage: pointer to the defined mem_usage_t
 * @return pointer to an allocated chunk or NULL if none are available
 */
extern byte_t* malloc_from(mem_usage_t * mem_usage);
/**
 * @brief see <stdlib.h>: void *calloc(size_t nitems, size_t size);
 *
 * @param nitems: number of items to be allocated
 * @param size: size of each item to be allocated
 * @param usage: (OUT) optional parameter used to record the mem_usage_t from which the block was allocated
 * @return pointer to a contiguous chunk of allocated memory or NULL if unable
 */
extern byte_t* abq_calloc_ex(size_t nitems, size_t size, mem_usage_t **usage);
static inline var_t abq_calloc(size_t nitems, size_t size) {
    return (var_t) abq_calloc_ex(nitems, size, NULL);
}

/**
 * @brief see <stdlib.h>: void free(void *ptr);
 *
 * @param ptr: previously allocated chunk of memory to be returned to the mem_usage_t as available for future allocations
 */
extern void abq_free(cvoidc_ptr ptr);
/**
 * @brief returns a block of memory into the mem_usage from which it was taken
 *  this eliminates the need for a lookup of which mem_usage_t it was allocated from
 *  and is considered the inverse 'malloc_from'
 *
 * @param ptr: ptr to the block of data which was allocated using malloc_from
 * @param mem_usage: the mem_usage from which this block of data was used from
 */
extern void abq_free_into(cvoidc_ptr ptr, mem_usage_t * mem_usage);
/**
 * @brief used to register a mem_usage_t in the global-list of all mem_usage_t objects as either private or shared
 *
 * @param mem_usage: pointer to a defined mem_usage_t that has yet to be registered
 * @param shared: flag indicating if it can be allocated from when 'abq_malloc' is called, or if caller must use 'malloc_from' to allocate from this pool
 */
extern void register_mem_usage(mem_usage_t *mem_usage, bool_t shared);
/**
 * @brief function which can be used to determine segment number of a given segment within a mem_usage's allocated partition
 *
 * @param mem_usage: mem_usage used to manage the memory pointed to by ptr
 * @param ptr: a pointer to memory which might be managed via the given mem_usage_t
 * @return index (segment number) of a given segment within the mem_usage's partition, or
 * @return UINT16_MAX if pointer is not managed by the given mem_usage_t
 */
extern uint16_t mem_usage_index_of(const mem_usage_t *mem_usage, cvoidc_ptr ptr);
/**
 * @brief check if a pointer allocated from a specific memory usage has been free'd or not
 *
 * @param mem_usage: the mem_usage used to track memory of interest
 * @param index: used to identify a specific block of memory within the mem_usage's partition(s)
 * @return true if memory of interest has been freed and is no longer valid for use, false otherwise
 */
static inline bool_t mem_usage_is_free(const mem_usage_t *mem_usage, uint16_t index) {
    ABQ_VITAL(index < mem_usage->total);
    return (false == uint64_bitmap_is_on(mem_usage->bitmap, (size_t)index));
}
/**
 * @brief resolves an chunk of memory tracked by a specific mem_usage_t
 *
 * @param mem_usage: mem_usage used to manage memory of interest
 * @param item: pointer to managed memory
 * @return NULL if memory is not managed by this mem_usage, or if it has been freed. Else pointer to aligned start of memory chunk in which the item pointer falls
 */
extern cvar_t mem_usage_resolve(const mem_usage_t *mem_usage, cvar_t item);
/**
 * @brief: iterator used to iterate over each allocated chunk of memory within a mem_usage
 *
 * @param mem_usage: mem_usage used to manage memory of interest
 * @param prev: pointer to last iterated chunk of memory within the mem_usage, use NULL to load initial chunk
 * @return pointer to next chunk of allocated memory (if any), or NULL if iteration has completed
 */
extern byte_t* mem_usage_iter(const mem_usage_t *mem_usage, const byte_t *prev);
/**
 * @brief function used to lookup the mem_usage_t associated with a given memory address
 *
 * @param ptr: allocated chunk of memory to investigate
 * @return pointer to a mem_usage_t used to govern memory at the given address
 */
extern mem_usage_t *lookup_mem_usage(cvoidc_ptr ptr);

static inline bool_t abq_memory_match(cstr_t chunk, uintptr_t range, cvoidc_ptr ptr){
    uintptr_t addr = ptr2addr(ptr);
    uintptr_t range_start = ptr2addr(chunk);
    uintptr_t range_end = range_start + (uintptr_t)range;
    return (bool_t) ((range_start <= addr) && (addr <range_end));
}

extern void mem_usage_lock_mutex(mem_usage_t *mem_usage);
extern void mem_usage_unlock_mutex(mem_usage_t *mem_usage);

/**
 *  @brief loops through mem_usage to load usage counts
 *
 * @param total_mem_registered: pointer to storage area for recording final tally of all registered memeory
 * @param current_mem_used: pointer to storage area for recording final tally of currently used memeory
 * @param peak_mem_used: pointer to storage area for recording final tally of peak usage of memeory
 * @param log_details: true if we wish to log details of each individual mem_usage to log, false to just load stats
 * @param reset_peak: true if we wish to reset peak usage count (to current usage count) of each mem_usage
 */
extern void mem_usage_stats(int32_t* total_mem_registered,
        int32_t* current_mem_used, int32_t* peak_mem_used, bool_t log_details);

extern void mem_usage_stats_ex(int32_t *total_mem_registered, int32_t *current_mem_used,
        int32_t *peak_mem_used, bool_t log_details, bool_t reset_peak);
/**
 * @brief check to see that all allocated memory has been freed up. usage abq_fatal to kill the program upon failure
 */
extern void abq_mem_check_all_chunks_freed(void);
/**
 * @brief free all allocated memory after a fatal as part of a reset
 */
extern void abq_mem_usage_free_all(void);

#define mem_usage_typecheck (void *)&total_registered_memory

/**
 * Used to statically allocate a region of memory
 *  with a total_size of size_of_chunks*number_of_chunks + metadata
 *  haven't found a way to register the mem_usage in the global scope yet
 *  so must be used in conjunction with 'malloc_from' and not 'abq_malloc'/'abq_calloc'
 */
#define DEFINE_STATIC_MEM_USAGE(variable_name, size_of_chunks, number_of_chunks)            \
    static byte_t variable_name ## _mem[sizeof(abq_word_t) + ((number_of_chunks) * ABQ_ALIGN_SIZE(size_of_chunks, abq_word_t))]; \
    static uint64_t variable_name ## _map[UINT64_BITMAP_LENGTH(number_of_chunks)];          \
    static mem_usage_t variable_name = {                                                    \
        .name = #variable_name,                                                             \
        .next = NULL,                                                                       \
        .mutex = NULL,                                                                      \
        .partition = (byte_t*) variable_name ## _mem,                                       \
        .increment = ABQ_ALIGN_SIZE(size_of_chunks, abq_word_t),                            \
        .range = (size_t) sizeof(variable_name ## _mem),                                    \
        .bitmap = variable_name ## _map,                                                    \
        .usage = 0U,                                                                        \
        .peak = 0U,                                                                         \
        .total = (uint16_t)number_of_chunks,                                                \
    };

/** A variable link used for creating linked list of cvar_t */
typedef struct var_link_t {
    cvar_t item;
    struct var_link_t *next;
} link_t;

typedef struct for_abq_ctx abq_ctx_t;

struct for_abq_ctx {
    /** should only by used when the context is locked using 'orch_context_lock' */
    char *buffer;
    size_t buf_size;
};

extern abq_ctx_t abq_ctx;

extern void abq_context_lock( void );
extern void abq_context_unlock( void );

#ifndef B16_SIZE
#define B16_SIZE    (16U)
#endif /* B16_SIZE */
#ifndef B32_SIZE
#define B32_SIZE    (32U)
#endif /* B32_SIZE */
#ifndef B48_SIZE
#define B48_SIZE    (48U)
#endif /* B48_SIZE */
#ifndef B64_SIZE
#define B64_SIZE    (64U)
#endif /* B64_SIZE */
#ifndef B80_SIZE
#define B80_SIZE    (80U)
#endif /* B80_SIZE */
#ifndef B96_SIZE
#define B96_SIZE    (96U)
#endif /* B96_SIZE */
#ifndef B112_SIZE
#define B112_SIZE   (112U)
#endif /* B112_SIZE */
#ifndef B128_SIZE
#define B128_SIZE   (128U)
#endif /* B128_SIZE */
#ifndef B256_SIZE
#define B256_SIZE   (256U)
#endif /* B256_SIZE */
#ifndef B512_SIZE
#define B512_SIZE   (512U)
#endif /* B512_SIZE */
#ifndef B1024_SIZE
#define B1024_SIZE  (1024U)
#endif /* B1024_SIZE */
#ifndef B2048_SIZE
#define B2048_SIZE  (2048U)
#endif /* B2048_SIZE */
#ifndef B4096_SIZE
#define B4096_SIZE  (4096U)
#endif /* B4096_SIZE */
#ifndef B8192_SIZE
#define B8192_SIZE  (8192U)
#endif /* B8192_SIZE */
#ifndef B16384_SIZE
#define B16384_SIZE (16384U)
#endif /* B16384_SIZE */
#ifndef B32768_SIZE
#define B32768_SIZE (32768U)
#endif /* B32768_SIZE */
#ifndef B65536_SIZE
#define B65536_SIZE (65536U)
#endif /* B65536_SIZE */

#endif /* SRC_ABQ_MEMORY_H_ */
