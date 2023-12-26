/****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file unicode/utf8_utils.h
 * @date Mar 28, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief utilities for working with UTF-8 encoded strings
 */

#ifndef SPIL_NIO_TEST_STR_UTILS_H
#define SPIL_NIO_TEST_STR_UTILS_H

#include <ontrac/text/utilities.h>
#include <ontrac/text/utf8_enc.h>

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
 * @param pattern: the pattern to match against
 * @param pattern_len: max length of pattern, -1 for until terminator
 * @param path: the character sequence to match pattern to
 * @param path_len: max length of path, -1 for until terminator
 * @param case_sensitive: true for case sensitive character matching, false for case-insensitive character matching
 * @return: true if path matches pattern, false otherwise
 */
static inline bool_t utf8_match_path(const cstr_t pattern, int32_t pattern_len,
        const cstr_t path, int32_t path_len, bool_t case_sensitive) {
    return text_match_path(&utf8_codec, pattern, pattern_len, path, path_len, case_sensitive);
}
/**
 * Check is the supplied string consists of at least 1 and limited to whitespace characters
 *
 * @param str: the string to check
 * @param length: the maximum number of octets to check
 * @return true if whitespace and only whitespace was found, false otherwise
 */
static inline bool_t utf8_is_whitespace(cstr_t str, int32_t length) {
    return text_is_whitespace(&utf8_codec, str, length);
}
/**
 * determines the length of a string in number of octets
 * @param str: the string to calculate length of
 * @param length: maximum number of bytes available in string, value of -1 to read to terminator
 * @return actual length of string in bytes, not including null terminator, -1 if it fails to parse a character error
 */
static inline int32_t utf8_byte_length(const cstr_t str, int32_t length) {
    return text_byte_length(&utf8_codec, str, length);
}
/** same as utf8_byteLength, but does not include trailing whitespace
 *
 * @param str the string to calculate length of
 * @param length: maximum number of bytes available in string, value of -1 to read to terminator
 * @return actual length of string in bytes, not including trailing whitespace or null terminator, -1 if it fails to parse a character error
 */
static inline int32_t utf8_trim_length(const cstr_t str, int32_t length) {
    return text_trim_length(&utf8_codec, str, length);
}
/**
 * determines number of octets contributed to leading-whitespace
 *
 * @param str: string to examine
 * @param length: maximum number of bytes to examine, -l for any amount
 * @return byte-count associated with leading whitespace characters
 */
static inline  int32_t utf8_leading_ws(const cstr_t str, int32_t length){
    return text_leading_ws(&utf8_codec, str, length);
}
/**
 * Writes string to destination buffer up to the allowed length, not including terminator
 *  only writes up to the first terminator fount in str or until allowance is reached
 *
 * @param dest: byte buffer to write string to
 * @param str: the string to write to destination
 * @param allowance: maximum number of bytes that can be written to destination
 * @return number of bytes written, or -1 on error and errno is set
 */
static inline int32_t utf8_write_bytes(byte_t *dest, const cstr_t str, int32_t allowance) {
 return text_write_bytes(&utf8_codec, dest, str, allowance);
}
/**
 * case-sensitive string compare, NULL is always considered greater then other values
 *
 * @param first: string to compare
 * @param second: string to compare
 * @param n: max number of bytes to compare, or -1 to compare until terminator
 * @returns 1 if first is greater then second, -1 is second is greater then second, 0 otherwise.
 */
static inline int8_t utf8_compare_exact(const cstr_t first, const cstr_t second, int32_t n) {
    return text_compare_exact(&utf8_codec, first, second, n);
}
/**
 * case-insensitive string compare, NULL is always considered greater then other values
 *
 * @param first: string to compare
 * @param second: string to compare
 * @param n: max number of bytes to compare, or -1 to compare until terminator
 * @returns 1 if first is greater then second, -1 is second is greater then second, 0 otherwise.
 */
static inline int8_t utf8_compare_insensitive(const cstr_t first, const cstr_t second, int32_t n){
    return text_compare_insensitive(&utf8_codec, first, second, n);
}
/**
 * Check if a given string begins with a prefix
 *
 * @param str: the parent string
 * @param str_len: maximum number of bytes to check on the string
 * @param prefix: a potential substring to be found at the beginning of parent
 * @param prefix_len: maximum number of bytes found in substring
 * @return true if str begins with prefix, false otherwise
 */
static inline bool_t utf8_starts_with(cstr_t str, int32_t str_len,
        cstr_t prefix, int32_t prefix_len) {
    return text_starts_with(&utf8_codec, str, str_len, prefix, prefix_len);
}
/**
 * @brief checks is the given string starts with digits and how many, returns zero if the first character is not a digit
 *
 * @param str: string to be inspected
 * @param str_len: maximum number of bytes to check on the string
 * @param radix: integer radix of digit type to be checked, usually DECIMAL_RADIX
 * @return 0 if no characters exist or the first character was not a digit, else number of prefixed digits on the string
 */
static inline int32_t utf8_starts_with_digits(cstr_t str, int32_t str_len, abq_radix_t radix) {
    return text_starts_with_digits(&utf8_codec, str, str_len, radix);
}
/**
 *  find the first index of a given substring within the parent string
 *
 * @param super: the parent string that may contain substring
 * @param super_len: maximum number of bytes to use in super
 * @param substring: a potential child string to be found in parent
 * @param sub_len: maximum number of bytes to use in substring
 * @return the index of beginning of substring if found, else -1.
 */
static inline int32_t utf8_index_of(const cstr_t super, int32_t super_len,
        const cstr_t substring, int32_t sub_len) {
    return text_index_of(&utf8_codec, super, super_len, substring, sub_len);
}
/**
 *  find the first index of a given character within the parent string
 *
 * @param super: the parent string that may contain character
 * @param super_len: maximum number of bytes to use in super
 * @param codepoint: a potential child character to be found in parent
 * @return the index of first instance of character if found, else -1.
 */
static inline int32_t utf8_index_of_char(const cstr_t super, int32_t super_len, byte_t codepoint) {
    return text_index_of_char(&utf8_codec, super, super_len, (int32_t) codepoint);
}
/**
 * finds the location of next un-escaped instance of one of the given delimiters
 *
 * @param super: the parent string that may contain character
 * @param super_len: maximum number of bytes to use in super
 * @param delimiters: a set of dividers that separate tokens from one another
 * @param esc_char: optional escape character
 * @return the location of the first delimiter found, else byte-length of string
 */
static inline int32_t utf8_end_of_token(const cstr_t super, int32_t super_len,
        const cstr_t delimiters, int32_t esc_char) {
    return text_end_of_token(&utf8_codec, super, super_len, delimiters, esc_char);
}
/**
 * Write the lower-case equivalent of the given string to destination
 *
 * @param source: the string to read from
 * @param source_len: max number of bytes to read from string
 * @param dest: the buffer to write to
 * @param dest_len: the maximum number of bytes that can be written to dest
 */
static inline int32_t utf8_to_lower_case(const cstr_t source, int32_t source_len,
        byte_t *dest, int32_t dest_len) {
    return text_to_lower_case(&utf8_codec, source, source_len, dest, dest_len);
}
/**
 * Writes a integer to string form
 *
 * @param dest: a character array to write the string to
 * @param dest_len: the maximum number of bytes that can be written to dest
 * @param source: the int to convert to string form
 * @param radix: The radix used for string representation, typically 10, must be between 2 and 16
 * @return number of bytes written to the dest buffer, or -1 on error
 */
static inline int32_t utf8_write_int(byte_t *dest, size_t dest_len,
        int64_t source, abq_radix_t radix) {
    return text_write_int(&utf8_codec, dest, dest_len, source, radix);
}

/**
 * Writes a number_t(double) to string form
 *
 * @param dest: a character array to write the string to
 * @param dest_len: the maximum number of characters that can be written to dest
 * @param source: the number to convert to string form
 * @return number of bytes written to the dest buffer, or -1 on error
 */
static inline int32_t utf8_write_number(byte_t *dest, size_t dest_len, number_t source){
    return  text_write_number(&utf8_codec, dest, dest_len, source);
}
/**
 * Parse an integer value from a string,
 * reads until non-integer character is reached,
 * supports a singular '+' or '-' start prefix
 *  returns -1 and sets errno to EILSEQ if no digits where parsed
 *
 * @param source: the character array to read from
 * @param source_len: max number of octets that can be read from array
 * @param dest: a int64_t pointer to write the result to
 * @param radix: The radix used for string representaion, typically 10, must be betweeen 2 and 16
 * @return number of characters read from source, or -1 on error
 */
static inline int32_t utf8_read_int(const cstr_t source, size_t source_len,
        int64_t* dest, abq_radix_t radix) {
    return text_read_int(&utf8_codec, source, source_len, dest, radix);
}

/**
 * Parse a number_t from the given string,
 *  reads until a character that can't be handled
 *  returns -1 and sets errno to EILSEQ if no digits where parsed
 *
 * @param source: the character array to read from
 * @param source_len: max number of characters that can be read from array
 * @param dest : a number_t pointer to write the result to
 * @return number of characters read from source, or -1 on error
 */
static inline int32_t utf8_read_number(const cstr_t source, size_t source_len, number_ptr dest) {
    return text_read_number(&utf8_codec, source, source_len, dest);
}

static inline bool_t UTF8_EQUALS(cstr_t left, cstr_t right) {
    bool_t retval = false;
    if(0 == utf8_compare_exact(left, right, -1)) {
        retval = true;
    }
    return retval;
}
static inline bool_t UTF8_CASE_EQUALS(cstr_t left, cstr_t right) {
    bool_t retval = false;
    if(0 == utf8_compare_insensitive(left, right, -1)) {
        retval = true;
    }
    return retval;
}

static inline bool_t UTF8_NOT_EQUALS(cstr_t left, cstr_t right) {
    bool_t retval = false;
    if(0 != utf8_compare_exact(left, right, -1)) {
        retval = true;
    }
    return retval;
}
#endif //SPIL_NIO_TEST_STR_UTILS_H
