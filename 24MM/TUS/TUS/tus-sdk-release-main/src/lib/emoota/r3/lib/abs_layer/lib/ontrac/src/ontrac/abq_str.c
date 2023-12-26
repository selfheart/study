//#line 2 "ontrac/abq_str.c"
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
 * @file ontrac/abq_str.c
 * @date Mar 26, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <string.h>

#include <ontrac/ontrac/abq_str.h>
#include <ontrac/ontrac/sorted_set.h>
#include <ontrac/ontrac/abq_class.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/util/byte_buffer.h>

static void string_class_delete(cvar_t old_str);
static cvar_t string_class_coerce(cvar_t source, cvar_t ref);
static int8_t string_class_compare(cvar_t left, cvar_t right);

EMPTY_CLASS(string_class, cstr_t, NULL, string_class_coerce, string_class_compare,
        NULL, global_property_getter, global_property_setter, string_class_delete, &string_class);
DEFINE_LIST_OF_CLASS(list_of_string_class, string_class);

static int8_t strpool_compare(cvar_t left, cvar_t right) {
    return utf8_compare_exact(ptr2cstr(left), ptr2cstr(right), -1);
}

static sorted_set_t abq_strpool = {
        .cmp_fctn = strpool_compare,
        .stack = NULL,
        .count = 0U,
        .capacity = 0U
};

static int8_t utf8_compare_decoder(cstr_t cached, abq_decoder_t *right) {
    int8_t retval = 0;
    int32_t left_cp = -1;
    int32_t right_cp = -1;
    size_t orig_pos = right->pos;
    ABQ_DECODER(left, &utf8_codec, cached, -1);
    do {
        // Iterate until a mismatch is found
        left_cp = abq_decode_cp(&left);
        right_cp = abq_decode_cp(right);
    }while((left_cp == right_cp) && (0 < right_cp));
    // Compare results
    if (left_cp > right_cp) {
        retval = 1;
    } else if (left_cp < right_cp) {
        retval = -1;
    } else {
        retval = 0;
    }
    right->pos = orig_pos;
    return retval;
}

static uint16_t strpool_match(abq_decoder_t *decoder) {
    // Do a binary search on the string pool a string matching the content of the decoder
    uint16_t index = 0U;
    uint16_t lower = 0U;
    uint16_t upper = abq_strpool.count;
    int8_t cmp_val = -1;
    while (lower < upper) {
        index = (uint16_t) ((upper + lower) >> 1); // Split the difference and compare value
        cmp_val = utf8_compare_decoder(ptr2cstr(abq_strpool.stack[index]), decoder);
        if (0 == cmp_val) {
            break;
        } else if(cmp_val < 0) {
            lower = (uint16_t) (1U + index);
        } else {
            upper = index;
        }
    }
    if (0 != cmp_val) {
        index = (uint16_t) ~lower;
    }
    return index;
}

// buffer length is different from byte_length in that it must include terminator
static cstr_t str_create_internal(abq_decoder_t *source, cvar_t ref) {
    cvar_t retval = NULL;
    (void) abq_context_lock();
    // Can't have NEW_FLEX_INSTANCE change the indexes in the abq_strpool after matching the index
    //  so we must preemptively run GC every time to prevent this
    uint16_t index = strpool_match(source);
    if (index < abq_strpool.count) {
        retval = abq_strpool.stack[index];
        EXPECT_IS_OK(obj_reserve(retval, ref));
        // Mark data as consumed if match was found
        source->pos = source->max;
    } else {
        // character content + '\0' terminator
        size_t bytes_needed = 1U + (source->max - source->pos);
        byte_t *newstr = NEW_FLEX_INSTANCE(string_class, byte_t*, bytes_needed);
        if(NULL != newstr) {
            if(1U != bytes_needed) {
                // Already decoded source once to compute bytes_needed
                //  should not be possible to get an error decoding again
                VITAL_IS_OK(abq_decode_text(source, &utf8_codec, newstr, bytes_needed));
            }
            newstr[bytes_needed-1U] = '\0';
            // new item not yet found in list
            //  convert from insert index (-1 * (1+index))
            // index = ~index Parasoft hates me, alt.:
            retval = sorted_set_intern(&abq_strpool, newstr);
            assert(retval == (cvar_t)newstr);
            if (NULL != ref) {
                EXPECT_IS_OK(obj_takeover(retval, ref));
            }
        }
    }
    (void) abq_context_unlock();
    return ptr2cstr(retval);
}

// create a null terminated str_t with a class of string_class
cstr_t str_create(cstr_t source, int32_t n, bool_t trim) {
    cstr_t retval = NULL;
    if (NULL == source) {
        abq_status_set(EFAULT, false);
    } else {
        err_t status = EXIT_SUCCESS;
        // trim of trailing whitespace here
        ABQ_DECODER(decoder, &utf8_codec, source, n);
        if ( trim) {
            status = abq_decode_skip_ws(&decoder);
        }
        if (EXIT_SUCCESS != status) {
            abq_status_set(status, false);
        } else {
            int32_t codepoint = -1;
            size_t start_pos = decoder.pos;
            size_t final_pos = decoder.pos;
            for(codepoint = abq_decode_cp(&decoder);
                    0 < codepoint;
                    codepoint = abq_decode_cp(&decoder)) {
                if ((trim) && (ascii_is_space(codepoint))) {
                    // Not to be included in final_pos enless non-whitespace comes later
                } else {
                    // To-be included in trimmed length if not trimmed or space
                    final_pos = decoder.pos;
                }
            }
            if(0 > codepoint) {
                // abq_status should have already been set by decoder
            } else {
                decoder.pos = start_pos;
                decoder.max = final_pos;
                retval = str_create_internal(&decoder, NULL);
            }
        }
    }
    return retval;
}

cstr_t abq_decode_str(abq_decoder_t *decoder) {
    cstr_t retval = NULL;
    if(NULL != decoder) {
        size_t orig_pos = decoder->pos;
        int32_t byte_length = 0;
        int32_t codepoint = -1;
        do {
            codepoint = abq_decode_cp(decoder);
            byte_length += utf8_codec.bytesize(codepoint);
        } while(0 < codepoint);
        // Reset the decoder to original position
        decoder->max = decoder->pos;
        decoder->pos = orig_pos;
        if (0 == codepoint) {
            retval = str_create_internal(decoder, NULL);
        }
    }
    return retval;
}

static cstr_t str_create_with_ref(cstr_t source, int32_t n, cvar_t ref) {
    cstr_t retval = NULL;
    int32_t tmp_value = -1;
    ABQ_DECODER(decoder, &utf8_codec, source, n);
    // Decode the string to get the length
    do {
        tmp_value = abq_decode_cp(&decoder);
    } while (0 < tmp_value);
    if (0 > tmp_value) {
        // Return error as is
    } else {
        // Valid string, reset and create
        decoder.max = decoder.pos;
        decoder.pos = 0UL;
        retval = str_create_internal(&decoder, ref);
    }
    return retval;
}

// attempts to coerce the source to a cstr_t with associated class_t of string_class
cstr_t str_coerce(cvar_t source, cvar_t ref) {
    cstr_t retval = NULL;
    int32_t length = -1;
    obj_t *obj = obj_lookup(source);

    if (NULL == obj) {
        // Assume string if unclassified
        retval = ptr2cstr(source);
        static const int32_t unknown_buf_size = (int32_t) BUFFER_UNKNOWN_SIZE;
        length = utf8_byte_length(retval, unknown_buf_size);
        if((0 <= length) && (length < unknown_buf_size)) {
            retval = str_create_with_ref(retval, length, ref);
        } else {
            retval = NULL;
        }
    } else if (&string_class == obj->meta) {
        retval = ptr2cstr(obj->member);
        (void) obj_reserve(retval, ref);
    } else if (&null_class == obj->meta) {
        retval = NULL; // Don't return abq_null_str or equivalent
    } else if (&bool_class == obj->meta) {
        if ((cvar_t)obj->member == (cvar_t) true_ptr) {
            retval = str_create_with_ref(abq_true_str, 4, ref);
        } else {
            retval = str_create_with_ref(abq_false_str, 5, ref);
        }
    } else if (&number_class == obj->meta) {
        number_t number = number_resolve(obj->member);
        if (IS_NAN(number)) {
            retval = NULL;
        } else {
            byte_t buf[B32_SIZE] = { 0 };
            length = utf8_write_number(buf, sizeof(buf), number);
            if (0 < length) {
                retval = str_create_with_ref(buf, length, ref);
            }
        }
    } else {
        (void) abq_status_set(ENOSYS, false);
        retval = NULL;
    }
    return retval;
}

static cvar_t string_class_coerce(cvar_t source, cvar_t ref) {
    return (cvar_t) str_coerce(source, ref);
}

cstr_t str_resolve(cvar_t item) {
    cstr_t retval = NULL;
    CLASS_RESOLVE(string_class, cstr_t, retval, item);
    return retval;
}

cstr_t str_lookup(cstr_t source) {
    (void) abq_context_lock();
    cvar_t retval
        = sorted_set_lookup(&abq_strpool, source);
    (void) abq_context_unlock();
    return ptr2cstr(retval);
}

static void string_class_delete(cvar_t old_str) {
    (void) abq_context_lock();
    VITAL_IS_OK(sorted_set_remove(&abq_strpool, old_str));
    (void) abq_context_unlock();
}

cstr_t str_to_lower(const cstr_t source, int32_t n) {
    cstr_t rvalue = NULL;
    int32_t len = utf8_byte_length(source, n);
    if (0 > len) {
        // Error verifying string length
    } else {
        byte_t *buff = abq_malloc_ex((size_t)len, NULL);
        if (NULL == buff) {
            abq_status_set(ENOMEM, false);
        } else {
            len = utf8_to_lower_case(source, len, buff, len);
            if(0 <= len) {
                rvalue = str_create(buff, len, false);
            }
            abq_free(buff);
        }
    }
    return rvalue;
}

static int8_t string_class_compare(cvar_t left, cvar_t right) {
    class_ptr left_meta = class_of(left);
    if (NULL == left_meta) {
        // Assume string if unclassified
        left_meta = &string_class;
    }
    class_ptr right_meta = class_of(right);
    if (NULL == right_meta) {
        // Assume string if unclassified
        right_meta = &string_class;
    }
    int8_t retval = class_class.compare((cvar_t)left_meta, (cvar_t)right_meta);
    if ((0 == retval) && (&string_class == left_meta)) {
        retval = utf8_compare_exact(ptr2cstr(left), ptr2cstr(right), -1);
    }
    return retval;
}

// splits string into segments that were joined with separator
vlist_t *str_split(cstr_t source,
                   const cstr_t separator,
                   bool_t trim_strings,
                   bool_t keep_empty_strings) {
    vlist_t *rvalue = vlist_create(&string_class);
    if (NULL == rvalue) {
        abq_status_set(EFAULT, false);
    } else {
        cstr_t input = source;
        int32_t sep_len = utf8_byte_length(separator, -1);
        int32_t loc = utf8_index_of(input, -1, separator, sep_len);
        cstr_t node = NULL;
        err_t err = EXIT_SUCCESS;
        while(0 <= loc) {
            if((0 < loc) || (keep_empty_strings)){
                node = str_create(input, loc, trim_strings);
                if (NULL == node) {
                    err = ENOMEM;
                    break;
                }
                VITAL_IS_OK(vlist_add(rvalue, node));
                // rvalue now has the reference to node
                VITAL_IS_OK(obj_release_self((var_t)node));
                // skip over the token
                input = &input[loc];
            }
            // skip over separator
            input = &input[sep_len];
            loc = utf8_index_of(input, -1, separator, sep_len);
        }
        if (EXIT_SUCCESS == err) {
            loc = utf8_byte_length(input, -1);
            if ((0 < loc) || (keep_empty_strings)) {
                node = str_create(input, loc, trim_strings);
                if (NULL == node) {
                    err = ENOMEM;
                } else {
                    VITAL_IS_OK(vlist_add(rvalue, node));
                    // rvalue now has the reference to node
                    VITAL_IS_OK(obj_release_self((var_t)node));
                }
            }
        }
        if (EXIT_SUCCESS != err) {
            abq_status_set(err, false);
            VITAL_IS_OK(obj_release_self((var_t)rvalue));
            rvalue = NULL;
        }
    }
    return rvalue;
}

// joins strings listed in str_list into a singular string coupled by separator
cstr_t str_join(vlist_t *str_list,
                const cstr_t separator,
                bool_t release_list_when_done) {
    cstr_t retval = NULL;
    abq_context_lock();
    ABQ_ENCODER(encoder,
            &utf8_codec, abq_ctx.buffer, abq_ctx.buf_size);

    err_t status = EXIT_SUCCESS;
    cstr_t substr = NULL;
    VLIST_LOOP (str_list, cstr_t, substr) {
        if((0u != encoder.pos) && (NULL != separator)) {
            status = abq_encode_text(&encoder, &utf8_codec, separator, -1); // delimit subsequent substr
        }
        if (EXIT_SUCCESS == status) {
            status =  abq_encode_text(&encoder, &utf8_codec, substr, -1);
        }
        if(EXIT_SUCCESS != status) {
            break;
        }
    }
    if (EXIT_SUCCESS != status) {
        abq_status_set(status, false);
    } else {
        retval = str_create(abq_ctx.buffer, (int32_t)encoder.pos, false);
    }
    if (release_list_when_done) {
        (void) obj_release_self(str_list);
    }
    abq_context_unlock();
    return retval;
}

cstr_t str_format_int(int64_t value) {
    cstr_t retval = NULL;
    byte_t tmp[32];
    ABQ_ENCODER(encoder, &utf8_codec, tmp, sizeof(tmp));
    err_t err = abq_encode_int(&encoder, value, DECIMAL_RADIX);
    if (EXIT_SUCCESS != err) {
        abq_status_set(err, false);
    } else {
        ABQ_DECODER(decoder, &utf8_codec, tmp, encoder.pos);
        retval = str_create_internal(&decoder, NULL);
    }
    return retval;
}

cstr_t str_format_oct(uint64_t value) {
    cstr_t retval = NULL;
    byte_t tmp[32];
    ABQ_ENCODER(encoder, &utf8_codec, tmp, sizeof(tmp));
    VITAL_IS_OK(abq_encode_char(&encoder, '0'));
    err_t err = abq_encode_uint(&encoder, value, OCTAL_RADIX);
    if (EXIT_SUCCESS != err) {
        abq_status_set(err, false);
    } else {
        retval = str_create(tmp, (int32_t)encoder.pos, false);
    }
    return retval;
}
