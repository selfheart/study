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
 * @file hex_enc.h
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_TEXT_HEX_ENC_H_
#define ONTRAC_TEXT_HEX_ENC_H_

#include <ontrac/text/codec.h>

extern const cstr_t upper_hex_digits;
extern const cstr_t lower_hex_digits;

#define LEFT_NIBBLE(byte) ((((uint8_t)byte) >> 4) & 0x0FU)
#define RIGHT_NIBBLE(byte) (((uint8_t)byte) & 0x0FU)

#define LEFT_NIBBLE_CHAR(byte) upper_hex_digits[LEFT_NIBBLE(byte)]
#define RIGHT_NIBBLE_CHAR(byte) upper_hex_digits[RIGHT_NIBBLE(byte)]

/**
 * Lookup hexidecimal value of a character
 *
 * @param codepoint: the character to lookup
 * @return: number ranging between 0 and 16, or -1 if no match was found
 */
extern int8_t hex_value_of_char(int32_t codepoint);
/**
 * @brief encode a single byte in hexadecimal form
 *
 * @param encoder: destination into which formatted results are written
 * @param value: byte value to convert into hexidecimal value
 * @param lower_case: Toggle between upper and lower case hex digits
 * @return EXIT_SUCCESS if value was converted and written into encoder
 */
extern err_t abq_encode_hex(abq_encoder_t *encoder, const byte_t value, bool_t lower_case);
/**
 * encoded raw octets into hexidecimal form (2 nibbles per octet, 1 hexidecimal character per nibble)
 *
 * @param source: raw data to be encoded
 * @param source_len: max number of octets that can be read from array
 * @param dest: byte array to place data once it has been encoded
 * @param dest_len: maximum number of octets that can be written to dest
 * @param lower_case: if true uses [a-f] else uses [A-F]
 * @return number of encoded octets written to dest, or -1 on error (errno is set)
 */
extern int32_t hex_encode(const byte_t* source, int32_t source_len,
        byte_t *dest, int32_t dest_len, bool_t lower_case);

/**
 * decodes hexadecimal data into raw octet data
 *
 * @param source: hexadecimal data to be decrypted
 * @param source_len: maximum number of octets to decode from source
 * @param dest: byte array used to store the decoded data
 * @param dest_len: maximum number of octets that can be written to dest
 * @return number of decoded octets written to dest, or -1 on error (errno is set)
 */
extern int32_t hex_decode(const byte_t* source, int32_t source_len,
        byte_t *dest, int32_t dest_len);

// ? TODO: extern const abq_codec_t hex_codec; // Does not translate well to codepoints

#endif /* ONTRAC_TEXT_HEX_ENC_H_ */
