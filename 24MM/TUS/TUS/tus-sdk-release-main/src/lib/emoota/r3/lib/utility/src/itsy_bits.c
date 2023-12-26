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
 */

#include <stddef.h>
#include "emoota_utility_types.h"
#include "itsy_bits.h"

/* parasoft-begin-suppress CERT_C-DCL06-a-3 Keeping hard coded values in favor of code readability */

void abq_bytes_set(var_t const dest, cbyte_t value, const size_t nbytes) {
    if (NULL == dest) {
        // Unexpected, print error ?
    } else if ((uint8_t) value <= UINT_0xFFU) {
        byte_t *dest_bytes = abq_ptr2raw(dest);
        size_t position = UINT_ZERO;
        // abq_ctx should be the largest configured buffer on the system, (96 KB)
        //  assume anything larger than a megabyte is an error in computation
        if (nbytes > (size_t) (UINT_TWO * sizeof(uint64_t))) {
            size_t offset = ABQ_ALIGN_NEXT_OFFSET(dest, uint64_t);
            uint64_t *const dest_words = abq_ptr2uint64(&dest_bytes[offset]);
            // Fill in any unaligned prefix + the first full word
            offset += sizeof(uint64_t);
            while (position < offset) {
                dest_bytes[position] = value;
                position += UINT_ONE;
            }
            // Now that we have filled a full word
            //  speedy copy that word onto the remaining words
            offset = (nbytes - position) / sizeof(uint64_t);
            position += (offset * sizeof(uint64_t));
            while(UINT_ZERO != offset) {
                // Write out the entire word value in quick operations
                dest_words[offset] = dest_words[SINT_ZERO];
                offset--;
            }
        }
        // Copy over any unaligned suffix / remainder data
        while (position < nbytes) {
            dest_bytes[position] = value;
            position += UINT_ONE;
        }
    } else {
        /* nothing */
    }
}

/* parasoft-end-suppress CERT_C-DCL06-a-3 */
