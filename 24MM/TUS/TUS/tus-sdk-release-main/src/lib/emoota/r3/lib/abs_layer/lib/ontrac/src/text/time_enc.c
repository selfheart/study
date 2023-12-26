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
 * @file time_enc.c
 * @date Jun 14, 2019
 * @author fteichmann
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/ontrac/status_codes.h>
#include <ontrac/text/time_enc.h>


#define ONE_MINUTE_IN_SEC (60.0)
#define ONE_HOUR_IN_SEC (3600.0)

#define en_month_short "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
#define de_month_short "Jan","Feb","Mär","Apr","Mai","Jun","Jul","Aug","Sep","Oct","Nov","Dez"
#define jp_month_short "01月","02月","03月","04月","05月","06月","07月","08月","09月","10月","11月","12月"

#define en_month_long "January","February","March","April","May","June","July","August","September","October","November","December"
#define de_month_long "Januar","Februar","März","April","Mai","Juni","Juli","August","September","Oktober","November","Dezember"
#define jp_month_long "01月","02月","03月","04月","05月","06月","07月","08月","09月","10月","11月","12月"

#define en_day_short "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
#define de_day_short "Son","Mon","Die","Mit","Don","Fre","Sam"
#define jp_day_short "日曜日","月曜日","火曜日","水曜日","木曜日","金曜日","土曜日"

#define en_day_long "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
#define de_day_long "Sonntag","Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag"
#define jp_day_long "日曜日","月曜日","火曜日","水曜日","木曜日","金曜日","土曜日"


/* !! the array sequence must match the enum sequence of type abq_locale_t !! */
static struct {
    abq_locale_t locale;
    cstr_t data[12];
} time_enc_month_short_locale_map[3] = {
        {.locale=abq_locale_en, .data={en_month_short}}, // parasoft-suppress CERT_C-MSC41-a-1 "c0530. This string (in array) does not contain sensitive information."
        {.locale=abq_locale_jp, .data={jp_month_short}}, // parasoft-suppress CERT_C-MSC41-a-1 "c0531. This string (in array) does not contain sensitive information."
        {.locale=abq_locale_de, .data={de_month_short}}  // parasoft-suppress CERT_C-MSC41-a-1 "c0532. This string (in array) does not contain sensitive information."
};

/* !! the array sequence must match the enum sequence of type abq_locale_t !! */
static struct {
    abq_locale_t locale;
    cstr_t data[12];
} time_enc_month_long_locale_map[3] = {
        {.locale=abq_locale_en, .data={en_month_long}}, // parasoft-suppress CERT_C-MSC41-a-1 "c0533. This string (in array) does not contain sensitive information."
        {.locale=abq_locale_jp, .data={jp_month_long}}, // parasoft-suppress CERT_C-MSC41-a-1 "c0534. This string (in array) does not contain sensitive information."
        {.locale=abq_locale_de, .data={de_month_long}}  // parasoft-suppress CERT_C-MSC41-a-1 "c0535. This string (in array) does not contain sensitive information."
};

/* !! the array sequence must match the enum sequence of type abq_locale_t !! */
static struct {
    abq_locale_t locale;
    cstr_t data[7];
} time_enc_wkday_short_locale_map[3] = {
        {.locale=abq_locale_en, .data={en_day_short}}, // parasoft-suppress CERT_C-MSC41-a-1 "c0536. This string (in array) does not contain sensitive information."
        {.locale=abq_locale_jp, .data={jp_day_short}}, // parasoft-suppress CERT_C-MSC41-a-1 "c0537. This string (in array) does not contain sensitive information."
        {.locale=abq_locale_de, .data={de_day_short}}  // parasoft-suppress CERT_C-MSC41-a-1 "c0538. This string (in array) does not contain sensitive information."
};

/* !! the array sequence must match the enum sequence of type abq_locale_t !! */
static struct {
    abq_locale_t locale;
    cstr_t data[7];
} time_enc_wkday_long_locale_map[3] = {
        {.locale=abq_locale_en, .data={en_day_long}}, // parasoft-suppress CERT_C-MSC41-a-1 "c0539. This string (in array) does not contain sensitive information."
        {.locale=abq_locale_jp, .data={jp_day_long}}, // parasoft-suppress CERT_C-MSC41-a-1 "c0540. This string (in array) does not contain sensitive information."
        {.locale=abq_locale_de, .data={de_day_long}}  // parasoft-suppress CERT_C-MSC41-a-1 "c0541. This string (in array) does not contain sensitive information."
};

static err_t enc_month_char_short(abq_encoder_t *enc, uint16_t input, abq_locale_t locale) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 11U) {
        cstr_t monthval = time_enc_month_short_locale_map[locale].data[input];
        retval = abq_encode_ascii(enc, monthval, 3);
    }
    return retval;
}

static uint16_t dec_month_char_short(abq_decoder_t *dec, abq_locale_t locale) {
    uint16_t rvalue = 0U;
    uint16_t cnt = 0U;
    while (cnt <= 11U) {
        if (EXIT_SUCCESS == abq_decode_skip_prefix(dec, time_enc_month_short_locale_map[locale].data[cnt], 3)) {
            rvalue = cnt;
            break;
        }
        cnt++;
    }
    return rvalue;
}

static err_t enc_month_char_long(abq_encoder_t *enc, uint16_t input, abq_locale_t locale) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 11U) {
        cstr_t monthval = time_enc_month_long_locale_map[locale].data[input];
        retval = abq_encode_ascii(enc, monthval, -1);
    }
    return retval;
}

static uint16_t dec_month_char_long(abq_decoder_t *dec, abq_locale_t locale) {
    uint16_t rvalue = 0U;
    uint16_t cnt = 0U;
    while (cnt <= 11U) {
        if (EXIT_SUCCESS == abq_decode_skip_prefix(dec, time_enc_month_long_locale_map[locale].data[cnt],
                                                   utf8_byte_length(time_enc_month_long_locale_map[locale].data[cnt],
                                                                    -1))) {
            rvalue = cnt;
            break;
        }
        cnt++;
    }
    return rvalue;
}

static err_t enc_month_dec(abq_encoder_t *enc, uint16_t input) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 11U) {
        retval =  abq_encode_left_padded_uint(enc,
                (1uL + (uint64_t) input), DECIMAL_RADIX, '0', 2U);
    }
    return retval;
}

static uint16_t dec_month_dec(abq_decoder_t *dec) {
    uint16_t retval = 1U;
    int64_t inv_val = 0;
    if (EXIT_SUCCESS == abq_decode_int(dec, &inv_val, DECIMAL_RADIX)) {
        if ((inv_val >= 1L) && (inv_val <= 12L)) {
            retval = (uint16_t) ((uint16_t) inv_val) - 1U;
        }
    }
    return retval;
}

static err_t enc_day(abq_encoder_t *enc, uint16_t input) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 31U) {
        retval = abq_encode_left_padded_uint(enc,
                (uint64_t)input, DECIMAL_RADIX, '0', 2U);
    }
    return retval;
}

static uint16_t dec_day(abq_decoder_t *dec) {
    uint16_t rvalue = 0U;
    int64_t out = 0;
    if (EXIT_SUCCESS == abq_decode_int(dec, &out, DECIMAL_RADIX)) {
        if ((out >= 1) && (out <= 31)) {
            rvalue = (uint16_t) out;
        }
    }
    return rvalue;
}

static err_t enc_hour(abq_encoder_t *enc, uint16_t input) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 23U) {
        retval = abq_encode_left_padded_uint(enc,
                (uint64_t)input, DECIMAL_RADIX, '0', 2U);
    }
    return retval;
}

static uint16_t dec_hour(abq_decoder_t *dec) {
    uint16_t rvalue = 0U;
    int64_t out = 0;
    if (EXIT_SUCCESS == abq_decode_int(dec, &out, DECIMAL_RADIX)) {
        if ((out >= 0) && (out <= 23)) {
            rvalue = (uint16_t) out;
        }
    }
    return rvalue;
}

static err_t enc_minute(abq_encoder_t *enc, uint16_t input) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 59U) {
        retval = abq_encode_left_padded_uint(enc,
                (uint64_t)input, DECIMAL_RADIX, '0', 2U);
    }
    return retval;
}

static uint16_t dec_minute(abq_decoder_t *dec) {
    uint16_t rvalue = 0U;
    int64_t out = 0;
    if (EXIT_SUCCESS == abq_decode_int(dec, &out, DECIMAL_RADIX)) {
        if ((out >= 0) && (out <= 59)) {
            rvalue = (uint16_t) out;
        }
    }
    return rvalue;
}

static err_t enc_second(abq_encoder_t *enc, uint16_t input) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 60U) {        /* The range is up to 60 to allow for occasional leap seconds */
        retval = abq_encode_left_padded_uint(enc,
                (uint64_t)input, DECIMAL_RADIX, '0', 2U);
    }
    return retval;
}

static uint16_t dec_second(abq_decoder_t *dec) {
    uint16_t rvalue = 0U;
    int64_t out = 0;
    if (EXIT_SUCCESS == abq_decode_int(dec, &out, DECIMAL_RADIX)) {
        if ((out >= 0) && (out <= 60)) {        /* The range is up to 60 to allow for occasional leap seconds */
            rvalue = (uint16_t) out;
        }
    }
    return rvalue;
}

static err_t enc_year(abq_encoder_t *enc, uint16_t input) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 8100U) {        /* keep it to 4 digits - up to year 9999 */
        retval = abq_encode_left_padded_uint(enc,
                (1900uL +(uint64_t)input), DECIMAL_RADIX, '0', 4U);
    }
    return retval;
}

static uint16_t dec_year(abq_decoder_t *dec) {
    uint16_t rvalue = 1900U;
    int64_t out = 0;
    if (EXIT_SUCCESS == abq_decode_int(dec, &out, DECIMAL_RADIX)) {
        if ((out >= 1900) && (out <= 9999)) {
            out -= 1900;
            rvalue = (uint16_t) out;
        }
    }
    return rvalue;
}

static err_t enc_weekday_char_short(abq_encoder_t *enc, uint16_t input, abq_locale_t locale) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 6U) {
        cstr_t weekday = time_enc_wkday_short_locale_map[locale].data[input];
        retval = abq_encode_ascii(enc, weekday, 3);
    }
    return retval;
}

static uint16_t dec_weekday_char_short(abq_decoder_t *dec, abq_locale_t locale) {
    uint16_t rvalue = 0U;
    uint16_t cnt = 0U;
    while (cnt <= 6U) {
        if (EXIT_SUCCESS == abq_decode_skip_prefix(dec, time_enc_wkday_short_locale_map[locale].data[cnt], 3)) {
            rvalue = cnt;
            break;
        }
        cnt++;
    }
    return rvalue;
}

static err_t enc_weekday_char_long(abq_encoder_t *enc, uint16_t input, abq_locale_t locale) {
    err_t retval = EINVAL;
    // valid ?
    if (input <= 6U) {
        cstr_t weekday = time_enc_wkday_long_locale_map[locale].data[input];
        retval = abq_encode_ascii(enc, weekday, -1);
    }
    return retval;
}

static uint16_t dec_weekday_char_long(abq_decoder_t *dec, abq_locale_t locale) {
    uint16_t rvalue = 0U;
    uint16_t cnt = 0U;
    while (cnt <= 6U) {
        if (EXIT_SUCCESS == abq_decode_skip_prefix(dec,
                time_enc_wkday_long_locale_map[locale].data[cnt], -1)) {
            rvalue = cnt;
            break;
        }
        cnt++;
    }
    return rvalue;
}

static abq_locale_t abq_locale_safe(abq_locale_t locale) {
    abq_locale_t retval = abq_locale_default;
    switch(locale) {
        case abq_locale_en:
            retval = abq_locale_en;
            break;
        case abq_locale_jp:
            retval = abq_locale_jp;
            break;
        case abq_locale_de:
            retval = abq_locale_de;
            break;
        default:
            ABQ_WARN_MSG_X("Invalid locale:", locale);
            // keep abq_locale_default as retval
            break;
    }
    return retval;
}

err_t abq_encode_datetime(abq_encoder_t *encoder,
        cstr_t input_format, abq_locale_t locale, const abq_datetime_t* datetime) {
    err_t retval = EXIT_SUCCESS;
    if((NULL == encoder) || (NULL == input_format) || (NULL == datetime)) {
        retval = EFAULT;
    } else {
        int32_t pos = 0;
        abq_locale_t safe_locale = abq_locale_safe(locale);
        while (('\0' != input_format[pos]) && (EXIT_SUCCESS == retval)) {
            if ('%' != input_format[pos]) {
                retval = abq_encode_char(encoder, input_format[pos]);
            } else {
                pos++;  // next char
                switch(input_format[pos]) {
                    case 'b':
                        retval = enc_month_char_short(encoder,
                                datetime->tm_mon, safe_locale);
                        break;
                    case 'B':
                        retval = enc_month_char_long(encoder,
                                datetime->tm_mon, safe_locale);
                        break;
                    case 'd':
                        retval = enc_day(encoder, datetime->tm_mday);
                        break;
                    case 'H':
                        retval = enc_hour(encoder, datetime->tm_hour);
                        break;
                    case 'M':
                        retval = enc_minute(encoder, datetime->tm_min);
                        break;
                    case 'S':
                        retval = enc_second(encoder, datetime->tm_sec);
                        break;
                    case 'Y':
                        retval = enc_year(encoder, datetime->tm_year);
                        break;
                    case 'm':
                        retval = enc_month_dec(encoder, datetime->tm_mon);
                        break;
                    case 'a':
                        retval = enc_weekday_char_short(encoder,
                                datetime->tm_wday, safe_locale);
                        break;
                    case 'A':
                        retval = enc_weekday_char_long(encoder,
                                datetime->tm_wday, safe_locale);
                        break;
                    default:
                        // treat '%' as an escape character (I.E. "%%" => "%"
                        retval = abq_encode_char(encoder, input_format[pos]);
                        break;
                }
            }
            pos++;  // next char
        }
    }
    return retval;
}

static int32_t decimal_prefix_bytecount(abq_decoder_t *decoder) {
    int32_t retval= -1;
    size_t prev_pos = decoder->pos;
    err_t status = abq_decode_skip_matching(decoder, ascii_is_decimal);
    if (EXIT_SUCCESS != status) {
        abq_status_set(status, false);
    } else {
        retval = (int32_t)decoder->pos;
        retval -= (int32_t)prev_pos;
    }
    decoder->pos = prev_pos;
    return retval;
}

static err_t decode_counted_decimal(abq_decoder_t *decoder,
        size_t count, int64_t* dest) {
    err_t retval = EXIT_SUCCESS;
    if((decoder->max - decoder->pos) < count) {
        retval= ENODATA;
    } else {
        size_t orig_max = decoder->max;
        decoder->max = decoder->pos + count;
        retval = abq_decode_int(decoder, dest, DECIMAL_RADIX);
        if (EXIT_SUCCESS != retval) {
            // Return error code as is
        } else if(decoder->max != decoder->pos) {
            retval = EILSEQ;
        } else {
            // OK
        }
        decoder->max = orig_max;
    }
    return retval;
}

static err_t abq_decode_mmdd(abq_decoder_t *decoder,
        abq_datetime_t *tm_var, int32_t *decode_count) {
    // read out the month
    int64_t int_value = -1L;
    err_t retval = decode_counted_decimal(decoder, 2U, &int_value);
    if (EXIT_SUCCESS != retval ) {
        // Return error code as is
    } else if((1L > int_value) || (int_value > 12L)) {
        retval = ERANGE;
    } else {
        (*decode_count)++;
        tm_var->tm_mon = (uint16_t) ((uint16_t)int_value - 1U);
        size_t prev_pos = decoder->pos;
        // skip over hyphen delimiter if any
        (void) abq_decode_skip_prefix(decoder, "-", 1); // parasoft-suppress CERT_C-MSC41-a-1 "c0542. This string does not contain sensitive information."
        // read out the day-of-month
        retval = decode_counted_decimal(decoder, 2U, &int_value);
        if (EXIT_SUCCESS != retval ) {
            // DD field is considered optional by the specification
            decoder->pos = prev_pos;
            retval = EXIT_SUCCESS;
        } else if((0L > int_value) || (int_value > 31L)) {
            retval = ERANGE;
        } else {
            (*decode_count)++;
            tm_var->tm_mday = (uint16_t) int_value;
        }
    }
    return retval;
}

static err_t abq_decode_iso8601_date(abq_decoder_t *decoder,
        abq_datetime_t *tm_var, int32_t *decode_count) {
    err_t retval = EXIT_SUCCESS;
    size_t prev_pos = decoder->pos;
    int32_t cp = abq_decode_cp(decoder);
    if (0 >= cp) {
        retval = abq_status_take(EILSEQ);
    } else if ((int32_t) '-' == cp) {
        // Either a negative date ('-N') or a date without year ('--MM-DD')
        ABQ_WARN_MSG("Ancient dates are not supported");
        retval = ENOSYS;
    } else {
        if ((int32_t) '+' != cp) {
            ABQ_DECODER_REWIND(decoder, cp);
        }
        int64_t int_value = -1;
        int32_t bytecount = decimal_prefix_bytecount(decoder);
        if (0 >= bytecount) {
            retval = abq_status_take(EILSEQ); // bad format
        } else if (8 <= bytecount) {
            // YYYYMMDD
            retval = decode_counted_decimal(decoder,
                    ((size_t)bytecount) - 4U, &int_value);
            if (EXIT_SUCCESS != retval ) {
                // Return error code as is
            } else {
                (*decode_count)++;
                tm_var->tm_year = (uint16_t) ((uint16_t)int_value - 1900U);
                // read out mmdd
                retval = abq_decode_mmdd(decoder, tm_var, decode_count);

            }
        } else if (7 == bytecount) {
            // YYYYDDD
            retval = decode_counted_decimal(decoder, 4U, &int_value);
            if (EXIT_SUCCESS != retval ) {
                // Return error code as is
            } else {
                (*decode_count)++;
                tm_var->tm_year = (uint16_t) ((uint16_t)int_value - 1900U);
                // read out the ordinal date
                retval = decode_counted_decimal(decoder, 3U, &int_value);
                if (EXIT_SUCCESS != retval ) {
                    // Return error code as is
                } else if((1L > int_value) || (int_value > 256L)) {
                    retval = ERANGE;
                } else {
                    (*decode_count)++;
                    tm_var->tm_yday = (uint16_t) ((uint16_t)int_value - 1U);
                }
            }
        } else if(6 == bytecount) {
            // YYMMDD (truncated representation)
            retval = decode_counted_decimal(decoder, 2U, &int_value);
            if (EXIT_SUCCESS != retval ) {
                // Return error code as is
            } else {
                (*decode_count)++;
                // Assumes '99 refers to year 2099 (as opposed to 1999)
                tm_var->tm_year = (uint16_t) ((uint16_t)int_value + 100U);
                // read out mmdd
                retval = abq_decode_mmdd(decoder, tm_var, decode_count);
            }
        } else {
            // YYYY
            retval = abq_decode_int(decoder, &int_value, DECIMAL_RADIX);
            if (EXIT_SUCCESS != retval) {
                // Return error as is
            } else {
                (*decode_count)++;
                tm_var->tm_year = (uint16_t) ((uint16_t)int_value - 1900U);
                cp = abq_decode_cp(decoder);
                if ((int32_t) '-' == cp) {
                    cp = abq_decode_cp(decoder);
                }
                if ((int32_t) 'W' == cp) {
                    // Weak of year format
                    // YYYYWww or YYYY-Www
                    // YYYYWww-D or YYYY-Www-D
                    ABQ_WARN_MSG("Weak of year format is not supported");
                    retval = ENOSYS;
                } else {
                    ABQ_DECODER_REWIND(decoder, cp);
                    bytecount = decimal_prefix_bytecount(decoder);
                    if ((0 >= bytecount) || (bytecount > 4)) {
                            retval = abq_status_take(EILSEQ); // bad format
                    } else if (bytecount == 4) {
                        // MMDD
                        // read out mmdd
                        retval = abq_decode_mmdd(decoder, tm_var, decode_count);
                    } else if (bytecount == 3) {
                        // DDD
                        // read out the ordinal date
                        retval = decode_counted_decimal(decoder, 3U, &int_value);
                        if (EXIT_SUCCESS != retval ) {
                            // Return error code as is
                        } else if((1L > int_value) || (int_value > 256L)) {
                            retval = ERANGE;
                        } else {
                            (*decode_count)++;
                            tm_var->tm_yday = (uint16_t) ((uint16_t)int_value - 1U);
                        }
                    } else {
                        // read out mm-dd
                        retval = abq_decode_mmdd(decoder, tm_var, decode_count);
                    }
                }
            }
        }
    }
    if (EXIT_SUCCESS != retval) {
        decoder->pos = prev_pos;
    }
    return retval;
}

static err_t abq_decode_iso8601_tzone(abq_decoder_t *decoder,
        abq_datetime_t *tm_var, int32_t *decode_count){
    err_t retval = EXIT_SUCCESS;
    size_t prev_pos = decoder->pos;
    int32_t cp = abq_decode_cp(decoder);
    if ((int32_t) 'z' == ascii_tolower(cp)) {
        // UTC time-zone, offset of zero
        (*decode_count)++;
        tm_var->tm_tz_offset = 0;
    } else {
        int64_t int_value = -1;
        // timezone info in format of [+/-]hh(:)(mm) ?
        decoder->pos = prev_pos; // Reset position
        // read out [+/-]HH (up to ?2 digits for hour)
        if (((int32_t) '+' == cp) || ((int32_t) '-' == cp)) {
            retval = decode_counted_decimal(decoder, 3U, &int_value);
        } else {
            retval = decode_counted_decimal(decoder, 2U, &int_value);
        }
        if (EXIT_SUCCESS != retval) {
            // Failed to parse optional time-zone
            decoder->pos = prev_pos;
            retval = EXIT_SUCCESS;
        } else if((-24L > int_value) || (int_value > 24L)) {
            retval = ERANGE;
        } else {
            // parsed hours segment of time-zone offset
            (*decode_count)++;  // increment only once for HOUR AND MINUTE
            tm_var->tm_tz_offset =  (int32_t)((int64_t) ONE_HOUR_IN_SEC * int_value);
            prev_pos = decoder->pos; // Update return to position
            cp = abq_decode_cp(decoder);
            if ((int32_t)  ':' == cp) {
                // skip over ':' delimiter if present
                cp = abq_decode_cp(decoder);
            }
            if(!ascii_is_decimal(cp)) {
                // MM value was omitted
                // Failed to parse optional timezone
                decoder->pos = prev_pos;
            } else {
                ABQ_DECODER_REWIND(decoder, cp);
                // parsing minutes segment of time-zone offset
                retval = abq_decode_int(decoder, &int_value, DECIMAL_RADIX);
                if (EXIT_SUCCESS != retval) {
                    // Failed to parse optional timezone
                    decoder->pos = prev_pos;
                    retval = EXIT_SUCCESS;
                } else if((0L > int_value) || (int_value > 60L)) {
                    retval = ERANGE;
                } else {
                    if (0 > tm_var->tm_tz_offset) {
                        tm_var->tm_tz_offset -= (int32_t)((int64_t) ONE_MINUTE_IN_SEC * int_value);
                    } else {
                        tm_var->tm_tz_offset += (int32_t)((int64_t) ONE_MINUTE_IN_SEC * int_value);
                    }
                }
            }
        }
    }
    return retval;
}

static err_t abq_decode_iso8601_time(abq_decoder_t *decoder,
        abq_datetime_t *tm_var, int32_t *decode_count) {
    err_t retval = EXIT_SUCCESS;
    size_t prev_pos = decoder->pos;
    int32_t cp = abq_decode_cp(decoder);
    if (0 >= cp) {
        retval = abq_status_take(EILSEQ);
    } else {
        if ((int32_t) 'T' == cp) {
            // skip over leading 'T' marker
            cp = abq_decode_cp(decoder);
        }
        if(!ascii_is_decimal(cp)) {
            retval = EILSEQ; // bad format
        } else if(decoder->max < (decoder->pos + 2U)){
            retval = ENODATA;
        } else {
            int64_t int_value = -1;
            ABQ_DECODER_REWIND(decoder, cp);
            // read out HH (up to 2 digits for hour)
            retval = decode_counted_decimal(decoder, 2U, &int_value);
            if (EXIT_SUCCESS != retval ) {
                // Return error code as is
            } else if((0L > int_value) || (int_value > 24L)) {
                retval = abq_status_take(ERANGE);
            } else {
                (*decode_count)++;
                tm_var->tm_hour = (uint16_t) int_value;
                cp = abq_decode_cp(decoder);
                if ((int32_t)  ':' == cp) {
                    // skip over ':' delimiter if pressent
                    cp = abq_decode_cp(decoder);
                }
                if(!ascii_is_decimal(cp)) {
                    retval = EILSEQ; // bad format
                } else if(decoder->max < (decoder->pos + 2U)){
                    retval = ENODATA;
                } else {
                    ABQ_DECODER_REWIND(decoder, cp);
                    // read out MM (up to 2 digits for minutes)
                    retval = decode_counted_decimal(decoder, 2U, &int_value);
                    if (EXIT_SUCCESS != retval ) {
                        // Return error code as is
                    } else if((0L > int_value) || (int_value > 60L)) {
                        retval = abq_status_take(ERANGE);
                    } else {
                        (*decode_count)++;
                        tm_var->tm_min = (uint16_t) int_value;
                        prev_pos = decoder->pos; // Update return to position
                        cp = abq_decode_cp(decoder);
                        if ((int32_t)  ':' == cp) {
                            // skip over ':' delimiter if present
                            cp = abq_decode_cp(decoder);
                        }
                        if(!ascii_is_decimal(cp)) {
                            // Seconds are optional, no error
                            tm_var->tm_sec = 0U;
                            decoder->pos = prev_pos;
                        } else {
                            number_t floating_val = 0.0;
                            ABQ_DECODER_REWIND(decoder, cp);
                            retval = abq_decode_number(decoder, &floating_val);
                            if (EXIT_SUCCESS != retval) {
                                // Return error code as is
                            } else if((0.0 > floating_val) || (floating_val > 60.0)) {
                                retval = ERANGE;
                            } else {
                                (*decode_count)++;
                                tm_var->tm_sec = (uint16_t) floating_val;
                            }
                        }
                    }
                }
            }
        }
    }
    if (EXIT_SUCCESS != retval) {
        decoder->pos = prev_pos;
    } else {
        retval = abq_decode_iso8601_tzone(decoder, tm_var, decode_count);
    }
    return retval;
}

static int32_t time_decode_iso8601(cstr_t datetime_input, abq_datetime_t *tm_var) {
    int32_t dec_cnt = 0;
    if ((NULL != datetime_input) && (NULL != tm_var)) {
        ABQ_DECODER(decoder, &ascii_codec, datetime_input, -1);
        err_t status = abq_decode_iso8601_date(&decoder, tm_var, &dec_cnt);
        if (EXIT_SUCCESS == status) {
            (void) abq_decode_iso8601_time(&decoder, tm_var, &dec_cnt);
        } else {
            // One or the other should return a success code for valid iso8601
            EXPECT_IS_OK(abq_decode_iso8601_time(&decoder, tm_var, &dec_cnt));
        }
    }
    return dec_cnt;
}

int32_t time_decode_format(cstr_t buffer,
        cstr_t input_format, abq_locale_t locale, abq_datetime_t *datetime) {
    int32_t retval = 0;
    if ((NULL == buffer) || (NULL == input_format) || (NULL == datetime)) {
        abq_status_set(EFAULT, false);
        retval = -1;
    } else {
        abq_locale_t safe_locale = abq_locale_safe(locale);
        ABQ_DECODER(decoder, &ascii_codec, buffer, -1);
        ABQ_DECODER(format, &ascii_codec, input_format, -1);
        int32_t codepoint = abq_decode_cp(&format);
        while((0 < codepoint) && (0 <= retval)) {
            // is placeholder?
            if (codepoint == (int32_t)'%') {
                switch (abq_decode_char(&format)) {
                    case 'b':
                        datetime->tm_mon = dec_month_char_short(&decoder, safe_locale);
                        retval++;
                        break;
                    case 'B':
                        datetime->tm_mon = dec_month_char_long(&decoder, safe_locale);
                        retval++;
                        break;
                    case 'd':
                        datetime->tm_mday = dec_day(&decoder);
                        retval++;
                        break;
                    case 'H':
                        datetime->tm_hour = dec_hour(&decoder);
                        retval++;
                        break;
                    case 'M':
                        datetime->tm_min = dec_minute(&decoder);
                        retval++;
                        break;
                    case 'S':
                        datetime->tm_sec = dec_second(&decoder);
                        retval++;
                        break;
                    case 'Y':
                        datetime->tm_year = dec_year(&decoder);
                        retval++;
                        break;
                    case 'm':
                        datetime->tm_mon = dec_month_dec(&decoder);
                        retval++;
                        break;
                    case 'a':
                        datetime->tm_wday = dec_weekday_char_short(&decoder, safe_locale);
                        retval++;
                        break;
                    case 'A':
                        datetime->tm_wday = dec_weekday_char_long(&decoder, safe_locale);
                        retval++;
                        break;
                    default:
                        (void) abq_decode_char(&decoder);
                        break;
                }
            } else if (abq_decode_cp(&decoder) == codepoint) {
                // ws or some other expected char
            } else {
                // Character did not match pattern
                abq_status_set(EILSEQ, false);
                retval = -1;
            }
            codepoint = abq_decode_cp(&format);
        }
        if (0 > retval) {
            // keep error as is
        } else if(0 > codepoint) {
            // illegal format
            abq_status_set(EILSEQ, false);
            retval = -1;
        } else {
            ABQ_VITAL(0 == codepoint);
            // happy case
        }
    }
    return retval;
}

int32_t time_decode(cstr_t buffer,
        abq_tm_format_t tm_format,
        abq_locale_t locale,
        abq_datetime_t *datetime) {
    int32_t rvalue = EXIT_SUCCESS;
    switch (tm_format) {
        case abq_tm_rfc5322:
            rvalue = time_decode_format(buffer, ABQ_TM_FRMT_RFC_5322, locale, datetime); // parasoft-suppress CERT_C-MSC41-a-1 "c0543. This string does not contain sensitive information."
            break;
        case abq_tm_iso8601:
            rvalue = time_decode_iso8601(buffer, datetime);
            break;
        default:
            rvalue = -1;
            break;
    }
    return rvalue;
}
