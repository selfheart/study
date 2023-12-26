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
 * @file ascii.h
 * @date Jan 15, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_TEXT_ASCII_H_
#define ONTRAC_TEXT_ASCII_H_

#include <platform/platformconfig.h>
/**
 * ASCII range from 0x00 to 0x7f
 *  0x7f is the DEL character which is not printable
 *  0x7e is the '~' character which is printable
 * Everything below ' ' (0x20) is a control character
 *  however we are going to consider a small subset of control characters as printable
 *  the exceptions are whitespace characters: '\t' (0x09), '\n' (0x0A), '\v' (0x0B). '\f' (0x0C) and '\r' (0x0D)
 * Consider everything between ' ' and '~' as printable
 *  also consider control characters between '\t' and '\r' (whitespace characters minus ' ') as printable
 *
 * @param cp: a unicode codepoint to check for friendly ascii characters
 * @return true for console friendly ascii characters, false otherwise
 */
extern bool_t ascii_is_friendly(int32_t cp);
/**
 * @brief, basically a custom implementation of isspace from  "ctype.h" header to keep MISRA from complaining
 *
 * @param cp, a unicode codepoint to check for ascii whitespace characters
 * @return  true for
 */
extern bool_t ascii_is_space(int32_t cp);
/**
 * @brief, basically a custom implementation of isalpha from  "ctype.h" header to keep MISRA from complaining
 *
 * @param cp, a unicode codepoint to check for ascii alphabetic characters
 * @return  true for
 */
extern bool_t ascii_is_alpha(int32_t cp);
/**
 * @brief, basically a custom implementation of isdigit from  "ctype.h" header to keep MISRA from complaining
 *
 * @param cp, a unicode codepoint to check for ascii decimal digit characters
 * @return  true for
 */
extern bool_t ascii_is_decimal(int32_t cp);
/**
 * @brief, basically a custom implementation of isalnum from  "ctype.h" header to keep MISRA from complaining
 *
 * @param cp, a unicode codepoint to check for ascii alphanumeric characters
 * @return  true for
 */
extern bool_t ascii_is_alnum(int32_t cp);

static inline int32_t ascii_tolower(int32_t cp) {
    int32_t retval = cp;
    if ((0x41 <= cp) && (cp <= 0x5A)) {
        // in capital letter range between 'A' (0x41) and 'Z' (0x5A) characters
        retval = (cp + 0x20); // ('a'(0x61) - 'A' (0x41)) = 0x20
    }
    return retval;
}
static inline bool_t ascii_cp_match(int32_t left,
        int32_t right, bool_t case_sensitive) {
    bool_t retval = false;
    if (0 > left) {
        // don't compare invalid code-points
    } else if(case_sensitive) {
        retval = (left == right);
    } else if(ascii_tolower(left) != ascii_tolower(right)){
        // mis-matching code-points
    } else {
        retval = true;
    }
    return retval;
}

// defined in <ontrac/text/codec.h>: extern abq_codec_t ascii_codec;

#endif /* ONTRAC_TEXT_ASCII_H_ */
