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
 *  Copyright (c) 2021 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/

#ifndef INCLUDE_ONTRAC_UTIL_ITSY_BITS_H_
#define INCLUDE_ONTRAC_UTIL_ITSY_BITS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <assert.h>

typedef const uint8_t const_uint8_t;
typedef const uint16_t const_uint16_t;
typedef const uint32_t const_uint32_t;
typedef const uint64_t const_uint64_t;

/**
 * @brief Compute number of bytes beyond previously aligned word boundary, will be zero if address is aligned
 *
 * @param addr: Memory Address (Pointer) to check for alignment
 * @param word_type: used to compute boundary sizes
 */
#define ABQ_ALIGN_LAST_OFFSET(addr, word_type) (((size_t)(uintptr_t)(addr)) & (sizeof(word_type)-1UL))
/**
 * @brief  Compute number of bytes until the next aligned word boundary, will be zero if address is aligned
 *
 * @param addr: Memory Address (Pointer) to check for alignment
 * @param word_type: used to compute boundary sizes
 */
#define ABQ_ALIGN_NEXT_OFFSET(addr, word_type) (((~(size_t)(uintptr_t)(addr))+1UL) & (sizeof(word_type)-1UL))

/**
 * @brief similar to memset, but internally controlled implementation
 *  Attempts to be as efficient as possible by moving data in blocks of uin64_t
 *   writes data from start to end of given byte arrays
 */
extern void abq_bytes_set(var_t dest, cbyte_t value, size_t nbytes);
/**
 * @brief MISRA "compliant" cast from a pointer to an integer
 *   See Exception to MISRA Rule 11.3:
 *     It is permitted to convert a pointer to object type into a pointer to one of the object types char, signed
 *     char or unsigned char. The Standard guarantees that pointers to these types can be used to access the
 *     individual bytes of an object.
 * @param ptr: pointer to data which we need to directly access
 * @return char representation of the pointer's address in memory
 */
static inline byte_t* abq_ptr2raw(var_t ptr) {
    return (byte_t*) ptr;
}

/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return uint64_t* representation of the pointer's address in memory
 */
static inline uint64_t* abq_ptr2uint64(var_t ptr) {
    return (uint64_t *) ptr; /* parasoft-suppress MISRA2012-RULE-11_3-2 "OK to cast to UINT64 type ONLY once the data has been previously aligned (above)" */
}


#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_ONTRAC_UTIL_ITSY_BITS_H_ */
