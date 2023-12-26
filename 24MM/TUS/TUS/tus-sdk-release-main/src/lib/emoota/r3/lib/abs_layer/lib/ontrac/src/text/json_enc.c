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
 * @file json_enc.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/text/json_enc.h>
#include <ontrac/text/hex_enc.h>
#include <ontrac/text/utf8_enc.h>
#include <ontrac/ontrac/status_codes.h>

static int32_t json_bytesize (int32_t codepoint) {
    int32_t retval = -1;
    if (0 > codepoint) {
        // Invalid codepoint
    } else if (0 == codepoint) {
        retval = 0;
    } else if (codepoint < 0x80) {
        // ASCII codepoint
        switch ((byte_t)codepoint) {
        case '\b':
            retval = 2; // Escaped char
            break;
        case '\f':
            retval = 2; // Escaped char
            break;
        case '\n':
            retval = 2; // Escaped char
            break;
        case '\r':
            retval = 2; // Escaped char
            break;
        case '\t':
            retval = 2; // Escaped char
            break;
        case '\v':
            retval = 2; // Escaped char
            break;
        case '\'':
            retval = 2; // Escaped char
            break;
        case '"':
            retval = 6; // One utf16 escaped value
            break;
        case '\\':
            retval = 2; // Escaped char
            break;
        default:
            if (codepoint < (int32_t) ' ') {
                // Command character
                retval = 6; // One utf16 escaped value
            } else {
                retval = 1;
            }
            break;
        }
#ifdef JSON_ESCAPE_UNICODE
    } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
        /* invalid code point (UTF-16 surrogate halves) */
    } else if (codepoint < 0x10000) {
        retval = 6; // One utf16 escaped value
    } else if (codepoint <= 0x10FFFF) {
        retval = 12;// Two utf16 escaped values
    } else {
        // Invalid codepoint
    }
#else /* !JSON_ESCAPE_UNICODE */
    } else {
        retval = utf8_codec.bytesize(codepoint);
#endif /* !JSON_ESCAPE_UNICODE */
    }
    return retval;
}

static inline err_t json_esc_ascii(abq_encoder_t *encoder, byte_t octet) {
    err_t retval = EXIT_SUCCESS;
    if(2U > (encoder->max - encoder->pos)) {
        retval = EOVERFLOW;
    }else{
        encoder->dest[encoder->pos+0U]='\\';
        encoder->dest[encoder->pos+1U]= octet;
        encoder->pos += 2U;
    }
    return retval;
}

static inline err_t json_esc_utf16(abq_encoder_t *encoder, uint16_t doublet) {
    err_t retval = EXIT_SUCCESS;
    if(6U > (encoder->max - encoder->pos)) {
        retval = EOVERFLOW;
    }else{
        encoder->dest[encoder->pos+0U]='\\';
        encoder->dest[encoder->pos+1U]= 'u';
        encoder->dest[encoder->pos+2U]
            = upper_hex_digits[(doublet>>12U) & 0xFU];
        encoder->dest[encoder->pos+3U]
            = upper_hex_digits[(doublet>>8U) & 0xFU];
        encoder->dest[encoder->pos+4U]
            = upper_hex_digits[(doublet>>4U) & 0xFU];
        encoder->dest[encoder->pos+5U]
            = upper_hex_digits[doublet & 0xFU];
        encoder->pos += 6U;
    }
    return retval;
}

static err_t json_encoding (abq_encoder_t *encoder, int32_t codepoint) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if (0 > codepoint) {
        retval = EINVAL;
    } else if(NULL == encoder->dest){
        retval = EFAULT;
    } else {
        // Expect begin the JSON string to be handled externally
        //  no longer writes '\"' character at 0 position
        if(0 == codepoint) {
            // Complete the JSON string on terminator
            if (2U > (encoder->max - encoder->pos)) {
                retval = EOVERFLOW;
            } else {
                encoder->dest[encoder->pos+0U] = '\"';
                encoder->dest[encoder->pos+1U] = '\0';
                encoder->pos += 1U;
                // Will force an error if another encode attempt is made
                encoder->max = encoder->pos;
            }
        } else if (codepoint < 0x80) {
            // ASCII codepoint
            switch ((byte_t)codepoint) {
            case '\b':
                retval = json_esc_ascii(encoder, 'b');
                break;
            case '\f':
                retval = json_esc_ascii(encoder, 'f');
                break;
            case '\n':
                retval = json_esc_ascii(encoder, 'n');
                break;
            case '\r':
                retval = json_esc_ascii(encoder, 'r');
                break;
            case '\t':
                retval = json_esc_ascii(encoder, 't');
                break;
            case '\v':
                retval = json_esc_ascii(encoder, 'v');
                break;
            case '\'':
                retval = json_esc_ascii(encoder, '\'');
                break;
            case '"':
                retval = json_esc_utf16(encoder, (uint16_t) codepoint);
                break;
            case '\\':
                retval = json_esc_ascii(encoder, '\\');
                break;
            default:
                if (codepoint < (int32_t)' ') {
                    // Command character
                    retval = json_esc_utf16(encoder, (uint16_t) codepoint);
                } else if (1U > (encoder->max - encoder->pos)) {
                    retval = EOVERFLOW;
                } else {
                    encoder->dest[encoder->pos] = (byte_t)codepoint;
                    encoder->pos += 1UL;
                }
                break;
            }
#ifdef JSON_ESCAPE_UNICODE
        } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
            /* invalid code point (UTF-16 surrogate halves) */
            retval = EINVAL;
        } else if (codepoint < 0x10000) {
            retval = json_esc_utf16(encoder, (uint16_t) codepoint);
        } else if (codepoint <= 0x10FFFF) {
            // a UTF-16 surrogate pair
            // https://en.wikipedia.org/wiki/UTF-16
            uint32_t surrogate_pair = ((uint32_t)codepoint) - 0x10000U;
            // Write out the high-surrogate
            retval = json_esc_utf16(encoder, (uint16_t) (0xD800U + ((surrogate_pair >> 10) & 0x03FFU)));
            if(EXIT_SUCCESS == retval) {
                // Then write out the low-surrogate
                retval = json_esc_utf16(encoder, (uint16_t) (0xDC00U + (surrogate_pair & 0x03FFU)));
            }
        } else {
            retval = EINVAL;
#else /* !JSON_ESCAPE_UNICODE */
        } else {
            retval = utf8_codec.encode(encoder, codepoint);
#endif /* !JSON_ESCAPE_UNICODE */
        }
        // write out terminator if space is available
        if (encoder->max > encoder->pos) {
            encoder->dest[encoder->pos] = '\0';
        }
    }
    return retval;
}

static uint32_t json_decode_utf16(cstr_t source, size_t remaining) {
    uint32_t utf16 = UINT32_MAX;
    if(7U > remaining) {
        // 6 bytes of encoded character + 1 byte for final JSON quotation
        (void)abq_status_set(ENODATA, true);
    }else{
        byte_t buffer[2];
        ABQ_VITAL('\\' == source[0]);
        ABQ_VITAL('u' == source[1]);
        if(hex_decode(&source[2], 4, buffer, 2) != 2){
            (void)abq_status_set(EILSEQ, false);
        } else {
            utf16 = (((uint32_t)(uint8_t) buffer[0]) & 0xFFU) << 8;
            utf16 |=  (((uint32_t)(uint8_t) buffer[1]) & 0xFFU);
        }
    }
    return utf16;
}

static int32_t json_decode_escaped(abq_decoder_t *decoder, size_t remaining) {
    int32_t codepoint = -1;
    // escape character, parse codepoint
    if(3U > remaining) {
        // Minimum of '\' + ch + '"'
        abq_status_set(ENODATA, true);
    } else {
        ABQ_VITAL('\\' == decoder->source[decoder->pos]);
        switch(decoder->source[decoder->pos+1U]) {
            case 'b':
                codepoint = (int32_t)'\b';
                decoder->pos += 2U;
                break;
            case 'f':
                codepoint = (int32_t) '\f';
                decoder->pos += 2U;
                break;
            case 'n':
                codepoint = (int32_t) '\n';
                decoder->pos += 2U;
                break;
            case 'r':
                codepoint = (int32_t) '\r';
                decoder->pos += 2U;
                break;
            case 't':
                codepoint = (int32_t) '\t';
                decoder->pos += 2U;
                break;
            case 'v':
                codepoint = (int32_t) '\v';
                decoder->pos += 2U;
                break;
            case 'u':
            {
                uint32_t high_surrogate = json_decode_utf16(&decoder->source[decoder->pos], remaining);
                if (((uint32_t) UINT16_MAX) < high_surrogate) {
                    // Return error as is
                } else if ((high_surrogate >= 0xd800U) && (high_surrogate <= 0xdbffU)) {
                    high_surrogate &= 0x3ffU; // or - 0xD800
                    // Read in the low surrogate
                    uint32_t low_surrogate = json_decode_utf16(&decoder->source[decoder->pos + 6U], (remaining - 6U));
                    if (((uint32_t) UINT16_MAX) < low_surrogate) {
                        // Return error as is
                    } else if ((low_surrogate >= 0xdc00U) && (low_surrogate <= 0xdfffU)) {
                        // decode low_surrogate
                        low_surrogate &= 0x3ffU; // or - 0xDC00
                        // Combine high and low surrogate values into final unicode
                        low_surrogate |= (high_surrogate << 10U);
                        codepoint = (int32_t) low_surrogate;
                        codepoint += 0x10000;
                        decoder->pos += 12U;
                    } else {
                        abq_status_set(EILSEQ, false);
                    }
                } else {
                    codepoint = (int32_t) high_surrogate;
                    decoder->pos += 6U;
                }
            }
                break;
            default:
                // assume '\"' '\'', '/' etc.
                codepoint = (int32_t) (uint8_t) decoder->source[decoder->pos+1U];
                decoder->pos += 2U;
                break;
        }
    }
    return codepoint;
}

static int32_t json_decoding (abq_decoder_t *decoder) {
    int32_t codepoint = -1;
    if ((NULL == decoder) || (NULL == decoder->source)) {
        abq_status_set(EFAULT, false);
    } else if(decoder->pos >= decoder->max) {
        // Must be terminated by a quotation
        abq_status_set(ENODATA, true);
    } else {
        // Updated to assume that the initial '\"' character has already been parsed
        switch( decoder->source[decoder->pos] ) {
        case '\0':
            // Expecting to terminate on a quotation
            abq_status_set(ENODATA, true);
            codepoint = -1;
            break;
        case '\"':
            // Termination character
            codepoint = (int32_t)'\0';
            decoder->pos += 1UL;
            // Will force an error if another encode attempt is made
            decoder->max = decoder->pos;
            break;
        case '\\':
            codepoint = json_decode_escaped(decoder, (decoder->max - decoder->pos));
            break;
        default:
            // temporary use as char-length (unless error)
            codepoint = utf8_char_length(&decoder->source[decoder->pos]);
            if (0 > codepoint) {
                // Return error as is
            } else if ((((size_t)codepoint) + 1U) >
                    (decoder->max - decoder->pos)) {
                // Not possible to read additional closing quotation
                abq_status_set(ENODATA, true);
                codepoint = -1;
            } else {
                // JSON escaped characters have been tested for
                //  now we attempt to read in any valid UTF8 character
                codepoint = utf8_codec.decode(decoder);
            }
            break;
        }
    }
    return codepoint;
}

const abq_codec_t json_codec = {
        .bytesize = json_bytesize,
        .encode = json_encoding,
        .decode = json_decoding
};
