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
 * @file util/abq_alist.h
 * @date Jun 22, 2018
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief Array-List macros for working with an array of static size full of structs or pointers
 *  the list is a data-oriented list, items in list are swapped by copying data,
 *  resulting in pointers to items within the list do NOT get preserved!
 *
 */

#ifndef HELLO_C_ABQ_ALIST_H
#define HELLO_C_ABQ_ALIST_H

#ifndef ABQ_VITAL
#include <assert.h>
#define ABQ_VITAL assert
#endif /* ABQ_VITAL */

#define ARRAY_COUNT(type, ...) (sizeof(__VA_ARGS__) / sizeof(type))

/** Type of a head of array struct of given type */
#define ALIST_HEAD_TYPE(type) struct type ## _alist

/** Defines the head of a array list of the given type, length and name */
#define ALIST_HEAD_DEF(type) ALIST_HEAD_TYPE(type) {size_t item_count; size_t max_items; type *array;}

/** Initialized the head of array list */
#define ALIST_HEAD_INIT(type, ...) { .item_count = 0U, .max_items = ARRAY_COUNT(type, __VA_ARGS__), .array = &(__VA_ARGS__)[0] }

/** Initialized the head of array list */
#define ALIST_HEAD_FILL(type, ...) { .item_count = ARRAY_COUNT(type, __VA_ARGS__), .max_items = ARRAY_COUNT(type, __VA_ARGS__), .array = &(__VA_ARGS__)[0] }

/** Defines and initializes a list of the given type, length and name */
#define ALIST_HEAD_WRAP(type, name, ...) ALIST_HEAD_DEF(type) name = ALIST_HEAD_FILL(type, __VA_ARGS__)

/** Returns the maximum capacity of the list as configured during compile time */
#define ALIST_CAPACITY(alist_ptr)  ((alist_ptr)->max_items)

/** Returns the current usage count (number of elements which have been added into the list) */
#define ALIST_SIZE(alist_ptr) ((alist_ptr)->item_count)

/** Returns a condition testing if the given list is empty */
#define ALIST_IS_EMPTY(alist_ptr) (0U == ALIST_SIZE(alist_ptr))

/** Returns a condition testing if the given list is completely filled */
#define ALIST_IS_FULL(alist_ptr) (ALIST_CAPACITY(alist_ptr) == ALIST_SIZE(alist_ptr))

/** Returns the entry at the given position */
#define ALIST_GET(alist_ptr, type, entry, index) do { \
    ABQ_VITAL(ALIST_SIZE(alist_ptr) > (index)); \
    ABQ_VITAL(0U <= (index));                   \
    entry = (type) (alist_ptr)->array[index];   \
} while(0)

/** Pushes an entry to the end of the array-list given alist_ptr, type of elements, and entry */
#define ALIST_PUSH(alist_ptr, type, entry) do {          \
    ABQ_VITAL(!ALIST_IS_FULL(alist_ptr));                   \
    (alist_ptr)->array[ALIST_SIZE(alist_ptr)] = (type) entry;  \
    ALIST_SIZE(alist_ptr) += 1U;                               \
} while(0)

/** Pops the last entry from the array list onto the entry parameter */
#define ALIST_POP(alist_ptr, type, entry) do{               \
    ABQ_VITAL(ALIST_SIZE(alist_ptr) > 0);                   \
    ALIST_SIZE(alist_ptr) -= 1U;                               \
    entry= (type) (alist_ptr)->array[ALIST_SIZE(alist_ptr)];   \
    (alist_ptr)->array[ALIST_SIZE(alist_ptr)] = (type){0};     \
} while(0)

#define ALIST_SWAP(alist_ptr, type, left_index, right_index) do{ \
    ABQ_VITAL(ALIST_SIZE(alist_ptr) > (left_index));    \
    ABQ_VITAL(0 <= (left_index));                       \
    ABQ_VITAL(ALIST_SIZE(alist_ptr) > (right_index));   \
    ABQ_VITAL(0 <= (right_index));                      \
    type tmp = (type) (alist_ptr)->array[left_index];   \
    (alist_ptr)->array[left_index]                      \
        = (type) (alist_ptr)->array[right_index];       \
    (alist_ptr)->array[right_index] = (type) tmp;       \
} while(0)

/** A for loop MACRO over all the entries in an array-list which uses and existing variable for storing each entry during loop */
#define ALIST_LOOP(alist_ptr, type, entry) for(int32_t alist_index=0; \
        (alist_index < ALIST_SIZE(alist_ptr))                         \
         ? ((entry = (type) (alist_ptr)->array[alist_index]) || true)   \
         : ((entry = (type) {0}) && false) ;                          \
         alist_index += 1)

/** Finds an entry in the array-list that matches a condition. if no matches are found, sets entry to {0} and index to -1 */
#define ALIST_FIND(alist_ptr, type, entry, index, condition) do{   \
    index = -1;                                         \
    ALIST_LOOP(alist_ptr, type, entry) {                \
        if(condition){                                  \
            index = alist_index;                        \
            break;                                      \
        }                                               \
    }                                                   \
} while(0)

/** Inserts a entry into the array-list at the index position, and keeps other elements in order relative to each order */
#define ALIST_INSERT_AT(alist_ptr, type, entry, index) do{  \
    ABQ_VITAL(!ALIST_IS_FULL(alist_ptr));                   \
    ABQ_VITAL(ALIST_SIZE(alist_ptr) >= index);              \
    ABQ_VITAL(0 <= index);                                  \
    int32_t alist_index = ALIST_SIZE(alist_ptr);            \
    while(alist_index > index) {                            \
        (alist_ptr)->array[alist_index]                     \
            = (type) (alist_ptr)->array[alist_index-1];     \
        alist_index -= 1;                                   \
    }                                                       \
    (alist_ptr)->array[alist_index] = (type)entry;          \
    ALIST_SIZE(alist_ptr) += 1;                            \
}while (0)

/** Removes the given entry from an array-list, and keeps other elements in order relative to each order */
#define ALIST_REMOVE_AT_ORDERED(alist_ptr, type, index) do{ \
    ABQ_VITAL(ALIST_SIZE(alist_ptr) > index);   \
    ABQ_VITAL(0 <= index);                      \
    int32_t alist_index = (index+1);            \
    while(alist_index < ALIST_SIZE(alist_ptr)){ \
        (alist_ptr)->array[alist_index-1]       \
            = (type) (alist_ptr)->array[alist_index]; \
        alist_index += 1;                       \
    }                                           \
    ALIST_SIZE(alist_ptr) -= 1;                    \
    (alist_ptr)->array[ALIST_SIZE(alist_ptr)] = (type){0}; \
}while (0)

/** Removes the given entry from an array-list, but does not maintain order of the list */
#define ALIST_REMOVE_AT_QUICK(alist_ptr, type, index) do{ \
    ABQ_VITAL(ALIST_SIZE(alist_ptr) > index);           \
    ABQ_VITAL(0 <= index);                              \
    if(index != (ALIST_SIZE(alist_ptr)-1)) {            \
        ALIST_SWAP(alist_ptr, type,                     \
                index, (ALIST_SIZE(alist_ptr)-1));      \
    }                                                   \
    ALIST_SIZE(alist_ptr) -= 1;                            \
    (alist_ptr)->array[ALIST_SIZE(alist_ptr)] = (type){0}; \
}while (0)

/** Finds and removes entry array-list that matches a condition, sets entry to NULL if none are found */
#define ALIST_FIND_AND_REMOVE(alist_ptr, type, entry, condition) do{ \
    int32_t matching_index = -1;                                    \
    ALIST_FIND(alist_ptr, type, entry, matching_index, condition);  \
    if( 0 <= matching_index) {                                      \
        ALIST_REMOVE_AT_ORDERED(alist_ptr, type, matching_index);   \
    }                                                               \
}while(0)

/** Clears all entries in the array-list for given alist_ptr and type of elements */
#define ALIST_CLEAR(alist_ptr, type) do{                    \
    while(0 != ALIST_SIZE(alist_ptr)) {                        \
        ALIST_SIZE(alist_ptr) -= 1;                            \
        (alist_ptr)->array[ALIST_SIZE(alist_ptr)] = (type){0}; \
    }                                                       \
} while(0)

#endif //HELLO_C_ABQ_ALIST_H
