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
 * @file url_enc.h
 * @date Jan 16, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_TEXT_URL_ENC_H_
#define ONTRAC_TEXT_URL_ENC_H_

#include <ontrac/text/codec.h>

extern const abq_codec_t url_codec;

/**
 * @brief encode with url encoding
 * letters will be lower-cased, and unsupported characters will be percent-encoded
 *
 * @param source: unencoded string to be url-encoded
 * @param source_len: length of unencoded string
 * @param dest: a byte array to which we should write the encoded string
 * @param dest_len: maximum number of bytes that can be written to dest
 * @return number of url encoded bytes written to dest, or -1 on error
 */
extern int32_t url_encode(
        cstr_t source, int32_t source_len,
        char *dest, int32_t dest_len);
/**
 * @brief decode a url encoded string
 *
 * @param source: the url encoded string we wish to decode
 * @param source_len: number of bytes in the source string
 * @param dest: a destination to which we should write the decoded data
 * @param dest_len: maximum number of bytes which can be written to dest
 * @return number of decoded bytes written to dest, or -1 on error
 */
extern int32_t url_decode(
        cstr_t source, int32_t source_len,
        char *dest, int32_t dest_len);

#endif /* ONTRAC_TEXT_URL_ENC_H_ */
