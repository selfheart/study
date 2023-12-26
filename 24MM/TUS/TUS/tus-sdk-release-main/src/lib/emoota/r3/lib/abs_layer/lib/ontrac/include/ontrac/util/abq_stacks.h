/*
 * abq_stack.h
 *
 *  Created on: May 6, 2019
 *      Author: mvogel
 */

#ifndef ONTRAC_INCLUDE_ONTRAC_UTIL_ABQ_STACKS_H_
#define ONTRAC_INCLUDE_ONTRAC_UTIL_ABQ_STACKS_H_

#include <platform/platformconfig.h>
#include <ontrac/ontrac/status_codes.h>

/**
 * @brief a compare function used to equate and sort related objects with relation to each other
 *
 * @param left the first instance to compare
 * @param right the second instance to compare
 * @return the value 0 if left instance has equal value to right instance
 *          a value less then 0 if left instance has higher value then right instance
 *          a value greater then 0 if left instance has smaller value then right instance
 */
typedef int8_t (*compare_function_t)(cvar_t left, cvar_t right);

#define ABQ_SEEK_NEXT(stack, type, size, start, index, item, condition) do { \
    for((index)=(start); (index) < (size); (index)++) {                 \
        (item) = (type)(stack)[(index)];                                \
        if (condition) {                                                \
            break;                                                      \
        } else {                                                        \
            (item) = NULL;                                              \
        }                                                               \
    }                                                                   \
}while(0)

// Should use a binary abq_bsearch for sorted arrays and this for unsorted
// Expect that items at the end of array (which have been added more recently) are more likely to be a match, thus reverse order index
#define ABQ_SEEK_LAST(stack, type, size, index, item, condition) do {   \
    for((index)=(size)-1U; ((size) > (index)) && (((int32_t)(index)) >= 0); (index)--) { \
        (item) = (type)(stack)[(index)];                            \
        if (condition) {                                            \
            break;                                                  \
        } else {                                                    \
            (item) = NULL;                                          \
        }                                                           \
    }                                                               \
}while(0)


// Compute sorted insert index of items without a match such that
//  ~index, ((~0)-index), or -1*(index+1) will result in the sorted index
// lower bound is inclusive, upper bound is exclusive
#define ABQ_SEEK_BINARY(stack, type, size, index, cmp_fctn, item)  do { \
        ABQ_VITAL(NULL != (stack));                                     \
        ABQ_VITAL(0 <= (int32_t)(size));                                \
        int8_t cmp_val = -1;                                            \
        uint16_t lower = 0U, upper = (uint16_t)(size);                  \
        while (lower < upper) {                                         \
            (index) = ((upper+lower) >> 1U);                            \
            cmp_val = cmp_fctn((stack)[index], (item));                 \
            if (0 == cmp_val) {                                         \
                break;                                                  \
            } else if(cmp_val < 0) {                                    \
                lower = 1U + (uint16_t)(index);                         \
            } else {                                                    \
                upper = (uint16_t)(index);                              \
            }                                                           \
        }                                                               \
        if (0 != cmp_val) {                                             \
            (index) = ((uint16_t)UINT16_MAX) - lower;                   \
        }                                                               \
}while(0)

#define ABQ_INSERT_AT(stack, type, size, max, item, index) do { \
    ABQ_VITAL(NULL != (stack));                                 \
    ABQ_VITAL((index) <= (size));                               \
    ABQ_VITAL((size) < (max));                                  \
    size_t position = (size_t) (size);                          \
    while (position > (size_t)(index)) {                        \
        (stack)[position] = (type) (stack)[position-1U];        \
        position -= 1U;                                         \
    }                                                           \
    (stack)[position] = (type) (item);                          \
    (size) += 1U;                                               \
}while(0)

#define ABQ_REMOVE_AT(stack, type, size, max, index) do { \
    ABQ_VITAL(NULL != (stack));                             \
    ABQ_VITAL((index) < (size));                            \
    ABQ_VITAL((size) <= (max));                             \
    size_t position = (size_t)((index)+1U);                 \
    while (position < (size_t)(size)) {                     \
        (stack)[position-1U] = (type) (stack)[position];    \
        position += 1U;                                     \
    }                                                       \
    (size) -= 1U;                                           \
}while(0)

#define ABQ_INSERT_SORTED(stack, type, size, max, index, cmp_fctn, item)  do {   \
    ABQ_SEEK_BINARY(stack, type, size, index, cmp_fctn, item);              \
    if ((index) >= (size)) {                                                \
        (index) = ((uint16_t)UINT16_MAX) - (index);                         \
    }                                                                       \
    ABQ_INSERT_AT(stack, type, size, max, item, index);                     \
}while(0)

#define ABQ_SWAP(stack, type, left_index, right_index) do{  \
    type tmp = (type) (stack)[(left_index)];                \
    (stack)[(left_index)] = (type)(stack)[(right_index)];   \
    (stack)[(right_index)] = (type) tmp;                    \
} while(0)

#define ABQ_BUBBLE_SORT(stack, type, size, cmp_fctn) do{  \
    bool_t sorted = true;                               \
    uint16_t i=0, j=0;                                  \
    for (i=0U; i<size; i+=1U) {                         \
        sorted = true;                                  \
        for (j=0U; j< (size-i-1U); j+=1U) {             \
            if(0 < cmp_fctn(stack[j], stack[j+1U])) {   \
                ABQ_SWAP(stack, type, j, j+1U);         \
                sorted = false;                         \
            }                                           \
        }                                               \
        if (sorted) { break; }                          \
    }                                                   \
}while(0)

#endif /* ONTRAC_INCLUDE_ONTRAC_UTIL_ABQ_STACKS_H_ */
