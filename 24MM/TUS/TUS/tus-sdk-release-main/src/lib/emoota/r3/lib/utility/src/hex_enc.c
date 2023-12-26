/* ****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */
/**
 * @file hex_enc.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 */
#include "hex_enc.h"
#include <limits.h>

/* parasoft-begin-suppress CERT_C-DCL06-a-3 Keeping hard coded values in favor of code readability */

const cstr_t abq_upper_hex_digits = "0123456789ABCDEF";       // parasoft-suppress CERT_C-MSC41-a-1 "c0528. This string does not contain sensitive information."
static int8_t abq_hex_value_of_char(const int32_t codepoint) {
    int8_t rvalue = SINT_MINUS_ONE;
    if (codepoint <= CHAR_MAX) {
        switch ((byte_t) codepoint) {
            case '0':
                rvalue = SINT_ZERO;
                break;
            case '1':
                rvalue = SINT_ONE;
                break;
            case '2':
                rvalue = 2;
                break;
            case '3':
                rvalue = 3;
                break;
            case '4':
                rvalue = 4;
                break;
            case '5':
                rvalue = 5;
                break;
            case '6':
                rvalue = 6;
                break;
            case '7':
                rvalue = 7;
                break;
            case '8':
                rvalue = 8;
                break;
            case '9':
                rvalue = 9;
                break;
            case 'a':
                rvalue = 10;
                break;
            case 'A':
                rvalue = 10;
                break;
            case 'b':
                rvalue = 11;
                break;
            case 'B':
                rvalue = 11;
                break;
            case 'c':
                rvalue = 12;
                break;
            case 'C':
                rvalue = 12;
                break;
            case 'd':
                rvalue = 13;
                break;
            case 'D':
                rvalue = 13;
                break;
            case 'e':
                rvalue = 14;
                break;
            case 'E':
                rvalue = 14;
                break;
            case 'f':
                rvalue = 15;
                break;
            case 'F':
                rvalue = 15;
                break;
            default:
                rvalue = SINT_MINUS_ONE;
                break;
        }
    }
    return rvalue;
}

err_t abq2_encode_hex(abq_encoder_t *const encoder, const byte_t value) {
    err_t retval = CHECK_NULL(encoder);
    const cstr_t abq_lower_hex_digits = "0123456789abcdef";// parasoft-suppress CERT_C-MSC41-a-1 "c0529. This string does not contain sensitive information."
    if ((EXIT_SUCCESS == retval) && ((uint8_t) value <= UINT_0xFFU)) {
        retval = abq2_encode_char(encoder,
                                  abq_lower_hex_digits[LEFT_NIBBLE(value)]);
        if (EXIT_SUCCESS == retval) {
            retval = abq2_encode_char(encoder,
                                      abq_lower_hex_digits[RIGHT_NIBBLE(value)]);
        }
    }
    return retval;
}

int32_t abq_hex_decode(const byte_t *const source, const int32_t source_len,
                       byte_t *const dest, const int32_t dest_len) {
    int32_t rvalue = SINT_ZERO;
    if ((NULL == source) || (NULL == dest) || (source_len < SINT_ZERO)) {
        rvalue = SINT_MINUS_ONE;
    } else if (SINT_ZERO > dest_len) {
        rvalue = SINT_MINUS_ONE;
    } else {
        ABQ_DECODER(decoder, &abq_ascii_codec, source, source_len)
        while ((SINT_ZERO <= rvalue) && (rvalue < dest_len) && (decoder.pos < decoder.max)) {
            const uint8_t left = (uint8_t) abq_hex_value_of_char(abq2_decode_cp(&decoder));
            uint8_t right = (uint8_t) abq_hex_value_of_char(abq2_decode_cp(&decoder));
            if ((left >= (uint8_t) HEX_RADIX) || (right >= (uint8_t) HEX_RADIX)) {
                // not a hexadecimal value
                rvalue = SINT_MINUS_ONE;
            } else {
                right |= (uint8_t) (left << UINT_FOUR);// merge left into right
                dest[rvalue] = (byte_t) (right);       // and cast to byte_T
                rvalue += SINT_ONE;
            }
        }

        if (SINT_ZERO > rvalue) {
            // Return error as is
        } else if (decoder.pos < decoder.max) {
            // Failed to decode all the data
            //            ABQ_EXPECT(rvalue == dest_len)
            rvalue = SINT_MINUS_ONE;
        } else {
            if (rvalue == dest_len) {
                // Exact buffer space to decode input string
            } else {
                // append a terminator to end-of-string
                dest[rvalue] = '\0';
            }
        }
    }
    return rvalue;
}

/* parasoft-end-suppress CERT_C-DCL06-a-3 */
