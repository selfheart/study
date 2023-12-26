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
 * @file utf8_enc.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 */

#include "utf8_enc.h"

/* parasoft-begin-suppress CERT_C-DCL06-a-3 Keeping hard coded values in favor of code readability */

static err_t abq_utf8_encoding(abq_encoder_t *const encoder, const int32_t codepoint) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if (SINT_ZERO > codepoint) {
        retval = EINVAL;
    } else if (NULL == encoder->dest) {
        retval = EFAULT;
    } else {
        uint32_t tmp = UINT_ZERO;
        const uint32_t bitwise = (uint32_t) codepoint;
        if (SINT_ZERO == codepoint) {
            if (encoder->pos >= encoder->max) {
                retval = EOVERFLOW;
            } else {
                // Completed iteration
                encoder->dest[encoder->pos] = '\0';
            }
        } else if (codepoint < 0x80) {
            if (encoder->pos >= encoder->max) {
                retval = EOVERFLOW;
            } else {
                encoder->dest[encoder->pos] = (byte_t) codepoint;
                encoder->pos += UINT_ONE;
            }
        } else if (codepoint < 0x800) {
            if (UINT_TWO > (encoder->max - encoder->pos)) {
                retval = EOVERFLOW;
            } else {
                tmp = (0xC0U + ((bitwise & 0x7C0U) >> UINT_SIX));
                encoder->dest[encoder->pos + UINT_ZERO] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x03FU)));
                encoder->dest[encoder->pos + UINT_ONE] = (byte_t) tmp;
                encoder->pos += UINT_TWO;
            }
        } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
            /* invalid code point (UTF-16 surrogate halves) */
            retval = EILSEQ;
        } else if (codepoint < 0x10000) {
            if (UINT_THREE > (encoder->max - encoder->pos)) {
                retval = EOVERFLOW;
            } else {
                tmp = (0xE0U + ((bitwise & 0xF000U) >> 12U));
                encoder->dest[encoder->pos + UINT_ZERO] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x0FC0U) >> UINT_SIX));
                encoder->dest[encoder->pos + UINT_ONE] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x003FU)));
                encoder->dest[encoder->pos + UINT_TWO] = (byte_t) tmp;
                encoder->pos += UINT_THREE;
            }
        } else if (codepoint <= 0x10FFFF) {
            if (UINT_FOUR > (encoder->max - encoder->pos)) {
                retval = EOVERFLOW;
            } else {
                tmp = (0xF0U + ((bitwise & 0x1C0000U) >> 18U));
                encoder->dest[encoder->pos + UINT_ZERO] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x03F000U) >> 12U));
                encoder->dest[encoder->pos + UINT_ONE] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x000FC0U) >> UINT_SIX));
                encoder->dest[encoder->pos + UINT_TWO] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x00003FU)));
                encoder->dest[encoder->pos + UINT_THREE] = (byte_t) tmp;
                encoder->pos += UINT_FOUR;
            }
        } else {
            retval = EILSEQ;
        }
        // write out terminator if space is available
        if (encoder->max > encoder->pos) {
            encoder->dest[encoder->pos] = '\0';
        }
    }
    return retval;
}

static inline bool_t abq_is_utf8_cont(const uint8_t ch){
    return ((0x80U <= (uint8_t)(ch)) && ((uint8_t)(ch) <= 0xBFU));
}

static int32_t abq_do_utf8_cont(const uint32_t cp, const byte_t ch) {
    const uint32_t bitwise = (cp << UINT_SIX) + (((uint32_t)(uint8_t)ch) & 0x3FU);
    return (int32_t) bitwise;
}

static int32_t abq_utf8_decoding (abq_decoder_t *const decoder) {
    int32_t codepoint = SINT_MINUS_ONE;
    if ((NULL == decoder) || (NULL == decoder->source)) {
    } else if(decoder->pos >= decoder->max) {
        codepoint = (int32_t) '\0';
    } else {
        const size_t remaining = decoder->max - decoder->pos;
        uint8_t ch = (uint8_t) decoder->source[decoder->pos];
        if (UINT_ZERO == ch) {
            // Previous check for (decoder->pos >= decoder->max)
            codepoint = SINT_ZERO;
            // Stop iterating, not a raw character
            decoder->max = decoder->pos;
        } else if (ch < 0x80U) {
            // Previous check for (decoder->pos >= decoder->max)
            codepoint = (int32_t) ch;
            decoder->pos += 1UL;
        } else if (ch < 0xC0U) {
            /* second, third or fourth byte of a multi-byte
             sequence, i.e. a "continuation byte" */
            codepoint = (int32_t) ch;
            decoder->pos += 1UL;
        } else if (ch == 0xC1U) {
            /* overlong encoding of an ASCII byte */
            codepoint = (int32_t) ch;
            decoder->pos += 1UL;
        } else if (ch < 0xE0U) {
            if (UINT_TWO > remaining) {
            }else if(abq_is_utf8_cont((uint8_t) decoder->source[decoder->pos + UINT_ONE])){
                ch &= 0x1FU;
                codepoint = (int32_t) ch;
                codepoint = abq_do_utf8_cont((uint32_t) codepoint,
                                             decoder->source[decoder->pos + UINT_ONE]);
                decoder->pos += UINT_TWO;
            }else{
                /* not a continuation byte */
                codepoint = (int32_t) ch;
                decoder->pos += 1UL;
            }
        } else if (ch < 0xF0U) {
            /* 3-byte sequence */
            if (UINT_THREE > remaining) {
            }else if((abq_is_utf8_cont((uint8_t) decoder->source[decoder->pos + UINT_ONE]))
                    && (abq_is_utf8_cont((uint8_t) decoder->source[decoder->pos + UINT_TWO]))){
                ch &= 0xFU;
                codepoint = (int32_t) ch;
                codepoint = abq_do_utf8_cont((uint32_t) codepoint,
                                             decoder->source[decoder->pos + UINT_ONE]);
                codepoint = abq_do_utf8_cont((uint32_t) codepoint,
                                             decoder->source[decoder->pos + UINT_TWO]);
                if (codepoint < 0x800) {
                    codepoint = SINT_MINUS_ONE;
                } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
                    /* invalid code point (UTF-16 surrogate halves) */
                    codepoint = SINT_MINUS_ONE;
                } else {
                    decoder->pos += UINT_THREE;
                    if ((UNICODE_BYTE_ORDER_MARK == codepoint)
                            || (UNICODE_WORD_JOINER == codepoint)) {
                        codepoint = abq_utf8_codec.decode(decoder);
                    }
                }
            }else{
                /* not a continuation byte */
                codepoint = (int32_t) ch;
                decoder->pos += 1UL;
            }
        } else if (ch < 0xF5U) {
            /* 4-byte sequence */
            if (UINT_FOUR > remaining) {
            }else if((abq_is_utf8_cont((uint8_t) decoder->source[decoder->pos + UINT_ONE]))
                    && (abq_is_utf8_cont((uint8_t) decoder->source[decoder->pos + UINT_TWO]))
                    && (abq_is_utf8_cont((uint8_t) decoder->source[decoder->pos + UINT_THREE]))){
                ch &= 0x7U;
                codepoint = (int32_t) ch;
                codepoint = abq_do_utf8_cont((uint32_t) codepoint,
                                             decoder->source[decoder->pos + UINT_ONE]);
                codepoint = abq_do_utf8_cont((uint32_t) codepoint,
                                             decoder->source[decoder->pos + UINT_TWO]);
                codepoint = abq_do_utf8_cont((uint32_t) codepoint,
                                             decoder->source[decoder->pos + UINT_THREE]);

                if ((codepoint < 0x10000) || (codepoint > 0x10FFFF)) {
                    codepoint = SINT_MINUS_ONE; /* not in Unicode range */
                } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
                    /* invalid code point (UTF-16 surrogate halves) */
                    codepoint = SINT_MINUS_ONE;
                }else{
                    decoder->pos += UINT_FOUR;
                }
            }else{
                /* not a continuation byte */
                codepoint = (int32_t) ch;
                decoder->pos += 1UL;
            }
        } else { /* ch >= 0xF5 */
            /* Restricted (start of 4, 5 or 6 byte sequence) or invalid UTF-8 */
            codepoint = (int32_t) ch;
            decoder->pos += 1UL;
        }
    }
    return codepoint;
}

const abq_codec_t abq_utf8_codec = {
        .bytesize = NULL,
        .encode = abq_utf8_encoding,
        .decode = abq_utf8_decoding};


#ifdef __cplusplus
const abq_codec_t *const get_utf8_codec() {
    return &abq_utf8_codec;
}
#endif

/* parasoft-end-suppress CERT_C-DCL06-a-3 */

