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
 * @brief TODO
 *
 */

#include <ontrac/text/hex_enc.h>
#include <ontrac/ontrac/status_codes.h>

const cstr_t upper_hex_digits = "0123456789ABCDEF"; // parasoft-suppress CERT_C-MSC41-a-1 "c0528. This string does not contain sensitive information."
const cstr_t lower_hex_digits = "0123456789abcdef"; // parasoft-suppress CERT_C-MSC41-a-1 "c0529. This string does not contain sensitive information."
int8_t hex_value_of_char(int32_t codepoint) {
    int8_t rvalue = -1;
    if (codepoint <= CHAR_MAX) {
        switch ((byte_t)codepoint) {
        case '0':
            rvalue = 0;
            break;
        case '1':
            rvalue = 1;
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
            rvalue = -1;
            break;
        }
    }
    return rvalue;
}

err_t abq_encode_hex(abq_encoder_t *encoder,
        const byte_t value, bool_t lower_case) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS == retval) {
        if (lower_case) {
            retval = abq_encode_char(encoder,
                    lower_hex_digits[LEFT_NIBBLE(value)]);
            if (EXIT_SUCCESS == retval) {
                retval = abq_encode_char(encoder,
                        lower_hex_digits[RIGHT_NIBBLE(value)]);
            }
        } else {
            retval = abq_encode_char(encoder,
                    upper_hex_digits[LEFT_NIBBLE(value)]);
            if (EXIT_SUCCESS == retval) {
                retval = abq_encode_char(encoder,
                        upper_hex_digits[RIGHT_NIBBLE(value)]);
            }
        }
    }
    return retval;
}

int32_t hex_encode(const byte_t* source, int32_t source_len,
        byte_t *dest, int32_t dest_len, bool_t lower_case) {
    err_t status = EXIT_SUCCESS;
    ABQ_DECODER(decoder, &raw_codec, source, source_len);
    ABQ_ENCODER(encoder, &ascii_codec, dest, dest_len);
    int32_t cp = abq_decode_cp(&decoder);
    do {
        if (0 > cp) {
            status = abq_status_take(EIO);
        } else {
            status = abq_encode_hex(&encoder, (const byte_t) cp, lower_case);
            if(EXIT_SUCCESS == status) {
                cp = abq_decode_cp(&decoder);
            }
        }
    } while(EXIT_SUCCESS == status);

    int32_t retval = -1;
    if (ENODATA != status) {
        // Unexpected status
        (void)abq_status_set(status, false);
    } else {
        // Read out all data successfully
        retval = (int32_t)encoder.pos;
    }
    return retval;
}

int32_t hex_decode(const byte_t* source, int32_t source_len,
        byte_t *dest, int32_t dest_len) {
    int32_t rvalue = 0;
    if((NULL == source) || (NULL == dest)){
        (void) abq_status_set(EFAULT, false);
        rvalue = -1;
    }else if (0 > dest_len) {
        (void) abq_status_set(EINVAL, false);
        rvalue = -1;
    } else {
        ABQ_DECODER(decoder, &ascii_codec, source, source_len);
        while ((0 <= rvalue) && (rvalue < dest_len)
                && (decoder.pos < decoder.max)) {
            uint8_t left = (uint8_t) hex_value_of_char(abq_decode_cp(&decoder));
            uint8_t right = (uint8_t) hex_value_of_char(abq_decode_cp(&decoder));
            if ((left >= (uint8_t) HEX_RADIX) || (right >= (uint8_t) HEX_RADIX) ) {
                // not a hexadecimal value
                (void) abq_status_set(EILSEQ, false);
                rvalue = -1;
            } else {
                right |= (uint8_t) (left << 4); // merge left into right
                dest[rvalue] = (byte_t)(right); // and cast to byte_T
                rvalue += 1;
            }
        }

        if (0 > rvalue) {
            // Return error as is
        }else if (decoder.pos < decoder.max) {
            // Failed to decode all the data
            ABQ_EXPECT(rvalue == dest_len);
            (void) abq_status_set(EOVERFLOW, false);
            rvalue = -1;
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
