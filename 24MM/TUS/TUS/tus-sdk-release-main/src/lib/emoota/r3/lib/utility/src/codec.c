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
 * @file codec.c
 * @date Jan 15, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 */
#include "codec.h"
#include "emoota_utility_types.h"
#include "hex_enc.h"
#include <string.h>

/* parasoft-begin-suppress CERT_C-DCL06-a-3 Keeping hard coded values in favor of code readability */


// limit to the total number of digits to format/parse for a given number
#define MAX_DIGITS_PER_NUMBER ((size_t) 32U)

static err_t abq_raw_encoding(abq_encoder_t *const encoder, const int32_t codepoint) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if ((SINT_ZERO > codepoint) || (INT_0xFF < codepoint)) {
        retval = EINVAL;
    } else if (NULL == encoder->dest) {
        retval = EFAULT;
    } else {
        if (encoder->pos >= encoder->max) {
            retval = EOVERFLOW;
        } else {
            encoder->dest[encoder->pos] = (byte_t) ((uint8_t) codepoint);
            encoder->pos += (size_t) SINT_ONE;
            if (encoder->max > encoder->pos) {
                // append a terminator to next position if able
                encoder->dest[encoder->pos] = '\0';
            }
        }
    }
    return retval;
}

const abq_codec_t abq_raw_codec = {
        .bytesize = NULL,
        .encode = abq_raw_encoding,
        .decode = NULL};

static err_t abq2_decode_encode(abq_decoder_t *const decoder, abq_encoder_t *const encoder) {
    err_t retval = EXIT_SUCCESS;
    if ((NULL == decoder) || (NULL == encoder)) {
        retval = EFAULT;
    } else {
        int32_t codepoint = SINT_ZERO;
        const size_t orig_decoder = decoder->pos;
        const size_t orig_encoder = encoder->pos;
        while (decoder->max > decoder->pos) {
            codepoint = abq2_decode_cp(decoder);
            if (SINT_ZERO >= codepoint) {
                if (SINT_ZERO == codepoint) {
                    decoder->max = decoder->pos;
                } else {
                    retval = EIO;
                }
            } else {
                // Write out the codepoint we just read in
                retval = abq_encode_cp(encoder, codepoint);
            }
            if (EXIT_SUCCESS != retval) {
                decoder->pos = orig_decoder;
                encoder->pos = orig_encoder;
                break;
            }
        }
    }
    return retval;
}

err_t abq2_encode_text(abq_encoder_t *const encoder, abq_codec_t *const codec, cstr_t const text, const int32_t length) {
    err_t retval = EXIT_SUCCESS;
    if ((NULL == encoder) || (NULL == codec) || (NULL == text)) {
        retval = EFAULT;
    } else {
        ABQ_DECODER(decoder, codec, text, length)
        retval = abq2_decode_encode(&decoder, encoder);
    }
    return retval;
}

typedef struct {
    uint64_t value;
    uint8_t radix;
    bool_t negative;
    size_t digit_count;
    int32_t digits[MAX_DIGITS_PER_NUMBER];
} abq_digitizer_t;

/** Fills in digit character for a integer value, in REVERSE order */
static err_t abq_digitize_int(abq_digitizer_t *const digitizer) {
    err_t retval = EXIT_SUCCESS;
    if (NULL != digitizer) {
        digitizer->digit_count = SINT_ZERO;
        do {
            if (digitizer->digit_count >= MAX_DIGITS_PER_NUMBER) {
                retval = ENOBUFS;
                break;
            }
            digitizer->digits[digitizer->digit_count] = (int32_t) (uint8_t) abq_upper_hex_digits[digitizer->value % (uint64_t) digitizer->radix];
            digitizer->value /= (uint64_t) digitizer->radix;
            digitizer->digit_count += (size_t) SINT_ONE;
        } while (0UL != digitizer->value);
    } else {
        retval = EFAULT;
    }
    return retval;
}

static err_t abq2_encode_digitizer(abq_encoder_t *const encoder,
                                   abq_digitizer_t *const digitizer, const byte_t pad, const size_t char_count) {
    err_t retval = EXIT_SUCCESS;
    if ((NULL != encoder) && (NULL != digitizer)) {   /* MISRAC2012-DIR_4_1-f-2 "digitizer is checked for NULL here and in the invoking function" */
        const size_t orig_pos = encoder->pos;     /* MISRAC2012-DIR_4_1-b-2 "encoder is checked for NULL here and in the invoking function" */
        if (digitizer->negative) {
            retval = abq2_encode_char(encoder, '-');
        }
        while ((EXIT_SUCCESS == retval) && (char_count > digitizer->digit_count) && ((encoder->pos - orig_pos) < (char_count - digitizer->digit_count))) {
            retval = abq2_encode_char(encoder, pad);
        }
        while ((EXIT_SUCCESS == retval) && (digitizer->digit_count > (size_t) SINT_ZERO)) {
            // Decrement the digit_count and encode the character
            digitizer->digit_count -= (size_t) SINT_ONE;
            // Encode the integer digit character
            retval = abq_encode_cp(encoder,
                                   digitizer->digits[digitizer->digit_count]);
        }
        if (EXIT_SUCCESS != retval) {
            // Reset encoder to orig_pos
            encoder->pos = orig_pos;
            // Return error value as is
        }
    } else {
        retval = EFAULT;
    }
    return retval;
}

static err_t abq_digitize_and_encode(abq_encoder_t *const encoder,
                                     abq_digitizer_t *const digitizer, const byte_t pad, const size_t char_count) {
    err_t retval = EXIT_SUCCESS;

    if ((NULL != encoder) && (NULL != digitizer)) {
        retval = abq_digitize_int(digitizer);
        // check global status
        if (EXIT_SUCCESS != retval) {
            // Return error value as is
        } else {
            retval = abq2_encode_digitizer(encoder, digitizer, pad, char_count);
        }
    } else {
        retval = EFAULT;
    }
    return retval;
}

static err_t abq2_encode_int(abq_encoder_t *const encoder, const int64_t value, const abq_radix_t radix) {
    err_t retval = CHECK_NULL(encoder);
    // write to the end of tmp so that we don't have to reverse later
    if (EXIT_SUCCESS != retval) {
        // Return error code
    } else if (abq2_radix_is_invalid(radix)) {
        retval = EINVAL;// Unsupported RADIX
    } else {
        abq_digitizer_t digitizer;
        (void) memset(&digitizer, SINT_ZERO, sizeof(abq_digitizer_t));
        digitizer.radix = (uint8_t) radix;
        digitizer.value = (uint64_t) value;
        if (SINT_ZERO > value) {
            digitizer.negative = true;
            digitizer.value = (~digitizer.value) + 1UL;
        } else {
            digitizer.negative = false;
        }
        retval = abq_digitize_and_encode(encoder,
                                         &digitizer, '0', UINT_ZERO);
    }
    return retval;
}

err_t abq2_encode_uint(abq_encoder_t *const encoder, const uint64_t value, const abq_radix_t radix) {
    err_t retval = CHECK_NULL(encoder);
    // write to the end of tmp so that we don't have to reverse later
    if (EXIT_SUCCESS != retval) {
        // Return error code
    } else if (abq2_radix_is_invalid(radix)) {
        retval = EINVAL;// Unsupported RADIX
    } else {
        abq_digitizer_t digitizer;
        (void) memset(&digitizer, SINT_ZERO, sizeof(abq_digitizer_t));
        digitizer.radix = (uint8_t) radix;
        digitizer.negative = false;
        digitizer.value = value;
        retval = abq_digitize_and_encode(encoder,
                                         &digitizer, '0', UINT_ZERO);
    }
    return retval;
}


static err_t abq2_encode_decimal(abq_encoder_t *const encoder, const number_t value) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error code
    } else {
        const int64_t integer = (int64_t) value;
        // encode the integer portion off the number
        retval = abq2_encode_int(encoder, integer, DECIMAL_RADIX);
    }
    return retval;
}

err_t abq2_encode_number(abq_encoder_t *const encoder, const number_t value) {
    return abq2_encode_decimal(encoder, value);
}

/* parasoft-end-suppress CERT_C-DCL06-a-3 */
