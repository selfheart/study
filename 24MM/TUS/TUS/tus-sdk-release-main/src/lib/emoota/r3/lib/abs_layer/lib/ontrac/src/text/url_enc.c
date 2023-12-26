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
 * @file url_enc.c
 * @date Jan 16, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ctype.h>

#include <ontrac/text/ascii.h>
#include <ontrac/text/hex_enc.h>
#include <ontrac/text/url_enc.h>
#include <ontrac/ontrac/status_codes.h>

static inline bool_t url_needs_esc(int32_t codepoint) {
    bool_t retval = true;
    if (ascii_is_alnum(codepoint)) {
        retval = false;
    } else if(0x2A == codepoint) {
        // '*' (0x2A) character
        retval = false;
    } else if(0x2D == codepoint) {
        // '-' (0x2D) character
        retval = false;
    } else if(0x5F == codepoint) {
        // '_' (0x5F) character
        retval = false;
    } else if(0x7E == codepoint) {
        // '~' (0x7E) character
        retval = false;
    } else if(0x2E == codepoint) {
        // '.' (0x2E) character
        retval = false;
    } else if(0x2F == codepoint) {
        // '/' (0x2F) character
        retval = false;
    } else {
        // will escape all other characters
        retval = true;
    }
    return retval;
}

static int32_t url_bytesize(int32_t codepoint) {
    int32_t retval = -1;
    if (0 > codepoint) {
        // Invalid codepoint
    } else if (0 == codepoint) {
        retval = 0;
    } else if (codepoint < 0xFF){
        if (url_needs_esc(codepoint)) {
            retval = 3;
        } else {
            retval = 1;
        }
    } else {
        // '%' escaped unicode is not supported
        retval = -1;
    }
    return retval;
}

static err_t url_encoding (abq_encoder_t *encoder, int32_t codepoint){
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if (0 > codepoint) {
        retval = EINVAL;
    } else if(NULL == encoder->dest){
        retval = EFAULT;
    } else if (encoder->pos >= encoder->max) {
        retval = EOVERFLOW;
    } else if (codepoint <= 0xFF) {
        // encoding raw byte data
        if(0 == codepoint) {
            // Completed iteration
            encoder->dest[encoder->pos] = '\0';
        } else if(url_needs_esc(codepoint)) {
            if(3U > (encoder->max - encoder->pos)) {
                retval = EOVERFLOW;
            } else {
                encoder->dest[encoder->pos+0U] = '%';
                encoder->dest[encoder->pos+1U] = LEFT_NIBBLE_CHAR(codepoint);
                encoder->dest[encoder->pos+2U] = RIGHT_NIBBLE_CHAR(codepoint);
                encoder->pos += 3U;
            }
        } else {
            encoder->dest[encoder->pos] = (byte_t)codepoint;
            encoder->pos += 1U;
        }
        if (encoder->max > encoder->pos) {
            // append a terminator to next position if able
            encoder->dest[encoder->pos] = '\0';
        }
    }else{
        // '%' escaped unicode is not supported
        retval = ENOSYS;
    }
    return retval;
}

static int32_t url_decoding (abq_decoder_t *decoder) {
    int32_t codepoint = -1;
    if ((NULL == decoder) || (NULL == decoder->source)) {
        abq_status_set(EFAULT, false);
    } else if(decoder->pos >= decoder->max) {
        codepoint = (int32_t) '\0';
    } else {
        byte_t ch = (byte_t) decoder->source[decoder->pos];
        if('\0'== ch){
            // Previous check for (decoder->pos >= decoder->max)
            codepoint = (int32_t) '\0';
            // Stop iterating, not a raw character
            decoder->max = decoder->pos;
        }else if ('%' == ch) {
            if (3U > (decoder->max - decoder->pos)) {
                abq_status_set(ENODATA, true);
            } else if('u' == decoder->source[decoder->pos+1U]) {
                // '%' escaped unicode is not supported
                abq_status_set(ENOSYS, false);
            } else {
                if(1 != hex_decode(&decoder->source[decoder->pos+1U], 2, &ch, 1)){
                    abq_status_set(EIO, false);
                }else{
                    // Successfully decoded the hex character
                    decoder->pos += 3U;
                    codepoint = (int32_t) ((uint8_t) ch);
                }
            }
        }else if('+' == ch){
            codepoint = (int32_t)' ';
            decoder->pos += 1U;
        }else if(ascii_is_friendly((int32_t)(uint8_t)ch)) {
            codepoint = (int32_t)(uint8_t)ch;
            decoder->pos += 1U;
        }else{
            abq_status_set(EILSEQ, false);
        }
    }
    return codepoint;
}

const abq_codec_t url_codec = {
        .bytesize = url_bytesize,
        .encode = url_encoding,
        .decode = url_decoding
};

int32_t url_encode(
        cstr_t source, int32_t source_len,
        char *dest, int32_t dest_len) {
    ABQ_DECODER(decoder, byte_codec(source_len), source, source_len);
    ABQ_ENCODER(encoder,&url_codec, dest, dest_len);
    err_t err = abq_decode_encode(&decoder, &encoder);
    if (EXIT_SUCCESS != err) {
        abq_status_set(err, false);
    }
    return (EXIT_SUCCESS != err) ? -1 : (int32_t) encoder.pos;
}

int32_t url_decode (
        cstr_t source, int32_t source_len,
        char *dest, int32_t dest_len) {
    ABQ_DECODER(decoder, &url_codec, source, source_len);
    ABQ_ENCODER(encoder,byte_codec(dest_len), dest, dest_len);
    err_t err = abq_decode_encode(&decoder, &encoder);
    if (EXIT_SUCCESS != err) {
        abq_status_set(err, false);
    }
    return (EXIT_SUCCESS != err) ? -1 : (int32_t) encoder.pos;
}
