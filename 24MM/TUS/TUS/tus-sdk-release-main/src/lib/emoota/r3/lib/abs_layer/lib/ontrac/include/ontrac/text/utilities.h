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
 * @file utilities.h
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_TEXT_UTILITIES_H_
#define ONTRAC_TEXT_UTILITIES_H_

#include <ontrac/text/codec.h>
#include <ontrac/text/ascii.h>
#include <ontrac/text/utf8_enc.h>
#include <ontrac/text/json_enc.h>
#include <ontrac/text/hex_enc.h>
#include <ontrac/text/b64_enc.h>

/**
 * Path matching utility build using simplified verson og UNIX shell "glob" rules
 * REVIEW: any reason why backslash '\\' characters shouldn't be treated as slash '/' characters?
 * Pattern matching supports the following wildcard characters:
 *  *   (asterisk) - matches zero or more characters, never matches "/"
 *                   (slash) character
 *  ?   (question mark) - matches exactly one character, never matches "/"
 *                         (slash) character
 *  [xy] (character set) - any one of the characters in the provided set
 *  [x-y] (character range) - any one of the ASCII characters in the provided range
 *  [abx-y] - set+range combinations are also supported
 *  \ (backslash) - escapes next character
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param pattern: the pattern to match against
 * @param pattern_len: max length of pattern, -1 for until terminator
 * @param path: the character sequence to match pattern to
 * @param path_len: max length of path, -1 for until terminator
 * @param case_sensitive: true for case sensitive character matching, false for case-insensitive character matching
 * @return: true if path matches pattern, false otherwise
 */
extern bool_t text_match_path(abq_codec_t *codec, const cstr_t pattern, int32_t pattern_len,
        const cstr_t path, int32_t path_len, bool_t case_sensitive);


/**
 * Check is the supplied string consists of at least 1 and limited to whitespace characters
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param str: the string to check
 * @param length: the maximum number of octets to check
 * @return true if whitespace and only whitespace was found, false otherwise
 */
extern bool_t text_is_whitespace(abq_codec_t *codec, cstr_t str, int32_t length);
/**
 * determines the length of a string in number of octets
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param str: the string to calculate length of
 * @param length: maximum number of bytes available in string, value of -1 to read to terminator
 * @return actual length of string in bytes, not including null terminator, -1 if it fails to parse a character error
 */
extern int32_t text_byte_length(abq_codec_t *codec, const cstr_t str, int32_t length);
/**
 * same as text_byte_length, but exludes any trailing whitespace
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param str the string to calculate length of
 * @param length: maximum number of bytes available in string, value of -1 to read to terminator
 * @return actual length of string in bytes, not including trailing whitespace or null terminator, -1 if it fails to parse a character error
 */
extern int32_t text_trim_length(abq_codec_t *codec, const cstr_t str, int32_t length);
/**
 * determines number of octets contributed to leading-whitespace
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param str: string to examine
 * @param length: maximum number of bytes to examine, -l for any amount
 * @return byte-count associated with leading whitespace characters
 */
extern int32_t text_leading_ws(abq_codec_t *codec, const cstr_t str, int32_t length);

/**
 * Writes string to destination buffer up to the allowed length, not including terminator
 *  only writes up to the first terminator fount in str or until allowance is reached
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param dest: byte buffer to write string to
 * @param str: the string to write to destination
 * @param allowance: maximum number of bytes that can be written to destination
 * @return number of bytes written, or -1 on error and errno is set
 */
extern int32_t text_write_bytes(abq_codec_t *codec, byte_t *dest, const cstr_t str, int32_t allowance);

/**
 * case-sensitive string compare, NULL is always considered greater then other values
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param first: string to compare
 * @param second: string to compare
 * @param n: max number of bytes to compare, or -1 to compare until terminator
 * @returns 1 if first is greater then second, -1 is second is greater then second, 0 otherwise.
 */
extern int8_t text_compare_exact(abq_codec_t *codec, const cstr_t first, const cstr_t second, int32_t n);
/**
 * case-insensitive string compare, NULL is always considered greater then other values
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param first: string to compare
 * @param second: string to compare
 * @param n: max number of bytes to compare, or -1 to compare until terminator
 * @returns 1 if first is greater then second, -1 is second is greater then second, 0 otherwise.
 */
extern int8_t text_compare_insensitive(abq_codec_t *codec,
        const cstr_t first, const cstr_t second, int32_t n);
/**
 * Check if a given string begins with a prefix
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param str: the parent string
 * @param str_len: maximum number of bytes to check on the string
 * @param prefix: a potential substring to be found at the beginning of parent
 * @param prefix_len: maximum number of bytes found in substring
 * @return true if str begins with prefix, false otherwise
 */
extern bool_t text_starts_with(abq_codec_t *codec, cstr_t str, int32_t str_len,
        cstr_t prefix, int32_t prefix_len);
/**
 * @brief checks is the given string starts with digits and how many, returns zero if the first character is not a digit
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param str: string to be inspected
 * @param str_len: maximum number of bytes to check on the string
 * @param radix: integer radix of digit type to be checked, usually DECIMAL_RADIX
 * @return 0 if no characters exist or the first character was not a digit, else number of prefixed digits on the string
 */
extern int32_t text_starts_with_digits(abq_codec_t *codec, cstr_t str, int32_t str_len, abq_radix_t radix);

/**
 *  find the first index of a given substring within the parent string
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param super: the parent string that may contain substring
 * @param super_len: maximum number of bytes to use in super
 * @param substring: a potential child string to be found in parent
 * @param sub_len: maximum number of bytes to use in substring
 * @return the index of beginning of substring if found, else -1.
 */
extern int32_t text_index_of(abq_codec_t *codec, const cstr_t super, int32_t super_len,
        const cstr_t substring, int32_t sub_len);
/**
 *  find the first index of a given character within the parent string
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param super: the parent string that may contain character
 * @param super_len: maximum number of bytes to use in super
 * @param codepoint: a potential child character to be found in parent
 * @return the index of first instance of character if found, else -1.
 */
extern int32_t text_index_of_char(abq_codec_t *codec, const cstr_t super, int32_t super_len, int32_t codepoint);
/**
 * finds the location of next un-escaped instance of one of the given delimiters
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param super: the parent string that may contain character
 * @param super_len: maximum number of bytes to use in super
 * @param delimiters: a set of dividers that separate tokens from one another
 * @param esc_char: optional escape character
 * @return the location of the first delimiter found, else byte-length of string
 */
extern int32_t text_end_of_token(abq_codec_t *codec, const cstr_t super, int32_t super_len,
        const cstr_t delimiters, int32_t esc_char);
/**
 * Write the lower-case equivalent of the given string to destination
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param source: the string to read from
 * @param source_len: max number of bytes to read from string
 * @param dest: the buffer to write to
 * @param dest_len: the maximum number of bytes that can be written to dest
 */
extern int32_t text_to_lower_case(abq_codec_t *codec, const cstr_t source, int32_t source_len,
        byte_t *dest, int32_t dest_len);
/**
 * Writes a integer to string form
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param dest: a character array to write the string to
 * @param dest_len: the maximum number of bytes that can be written to dest
 * @param source: the int to convert to string form
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @return number of bytes written to the dest buffer, or -1 on error
 */

extern int32_t text_write_int(abq_codec_t *codec, byte_t *dest, size_t dest_len, int64_t source,
        abq_radix_t radix);
/**
 * Writes a number_t(double) to string form
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param dest: a character array to write the string to
 * @param dest_len: the maximum number of characters that can be written to dest
 * @param source: the number to convert to string form
 * @return number of bytes written to the dest buffer, or -1 on error
 */
extern int32_t text_write_number(abq_codec_t *codec, byte_t *dest, size_t dest_len, number_t source);
/**
 * Parse an integer value from a string,
 * reads until non-integer character is reached,
 * supports a singular '+' or '-' start prefix
 *  returns -1 and sets errno to EILSEQ if no digits where parsed
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param source: the character array to read from
 * @param source_len: max number of octets that can be read from array
 * @param dest: a int64_t pointer to write the result to
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @return number of characters read from source, or -1 on error
 */
extern int32_t text_read_int(abq_codec_t *codec,
        const cstr_t source, size_t source_len,
        int64_t* dest, abq_radix_t radix);
/**
 * Parse a number_t from the given string,
 *  reads until a character that can't be handled
 *  returns -1 and sets errno to EILSEQ if no digits where parsed
 *
 * @param codec: character encoder/decoder used to interpret octet-stream
 * @param source: the character array to read from
 * @param source_len: max number of characters that can be read from array
 * @param dest : a number_t pointer to write the result to
 * @return number of characters read from source, or -1 on error
 */
extern int32_t text_read_number(abq_codec_t *codec, const cstr_t source, size_t source_len, number_ptr dest);

typedef const struct for_unit_modifier unit_modifier_t;
struct for_unit_modifier {
    str_t unit_identifier;
    number_t multipler;
};

#define SECONDS_PER_MILLI (1E-3)
extern unit_modifier_t *units_of_time;
extern unit_modifier_t *metric_postfixes;

/**
 * @brief parses a single numeric value followed by a unit-designator
 *
 * @param codec: text encoding of supplied str_t
 * @param time_segment: string formatted time-segment
 * @param segment_len: maximum number of bytes that can be read from string
 * @return parsed time value in units of seconds, abq_nan on failure to parse number
 */
extern number_t parse_time_with_units(abq_codec_t *codec, cstr_t time_segment, size_t segment_len);
/**
 * @brief parses a single numeric value followed by a unit-designator
 *
 * @param decoder: source data from which to parse value from
 * @param units: list of units with magnitude (modifiers) for each
 * @param case_sensitive: true if units are case-sensitive ('B' for bytes vs 'b' for bits)
 * @return
 */
extern number_t abq_decode_number_with_units(abq_decoder_t *decoder,
        const unit_modifier_t *units, bool_t case_sensitive);

#endif /* ONTRAC_TEXT_UTILITIES_H_ */
