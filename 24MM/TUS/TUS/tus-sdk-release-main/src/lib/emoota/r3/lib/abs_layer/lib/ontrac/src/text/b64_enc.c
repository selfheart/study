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
 * @file b64_enc.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ctype.h>

#include <ontrac/text/ascii.h>
#include <ontrac/text/b64_enc.h>
#include <ontrac/ontrac/status_codes.h>
// parasoft-begin-suppress CERT_C-MSC41-a-1 "c0518. This string does not contain sensitive information."
const cstr_t base_64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/=";
// parasoft-end-suppress CERT_C-MSC41-a-1
#define B64_LEN (64U)

int32_t b64_encode(const byte_t* source, int32_t source_len,
        byte_t *dest, int32_t dest_len, bool_t newlines) {
    uint32_t merged = 0;
    ABQ_DECODER(decoder, &raw_codec, source, source_len);
    ABQ_ENCODER(encoder, &ascii_codec, dest, dest_len);
    if((NULL == decoder.source) || (NULL == dest)){
        (void) abq_status_set(EFAULT, false);
        merged = UINT32_MAX;
    }else if((0 > source_len) || (0 > dest_len)){
        // Could be any kind of data, can't attempt to deduce input length
        (void)abq_status_set(EINVAL, false);
        merged = UINT32_MAX;
    }else{
        while((decoder.pos < decoder.max) && (UINT32_MAX != merged)) {
            uint32_t first = (uint32_t) abq_decode_cp(&decoder);
            uint32_t second = (uint32_t) abq_decode_cp(&decoder);
            uint32_t third = (uint32_t) abq_decode_cp(&decoder);
            byte_t b64_char = '\0';
            if (first > (uint32_t)UINT8_MAX) {
                // But decoder.pos < decoder.max ?
                abq_status_set(ERANGE, false);
                merged = UINT32_MAX;
            } else if(encoder.max < (encoder.pos + 4U)) {
                abq_status_set(EOVERFLOW, false);
                merged = UINT32_MAX;
            } else if(second > (uint32_t)UINT8_MAX) {
                merged = (first << 16) ;
                b64_char = base_64_chars[(merged >> 18) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
                b64_char = base_64_chars[(merged >> 12) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
                VITAL_IS_OK(abq_encode_char(&encoder, '='));
                VITAL_IS_OK(abq_encode_char(&encoder, '='));
                (void) abq_status_pop();
                decoder.pos = decoder.max;
            } else if(third > (uint32_t)UINT8_MAX) {
                merged = (first << 16) | (second << 8) ;
                b64_char = base_64_chars[(merged >> 18) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
                b64_char = base_64_chars[(merged >> 12) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
                b64_char = base_64_chars[(merged >> 6) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
                VITAL_IS_OK(abq_encode_char(&encoder, '='));
                (void) abq_status_pop();
                decoder.pos = decoder.max;
            } else {
                merged = (first << 16) | (second << 8) | third;
                // FULL sextet was loaded
                b64_char = base_64_chars[(merged >> 18) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
                b64_char = base_64_chars[(merged >> 12) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
                b64_char = base_64_chars[(merged >> 6) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
                b64_char = base_64_chars[(merged) & 0x3FU];
                VITAL_IS_OK(abq_encode_char(&encoder, b64_char));
            }
            if((UINT32_MAX != merged) && (newlines)
                    && (64UL == (encoder.pos % 66UL))){
                // write a newline every 64 b64 bytes + 2 ("\r\n") bytes
                if(encoder.max < (encoder.pos + 2UL)){
                    (void)abq_status_set(EOVERFLOW, false);
                    merged = UINT32_MAX;
                } else {
                    VITAL_IS_OK(abq_encode_char(&encoder, '\r'));
                    VITAL_IS_OK(abq_encode_char(&encoder, '\n'));
                    // should continue looping through input
                }
            }
        }
    }
    return (UINT32_MAX == merged) ? -1 : (int32_t) encoder.pos;
}

static uint32_t b64_reverse_lookup(int32_t cp){
    static const uint8_t b64_invalid = (uint8_t) 0xFFU;
    static uint8_t b64_reverse_table[128] = {0};
    if(b64_reverse_table[(uint8_t)base_64_chars[1]] != (uint8_t) 1) {
        // need to fill in the reverse lookup table
        //  but first fill in all invalid characters with '-1'
        for (uint8_t i = 0; i < (uint8_t) sizeof(b64_reverse_table); i++) {
            b64_reverse_table[i] = b64_invalid;
        }
        for (uint8_t i = 0; i <= (uint8_t) B64_LEN; i++) {
            b64_reverse_table[(uint8_t)base_64_chars[i]] = i;
        }
        // terminator character translation
        b64_reverse_table[0] = B64_LEN;
    }
    uint32_t rvalue = UINT32_MAX;
    if ((0 <= cp) && (cp < 128)) {
        if(b64_invalid == b64_reverse_table[cp]) {
            rvalue = UINT32_MAX;
        } else {
            rvalue = (uint32_t) b64_reverse_table[cp];
        }
    }
    return rvalue;
}

static uint32_t abq_decode_b64(abq_decoder_t *decoder){
    // skip over whitespace
    int32_t cp = abq_decode_cp(decoder);
    while(ascii_is_space(cp) ){
        // Continue iterating over characters
        cp = abq_decode_cp(decoder);
    }
    return b64_reverse_lookup(cp);
}

int32_t b64_decode(const byte_t* source, int32_t source_len, byte_t *dest, int32_t dest_len) {
    ABQ_DECODER(decoder, &ascii_codec, source, source_len);
    ABQ_ENCODER(encoder, &raw_codec, dest, dest_len);
    uint32_t merged = 0;
    if ((NULL == source) || (NULL == dest)) {
        (void) abq_status_set(EFAULT, false);
        merged = UINT32_MAX;
    } else if (0 > dest_len) {
        (void) abq_status_set(EINVAL, false);
        merged = UINT32_MAX;
    } else {
        while((decoder.pos < decoder.max) && (UINT32_MAX != merged)) {
            uint32_t first = abq_decode_b64(&decoder);
            uint32_t second = abq_decode_b64(&decoder);
            uint32_t third = abq_decode_b64(&decoder);
            uint32_t fourth = abq_decode_b64(&decoder);
            merged = (first << 18) | (second << 12);
            if(B64_LEN == first) {
                // correctly terminated input string
                (void) abq_status_pop();
                decoder.pos = decoder.max;
                merged = 0;
            }else if ((UINT32_MAX == first) || (UINT32_MAX == second)
                    || (UINT32_MAX == third) || (UINT32_MAX == fourth)) {
                (void) abq_status_set(EILSEQ, false);
                merged = UINT32_MAX;
            }else if (third >= B64_LEN) { // third is B64_LEN
                if (encoder.max < (1UL + encoder.pos)) {
                    (void) abq_status_set(EOVERFLOW, false);
                    merged = UINT32_MAX;
                } else {
                    VITAL_IS_OK(abq_encode_char(&encoder,
                            (byte_t)((uint8_t)(merged >> 16))));
                    // '=' character shall be treated as a base64 terminator '\0'
                    decoder.max = decoder.pos;
                }
            } else if (fourth >= B64_LEN) { // fourth is B64_LEN
                if (encoder.max < (2UL + encoder.pos)) {
                    (void) abq_status_set(EOVERFLOW, false);
                    merged = UINT32_MAX;
                } else {
                    merged |= (third << 6);
                    VITAL_IS_OK(abq_encode_char(&encoder,
                            (byte_t)((uint8_t)(merged >> 16))));
                    VITAL_IS_OK(abq_encode_char(&encoder,
                            (byte_t)((uint8_t)(merged >> 8))));
                    // '=' character shall be treated as a base64 terminator '\0'
                    decoder.max = decoder.pos;
                }
            } else { // full set of 4 characters
                if (encoder.max < (3U + encoder.pos)) {
                    (void) abq_status_set(EOVERFLOW, false);
                    merged = UINT32_MAX;
                } else {
                    merged |= (third << 6);
                    merged |= (fourth);
                    VITAL_IS_OK(abq_encode_char(&encoder,
                            (byte_t)((uint8_t)(merged >> 16))));
                    VITAL_IS_OK(abq_encode_char(&encoder,
                            (byte_t)((uint8_t)(merged >> 8))));
                    VITAL_IS_OK(abq_encode_char(&encoder,
                            (byte_t)((uint8_t)(merged))));
                    // should continue looping through input
                }
            }
        }
    }
    return (UINT32_MAX == merged) ? -1 : (int32_t) encoder.pos;
}

