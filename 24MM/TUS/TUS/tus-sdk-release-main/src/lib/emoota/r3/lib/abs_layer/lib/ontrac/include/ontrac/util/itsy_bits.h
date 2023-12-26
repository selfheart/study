//#line 2 "itsy_bits.c"
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
 * @file itsy_bits.c
 * @date May 19, 2020
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#ifndef INCLUDE_ONTRAC_UTIL_ITSY_BITS_H_
#define INCLUDE_ONTRAC_UTIL_ITSY_BITS_H_


#include <platform/platformconfig.h>
// For use of ABQ_VATAL assertions
#include <ontrac/ontrac/status_codes.h>

typedef const uint8_t const_uint8_t;
typedef const uint16_t const_uint16_t;
typedef const uint32_t const_uint32_t;
typedef const uint64_t const_uint64_t;

/**
 * @brief from https://en.wikipedia.org/wiki/Data_structure_alignment
 * aligned = (offset + (align - 1)) & ~(align - 1)
 *         = (offset + (align - 1)) & -align
 */
#define ABQ_ALIGN_SIZE(sized, word_type) (((size_t)(sized) + (sizeof(word_type)-1UL)) & (~(sizeof(word_type)-1UL)))
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
 * @brief Can be used to align an octet array to the next boundry based on type
 *
 * @param addr: Memory Address (Pointer) to check for alignment
 * @param word_type: used to compute boundary sizes
 */
#define ABQ_ALIGN_BYTES(addr, word_type) &addr[ABQ_ALIGN_NEXT_OFFSET(addr, word_type)]

/**
 * @brief Memory copy left-to-right. Similar to memcpy, but where we can define behavior when memory from source overlaps with dest
 *  Attempts to be as efficient as possible by moving data in blocks of uin64_t
 *   writes data starting from the 0 position (left) to ending position nbytes-1 (right) of given byte arrays
 *
 */
extern void memcpy_ltr(var_t dest, cvar_t source, size_t nbytes);
#define bytes_copy memcpy_ltr

/**
 * @brief similar to memset, but internally controlled implementation
 *  Attempts to be as efficient as possible by moving data in blocks of uin64_t
 *   writes data from start to end of given byte arrays
 */
extern void bytes_set(var_t dest, cbyte_t value, size_t nbytes);
/**
 * @brief MISRA "compliant" cast from a pointer to an integer
 *   See Exception to MISRA Rule 11.3:
 *     It is permitted to convert a pointer to object type into a pointer to one of the object types char, signed
 *     char or unsigned char. The Standard guarantees that pointers to these types can be used to access the
 *     individual bytes of an object.
 * @param ptr: pointer to data which we need to directly access
 * @return const char representation of the pointer's address in memory
 */
static inline cstr_t ptr2cstr(cvar_t ptr) {
    return (cstr_t) ptr;
}
/**
 * @brief MISRA "compliant" cast from a pointer to an integer
 *   See Exception to MISRA Rule 11.3:
 *     It is permitted to convert a pointer to object type into a pointer to one of the object types char, signed
 *     char or unsigned char. The Standard guarantees that pointers to these types can be used to access the
 *     individual bytes of an object.
 * @param ptr: pointer to data which we need to directly access
 * @return char representation of the pointer's address in memory
 */
static inline byte_t* ptr2raw(var_t ptr) {
    return (byte_t*) ptr;
}

/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return const uint8_t representation of the pointer's address in memory
 */
static inline const_uint8_t* ptr2const8(cvar_t ptr) {
    return (const_uint8_t *) ptr;
}
/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return abq_cword_t representation of the pointer's address in memory
 */
static inline uint8_t* ptr2uint8(var_t ptr) {
    return (uint8_t*) ptr;
}
/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return const_uint16_t representation of the pointer's address in memory
 */
static inline const_uint16_t* ptr2const16(cvar_t ptr) {
    ABQ_VITAL(0U == ABQ_ALIGN_LAST_OFFSET(ptr, uint16_t));
    return (const_uint16_t*) ptr; /* parasoft-suppress MISRA2012-RULE-11_3-2 "OK to cast to UINT16 type ONLY once the data has been previously aligned (above)" */
}
/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return uint16_t representation of the pointer's address in memory
 */
static inline uint16_t* ptr2uint16(var_t ptr) {
    ABQ_VITAL(0U == ABQ_ALIGN_LAST_OFFSET(ptr, uint16_t));
    return (uint16_t*) ptr; /* parasoft-suppress MISRA2012-RULE-11_3-2 "OK to cast to UINT16 type ONLY once the data has been previously aligned (above)" */
}
/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return const_uint32_t* representation of the pointer's address in memory
 */
static inline const_uint32_t* ptr2const32(cvar_t ptr) {
    ABQ_VITAL(0U == ABQ_ALIGN_LAST_OFFSET(ptr, const_uint32_t));
    return (const_uint32_t*) ptr; /* parasoft-suppress MISRA2012-RULE-11_3-2 "OK to cast to UINT64 type ONLY once the data has been previously aligned (above)" */
}
/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return uint32_t* representation of the pointer's address in memory
 */
static inline uint32_t* ptr2uint32(var_t ptr) {
    ABQ_VITAL(0U == ABQ_ALIGN_LAST_OFFSET(ptr, uint32_t));
    return (uint32_t*) ptr; /* parasoft-suppress MISRA2012-RULE-11_3-2 "OK to cast to UINT64 type ONLY once the data has been previously aligned (above)" */
}
/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return const_uint64_t* representation of the pointer's address in memory
 */
static inline const_uint64_t* ptr2const64(cvar_t ptr) {
    ABQ_VITAL(0U == ABQ_ALIGN_LAST_OFFSET(ptr, const_uint64_t));
    return (const_uint64_t*) ptr; /* parasoft-suppress MISRA2012-RULE-11_3-2 "OK to cast to UINT64 type ONLY once the data has been previously aligned (above)" */
}
/**
 * @brief while not MISRA compliant, this cast is used in the spirit of exception to MISRA Rule 11.3. However the pointer MUST be aligned prior to cast
 *
 * @param ptr: pointer to data which we need to directly access
 * @return uint64_t* representation of the pointer's address in memory
 */
static inline uint64_t* ptr2uint64(var_t ptr) {
    ABQ_VITAL(0U == ABQ_ALIGN_LAST_OFFSET(ptr, uint64_t));
    return (uint64_t*) ptr; /* parasoft-suppress MISRA2012-RULE-11_3-2 "OK to cast to UINT64 type ONLY once the data has been previously aligned (above)" */
}

/**
 * @brief MISRA "compliant" cast from a pointer to an integer
    See MISRA Rules 18.2 & 18.3

 * @param ptr: pointer which we want to cast into an integer
 * @return integer representation of the pointer's address in memory
 */
static inline uintptr_t ptr2addr(cvar_t ptr) {
    return (uintptr_t) ptr;
}

static inline uint8_t uint8_rightshift(uint8_t bits, size_t shifts) {
    uint8_t retval = 0UL;
    // CERT_C-INT34-a-3: Shifting operation should be enclosed with if checking
    //   that RHS operand lies between zero and one less than the width in bits
    //   of the underlying type of the LHS operand
    if (shifts < 8U) {
        retval = (bits >> shifts);;
    }
    return retval;
}

static inline uint8_t uint8_leftshift(uint8_t bits, size_t shifts) {
    uint8_t retval = 0UL;
    // CERT_C-INT34-a-3: Shifting operation should be enclosed with if checking
    //   that RHS operand lies between zero and one less than the width in bits
    //   of the underlying type of the LHS operand
    if (shifts < 8U) {
        retval = (bits << shifts);;
    }
    return retval;
}

/**
 * @brief check if a bit is set within a given byte
 *
 * @param octet: byte containing the bit of interest
 * @param bit_index: specifies which bit within the byte to check, must be zero or greater and less then eight (exclusive)
 * @return true if specified bit is set, false otherwise
 */
static inline bool_t uint8_get_bit(uint8_t octet, uint8_t bit_index) {
    ABQ_VITAL(bit_index < 8U);
    uint8_t shifted = uint8_rightshift(octet, bit_index);
    return (bool_t) (0U != (shifted & 0x01U));
}

static inline bool_t uint8_bitmap_is_on(const uint8_t *bitmap, size_t index) {
    size_t offset = index / 8UL;
    size_t bit_index = index % 8UL;
    return (0U != (uint8_rightshift(bitmap[offset], bit_index) & 0x01U));
}

static inline void uint8_bitmap_set_on(uint8_t *bitmap, size_t index) {
    size_t offset = index / 8UL;
    size_t bit_index = index % 8UL;
    bitmap[offset] |= uint8_leftshift(1UL, bit_index);
}

static inline void uint8_bitmap_set_off(uint8_t *bitmap, size_t index) {
    size_t offset = index / 8UL;
    size_t bit_index = index % 8UL;
    uint64_t bit = uint8_leftshift(1UL, bit_index);
    bitmap[offset] &= ~bit;
}

static inline uint16_t uint16_rightshift(uint16_t bits, size_t shifts) {
    uint16_t retval = 0UL;
    // CERT_C-INT34-a-3: Shifting operation should be enclosed with if checking
    //   that RHS operand lies between zero and one less than the width in bits
    //   of the underlying type of the LHS operand
    if (shifts < 16U) {
        retval = (bits >> shifts);;
    }
    return retval;
}

static inline uint16_t uint16_leftshift(uint16_t bits, size_t shifts) {
    uint16_t retval = 0UL;
    // CERT_C-INT34-a-3: Shifting operation should be enclosed with if checking
    //   that RHS operand lies between zero and one less than the width in bits
    //   of the underlying type of the LHS operand
    if (shifts < 16U) {
        retval = (bits << shifts);;
    }
    return retval;
}
/**
 * @brief write a big-endian (network-byte-order) uint16_t to a buffer
 *  This function writes out sizeof(uint16_t) to buffer and does not check for overflow or update position
 *
 * @param value: a uint16_t value to write out
 * @param dest: buffer into which to write value
 */
static inline void big_endian_write_uint16(uint16_t value, uint8_t *dest) {
    VITAL_NOT_NULL(dest);
    dest[0] = (uint8_t) uint16_rightshift(value, 8U);
    dest[1] = (uint8_t) value;
}
/**
 * @brief read a big-endian (network-byte-order) uint16_t from a buffer
 *  This function will read out sizeof(uint16_t) from buffer and does not check for sufficient data or update position
 *
 * @param source: source data with an encoded uint16_t value
 * @return decoded uint16_t value
 */
static inline uint16_t big_endian_read_uint16(const_uint8_t *source) {
    VITAL_NOT_NULL(source);
    uint16_t retval = (uint16_t) source[1];
    retval |= uint16_leftshift((uint16_t)source[0], 8U);
    return retval;
}

static inline uint32_t uint32_rightshift(uint32_t bits, size_t shifts) {
    uint32_t retval = 0UL;
    // CERT_C-INT34-a-3: Shifting operation should be enclosed with if checking
    //   that RHS operand lies between zero and one less than the width in bits
    //   of the underlying type of the LHS operand
    if (shifts < 32U) {
        retval = (bits >> shifts);;
    }
    return retval;
}

static inline uint32_t uint32_leftshift(uint32_t bits, size_t shifts) {
    uint32_t retval = 0UL;
    // CERT_C-INT34-a-3: Shifting operation should be enclosed with if checking
    //   that RHS operand lies between zero and one less than the width in bits
    //   of the underlying type of the LHS operand
    if (shifts < 32U) {
        retval = (bits << shifts);;
    }
    return retval;
}
/**
 * @brief write a big-endian (network-byte-order) uint32_t to a buffer
 *  This function writes out sizeof(uint32_t) to buffer and does not check for overflow or update position
 *
 * @param value: a uint32_t value to write out
 * @param dest: buffer into which to write value
 */
static inline void big_endian_write_uint32(uint32_t value, uint8_t *dest) {
    VITAL_NOT_NULL(dest);
    dest[0] = (uint8_t) uint32_rightshift(value, 24U);
    dest[1] = (uint8_t) uint32_rightshift(value, 16U);
    dest[2] = (uint8_t) uint32_rightshift(value, 8U);
    dest[3] = (uint8_t) value;
}
/**
 * @brief read a big-endian (network-byte-order) uint32_t from a buffer
 *  This function will read out sizeof(uint32_t) from buffer and does not check for sufficient data or update position
 *
 * @param source: source data with an encoded uint32_t value
 * @return decoded uint32_t value
 */
static inline uint32_t big_endian_read_uint32(const_uint8_t *source) {
    VITAL_NOT_NULL(source);
    uint32_t retval = (uint32_t) source[3];
    retval |= uint32_leftshift((uint32_t)source[2], 8U);
    retval |= uint32_leftshift((uint32_t)source[1], 16U);
    retval |= uint32_leftshift((uint32_t)source[0], 24U);
    return retval;
}
/**
 * @brief set the bit on a given byte with a value corresponding to given boolean
 *
 * @param octet: byte containing the bit of interest
 * @param bit_index: specifies which bit within the byte to check, must be zero or greater and less then eight (exclusive)
 * @param val: true if the bit is to be set to 1, or false if the bit is to be set to zero
 */
static inline void uint8_set_bit(uint8_t *octet, uint8_t bit_index, bool_t val) {
    VITAL_NOT_NULL(octet);
    ABQ_VITAL(bit_index < 8U);
    if (val) {
        uint8_bitmap_set_on(octet, bit_index);
    } else {
        uint8_bitmap_set_off(octet, bit_index);
    }
}


static inline uint64_t uint64_rightshift(uint64_t bits, size_t shifts) {
    uint64_t retval = 0UL;
    // CERT_C-INT34-a-3: Shifting operation should be enclosed with if checking
    //   that RHS operand lies between zero and one less than the width in bits
    //   of the underlying type of the LHS operand
    if (shifts < 64UL) {
        retval = (bits >> shifts);;
    }
    return retval;
}

static inline uint64_t uint64_leftshift(uint64_t bits, size_t shifts) {
    uint64_t retval = 0UL;
    // CERT_C-INT34-a-3: Shifting operation should be enclosed with if checking
    //   that RHS operand lies between zero and one less than the width in bits
    //   of the underlying type of the LHS operand
    if (shifts < 64UL) {
        retval = (bits << shifts);;
    }
    return retval;
}

/**
 * @brief write a big-endian (network-byte-order) uint64_t to a buffer
 *  This function writes out sizeof(uint64_t) to buffer and does not check for overflow or update position
 *
 * @param value: a uint64_t value to write out
 * @param dest: buffer into which to write value
 */
static inline void big_endian_write_uint64(uint64_t value, uint8_t *dest) {
    VITAL_NOT_NULL(dest);
    dest[0] = (uint8_t) uint64_rightshift(value, 56U);
    dest[1] = (uint8_t) uint64_rightshift(value, 48U);
    dest[2] = (uint8_t) uint64_rightshift(value, 40U);
    dest[3] = (uint8_t) uint64_rightshift(value, 32U);
    dest[4] = (uint8_t) uint64_rightshift(value, 24U);
    dest[5] = (uint8_t) uint64_rightshift(value, 16U);
    dest[6] = (uint8_t) uint64_rightshift(value, 8U);
    dest[7] = (uint8_t) value;
}
/**
 * @brief read a big-endian (network-byte-order) uint64_t from a buffer
 *  This function will read out sizeof(uint64_t) from buffer and does not check for sufficient data or update position
 *
 * @param source: source data with an encoded uint64_t value
 * @return decoded uint64_t value
 */
static inline uint64_t big_endian_read_uint64(const_uint8_t *source) {
    VITAL_NOT_NULL(source);
    uint64_t retval = (uint64_t) source[7];
    retval |= uint64_leftshift((uint64_t)source[6], 8U);
    retval |= uint64_leftshift((uint64_t)source[5], 16U);
    retval |= uint64_leftshift((uint64_t)source[4], 24U);
    retval |= uint64_leftshift((uint64_t)source[3], 32U);
    retval |= uint64_leftshift((uint64_t)source[2], 40U);
    retval |= uint64_leftshift((uint64_t)source[1], 48U);
    retval |= uint64_leftshift((uint64_t)source[0], 56U);
    return retval;
}

#define UINT64_BITMAP_LENGTH(bit_count) (size_t)((63UL + (size_t)(bit_count)) / 64UL)

static inline bool_t uint64_bitmap_is_on(const uint64_t *bitmap, size_t index) {
    size_t offset = index / 64UL;
    size_t bit_index = index % 64UL;
    return (0U != (uint64_rightshift(bitmap[offset], bit_index) & 0x01U));
}

static inline void uint64_bitmap_set_on(uint64_t *bitmap, size_t index) {
    size_t offset = index / 64UL;
    size_t bit_index = index % 64UL;
    bitmap[offset] |= uint64_leftshift(1UL, bit_index);
}

static inline void uint64_bitmap_set_off(uint64_t *bitmap, size_t index) {
    size_t offset = index / 64UL;
    size_t bit_index = index % 64UL;
    uint64_t bit = uint64_leftshift(1UL, bit_index);
    bitmap[offset] &= ~bit;
}

extern size_t uint64_bitmap_next_on(const uint64_t *bitmap,
        size_t start_index, size_t max_index);

extern size_t uint64_bitmap_next_off(const uint64_t *bitmap,
        size_t start_index, size_t max_index);

#endif /* INCLUDE_ONTRAC_UTIL_ITSY_BITS_H_ */
