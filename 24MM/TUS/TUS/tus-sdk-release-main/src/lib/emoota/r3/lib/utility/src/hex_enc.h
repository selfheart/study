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
 *  Copyright (c) 2021 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

#ifndef ONTRAC_TEXT_HEX_ENC_H_
#define ONTRAC_TEXT_HEX_ENC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "codec.h"
#include "emoota_utility_types.h"

extern const cstr_t abq_upper_hex_digits;

#define LEFT_NIBBLE(byte) ((((uint8_t)(byte)) >> 4U) & 0x0FU)
#define RIGHT_NIBBLE(byte) (((uint8_t)(byte)) & 0x0FU)

#define LEFT_NIBBLE_CHAR(byte) upper_hex_digits[LEFT_NIBBLE(byte)]
#define RIGHT_NIBBLE_CHAR(byte) upper_hex_digits[RIGHT_NIBBLE(byte)]


/**
 * @brief encode a single byte in hexadecimal form
 *
 * @param encoder: destination into which formatted results are written
 * @param value: byte value to convert into hexadecimal value
 * @return EXIT_SUCCESS if value was converted and written into encoder
 */
extern err_t abq2_encode_hex(abq_encoder_t *const encoder, const byte_t value);

/**
 * decodes hexadecimal data into raw octet data
 *
 * @param source: hexadecimal data to be decrypted
 * @param source_len: maximum number of octets to decode from source
 * @param dest: byte array used to store the decoded data
 * @param dest_len: maximum number of octets that can be written to dest
 * @return number of decoded octets written to dest, or -1 on error (errno is set)
 */
extern int32_t abq_hex_decode(const byte_t* source, int32_t source_len,
                              byte_t *dest, int32_t dest_len);


#ifdef __cplusplus
}
#endif
#endif /* ONTRAC_TEXT_HEX_ENC_H_ */
