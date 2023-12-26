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

#ifndef ONTRAC_TEXT_CODEC_H_
#define ONTRAC_TEXT_CODEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emoota_utility_types.h"
#include <errno.h>

#define CHECK_NULL(item) ((NULL == (item)) ? EFAULT : EXIT_SUCCESS)

typedef int32_t err_t;

typedef const struct for_abq_codec abq_codec_t;
/** Reads/writes raw bytes as characters until size is reached */
extern const abq_codec_t abq_raw_codec;
/** Reads any ascii byte, and writes simple printable characters until size OR '\0' is reached */
extern const abq_codec_t abq_ascii_codec;


#ifndef ABQ_BUFFER_SIZE
/**
 * C99 standard only allows for objects with a max size of INT16_MAX
 *  but AQDIFF1.0 uses three (3) gzip streams, each of size 32k (INT16_MAX+1), in parallel
 *  if your project uses AQDIFF1.0, please set ABQ_BUFFER_SIZE to a higher value (e.g. 3*32768)
 **/
#define ABQ_BUFFER_SIZE ((size_t)INT16_MAX)
#endif /* ABQ_BUFFER_SIZE */


/** Don't want to UNKNOWN buffer size to be confused with the MAX SIZE */
#ifndef BUFFER_UNKNOWN_SIZE
#define BUFFER_UNKNOWN_SIZE (ABQ_BUFFER_SIZE-1U)
#endif /* BUFFER_UNKNOWN_SIZE */

// https://en.wikipedia.org/wiki/Byte_order_mark
#define UNICODE_BYTE_ORDER_MARK (0xFEFF)
#define UNICODE_WORD_JOINER (0x2060)

static inline size_t str_size(int64_t size) {
    return (0 > size) ? BUFFER_UNKNOWN_SIZE : (size_t) size;
}

typedef int32_t (*abq_bytesize)(int32_t codepoint);

typedef struct for_abq_encoder abq_encoder_t;
struct for_abq_encoder {
    abq_codec_t *codec;
    str_t dest;
    size_t pos;
    size_t max;
};

typedef err_t (*abq_encode)(abq_encoder_t *const encoder, const int32_t codepoint);

static inline abq_encoder_t abq_encoder_init(abq_codec_t *codec, str_t dest, int64_t max) {
    return (abq_encoder_t){
            .codec = codec,
            .dest = dest,
            .pos = 0,
            .max = str_size(max)};
}

#define ABQ_ENCODER(name, coder, text, size) abq_encoder_t name = abq_encoder_init((coder), (text), (int64_t)(size))
#define ABQ_CLEAR_ENCODER(coder) abq_bytes_set(encoder.dest, 0x00U, encoder.max)

typedef struct for_abq_decoder abq_decoder_t;
struct for_abq_decoder {
    abq_codec_t *codec;
    cstr_t source;
    size_t pos;
    size_t max;
};

typedef int32_t (*abq_decode)(abq_decoder_t *const decoder);

static inline abq_decoder_t abq_decoder_init(abq_codec_t *codec, cstr_t source, int64_t max) {
    return (abq_decoder_t){
            .codec = codec,
            .source = source,
            .pos = 0,
            .max = str_size(max)};
}

#define ABQ_DECODER(name, coder, text, size) abq_decoder_t name = abq_decoder_init((coder), (text), (int64_t)(size));
#define ABQ_DECODER_REWIND(decoder, codepoint) (decoder)->pos -= (decoder)->codec->bytesize((int32_t)(codepoint));

struct for_abq_codec {
    abq_bytesize bytesize;
    abq_encode encode;
    abq_decode decode;
};

typedef enum {
    BINARY_RADIX=2,
    OCTAL_RADIX=8,
    DECIMAL_RADIX=10,
    HEX_RADIX=0x10
} abq_radix_t;

static inline bool_t abq2_radix_is_invalid(abq_radix_t radix){
    return ((radix < BINARY_RADIX) || (HEX_RADIX < radix));
}

static inline err_t abq_encode_cp(abq_encoder_t *encoder, int32_t codepoint) {
    return (encoder)->codec->encode((encoder), codepoint);
}
static inline int32_t abq2_decode_cp(abq_decoder_t *decoder) {
    return (decoder)->codec->decode(decoder);
}
static inline err_t abq2_encode_char(abq_encoder_t *encoder, byte_t octet) {
    // UTF16 encodes each ASCII char as an octet pair
    return (encoder)->codec->encode((encoder), (int32_t)((uint8_t)octet));
}

static inline byte_t abq_decode_char(abq_decoder_t *decoder) {
    // UTF16 encodes each ASCII char as an octet pair
    return (byte_t) (decoder)->codec->decode(decoder);
}

typedef bool_t (*abq_cp_matcher)(int32_t cp);
extern err_t abq_decode_skip_matching(abq_decoder_t *decoder, abq_cp_matcher matcher) ;


/**
 * @brief decodes codepoints from text using the codec then re-encodes them to the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An numeric value to be converted to string form
 * @param codec: A character encoding used to decode text prior to re-encoding it
 * @param text: source of data to be writen to the decoder
 * @param length: max number of bytes to decode from the ascii string
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq2_encode_text(abq_encoder_t *encoder,
                              abq_codec_t *codec, cstr_t text, int32_t length);

/**
 * Converts an unsigned into readable string form using the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An unsigned value to be converted to string form
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq2_encode_uint(abq_encoder_t *encoder, uint64_t value, abq_radix_t radix);

/**
 * @brief Attempts to text encode the given number into the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An numeric value to be converted to string form
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq2_encode_number(abq_encoder_t *encoder, number_t value);


#if 0
/**
 * Parse a number_t from the given decoder,
 *  reads until a character that can't be handled
 *  returns -1 and sets errno to EILSEQ if no digits where parsed
 *
 * @param decoder: a text decoder configured with a codec used to read decimal characters
 * @param dest: Optional place to write resulting number on success
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_decode_number(abq_decoder_t *decoder, number_ptr dest);
#endif


#ifdef __cplusplus
}
#endif

#endif /* ONTRAC_TEXT_CODEC_H_ */
