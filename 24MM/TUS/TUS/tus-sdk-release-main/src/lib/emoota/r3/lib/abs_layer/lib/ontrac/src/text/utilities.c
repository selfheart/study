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
 * @file utilities.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ctype.h>
#include <string.h>

#include <ontrac/text/utilities.h>
#include <ontrac/ontrac/status_codes.h>

const unit_modifier_t *units_of_time = (unit_modifier_t[]) {
	{.unit_identifier="nanos",  .multipler=(number_t)(1E-9) },
	{.unit_identifier="micro",  .multipler=(number_t)(1E-6) },
    {.unit_identifier="milli",  .multipler=(number_t)SECONDS_PER_MILLI },
    {.unit_identifier="sec",    .multipler=(number_t)(1.0) },
    {.unit_identifier="min",    .multipler=(number_t)(60.0) },
    {.unit_identifier="hr",     .multipler=(number_t)(3600.0) },
    {.unit_identifier="hour",   .multipler=(number_t)(3600.0) },
    {.unit_identifier="day",    .multipler=(number_t)(24*3600.0) },
    {.unit_identifier="week",   .multipler=(number_t)(7*24*3600.0) },
    {.unit_identifier="ms",     .multipler=(number_t)SECONDS_PER_MILLI },
	/** Defaults to units of milliseconds (Back-end uses Java-time) */
    {.unit_identifier=NULL,     .multipler=(number_t)SECONDS_PER_MILLI },
};

const unit_modifier_t *metric_postfixes = (unit_modifier_t[]) {
	{.unit_identifier="EB",  .multipler=(number_t)(((uint64_t)1UL) << 60U) }, // Exabytes
	{.unit_identifier="Eb",  .multipler=(number_t)(((uint64_t)1UL) << 57U) }, // Exabits
	{.unit_identifier="PB",  .multipler=(number_t)(((uint64_t)1UL) << 50U) }, // Petabytes
	{.unit_identifier="Pb",  .multipler=(number_t)(((uint64_t)1UL) << 47U) }, // Petabits
	{.unit_identifier="TB",  .multipler=(number_t)(((uint64_t)1UL) << 40U) }, // Terabytes
	{.unit_identifier="Tb",  .multipler=(number_t)(((uint64_t)1UL) << 37U) }, // Terabits
	{.unit_identifier="GB",  .multipler=(number_t)(((uint64_t)1UL) << 30U) }, // Gigabytes
	{.unit_identifier="Gb",  .multipler=(number_t)(((uint64_t)1UL) << 27U) }, // Gigabits
	{.unit_identifier="MB",  .multipler=(number_t)(((uint64_t)1UL) << 20U) }, // Megabytes
	{.unit_identifier="Mb",  .multipler=(number_t)(((uint64_t)1UL) << 17U) }, // Megabits
	{.unit_identifier="kB",  .multipler=(number_t)(((uint64_t)1UL) << 10U) }, // Kilobytes
	{.unit_identifier="kb",  .multipler=(number_t)(((uint64_t)1UL) << 7U) }, // Kilobits
	{.unit_identifier="B",   .multipler=(number_t)(1.0) }, // Bytes
	{.unit_identifier="b",   .multipler=(number_t)(1.0 / 8.0) }, // Bits
	/** Defaults to bytes if no unit-multiplier was specified */
    {.unit_identifier=NULL,     .multipler= (number_t)(1L) },
};

#ifdef MAX_WILDCARD_CHARS
// No need for a default if explicitly defined
#elif defined(PLATFORM_MAX_FILE_PATH_LEN)
#define MAX_WILDCARD_CHARS PLATFORM_MAX_FILE_PATH_LEN
#else /* !MAX_WILDCARD_CHARS && !PLATFORM_MAX_FILE_NAME_LEN */
#define MAX_WILDCARD_CHARS (64U)
#endif

typedef struct for_pm_state {
    uint16_t pattern_pos;
    uint16_t path_pos;
} pm_state_t;

typedef struct for_pm_stack_info {
    pm_state_t stack[MAX_WILDCARD_CHARS];
    uint16_t index;
} pm_stack_info_t;

static int32_t pm_push_state(pm_stack_info_t *info,
        const abq_decoder_t* pattern, abq_decoder_t* path) {
    VITAL_NOT_NULL(info);
    VITAL_NOT_NULL(pattern);
    VITAL_NOT_NULL(path);
    int32_t retval = -1;
    if(MAX_WILDCARD_CHARS <= info->index){
        abq_status_set(EOVERFLOW, false);
    } else {
        info->stack[info->index].path_pos = (uint16_t) path->pos;
        retval = abq_decode_cp(path);
        if((0 < retval) && (((int32_t)'/') != retval)) {
            info->stack[info->index].pattern_pos = (uint16_t) pattern->pos;
            info->index++;
        } else {
            // ABQ_DECODER_REWIND(path, retval)
        }
    }
    return retval;
}

static int32_t pm_pop_state(pm_stack_info_t *info,
        abq_decoder_t* pattern, abq_decoder_t* path) {
    VITAL_NOT_NULL(info);
    VITAL_NOT_NULL(pattern);
    VITAL_NOT_NULL(path);
    int32_t retval = -1;
    if (0U == info->index) {
        info->index = (uint16_t) SIZE_MAX;
    } else if(info->index > MAX_WILDCARD_CHARS) {
        abq_status_set(ERANGE, false);
    } else {
        info->index--;
        pattern->pos = (size_t) info->stack[info->index].pattern_pos;
        path->pos = (size_t) info->stack[info->index].path_pos;
        retval = abq_decode_cp(path);
    }
    return retval;
}

static bool_t rangematch(abq_decoder_t *decoder,
        int32_t match_cp, bool_t case_sensitive) {
    bool_t has_match = false;
    bool_t negate = false;
    size_t orig_pos = decoder->pos;
    int32_t codepoint=-1;
    codepoint = abq_decode_cp(decoder);
    if ('[' == (byte_t) codepoint) {
        int32_t previous = -1;
        int32_t target = match_cp;
        codepoint = abq_decode_cp(decoder);
        negate = (bool_t) (('!' == (byte_t) codepoint)
                        || ('^' ==  (byte_t) codepoint));
        if (negate) {
            codepoint = abq_decode_cp(decoder);
        }
        if(!case_sensitive){
            target = ascii_tolower(target);
        }
        while((0 < codepoint) && (']' != (byte_t) codepoint)){
            if (!has_match) {
                if(!case_sensitive) {
                    codepoint = ascii_tolower(codepoint);
                }
                if('\\' == (byte_t) codepoint){
                    codepoint = abq_decode_cp(decoder);
                    if(codepoint == target) {
                        has_match = true;
                    }
                }else if(('-' == (byte_t)codepoint) && (previous >=0)){
                    codepoint = abq_decode_cp(decoder);
                    if ((previous <= target) && (target <= codepoint)) {
                        has_match = true;
                    }
                }else if(codepoint == target) {
                    has_match = true;
                }else{
                    // no match
                }
            }
            codepoint = abq_decode_cp(decoder);
            previous = codepoint;
        }
    }
    if(has_match != negate) {
        has_match = true;
    }else{
        decoder->pos = orig_pos;
        has_match = false;
    }
    return has_match;
}

bool_t text_match_path(abq_codec_t *codec, const cstr_t pattern, int32_t pattern_len,
        const cstr_t path, int32_t path_len, bool_t case_sensitive) {
    // modeled after fnmatch.c under Berkeley liscence @ http://web.mit.edu/freebsd/csup/fnmatch.c
    //  however it uses recursive function call for '*' characters
    //  which violoates MISRA Rule 17.2
    //   should behave as: (FNM_NOMATCH != fnmatch(pattern, path, FNM_PATHNAME))
    bool_t rvalue = false;
    if ((NULL == pattern) || (NULL == path)) {
        if (pattern == path) {
            rvalue = true;
        }
    } else {
        pm_stack_info_t stack_info = { 0 };
        int32_t pattern_cp=-1;
        int32_t path_cp=-1;
        // Checks for NULL codec internally
        ABQ_DECODER(pattern_decoder, codec, pattern, pattern_len);
        ABQ_DECODER(path_decoder, codec, path, path_len);
        path_cp = abq_decode_cp(&path_decoder);
        bool_t stop = false;
        while ((false == stop) && (0 <= path_cp)) {
            pattern_cp = abq_decode_cp(&pattern_decoder);
            switch ((byte_t)pattern_cp) {
            case '\0':
                if (0 == path_cp) {
                    // happy case everything matched
                    rvalue = true;
                    stop = true;
                } else {
                    path_cp = pm_pop_state(&stack_info,
                        &pattern_decoder, &path_decoder);
                }
                break;
            case '?':
                if ((0 >= path_cp) || (((int32_t)'/') == path_cp)) {
                    path_cp = pm_pop_state(&stack_info,
                        &pattern_decoder, &path_decoder);
                } else {
                    // no mismatch, move along
                    path_cp = abq_decode_cp(&path_decoder);
                }
                break;
            case '*':
                ABQ_DECODER_REWIND(&path_decoder, path_cp);
                do{
                    path_cp = pm_push_state(&stack_info,
                            &pattern_decoder, &path_decoder);
                } while ((0 < path_cp) && (((int32_t)'/') != path_cp));
                break;
            case '[':
                ABQ_DECODER_REWIND(&pattern_decoder, pattern_cp);
                if (rangematch(&pattern_decoder, path_cp, case_sensitive)) {
                    // Successful match, move along
                    path_cp = abq_decode_cp(&path_decoder);
                } else {
                    path_cp = pm_pop_state(&stack_info,
                        &pattern_decoder, &path_decoder);
                }
                break;
            case '\\':
                pattern_cp = abq_decode_cp(&pattern_decoder);
                if (ascii_cp_match(pattern_cp, path_cp, case_sensitive)) {
                    // no mismatch, move along
                    path_cp = abq_decode_cp(&path_decoder);
                } else {
                    path_cp = pm_pop_state(&stack_info,
                        &pattern_decoder, &path_decoder);
                }
                break;
            default:
                if(0 > pattern_cp) {
                    // Invalid text for the given codec
                    ABQ_WARN_STATUS(abq_status_pop(), "pattern");
                    stop = true;
                } else {
                    if(ascii_cp_match(pattern_cp, path_cp, case_sensitive)) {
                        // no mismatch, move along
                        path_cp = abq_decode_cp(&path_decoder);
                    } else {
                        path_cp = pm_pop_state(&stack_info,
                            &pattern_decoder, &path_decoder);
                    }
                }
                break;
            }
        }
        if((0 > path_cp) && (stack_info.index != (size_t) SIZE_MAX)) {
            EXPECT_IS_OK(abq_status_pop());
        }
    }
    return rvalue;
}

bool_t text_is_whitespace(abq_codec_t *codec, cstr_t str, int32_t length) {
    bool_t retval = true;
    // Checks for NULL codec internally
    ABQ_DECODER(decoder, codec, str, length);
    int32_t codepoint=-1;
    for(codepoint = abq_decode_cp(&decoder);
            0 < codepoint;
            codepoint = abq_decode_cp(&decoder)) {
        if(false == ascii_is_space(codepoint)){
            retval = false;
            break;
        }
    }
    if(0 > codepoint) {
        ABQ_WARN_STATUS(abq_status_pop(), "text_is_whitespace");
        retval = false;
    }
    return retval;
}

int32_t text_byte_length(abq_codec_t *codec, const cstr_t str, int32_t length) {
    // Checks for NULL codec internally
    ABQ_DECODER(decoder, codec, str, length);
    int32_t codepoint=-1;
    for(codepoint =abq_decode_cp(&decoder);
            0 < codepoint;
            codepoint = abq_decode_cp(&decoder)) {
        // Keep iterating
    }
    return (0 > codepoint) ? -1 : (int32_t)decoder.pos;
}

int32_t text_trim_length(abq_codec_t *codec, const cstr_t str, int32_t length) {
    // Checks for NULL codec internally
    ABQ_DECODER(decoder, codec, str, length);
    int32_t retval=0;
    int32_t codepoint=-1;
    for(codepoint = abq_decode_cp(&decoder);
            0 < codepoint;
            codepoint = abq_decode_cp(&decoder)) {
        if (false == ascii_is_space(codepoint)) {
            // To-be included in trimmed length if not a space
            retval = (int32_t) decoder.pos;
        }
    }
    return (0 > codepoint) ? -1 : retval;
}

int32_t text_leading_ws(abq_codec_t *codec, const cstr_t str, int32_t length){
    // Checks for NULL codec internally
    ABQ_DECODER(decoder, codec, str, length);
    err_t err = abq_decode_skip_ws(&decoder);
    if(EXIT_SUCCESS != err) {
        abq_status_set(err, false);
    }
    return (EXIT_SUCCESS != err) ? -1 : (int32_t)decoder.pos;

}

int32_t text_write_bytes(abq_codec_t *codec, byte_t *dest, const cstr_t str, int32_t allowance) {
    ABQ_DECODER(decoder, codec, str, allowance);
    ABQ_ENCODER(encoder, codec, dest, allowance);
    err_t err = abq_decode_encode(&decoder, &encoder);
    if (EXIT_SUCCESS != err) {
        abq_status_set(err, false);
    }
    return (EXIT_SUCCESS != err) ? -1 : (int32_t)encoder.pos;
}

int8_t text_compare_exact(abq_codec_t *codec, const cstr_t first, const cstr_t second, int32_t n) {
    int8_t retval = 0;
    if (NULL == first) {
        retval = (int8_t) ((NULL == second) ? 0 : 1);
    } else if (NULL == second) {
        retval = -1;
    } else if(0 != n){
        int32_t left_cp=-1;
        int32_t right_cp=-1;
        ABQ_DECODER(left, codec, first, n);
        ABQ_DECODER(right, codec, second, n);
        do {
            // Iterate until a mismatch is found
            left_cp = abq_decode_cp(&left);
            right_cp = abq_decode_cp(&right);
        }while((left_cp == right_cp) && (0 < right_cp));
        // Compare results
        if (left_cp > right_cp) {
            retval = 1;
        } else if (left_cp < right_cp) {
            retval = -1;
        } else {
            retval = 0;
        }
    } else {
        // All zero bytes are considered equal
    }
    return retval;
}

int8_t text_compare_insensitive(abq_codec_t *codec, const cstr_t first, const cstr_t second, int32_t n){
    int8_t retval = 0;
    if (NULL == first) {
        retval = (int8_t) ((NULL == second) ? 0 : 1);
    } else if (NULL == second) {
        retval = -1;
    } else if(0 != n){
        int32_t left_cp=-1;
        int32_t right_cp=-1;
        ABQ_DECODER(left, codec, first, n);
        ABQ_DECODER(right, codec, second, n);
        do {
            left_cp = abq_decode_cp(&left);
            right_cp = abq_decode_cp(&right);
            // Iterate until either the end-of string OR mismatch is found
        }while((0 != left_cp) && (ascii_cp_match(left_cp, right_cp, false)));
        // Compare results
        if (left_cp > right_cp) {
            retval = 1;
        } else if (left_cp < right_cp) {
            retval = -1;
        } else {
            retval = 0;
        }
    } else {
        // All zero bytes are considered equal
    }
    return retval;
}

bool_t text_starts_with(abq_codec_t *codec, cstr_t str, int32_t str_len,
        cstr_t prefix, int32_t prefix_len) {
    bool_t retval = false;
    ABQ_DECODER(str_decoder, codec, str, str_len);
    if(EXIT_SUCCESS == abq_decode_skip_prefix(&str_decoder, prefix, prefix_len)) {
        retval = true;
    }
    return retval;
}

int32_t text_starts_with_digits(abq_codec_t *codec, cstr_t str, int32_t str_len, abq_radix_t radix){
    int32_t count=0;
    int32_t codepoint=-1;
    int8_t digit_value = -1;
    ABQ_DECODER(decoder, codec, str, str_len);
    for (codepoint = abq_decode_cp(&decoder);
            0 < codepoint;
            codepoint = abq_decode_cp(&decoder)) {

        digit_value = hex_value_of_char(codepoint);
        if ((0 > digit_value) || (digit_value >= (int8_t)radix)){
            break;
        }
        count = (int32_t) decoder.pos;
    }
    return (0 > codepoint) ? -1 : count;
}

int32_t text_index_of(abq_codec_t *codec,
        const cstr_t super, int32_t super_len,
        const cstr_t substring, int32_t sub_len) {
    int32_t super_cp=-1;
    int32_t retval=-1;
    ABQ_DECODER(super_decoder, codec, super, super_len);
    do {
        // If text at current position starts with substring, return current position
        super_cp = (int32_t) super_decoder.pos;
        if(EXIT_SUCCESS == abq_decode_skip_prefix(&super_decoder, substring, sub_len)){
            retval = super_cp; // Return the original (not skipped) position
            break;
        }else{
            // If no match, iterate and try again
            super_cp = abq_decode_cp(&super_decoder);
        }
    }while(0 < super_cp);
    return retval;
}

int32_t text_index_of_char(abq_codec_t *codec,
        const cstr_t super, int32_t super_len, int32_t codepoint) {
    int32_t super_cp=-1;
    ABQ_DECODER(decoder, codec, super, super_len);
    for (super_cp = abq_decode_cp(&decoder);
            0 < super_cp;
            super_cp = abq_decode_cp(&decoder)) {
        if(codepoint == super_cp) {
            ABQ_DECODER_REWIND(&decoder, super_cp);
            break;
        }
    }
    return (0 >= super_cp) ? -1 : (int32_t)decoder.pos;
}

int32_t text_end_of_token(abq_codec_t *codec,
        const cstr_t super, int32_t super_len,
        const cstr_t delimiters, int32_t esc_char) {
    bool_t escaped = false;
    int32_t codepoint=-1;
    ABQ_DECODER(decoder, codec, super, super_len);
    for (codepoint = abq_decode_cp(&decoder);
            0 < codepoint;
            codepoint = abq_decode_cp(&decoder)) {
        if (escaped) {
            escaped = false;
        } else if (-1 != text_index_of_char(codec, delimiters, -1, codepoint)) {
            ABQ_DECODER_REWIND(&decoder, codepoint);
            // found a match
            break;
        } else if(codepoint == esc_char) {
            escaped = true;
        } else {
            // simple mismatch, continue iterating
        }
    }
    return (0 > codepoint) ? -1 : (int32_t)decoder.pos;
}

int32_t text_to_lower_case(abq_codec_t *codec, const cstr_t source, int32_t source_len,
        byte_t *dest, int32_t dest_len) {
    ABQ_ENCODER(encoder, codec, dest, dest_len);
    ABQ_DECODER(decoder, codec, source, source_len);
    err_t write_status = EXIT_SUCCESS;

    int32_t codepoint = abq_decode_cp(&decoder);
    while ((0 < codepoint) && (EXIT_SUCCESS == write_status)){
        // Write out the lower-case codepoint that we just read in
        write_status = abq_encode_cp(&encoder,
                ascii_tolower(codepoint));
        codepoint = abq_decode_cp(&decoder);
    }
    if (EXIT_SUCCESS != write_status) {
        abq_status_set(write_status, false);
        codepoint = -1;
    }
    return (0 > codepoint) ? -1 :  (int32_t)decoder.pos;
}

int32_t text_write_int(abq_codec_t *codec,
        byte_t *dest, size_t dest_len, int64_t source, abq_radix_t radix) {
    err_t err = EXIT_SUCCESS;
    ABQ_ENCODER(encoder, codec, dest, dest_len);
    if (NULL == dest) {
        (void) abq_status_set(EFAULT, false);
    } else {
        err = abq_encode_int(&encoder, source, radix);
        if(EXIT_SUCCESS != err) {
            abq_status_set(err, false);
        }
    }
    return (EXIT_SUCCESS != err) ? -1 :  (int32_t)encoder.pos;
}

int32_t text_write_number(abq_codec_t *codec, byte_t *dest, size_t dest_len, number_t source){
    // extract integer part
    ABQ_ENCODER(encoder, codec, dest, dest_len);
    err_t err = CHECK_NULL(dest);
    if (EXIT_SUCCESS != err) {
        // handle err below
    }else{
        err = abq_encode_number(&encoder, source);
    }
    if (EXIT_SUCCESS != err) {
        abq_status_set(err, false);
    }
    return (EXIT_SUCCESS != err) ? -1 :  (int32_t)encoder.pos;
}

int32_t text_read_int(abq_codec_t *codec,
        const cstr_t source, size_t source_len, int64_t* dest, abq_radix_t radix) {
    ABQ_DECODER(decoder, codec, source, source_len);
    err_t err = abq_decode_int(&decoder, dest, radix);
    if (EXIT_SUCCESS != err) {
        (void)abq_status_set(err, false);
    }
    return (EXIT_SUCCESS != err) ? -1 :  (int32_t)decoder.pos;
}

int32_t text_read_number(abq_codec_t *codec,
        const cstr_t source, size_t source_len, number_ptr dest) {
    ABQ_DECODER(decoder, codec, source, source_len);
    err_t err = abq_decode_number(&decoder, dest);
    if (EXIT_SUCCESS != err) {
        (void)abq_status_set(err, false);
    }
    return (EXIT_SUCCESS != err) ? -1 : (int32_t)decoder.pos;
}

number_t abq_decode_number_with_units(abq_decoder_t *decoder,
        const unit_modifier_t *units, bool_t case_sensitive) {
    number_t retval = abq_nan;
    if((NULL == decoder) || (NULL == units)) {
        abq_status_set(EFAULT, false);
    } else {
        size_t orig_pos = decoder->pos;
        err_t status = abq_decode_skip_ws(decoder);
        if (EXIT_SUCCESS == status) {
            status = abq_decode_number(decoder, &retval);
            if (EXIT_SUCCESS == status) {
                status = abq_decode_skip_ws(decoder);
            }
        }
        if(EXIT_SUCCESS != status) {
            decoder->pos = orig_pos;
            abq_status_set(status, false);
        } else if((IS_NAN(retval)) || (IS_INF(retval))){
            // Don't apply modifiers to indefinite numbers
        } else {
            unit_modifier_t *modifier = &units[0];
            while((NULL != modifier) && (NULL != modifier->unit_identifier)) {
                if(EXIT_SUCCESS == abq_decode_skip_prefix_ex(decoder,
                        modifier->unit_identifier, -1, case_sensitive)) {
                    // Match found, skip over remaining alphabetic characters ...
                    VITAL_IS_OK(abq_decode_skip_matching(decoder, ascii_is_alpha));
                    // ... and  whitespace that might follow
                    VITAL_IS_OK(abq_decode_skip_ws(decoder));
                    break;
                }
                // Iterate to next set of units
                modifier = &modifier[1];
            }

            if((NULL != modifier) && (NULL != modifier->unit_identifier)){
                retval *= modifier->multipler;
            } else if (NULL != modifier) {
                // Apply default modifier if provided
                retval *= modifier->multipler;
            } else {
                ABQ_WARN_MSG_Y("Failed to match units: ",
                        &decoder->source[decoder->pos], (decoder->max - decoder->pos));
            }
        }
    }
    return retval;
}

number_t parse_time_with_units(abq_codec_t *codec, cstr_t time_segment, size_t segment_len) {
    number_t retval = 0.0;
    if ((NULL != codec) && (NULL != time_segment) && (0U < segment_len)) {
        ABQ_DECODER(decoder, codec, time_segment, segment_len);
        retval = abq_decode_number_with_units(&decoder, units_of_time, false);
    }
    return retval;
}
