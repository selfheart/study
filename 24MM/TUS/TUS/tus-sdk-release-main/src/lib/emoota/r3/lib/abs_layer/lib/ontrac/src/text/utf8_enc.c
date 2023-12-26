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
 * @brief TODO
 *
 */

#include <ontrac/text/utf8_enc.h>
#include <ontrac/ontrac/status_codes.h>

static int32_t utf8_bytesize(int32_t codepoint) {
    int32_t retval = -1;
    if (0 > codepoint) {
        // Invalid codepoint
    } else if (0 == codepoint) {
        retval = 0;
    } else if (codepoint < 0x80) {
        retval = 1;
    } else if (codepoint < 0x800) {
        retval = 2;
    } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
        /* invalid code point (UTF-16 surrogate halves) */
    } else if (codepoint < 0x10000) {
        retval = 3;
    } else if (codepoint <= 0x10FFFF) {
        retval = 4;
    } else {
        // Invalid codepoint
    }
    return retval;
}

static err_t utf8_encoding(abq_encoder_t *encoder, int32_t codepoint) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if (0 > codepoint) {
        retval = EINVAL;
    } else if (NULL == encoder->dest) {
        retval = EFAULT;
    } else {
        uint32_t tmp = 0U;
        uint32_t bitwise = (uint32_t) codepoint;
        if (0 == codepoint) {
            if (encoder->pos >= encoder->max) {
                //retval = NOT_MODIFIED;
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
                encoder->pos += 1U;
            }
        } else if (codepoint < 0x800) {
            if (2U > (encoder->max - encoder->pos)) {
                retval = EOVERFLOW;
            } else {
                tmp = (0xC0U + ((bitwise & 0x7C0U) >> 6));
                encoder->dest[encoder->pos + 0U] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x03FU)));
                encoder->dest[encoder->pos + 1U] = (byte_t) tmp;
                encoder->pos += 2U;
            }
        } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
            /* invalid code point (UTF-16 surrogate halves) */
            retval = EILSEQ;
        } else if (codepoint < 0x10000) {
            if (3U > (encoder->max - encoder->pos)) {
                retval = EOVERFLOW;
            } else {
                tmp = (0xE0U + ((bitwise & 0xF000U) >> 12));
                encoder->dest[encoder->pos + 0U] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x0FC0U) >> 6));
                encoder->dest[encoder->pos + 1U] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x003FU)));
                encoder->dest[encoder->pos + 2U] = (byte_t) tmp;
                encoder->pos += 3U;
            }
        } else if (codepoint <= 0x10FFFF) {
            if (4U > (encoder->max - encoder->pos)) {
                retval = EOVERFLOW;
            } else {
                tmp = (0xF0U + ((bitwise & 0x1C0000U) >> 18));
                encoder->dest[encoder->pos + 0U] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x03F000U) >> 12));
                encoder->dest[encoder->pos + 1U] = (byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x000FC0U) >> 6));
                encoder->dest[encoder->pos + 2U] =(byte_t) tmp;
                tmp = (0x80U + ((bitwise & 0x00003FU)));
                encoder->dest[encoder->pos + 3U] = (byte_t) tmp;
                encoder->pos += 4U;
            }
        } else {
            retval = EILSEQ;
        }
        // write out terminator if space is available
        if(encoder->max > encoder->pos) {
            encoder->dest[encoder->pos] = '\0';
        }
    }
    return retval;
}

static inline bool_t is_utf8_cont(uint8_t ch){
    return ((0x80U <= (uint8_t)(ch)) && ((uint8_t)(ch) <= 0xBFU));
}

static int32_t do_utf8_cont(uint32_t cp, byte_t ch) {
    uint32_t bitwise = (cp << 6) + (((uint32_t)(uint8_t)ch) & 0x3FU);
    return (int32_t) bitwise;
}

static int32_t utf8_decoding (abq_decoder_t *decoder) {
    int32_t codepoint = -1;
    if ((NULL == decoder) || (NULL == decoder->source)) {
        abq_status_set(EFAULT, false);
    } else if(decoder->pos >= decoder->max) {
        codepoint = (int32_t) '\0';
    } else {
        size_t remaining = decoder->max - decoder->pos;
        uint8_t ch = (uint8_t) decoder->source[decoder->pos];
        if (0U == ch) {
            // Previous check for (decoder->pos >= decoder->max)
            codepoint = 0;
            // Stop iterating, not a raw character
            decoder->max = decoder->pos;
        } else if (ch < 0x80U) {
            // Previous check for (decoder->pos >= decoder->max)
            codepoint = (int32_t) ch;
            decoder->pos += 1UL;
        } else if (ch < 0xC0U) {
            /* second, third or fourth byte of a multi-byte
             sequence, i.e. a "continuation byte" */
            abq_status_set(EILSEQ, false);
        } else if (ch == 0xC1U) {
            /* overlong encoding of an ASCII byte */
            abq_status_set(EILSEQ, false);
        } else if (ch < 0xE0U) {
            if (2U > remaining) {
                abq_status_set(ENODATA, true);
            }else if(is_utf8_cont((uint8_t)decoder->source[decoder->pos+1U])){
                ch &= 0x1FU;
                codepoint = (int32_t) ch;
                codepoint = do_utf8_cont((uint32_t)codepoint,
                        decoder->source[decoder->pos+1U]);
                decoder->pos += 2U;
            }else{
                /* not a continuation byte */
                abq_status_set(EILSEQ, false);
            }
        } else if (ch < 0xF0U) {
            /* 3-byte sequence */
            if (3U > remaining) {
                abq_status_set(ENODATA, true);
            }else if((is_utf8_cont((uint8_t)decoder->source[decoder->pos+1U]))
                    && (is_utf8_cont((uint8_t)decoder->source[decoder->pos+2U]))){
                ch &= 0xFU;
                codepoint = (int32_t) ch;
                codepoint = do_utf8_cont((uint32_t)codepoint,
                        decoder->source[decoder->pos+1U]);
                codepoint = do_utf8_cont((uint32_t)codepoint,
                        decoder->source[decoder->pos+2U]);
                if (codepoint < 0x800) {
                    codepoint = -1;
                    abq_status_set(EILSEQ, false);
                } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
                    /* invalid code point (UTF-16 surrogate halves) */
                    codepoint = -1;
                    abq_status_set(EILSEQ, false);
                } else {
                    decoder->pos += 3U;
                    if ((UNICODE_BYTE_ORDER_MARK == codepoint)
                            || (UNICODE_WORD_JOINER == codepoint)) {
                        // FIXME: Recursion not allowed by MISRA
                        codepoint = utf8_codec.decode(decoder);
                    }
                }
            }else{
                /* not a continuation byte */
                abq_status_set(EILSEQ, false);
            }
        } else if (ch < 0xF5U) {
            /* 4-byte sequence */
            if (4U > remaining) {
                abq_status_set(ENODATA, true);
            }else if((is_utf8_cont((uint8_t)decoder->source[decoder->pos+1U]))
                    && (is_utf8_cont((uint8_t)decoder->source[decoder->pos+2U]))
                    && (is_utf8_cont((uint8_t)decoder->source[decoder->pos+3U]))){
                ch &= 0x7U;
                codepoint = (int32_t) ch;
                codepoint = do_utf8_cont((uint32_t)codepoint,
                        decoder->source[decoder->pos+1U]);
                codepoint = do_utf8_cont((uint32_t)codepoint,
                        decoder->source[decoder->pos+2U]);
                codepoint = do_utf8_cont((uint32_t)codepoint,
                        decoder->source[decoder->pos+3U]);

                if ((codepoint < 0x10000) || (codepoint > 0x10FFFF)) {
                    codepoint = -1; /* not in Unicode range */
                    abq_status_set(EILSEQ, false);
                } else if ((0xD800 <= codepoint) && (codepoint <= 0xDFFF)) {
                    /* invalid code point (UTF-16 surrogate halves) */
                    codepoint = -1;
                    abq_status_set(EILSEQ, false);
                }else{
                    decoder->pos += 4U;
                }
            }else{
                /* not a continuation byte */
                abq_status_set(EILSEQ, false);
            }
        } else { /* ch >= 0xF5 */
            /* Restricted (start of 4, 5 or 6 byte sequence) or invalid UTF-8 */
            // todo: check for utf16le and/or utf16be BOM and switch encoder->codec
            // https://en.wikipedia.org/wiki/Byte_order_mark
            abq_status_set(EILSEQ, false);
        }
    }
    return codepoint;
}

const abq_codec_t utf8_codec = {
        .bytesize = utf8_bytesize,
        .encode = utf8_encoding,
        .decode = utf8_decoding
};

int32_t utf8_char_length(const cstr_t buffer) {
    int32_t rvalue = -1;
    if(NULL != buffer){
        uint8_t u = (uint8_t) buffer[0];
        if (u < 0x80U) {
            rvalue = 1;
        } else if (u < 0xC0U) {
          /* second, third or fourth byte of a multi-byte
             sequence, i.e. a "continuation byte" */
            rvalue = -1;
        } else if (u == 0xC1U) {
          /* overlong encoding of an ASCII byte */
            rvalue = -1;
        } else if (u < 0xE0U) {
            /* 2-byte sequence */
            rvalue = 2;
        } else if (u < 0xF0U) {
            /* 3-byte sequence */
            rvalue = 3;
        } else if (u < 0xF5U) {
            /* 4-byte sequence */
            rvalue = 4;
        } else { /* u >= 0xF5 */
            /* Restricted (start of 4-, 5- or 6-byte sequence) or invalid
             UTF-8 */
            rvalue = -1;
        }
    }
    return rvalue;
}
