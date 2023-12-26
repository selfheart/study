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
 * @file b64_enc.h
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_TEXT_B64_ENC_H_
#define ONTRAC_TEXT_B64_ENC_H_

#include <ontrac/text/codec.h>

#define B64_ENCODED_LENGTH(decoded_length) (4L*(((int64_t)(decoded_length)+2L)/3L))

extern const cstr_t base_64_chars;

/**
 * @brief encodes raw octet data (base 256) into base64 https://en.wikipedia.org/wiki/Base64
 *
 * @param source: raw data to be encoded
 * @param source_len: number of octets to be encoded
 * @param dest: byte array to place data once it has been encoded
 * @param dest_len: maximum number of octets that can be written to dest
 * @param newlines: should we write a newline after each set of 64 encoded characters (PEM files)
 * @return number of encoded octets written to dest, or -1 on error (errno is set)
 */
extern int32_t b64_encode(const byte_t* source, int32_t source_len,
        byte_t *dest, int32_t dest_len, bool_t newlines);
/**
 * decodes base64 encoded data into raw octet data
 *
 * @param source: base64 data to be decrypted
 * @param source_len: maximum number of octets to decode from source
 * @param dest: byte array used to store the decoded data
 * @param dest_len: maximum number of octets that can be written to dest
 * @return number of decoded octets written to dest, or -1 on error (errno is set)
 */
extern int32_t b64_decode(const byte_t* source, int32_t source_len,
        byte_t *dest, int32_t dest_len);

// ? TODO: extern const abq_codec_t b64_codec; // Does not translate well to codepoints

#endif /* ONTRAC_TEXT_B64_ENC_H_ */
