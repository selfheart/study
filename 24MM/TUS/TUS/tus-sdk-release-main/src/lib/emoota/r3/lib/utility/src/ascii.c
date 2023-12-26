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
 */

#include "emoota_utility_types.h"
#include "codec.h"

/* parasoft-begin-suppress CERT_C-DCL06-a-3 Keeping hard coded values in favor of code readability */

static int32_t abq_ascii_decoding (abq_decoder_t *const decoder) {
    int32_t codepoint = SINT_MINUS_ONE;
    if ((NULL == decoder) || (NULL == decoder->source)) {
    } else if(decoder->pos >= decoder->max) {
        codepoint = SINT_ZERO;
    } else {
        codepoint = (int32_t) ((uint8_t) decoder->source[decoder->pos]);
        if (SINT_ZERO == codepoint ) {
            // Previous check for (decoder->pos >= decoder->max)
            // Stop iterating, not a raw character
            decoder->max = decoder->pos;
        } else {
            decoder->pos += (size_t) 1;
        }
    }
    return codepoint;
}

const abq_codec_t abq_ascii_codec = {
        .bytesize = NULL,
        .encode = NULL,
        .decode = abq_ascii_decoding};

/* parasoft-end-suppress CERT_C-DCL06-a-3 */
