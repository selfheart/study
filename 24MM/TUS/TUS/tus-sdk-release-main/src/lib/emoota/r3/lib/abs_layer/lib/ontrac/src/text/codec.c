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
 * @file codec.c
 * @date Jan 15, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */
#include <ctype.h>
#include <string.h>

#include <ontrac/text/codec.h>
#include <ontrac/text/ascii.h>
#include <ontrac/text/hex_enc.h>
#include <ontrac/ontrac/status_codes.h>

// limit to the  number of digits to print after decimal point
#define DEFAULT_MAX_MANTISSA ((size_t)16U)
// limit to the total number of digits to format/parse for a given number
#define MAX_DIGITS_PER_NUMBER ((size_t)32U)

const number_t abq_nan = 0.0/0.0;
const number_t abq_inf_pos = 1.0/0.0;
const number_t abq_inf_neg = -1.0/0.0;

const cstr_t abq_null_str = "null"; // parasoft-suppress CERT_C-MSC41-a-1 "c0519. This string does not contain sensitive information."
const cstr_t abq_true_str = "true"; // parasoft-suppress CERT_C-MSC41-a-1 "c0520. This string does not contain sensitive information."
const cstr_t abq_false_str = "false"; // parasoft-suppress CERT_C-MSC41-a-1 "c0521. This string does not contain sensitive information."
const cstr_t abq_nan_str = "nan"; // parasoft-suppress CERT_C-MSC41-a-1 "c0522. This string does not contain sensitive information."
const cstr_t abq_inf_pos_str = "+inf"; // parasoft-suppress CERT_C-MSC41-a-1 "c0523. This string does not contain sensitive information."
const cstr_t abq_inf_neg_str = "-inf"; // parasoft-suppress CERT_C-MSC41-a-1 "c0524. This string does not contain sensitive information."

// Decimal characters only
// TODO add "," for thousands separator & "eE" for scientific notation
const cstr_t numeric_characters = "0123456789.+-"; // parasoft-suppress CERT_C-MSC41-a-1 "c0525. This string does not contain sensitive information."

static int32_t raw_bytesize(int32_t codepoint) {
    return ((0 > codepoint) || (0xFF < codepoint)) ? -1 : 1;
}

static err_t raw_encoding(abq_encoder_t *encoder, int32_t codepoint) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if ((0 > codepoint) || (0xFF < codepoint)) {
        retval = EINVAL;
    } else if(NULL == encoder->dest) {
        retval = EFAULT;
    } else if( ((size_t)0 == encoder->pos)
            && ((size_t)BUFFER_UNKNOWN_SIZE == encoder->max) ) {
        // DO NOT use the RAW codec for indeterminate lengths,
        //  Overwrite the codec with the ascii_codec
        ABQ_WARN_STATUS(EPERM, "RAW bytes of unknown size");
        encoder->codec = &ascii_codec;
        retval = ascii_codec.encode(encoder, codepoint);
    } else {
        if (encoder->pos >= encoder->max) {
            retval = EOVERFLOW;
        } else {
            encoder->dest[encoder->pos] = (byte_t) ((uint8_t) codepoint);
            encoder->pos += (size_t) 1;
            if (encoder->max > encoder->pos) {
                // append a terminator to next position if able
                encoder->dest[encoder->pos] = '\0';
            }
        }
    }
    return retval;
}

static int32_t raw_decoding (abq_decoder_t *decoder) {
    int32_t codepoint = -1;
    if ((NULL == decoder) || (NULL == decoder->source)) {
        abq_status_set(EFAULT, false);
    } else if( ((size_t)0 == decoder->pos)
            && ((size_t)BUFFER_UNKNOWN_SIZE == decoder->max) ) {
        // DO NOT use the RAW codec for indeterminate lengths,
        //  Overwrite the codec with the ascii_codec
        ABQ_WARN_STATUS(EPERM, "RAW bytes of unknown size");
        decoder->codec = &ascii_codec;
        codepoint = ascii_codec.decode(decoder);
    } else {
        if (decoder->pos >= decoder->max) {
            abq_status_set(ENODATA, true);
        } else {
            codepoint = (int32_t) ((uint8_t) decoder->source[decoder->pos]);
            decoder->pos += (size_t) 1U;
        }
    }
    return codepoint;
}

const abq_codec_t raw_codec = {
        .bytesize = raw_bytesize,
        .encode = raw_encoding,
        .decode = raw_decoding
};


err_t abq_decode_skip_matching(abq_decoder_t *decoder, abq_cp_matcher matcher) {
    err_t retval = EXIT_SUCCESS;
    if((NULL == decoder) || (NULL == matcher)) {
        retval = EFAULT;
    }else{
        size_t orig_pos = decoder->pos;
        int32_t codepoint = -1;
        for(codepoint = abq_decode_cp(decoder);
                0 < codepoint;
                codepoint = abq_decode_cp(decoder)) {
            if(false == matcher(codepoint)){
                // Not to be included in leading whitespace
                ABQ_DECODER_REWIND(decoder, codepoint);
                break;
            }
        }
        if (0 > codepoint) {
            retval = abq_status_take(EIO);
            if (ENODATA == retval) {
                retval = EXIT_SUCCESS;
            } else {
                decoder->pos = orig_pos;
            }
        }
    }
    return retval;
}

err_t abq_decode_skip_prefix_ex(abq_decoder_t *decoder,
        cstr_t prefix, int32_t length, bool_t case_sensitive) {
    err_t retval = CHECK_NULL(decoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else {
        int32_t prefix_cp = -1;
        size_t orig_pos = decoder->pos;
        ABQ_DECODER(prefix_decoder, decoder->codec, prefix, length);
        // Loop through each character of the prefix
        for (prefix_cp = abq_decode_cp(&prefix_decoder);
                0 < prefix_cp;
                prefix_cp = abq_decode_cp(&prefix_decoder)) {
            // And check for a matching character in the string
            int32_t codepoint = abq_decode_cp(decoder);
            if (!ascii_cp_match(prefix_cp, codepoint, case_sensitive)) {
                decoder->pos = orig_pos;
                if (0 > codepoint) {
                    retval = abq_status_take(EILSEQ);
                }else if(0 == codepoint) {
                    retval = ENODATA;
                } else {
                    retval = EINVAL;
                }
                break;
            }
        }
        if (0 > prefix_cp) {
            retval = abq_status_take(EINVAL);
            if (ENODATA == retval) {
                retval = EXIT_SUCCESS;
            } else {
                decoder->pos = orig_pos;
            }
        }
    }
    return retval;
}

err_t abq_decode_encode(abq_decoder_t *decoder, abq_encoder_t *encoder) {
    err_t retval = EXIT_SUCCESS;
    if ((NULL == decoder) || (NULL == encoder)) {
            retval = EFAULT;
    } else {
        int32_t codepoint=0;
        size_t orig_decoder = decoder->pos;
        size_t orig_encoder = encoder->pos;
        while (decoder->max > decoder->pos) {
            codepoint = abq_decode_cp(decoder);
            if(0 >= codepoint) {
                if (0 == codepoint) {
                    decoder->max = decoder->pos;
                } else {
                    retval = abq_status_take(EIO);
                }
            } else {
                // Write out the codepoint we just read in
                retval = abq_encode_cp(encoder, codepoint);
            }
            if (EXIT_SUCCESS != retval) {
                decoder->pos = orig_decoder;
                encoder->pos = orig_encoder;
                break;
            }
        }
    }
    return retval;
}

err_t abq_encode_ascii(abq_encoder_t *encoder, cstr_t ascii, int32_t length) {
    ABQ_DECODER(decoder, &ascii_codec, ascii, length);
    if(NULL == decoder.source) {
        decoder.source = abq_null_str; // or abq_empty_str ?
        decoder.max = 4;
    }
    return abq_decode_encode(&decoder, encoder);
}

err_t abq_encode_text(abq_encoder_t *encoder, abq_codec_t *codec, cstr_t text, int32_t length) {
    ABQ_DECODER(decoder, codec, text, length);
    return abq_decode_encode(&decoder, encoder);
}

err_t abq_encode_loginfo(abq_encoder_t *encoder, cstr_t loginfo, int32_t length) {
    cstr_t printable = (NULL == loginfo) ? abq_null_str : loginfo;
    ABQ_DECODER(decoder, &ascii_codec, printable, length);
    err_t retval = abq_decode_encode(&decoder, encoder);
    static cstr_t ellipses = "..."; // parasoft-suppress CERT_C-MSC41-a-1 "c0526. This string does not contain sensitive information."
    if ((EOVERFLOW == retval)
            && ((encoder->max - encoder->pos) > (size_t) text_byte_length(encoder->codec, ellipses, -1))) {
        encoder->pos = encoder->max;
        encoder->pos -= 1U + (size_t) text_byte_length(encoder->codec, ellipses, -1);
        retval = abq_encode_text(encoder, encoder->codec, ellipses, -1);
    }
    return retval;
}

typedef struct {
    uint64_t value;
    uint8_t radix;
    bool_t negative;
    size_t digit_count;
    int32_t digits[MAX_DIGITS_PER_NUMBER];
} abq_digitizer_t;

/** Fills in digit character for a integer value, in REVERSE order */
static err_t abq_digitize_int(abq_digitizer_t *digitizer) {
    err_t retval = EXIT_SUCCESS;
    digitizer->digit_count = 0;
    do {
        if(digitizer->digit_count >= MAX_DIGITS_PER_NUMBER){
            retval = ENOBUFS;
            break;
        }
        digitizer->digits[digitizer->digit_count]
             = (int32_t) (uint8_t) upper_hex_digits[digitizer->value % (uint64_t) digitizer->radix];
        digitizer->value /= (uint64_t) digitizer->radix;
        digitizer->digit_count += (size_t) 1;
    } while (0UL != digitizer->value);
    return retval;
}

static err_t abq_encode_digitizer(abq_encoder_t *encoder,
        abq_digitizer_t *digitizer, byte_t pad, size_t char_count) {
    err_t retval = EXIT_SUCCESS;
    size_t orig_pos = encoder->pos;
    if (digitizer->negative) {
        retval = abq_encode_char(encoder, '-');
    }
    while ((EXIT_SUCCESS == retval) && (char_count > digitizer->digit_count)
            && ((encoder->pos - orig_pos) < (char_count - digitizer->digit_count))) {
        retval = abq_encode_char(encoder, pad);
    }
    while ((EXIT_SUCCESS == retval)
            && (digitizer->digit_count > (size_t) 0)) {
        // Decrement the digit_count and encode the charcater
        digitizer->digit_count -= (size_t) 1;
        // Encode the integer digit character
        retval = abq_encode_cp(encoder,
                digitizer->digits[digitizer->digit_count]);
    }
    if (EXIT_SUCCESS != retval) {
        // Reset encoder to orig_pos
        encoder->pos = orig_pos;
        // Return error value as is
    }
    return retval;
}

static err_t abq_digitize_and_encode(abq_encoder_t *encoder,
        abq_digitizer_t *digitizer, byte_t pad, size_t char_count) {
    err_t retval = abq_digitize_int(digitizer);
    // check global status
    if (EXIT_SUCCESS != retval) {
        // Return error value as is
    } else {
        retval = abq_encode_digitizer(encoder, digitizer, pad, char_count);
    }
    return retval;
}

err_t abq_encode_int(abq_encoder_t *encoder, int64_t value, abq_radix_t radix) {
    err_t retval = CHECK_NULL(encoder);
    // write to the end of tmp so we don't have to reverse later
    if(EXIT_SUCCESS != retval) {
        // Return error code
    } else if (abq_radix_is_invalid(radix)) {
        retval = EINVAL; // Unsupported RADIX
    }else{
        abq_digitizer_t digitizer;
        (void)memset(&digitizer, 0, sizeof(abq_digitizer_t));
        digitizer.radix = (uint8_t) radix;
        digitizer.value = (uint64_t) value;
        if (0 > value) {
            digitizer.negative = true;
            digitizer.value = (~digitizer.value) + 1UL;
        }else{
            digitizer.negative = false;
        }
        retval = abq_digitize_and_encode(encoder,
                &digitizer, '0', 0U);
    }
    return retval;
}

err_t abq_encode_uint(abq_encoder_t *encoder, uint64_t value, abq_radix_t radix) {
    err_t retval = CHECK_NULL(encoder);
    // write to the end of tmp so we don't have to reverse later
    if(EXIT_SUCCESS != retval) {
        // Return error code
    } else if (abq_radix_is_invalid(radix)) {
        retval = EINVAL; // Unsupported RADIX
    }else{
        abq_digitizer_t digitizer;
        (void) memset(&digitizer, 0, sizeof(abq_digitizer_t));
        digitizer.radix = (uint8_t) radix;
        digitizer.negative = false;
        digitizer.value = value;
        retval = abq_digitize_and_encode(encoder,
                &digitizer, '0', 0U);
    }
    return retval;
}

err_t abq_encode_left_padded_int(abq_encoder_t *encoder,
        int64_t value, abq_radix_t radix, byte_t pad, size_t char_count) {
    err_t retval = CHECK_NULL(encoder);
    // write to the end of tmp so we don't have to reverse later
    if (EXIT_SUCCESS != retval) {
        // Return error code
    } else if (abq_radix_is_invalid(radix)) {
        retval = EINVAL; // Unsupported RADIX
    } else {
        abq_digitizer_t digitizer;
        (void) memset(&digitizer, 0, sizeof(abq_digitizer_t));
        digitizer.radix = (uint8_t) radix;
        digitizer.value = (uint64_t) value;
        if (0 > value) {
            digitizer.negative = true;
            digitizer.value = (~digitizer.value) + 1UL;
        }else{
            digitizer.negative = false;
        }
        retval = abq_digitize_and_encode(encoder,
                &digitizer, pad, char_count);
    }
    return retval;
}

err_t abq_encode_left_padded_uint(abq_encoder_t *encoder,
        uint64_t value, abq_radix_t radix, byte_t pad, size_t char_count) {
    err_t retval = CHECK_NULL(encoder);
    // write to the end of tmp so we don't have to reverse later
    if (EXIT_SUCCESS != retval) {
        // Return error code
    } else if (abq_radix_is_invalid(radix)) {
        retval = EINVAL; // Unsupported RADIX
    } else {
        abq_digitizer_t digitizer;
        (void) memset(&digitizer, 0, sizeof(abq_digitizer_t));
        digitizer.radix = (uint8_t)radix;
        digitizer.negative = false;
        digitizer.value = value;
        retval = abq_digitize_and_encode(encoder,
                &digitizer, pad, char_count);
    }
    return retval;
}

err_t abq_encode_ptraddr(abq_encoder_t *encoder, cvar_t ptr) {
    err_t retval = abq_encode_ascii(encoder, " 0x", 3); // parasoft-suppress CERT_C-MSC41-a-1 "c0527. This string does not contain sensitive information."
    if (EXIT_SUCCESS != retval) {
        // Return error code
    } else {
        retval = abq_encode_left_padded_uint(encoder,
                        ptr2addr(ptr), HEX_RADIX, '0', 8);
    }
    return retval;
}

err_t abq_encode_decimal(abq_encoder_t *encoder,
        number_t value, size_t max_mantissa, bool_t trim) {
    err_t retval = CHECK_NULL(encoder);
    if (EXIT_SUCCESS != retval) {
        // Return error code
    } else if(max_mantissa > DEFAULT_MAX_MANTISSA){
        retval = ERANGE;
    } else if (IS_NAN(value)) {
        retval = abq_encode_ascii(encoder, abq_nan_str, -1);
    } else if (abq_inf_pos == value) {
        retval = abq_encode_ascii(encoder, abq_inf_pos_str, -1);
    } else if (abq_inf_neg == value) {
        retval = abq_encode_ascii(encoder, abq_inf_neg_str, -1);
    } else {
        size_t orig_pos = encoder->pos;
        // Break number into integer and mantissa values
        int64_t integer = (int64_t) value;
        number_t mantissa = (value - (number_t) integer);
        // encode the integer portion off the number
        retval = abq_encode_int(encoder, integer, DECIMAL_RADIX);
        if ((EXIT_SUCCESS == retval) && (0U != max_mantissa)) {
            uint8_t digits[DEFAULT_MAX_MANTISSA] = {0};
            size_t digit_count = 0U;
            if(0.0 != mantissa) {
                if (mantissa < 0.0) {
                    mantissa *= -1.0;
                }
                // write out each digit of the mantissa into an array
                while (digit_count < max_mantissa) {
                    mantissa *= 10.0;
                    digits[digit_count] = (uint8_t) mantissa;
                    mantissa -= (number_t) digits[digit_count];
                    digit_count += 1U;
                    if (0.0 == mantissa) {
                        break;
                    }
                }
            }
            if (trim) {
                // remove trailing zero's from the mantissa
                while(0U != digit_count) {
                    // Optimized for trailing zeros
                    digit_count -= 1U;
                    if((uint8_t) 0U != digits[digit_count]) {
                        // Wasn't a trailing zero, re-add to count
                        digit_count += 1U;
                        break;
                    }
                }
            } else if(digit_count < max_mantissa){
                // Include trailing zeros
                digit_count = max_mantissa;
            } else {
                // Already optimized
            }
            if (0U != digit_count) {
                size_t digit_index = 0U;
                // write out mantissa if has any value
                retval = abq_encode_char(encoder, '.');
                while((EXIT_SUCCESS == retval)
                        && (digit_index < digit_count)) {
                    retval = abq_encode_char(encoder, upper_hex_digits[digits[digit_index]]);
                    digit_index += 1U;
                }
            }
        }
        if (EXIT_SUCCESS != retval) {
            // Reset encoder pos on error
            encoder->pos = orig_pos;
        }
    }
    return retval;
}

err_t abq_encode_number(abq_encoder_t *encoder, number_t value) {
    return abq_encode_decimal(encoder, value, DEFAULT_MAX_MANTISSA, true);
}

err_t abq_decode_text(abq_decoder_t *decoder, abq_codec_t *codec, byte_t *dest, size_t limit) {
    err_t retval = EXIT_SUCCESS;
    ABQ_ENCODER(encoder, codec, dest, limit);
    if (0U < limit) {
        retval = abq_decode_encode(decoder, &encoder);
    }
    return retval;
}

err_t abq_decode_int(abq_decoder_t *decoder, int64_t* dest, abq_radix_t radix) {
    err_t retval = CHECK_NULL(decoder);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if (abq_radix_is_invalid(radix)) {
        retval = EINVAL; // Unsupported RADIX
    } else {
        size_t orig_pos = decoder->pos;
        byte_t ascii = abq_decode_char(decoder);
        bool_t negative = false;
        if ('\0' == ascii) {
            retval = ENODATA;
        } else if (ascii_is_friendly((int32_t)(uint8_t)ascii)) {
            if ('-' == ascii) {
                negative = true;
                ascii = abq_decode_char(decoder);
            } else if ('+' == ascii) {
                ascii = abq_decode_char(decoder);
            } else {
                // Handle numeric characters below
            }
            if ('\0' == ascii) {
                retval = ENODATA;
            }
        } else {
            retval = abq_status_take(EINVAL);
        }
        if (EXIT_SUCCESS != retval) {
            // error already set
        } else {
            int8_t digit_value = hex_value_of_char((int32_t)(uint8_t)ascii);
            if ((-1 == digit_value) || (digit_value >= (int8_t)radix)) {
                // Error, reset index
                decoder->pos = orig_pos;
                retval = EILSEQ;
            } else {
                int64_t int_value = (int64_t) digit_value;
                while (decoder->max > decoder->pos) {
                    ascii = abq_decode_char(decoder);
                    digit_value = hex_value_of_char((int32_t)(uint8_t)ascii);
                    if ((0 > digit_value) || (digit_value >= (int8_t) radix)) {
                        if ((byte_t)0 > ascii) {
                            // Error, reset index
                            decoder->pos = orig_pos;
                            retval = abq_status_take(EINVAL);
                        } else {
                            // rewind decoder so that the final non-decimal
                            //  character is not included in results
                            ABQ_DECODER_REWIND(decoder, ascii);
                        }
                        break;
                    }
                    int_value *= (int64_t) radix;
                    int_value += (int64_t) digit_value;
                }
                if(EXIT_SUCCESS != retval){
                    // Return error as is
                    VITAL_VALUE(decoder->pos, orig_pos);
                }else{
                    if (negative) {
                        int_value *= -1;
                    }
                    if (NULL != dest) {
                        *dest = int_value;
                    }
                }
            }
        }
    }
    return retval;
}

err_t abq_decode_number(abq_decoder_t *decoder, number_ptr dest) {
    err_t retval = CHECK_NULL(decoder);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else {
        int64_t integer = 0; // leading integer
        size_t orig_pos = decoder->pos;
        retval = abq_decode_int(decoder, &integer, DECIMAL_RADIX);
        if (EILSEQ == retval) {
            if( (int32_t)'.' == abq_decode_cp(decoder)){
                // clear the illegal-sequence err from parsing integer
                retval = EXIT_SUCCESS;
                integer = 0; // Default value for omitted integer
            }
            decoder->pos = orig_pos;
        }
        if (EXIT_SUCCESS != retval) {
            // TODO build ignore-case parameter into list of arguments
            if (EXIT_SUCCESS == abq_decode_skip_prefix(decoder, abq_nan_str, -1)) {
                // clear the illegal-sequence err from parsing integer
                retval = EXIT_SUCCESS;
                if (NULL != dest) {
                    *dest = abq_nan;
                }
            } else if (EXIT_SUCCESS == abq_decode_skip_prefix(decoder, abq_inf_pos_str, -1)) {
                // clear the illegal-sequence err from parsing integer
                retval = EXIT_SUCCESS;
                if (NULL != dest) {
                    *dest = abq_inf_pos;
                }
            } else if (EXIT_SUCCESS == abq_decode_skip_prefix(decoder, abq_inf_neg_str, -1)) {
                // clear the illegal-sequence err from parsing integer
                retval = EXIT_SUCCESS;
                if (NULL != dest) {
                    *dest = abq_inf_neg;
                }
                // check for leading decimal point, would cause integer parser to fail on (example: ".1")
            } else {
                // Return as is if failed to parse integer
            }
        } else {
            // store number in temp area and don't set dest until it is ready
            number_t numeric = 0.0;
            int32_t codepoint = abq_decode_cp(decoder);
            if ( (int32_t)'.' == codepoint) {
                size_t digit_index = 0U;
                int8_t digits[DEFAULT_MAX_MANTISSA];
                while (digit_index < (size_t) DEFAULT_MAX_MANTISSA) {
                    codepoint = abq_decode_cp(decoder);
                    digits[digit_index] = hex_value_of_char(codepoint);
                    if ( (0 > digits[digit_index])
                            || (digits[digit_index] >= (int8_t)DECIMAL_RADIX) ){
                        if (0 > codepoint) {
                            // Error reading steam
                            retval = abq_status_take(EIO);
                            decoder->pos = orig_pos;
                        } else {
                            // rewind decoder so that character is not included in retval
                            ABQ_DECODER_REWIND(decoder, codepoint);
                        }
                        break;
                    }
                    digit_index += 1U;
                }
                while (0U != digit_index) {
                    digit_index -= 1U;
                    // read in decimals from right to left
                    numeric += (number_t) digits[digit_index];
                    numeric *= 0.1;
                }
            } else if (0 > codepoint) {
                // Error reading steam
                retval = abq_status_take(EIO);
                decoder->pos = orig_pos;
            } else {
                // rewind decoder so that character is not included in retval
                ABQ_DECODER_REWIND(decoder, codepoint);
            }
            if (EXIT_SUCCESS != retval) {
                // Return error as is
            } else {
                if ( (int32_t)'e' == tolower(codepoint)) {
                    ABQ_WARN_MSG("TODO handle scientific notation");
                }
                if (integer < 0) {
                    numeric *= -1.0;
                }
                numeric += (number_t) integer;
                if (NULL != dest) {
                    *dest = numeric;
                }
            }
        }
    }
    return retval;
}
