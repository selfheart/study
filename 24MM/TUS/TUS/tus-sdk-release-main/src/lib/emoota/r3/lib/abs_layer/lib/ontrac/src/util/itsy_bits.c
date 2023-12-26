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

#include <ontrac/util/itsy_bits.h>

static const size_t one_megabyte = (1UL << 20UL);

void memcpy_ltr(var_t dest, cvar_t source, size_t nbytes) {
    if ((NULL != dest) && (NULL != source) && (0U != nbytes)) {
        // Since CPU registers can operate on multiple bytes per register
        //  it should be more efficient to transfer data in words equal to
        //  or larger then the size of the register.
        cbyte_t *source_bytes = ptr2cstr(source);
        byte_t *dest_bytes = ptr2raw(dest);
        if ((NULL == dest_bytes) || (NULL == source_bytes)) {
            // Unexpected, print error ?
        } else {
            size_t position = 0U;
            // abq_ctx should be the largest configured buffer on the system, (96 KB)
            //  assume anything larger then a megabyte is an error in computation
            ABQ_VITAL(nbytes <= one_megabyte);
            if (nbytes > sizeof(uint64_t)) {
                size_t offset1 = (size_t) ABQ_ALIGN_NEXT_OFFSET(dest_bytes, uint64_t);
                size_t offset2 = (size_t) ABQ_ALIGN_NEXT_OFFSET(source_bytes, uint64_t);
                // Check to see if memory can be aligned
                if (offset1 == offset2) { // Check for alignment on abq_word_t boundary
                    // Read all data up to aligned memory
                    while (position < offset1) {//  (position < nbytes) was ensured via if (nbytes > sizeof(abq_word_t))
                        dest_bytes[position] = source_bytes[position];
                        position += 1U;
                    }
                    // Now that bytes are aligned, speedy copy in word-sized blocks may commence
                    const_uint64_t *source_words = ptr2const64(&source_bytes[position]);
                    uint64_t *dest_words = ptr2uint64(&dest_bytes[position]);
                    offset1 = (nbytes - position) / sizeof(uint64_t);
                    position += (offset1 * sizeof(uint64_t));
                    // Use of offset2 is so that bytes are written left-to-right
                    for (offset2 = 0U; offset2 < offset1; offset2++) {
                        // Write out the entire word value in quick operations
                        dest_words[offset2] = source_words[offset2];
                    }
                } else if ((offset1 & 1UL) == (offset2 &
                                               1UL)) { // Equivalently: (offset1 % sizeof(uint16_t)) == (offset2 % sizeof(uint16_t))
                    // Align-able on a 16-bit boundary, speedy copy in 16 bit blocks as able
                    // Read all data up to aligned memory
                    if (0UL != (offset1 & 1UL)) {
                        dest_bytes[position] = source_bytes[position];
                        position += 1U;
                    }
                    // Now that bytes are aligned, speedy copy in word-sized blocks may commence
                    const_uint16_t *source_shorts = ptr2const16(&source_bytes[position]);
                    uint16_t *dest_shorts = ptr2uint16(&dest_bytes[position]);
                    offset1 = (nbytes - position) / sizeof(uint16_t);
                    position += (offset1 * sizeof(uint16_t));
                    // Use of offset2 is so that bytes are written left-to-right
                    for (offset2 = 0U; offset2 < offset1; offset2++) {
                        // Write out the entire word value in quick operations
                        dest_shorts[offset2] = source_shorts[offset2];
                    }
                } else {
                    // Not align-able on anything larger then 8-bit boundary, fall through
                }
            }
            // Copy over any  remainder data / unaligned data
            while (position < nbytes) {
                dest_bytes[position] = source_bytes[position];
                position += 1U;
            }
        }
    }
}

void bytes_set(var_t dest, cbyte_t value, size_t nbytes) {
    if (NULL == dest) {
        // Unexpected, print error ?
    } else {
        byte_t *dest_bytes = ptr2raw(dest);
        size_t position = 0U;
        // abq_ctx should be the largest configured buffer on the system, (96 KB)
        //  assume anything larger then a megabyte is an error in computation
        ABQ_VITAL(nbytes <= one_megabyte);
        if (nbytes > (size_t) (2U*sizeof(uint64_t))) {
            size_t offset = ABQ_ALIGN_NEXT_OFFSET(dest, uint64_t);
            uint64_t *dest_words = ptr2uint64(&dest_bytes[offset]);
            // Fill in any unaligned prefix + the first full word
            offset += sizeof(uint64_t);
            while (position < offset) {
                dest_bytes[position] = value;
                position += 1U;
            }
            // Now that we have filled a full word
            //  speedy copy that word onto the remaining words
            offset = (nbytes - position) / sizeof(uint64_t);
            position += (offset * sizeof(uint64_t));
            while(0U != offset) {
                // Write out the entire word value in quick operations
                dest_words[offset] = dest_words[0];
                offset--;
            }
        }
        // Copy over any unaligned suffix / remainder data
        while (position < nbytes) {
            dest_bytes[position] = value;
            position += 1U;
        }
    }
}

size_t uint64_bitmap_next_on(const uint64_t *bitmap, size_t start_index, size_t max_index) {
    size_t retval = max_index;
    size_t max_offset = max_index / 64UL;
    size_t bit_index = start_index % 64UL;
    for (size_t offset = start_index / 64UL; offset <= max_offset; offset++) {
        // Check if any bits are set in this uint64_t
        if (0UL != bitmap[offset]) {
            while (bit_index < 64UL) {
                uint64_t bitshifted = uint64_rightshift(bitmap[offset], bit_index);
                if (0U != (bitshifted & 0x0001UL)) {
                    // Found a match!
                    break;
                } else if(0U == (bitshifted & 0xFFFFU)) {
                    // None of the next 16 bits are set
                    bit_index += 16UL;
                } else if(0U == (bitshifted & 0x000FU)) {
                    // None of the next 4 bits are set
                    bit_index += 4UL;
                } else {
                    // Not the next bit, keep iterating
                    bit_index += 1UL;
                }
            }
            if (bit_index < 64UL) {
                // Found a match
                retval = (offset * 64UL) + bit_index;
                break;
            }
        }
        bit_index = 0UL;
    }
    return retval;
}

size_t uint64_bitmap_next_off(const uint64_t *bitmap, size_t start_index, size_t max_index) {
    size_t retval = max_index;
    size_t max_offset = max_index / 64UL;
    size_t bit_index = start_index % 64UL;
    for (size_t offset = start_index / 64UL; offset <= max_offset; offset++) {
        // Check if any bits are set in this uint64_t
        if (((uint64_t) UINT64_MAX) != bitmap[offset]) { // if not all_bits on
            while (bit_index < 64UL) {
                uint64_t bitshifted = uint64_rightshift(bitmap[offset], bit_index);
                if (0UL == (bitshifted & 0x0001UL)) {
                    // Found a match!
                    break;
                } else if(0xFFFFUL == (bitshifted & 0xFFFFUL)) {
                    // All of the next 16 bits are set
                    bit_index += 16UL;
                } else if(0x000FUL == (bitshifted & 0x000FUL)) {
                    // All of the next 4 bits are set
                    bit_index += 4UL;
                } else {
                    // Not the next bit, keep iterating
                    bit_index += 1UL;
                }
            }
            if (bit_index < 64UL) {
                // Found a match
                retval = (offset * 64UL) + bit_index;
                break;
            }
        }
        bit_index = 0UL;
    }
    return retval;
}
