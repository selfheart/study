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
 * @file ascii.c
 * @date Jan 15, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/text/ascii.h>
#include <ontrac/ontrac/status_codes.h>


bool_t ascii_is_friendly(int32_t cp) {
    bool_t retval = false;
    if ((0x20 <= cp) && (cp <= 0x7e)) {
        // Non-control character
        retval = true;
    } else if ((0x09 <= cp) && (cp <= 0x0D)) {
        // Valid whitespace charater
        retval = true;
    } else {
        // Non-printable ascii
        retval = false;
    }
    return retval;
}

bool_t ascii_is_space(int32_t cp) {
    bool_t retval = false;
    if (0x20 == cp) {
        // ' ' (0x20) character
        retval = true;
    } else if ((0x09 <= cp) && (cp <= 0x0D)) {
        // Valid whitespace control charater
        // '\t' (0x09), '\n' (0x0A), '\v' (0x0B). '\f' (0x0C) and '\r' (0x0D)
        retval = true;
    } else {
        // Non-whitespace character
        retval = false;
    }
    return retval;
}

bool_t ascii_is_alpha(int32_t cp) {
    bool_t retval = false;
    if ((0x41 <= cp) && (cp <= 0x5A)) {
        // in capital letter range between 'A' (0x41) and 'Z' (0x5A) characters
        retval = true;
    } else if ((0x61 <= cp) && (cp <= 0x7A)) {
        // in lower-case letter range between 'a' (0x61) and 'z' (0x7A) characters
        retval = true;
    } else {
        // Non-alpha-numeric character
        retval = false;
    }
    return retval;
}

bool_t ascii_is_decimal(int32_t cp) {

    bool_t retval = false;
    if ((0x30 <= cp) && (cp <= 0x39)) {
        // numeric_characters
        // in numeric range between '0' (0x30) and '9' (0x39) characters
        retval = true;
    }
    return retval;
}

bool_t ascii_is_alnum(int32_t cp) {
    bool_t retval = false;
    if ((ascii_is_alpha(cp)) || (ascii_is_decimal(cp))) {
        retval = true;
    }
    return retval;
}

static int32_t ascii_bytesize(int32_t codepoint) {
    // Use this since we still iterate over unsupported characters
    int32_t retval = -1;
    if (0 > codepoint) {
        retval = -1; // Out of bounds
    }else if(0 == codepoint){
        retval = 0; // terminator
    }else if(0xFF >= codepoint){
        retval = 1;
    }else{
        retval = -1; // Out of bounds
    }
    return retval;
}

static err_t ascii_encoding(abq_encoder_t *encoder, int32_t codepoint) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if ((0 > codepoint) || (0xFF < codepoint)) {
        retval = EINVAL;
    } else if(NULL == encoder->dest) {
        retval = EFAULT;
    } else if(0 == codepoint) {
        if (encoder->max > encoder->pos) {
            // append a terminator to next position if able
            encoder->dest[encoder->pos] = '\0';
        }else{
            retval = EOVERFLOW;
        }
    } else if (encoder->pos >= encoder->max) {
        retval = EOVERFLOW;
    } else {
        if(ascii_is_friendly(codepoint)) {
            encoder->dest[encoder->pos] = (byte_t) codepoint;
        }else{
            // Print a visible replacement instead of unsupported/control characters
            encoder->dest[encoder->pos] = '.';
        }
        encoder->pos += (size_t) 1;
        if (encoder->max > encoder->pos) {
            // append a terminator to next position if able
            encoder->dest[encoder->pos] = '\0';
        }
    }
    return retval;
}

static int32_t ascii_decoding (abq_decoder_t *decoder) {
    int32_t codepoint = -1;
    if ((NULL == decoder) || (NULL == decoder->source)) {
        abq_status_set(EFAULT, false);
    } else if(decoder->pos >= decoder->max) {
        codepoint = 0;
    } else {
        codepoint = (int32_t) ((uint8_t) decoder->source[decoder->pos]);
        if (0 == codepoint ) {
            // Previous check for (decoder->pos >= decoder->max)
            // Stop iterating, not a raw character
            decoder->max = decoder->pos;
        } else {
            decoder->pos += (size_t) 1;
        }
    }
    return codepoint;
}

const abq_codec_t ascii_codec = {
        .bytesize = ascii_bytesize,
        .encode = ascii_encoding,
        .decode = ascii_decoding
};
