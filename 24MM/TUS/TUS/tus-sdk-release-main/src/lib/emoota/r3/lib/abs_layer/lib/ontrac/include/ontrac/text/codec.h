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
 * @file codec.h
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_TEXT_CODEC_H_
#define ONTRAC_TEXT_CODEC_H_

#include <platform/platformconfig.h>
#include <ontrac/text/ascii.h>
#include <ontrac/ontrac/ontrac_config.h>

typedef const struct for_abq_codec abq_codec_t;
/** Reads/writes raw bytes as characters until size is reached */
extern const abq_codec_t raw_codec;
/** Reads any ascii byte, and writes simple printable characters until size OR '\0' is reached */
extern const abq_codec_t ascii_codec;



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

/** Use raw_code for well known sizes as it can read & write '\0' charcters, but used ascii for unknown sizes */
static inline abq_codec_t* byte_codec(int64_t size) {
    abq_codec_t *retval = &ascii_codec;
    if((0 <= size) &&(((size_t)size) <= ABQ_BUFFER_SIZE) && (((size_t)size) != BUFFER_UNKNOWN_SIZE)) {
        retval = &raw_codec;
    }
    return retval;
}

typedef int32_t (*abq_bytesize)(int32_t codepoint);

typedef struct for_abq_encoder abq_encoder_t;
struct for_abq_encoder {
    abq_codec_t *codec;
    str_t dest;
    size_t pos;
    size_t max;
};

typedef err_t (*abq_encode)(abq_encoder_t *encoder, int32_t codepoint);

static inline abq_encoder_t abq_encoder_init(abq_codec_t *codec, str_t dest, int64_t max) {
    return (abq_encoder_t) {
            .codec= (NULL == codec) ? byte_codec(max) : codec,
            .dest=dest,
            .pos=0,
            .max=str_size(max)
    };
}

#define ABQ_ENCODER(name, coder, text, size) abq_encoder_t name = abq_encoder_init(coder, text, (int64_t)size);

typedef struct for_abq_decoder abq_decoder_t;
struct for_abq_decoder {
    abq_codec_t *codec;
    cstr_t source;
    size_t pos;
    size_t max;
};

typedef int32_t (*abq_decode)(abq_decoder_t *decoder);

static inline abq_decoder_t abq_decoder_init(abq_codec_t *codec, cstr_t source, int64_t max) {
    return (abq_decoder_t) {
            .codec=(NULL == codec) ? byte_codec(max) : codec,
            .source=source,
            .pos=0,
            .max=str_size(max)
    };
}

#define ABQ_DECODER(name, coder, text, size) abq_decoder_t name = abq_decoder_init(coder, text, (int64_t)size);
#define ABQ_DECODER_REWIND(decoder, codepoint) (decoder)->pos -= (decoder)->codec->bytesize((int32_t)(codepoint));

struct for_abq_codec {
    abq_bytesize bytesize;
    abq_encode encode;
    abq_decode decode;
};

extern const cstr_t abq_null_str;
extern const cstr_t abq_true_str;
extern const cstr_t abq_false_str;

extern const cstr_t abq_nan_str;
extern const cstr_t abq_inf_pos_str;
extern const cstr_t abq_inf_neg_str;

extern const number_t abq_nan;
extern const number_t abq_inf_pos;
extern const number_t abq_inf_neg;
extern const cstr_t numeric_characters;

static inline bool_t IS_NAN(number_t x) {
    return (x != x);
}
static inline bool_t IS_INF(number_t x) {
    return ((abq_inf_pos == x) || (abq_inf_neg == x));
}

typedef enum {
    BINARY_RADIX=2,
    OCTAL_RADIX=8,
    DECIMAL_RADIX=10,
    HEX_RADIX=0x10
} abq_radix_t;

static inline bool_t abq_radix_is_invalid(abq_radix_t radix){
    return ((radix < BINARY_RADIX) || (HEX_RADIX < radix));
}

static inline err_t abq_encode_cp(abq_encoder_t *encoder, int32_t codepoint) {
    return (encoder)->codec->encode((encoder), codepoint);
}
static inline int32_t abq_decode_cp(abq_decoder_t *decoder) {
    return (decoder)->codec->decode(decoder);
}
static inline err_t abq_encode_char(abq_encoder_t *encoder, byte_t octet) {
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
 * @brief skips over any leading-whitespace characters
 *
 * @param decoder: a text decoder configured with a codec used to read characters
 * @return EXIT_SUCCESS on success, else an error code
 */
static inline err_t abq_decode_skip_ws(abq_decoder_t *decoder){
    return abq_decode_skip_matching(decoder, ascii_is_space);
}

/**
 * @brief skips over expected prefix characters, returns error and resets decoder.pos if failure to match
 *
 * @param decoder: a text decoder configured with a codec used to read characters
 * @param prefix: expected start byte sequence
 * @param length: max number of bytes in start sequence
 * @param ignore_case: continue to match even if character case is different
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_decode_skip_prefix_ex(abq_decoder_t *decoder,
        cstr_t prefix, int32_t length, bool_t case_sensitive);

static inline err_t abq_decode_skip_prefix(abq_decoder_t *decoder, cstr_t prefix, int32_t length){
    return abq_decode_skip_prefix_ex(decoder, prefix, length, true);
}

/**
 * @brief: decodes characters from decoder and writes them to encoder until end of string is reached
 *
 * @param decoder: a text decoder configured with a codec used to read characters
 * @param encoder: pointer to a abq_encoder_t used to store the resulting text
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_decode_encode(abq_decoder_t *decoder, abq_encoder_t *encoder);

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
extern err_t abq_encode_text(abq_encoder_t *encoder,
                             abq_codec_t *codec, cstr_t text, int32_t length);

/**
 * @brief decodes codepoints from ascii and encodes them using the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An numeric value to be converted to string form
 * @param ascii: reads printable ascii bytes from the given string
 * @param length: max number of bytes to decode from the ascii string
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_ascii(abq_encoder_t *encoder, cstr_t ascii, int32_t length);

/**
 * @brief decodes codepoints from ascii and encodes them using the given encoder
 *  however, unlike the above abq_encode_ascii, if this function overfills the buffer,
 *  then it will rewrite  an ellipses ("...\0") as the final 4 characters of the encoder
 *  instead of reseting it
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An numeric value to be converted to string form
 * @param loginfo: reads printable ascii encoded bytes from the given string
 * @param length: max number of bytes to decode from the ascii string
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_loginfo(abq_encoder_t *encoder, cstr_t loginfo, int32_t length);

/**
 * Converts an integer into readable string form using the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An integer value to be converted to string form
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_int(abq_encoder_t *encoder, int64_t value, abq_radix_t radix);
/**
 * Converts an unsigned into readable string form using the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An unsigned value to be converted to string form
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_uint(abq_encoder_t *encoder, uint64_t value, abq_radix_t radix);

/**
 * @brief Converts an integer into readable string of padded size using the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An integer value to be converted to string form
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @param padding: the character to pad with, often '0' or ' '
 * @param char_count: minimum number of bytes to write to dest buffer
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_left_padded_int(abq_encoder_t *encoder,
                                        int64_t value, abq_radix_t radix, byte_t pad, size_t char_count);
/**
 * @brief Converts an unsigned into readable string of padded size using the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An unsigned value to be converted to string form
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @param padding: the character to pad with, often '0' or ' '
 * @param char_count: minimum number of bytes to write to dest buffer
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_left_padded_uint(abq_encoder_t *encoder,
        uint64_t value, abq_radix_t radix, byte_t pad, size_t char_count);

/**
 * @brief converts a pointer to an uint64_t address before hex encodes is (8 byte left padded with '0', prepended with "0x")
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param ptr: pointer to be converted into an address
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_ptraddr(abq_encoder_t *encoder, cvar_t ptr);
/**
 * @brief Attempts to text encode the given number into the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An numeric value to be converted to string form
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_number(abq_encoder_t *encoder, number_t value);

/**
 * @brief Attempts to text encode the given decimal number into the given encoder
 *
 * @param encoder: pointer to a abq_encoder_t used to store the resulting string
 * @param value: An numeric value to be converted to string form
 * @param max_mantissa: Max decimal digits following the decimal point to write out
 * @param trim: True to remove trailing zeroes
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_decimal(abq_encoder_t *encoder, number_t value, size_t max_mantissa, bool_t trim);

/**
 * @brief decodes codepoints from decoder then re-encodes them into dest buffer using the codec
 *
 * @param decoder: a text decoder configured with a codec used to read decimal characters
 * @param codec: A character encoding used to decode text prior to re-encoding it
 * @param dest: byte buffer to write string to
 * @param limit: max number of bytes that can be written to dest
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_decode_text(abq_decoder_t *decoder, abq_codec_t *codec, byte_t *dest, size_t limit);
/**
 * Parse an integer value from a decoder,
 * reads until non-integer character is reached,
 * supports a singular '+' or '-' start prefix
 *  returns -1 and sets errno to EILSEQ if no digits where parsed
 *
 * @param decoder: a text decoder configured with a codec used to read decimal characters
 * @param dest: Optional place to write resulting integer on success
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_decode_int(abq_decoder_t *decoder, int64_t *dest, abq_radix_t radix);

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

#endif /* ONTRAC_TEXT_CODEC_H_ */
