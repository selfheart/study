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
 *  Copyright (c) 2021 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * aqTimeImpl.c for FreeRTOS
 */

#include <spil_os/aqTime.h>
#include <spil_os/aqSpilImpl.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/ontrac/abq_str.h>
#include <ontrac/text/time_enc.h>

/* time.h does not come with FreeRTOS, but come with GCC */
/**
 * NOTE: Not all environment has timegm(), so this implementation uses mktime()
 * It works fine on embedded devices such as Atmel board.
 * However, it will be affected by the timezone settings if running in the FreeRTOS POSIX port.
 *
 * So When running in the FreeRTOS POSIX simulator, the executable should be run as:
 *     TZ=UTC <executable_command>
 */
#include <time.h>

#include <abq_bsp_entry.h>

// comment maintained in header
err_t aqSpilClkGetContinuousTime(aqSpilClkContinuousTime_t *tStamp) {
    err_t retval = EUNSPECIFIED;
    if( NULL != tStamp ) {
        TickType_t tick = xTaskGetTickCount();

        uint64_t ms = (uint64_t)(tick * portTICK_PERIOD_MS);
        tStamp->tv_sec = (aqSpilTvSec_t)(ms / 1000);
        tStamp->tv_nsec = (aqSpilTvNsec_t)((ms % 1000) * 1000);
        retval = EXIT_SUCCESS;
    }
    return retval;
}

/** @private
 *  Normalize the contents of a continuous time object.  This is needed
 *  when adding or subtracting nano-seconds from the nanosecond part
 *  of the timestamp.  The normalization routine insures that the
 *  nano-seconds stay in the proper range and will adjust secs/nsecs
 *  as needed.
 *
 * @param ts The continuous time object to be normalized.
 */
static void normalizeContinuousTime( aqSpilClkContinuousTime_t *ts ) {
    if( NULL != ts ) {
        while (ts->tv_nsec >= NS_PER_SEC) {
            ts->tv_nsec -= NS_PER_SEC;
            ts->tv_sec++;
        }
    }
}

// comment maintained in aqTime.h
err_t aqSpilClkContinuousTimeToString(aqSpilClkContinuousTime_t *tStamp, char timeString[], size_t sz ) {

    /* MISRA - Fix this so it doesn't use snprintf */
    err_t status = EUNSPECIFIED;

    if( ( NULL != tStamp ) && ( NULL != timeString ) && ( sz > 0u ) ) {

        // normalize the timestamp
        normalizeContinuousTime(tStamp);
        int32_t msec = (int32_t) (tStamp->tv_nsec + 500000) / 1000000;
        /* Build the timestamp string */
        ABQ_ENCODER(encoder, &utf8_codec, timeString, sz);
        status = abq_encode_int(&encoder, tStamp->tv_sec, DECIMAL_RADIX);
        if (EXIT_SUCCESS == status) {
            status = abq_encode_char(&encoder, '.');
            if (EXIT_SUCCESS == status) {
                status = abq_encode_left_padded_uint(&encoder,
                        (uint64_t) msec, DECIMAL_RADIX, '0', 3U);
            }
        }
    }

    return status;
}

// comment maintained in aqTime.h
err_t aqSpilClkGetRtc(aqSpilClkRtc_t *rtc_time)
{
    err_t retval = EUNSPECIFIED;
    struct tm rtc = {0};
    if (0 == abq_bsp_get_rtc(&rtc)) {
        time_t time_result = mktime(&rtc);
        rtc_time->rtc.tv_sec = time_result;
        retval = EXIT_SUCCESS;
    }
    return retval;
}

static err_t abq_encode_logtime(abq_encoder_t *encoder,
        const abq_datetime_t *dt,uint64_t msec) {
    err_t status = abq_encode_datetime(encoder,
            "%Y-%m-%d %H:%M:%S", abq_locale_default, dt);
    if (EXIT_SUCCESS != status) {
        // Return status code as is
    } else if (msec >= 1000UL) {
        status = EINVAL; // Unsupported millisecond value
    } else {
        status = abq_encode_char(encoder, '.');
        if (EXIT_SUCCESS == status) {
            status = abq_encode_left_padded_uint(encoder,
                    msec, DECIMAL_RADIX, '0', 3U);
            if (EXIT_SUCCESS == status) {
                status = abq_encode_ascii(encoder, " UTC", 4);
            }
        }
    }
    return status;
}

err_t aqSpilClkRtcToString(const aqSpilClkRtc_t *rtc_time, char timeString[], size_t sz ) {
    err_t status = EUNSPECIFIED;
    if( ( NULL != rtc_time ) && ( NULL != timeString ) && ( sz > 0u ) ) {
        time_t t=0;
        uint64_t msec = ((uint64_t)rtc_time->rtc.tv_nsec + 500000U) / 1000000U;
        struct tm stm = {0}, *stm_ptr = NULL;
        t = rtc_time->rtc.tv_sec;
        stm_ptr = gmtime_r(&t, &stm);
        if (stm_ptr != NULL) {
            abq_datetime_t dt = {
                    .tm_sec = (uint16_t)stm_ptr->tm_sec,
                    .tm_min = (uint16_t)stm_ptr->tm_min,
                    .tm_hour = (uint16_t)stm_ptr->tm_hour,
                    .tm_mday = (uint16_t)stm_ptr->tm_mday,
                    .tm_mon = (uint16_t)stm_ptr->tm_mon,
                    .tm_year = (uint16_t)stm_ptr->tm_year,
                    .tm_wday = (uint16_t)stm_ptr->tm_wday
            };
            ABQ_ENCODER(encoder, &utf8_codec, timeString, sz);
            status = abq_encode_logtime(&encoder, &dt, msec);
        } else {
            status = abq_status_take(ENOSYS);
            timeString[0] = '\0';
        }
    }

    return status;
}

// comment maintained in aqTime.h
aqSpilClkStatus_t aqSpilDiffContinuousTime(const aqSpilClkContinuousTime_t *in_time2,
                                           const aqSpilClkContinuousTime_t *in_time1,
                                           int64_t *diffInUs)
{
    aqSpilClkStatus_t status = aqSpilClkStatusError;
    if( ( NULL != in_time2 ) && ( NULL != in_time1 ) && ( NULL != diffInUs ) ) {
        aqSpilClkContinuousTime_t cp_time_1  = {
                .tv_sec = in_time1->tv_sec,
                .tv_nsec = in_time1->tv_nsec
        };
        aqSpilClkContinuousTime_t cp_time_2 = {
                .tv_sec = in_time2->tv_sec,
                .tv_nsec = in_time2->tv_nsec
        };

        normalizeContinuousTime(&cp_time_1);
        normalizeContinuousTime(&cp_time_2);

        int64_t diffSecs = cp_time_2.tv_sec - cp_time_1.tv_sec;
        if ( diffSecs <= (INT64_MIN / USEC_PER_SEC) ) {
            // Check for gross underflow (too few seconds)
            *diffInUs = INT64_MIN;
            status = aqSpilClkStatusUnderflow;
        } else if ( diffSecs >= (INT64_MAX / USEC_PER_SEC) )  {
                // Check for gross overflow (too many seconds)
                *diffInUs = INT64_MAX;
                status = aqSpilClkStatusOverflow;
        } else {
            // Combine the difference in seconds to the difference in nanos
            //  Adding casts to (int64_t) since RCar-E2 was producing
            //  a negative diffInUs if diffSecs >= 2148
            //  because 2148000000 > (INT32_MAX) 2147483647
            *diffInUs = (int64_t)(((int64_t)diffSecs) * ((int64_t)USEC_PER_SEC))
                    +  (int64_t)((cp_time_2.tv_nsec - cp_time_1.tv_nsec) /  NS_PER_USEC );
            status = aqSpilClkStatusOK;
        }
    }
   return status;
}

number_t monotonic(void) {
    number_t retval = abq_nan;
    TickType_t tick = xTaskGetTickCount();

    retval = (number_t)tick / 1000.;
    return retval;
}


number_t realtime(void) {
    number_t retval = abq_nan;

    aqSpilClkRtc_t rtc_time;
    if (EXIT_SUCCESS == aqSpilClkGetRtc(&rtc_time)) {
        retval = (number_t)rtc_time.rtc.tv_sec;
    }
    return retval;
}

number_t parse_iso_8601(cstr_t datetime_input) {
    cstr_t datetime = datetime_input;
    number_t seconds = 0.0;
    abq_datetime_t dt = {0};
    int32_t count = time_decode(datetime, abq_tm_iso8601, abq_locale_default, &dt);
    if(0 < count) {
        struct tm tm_var = {
                .tm_sec = 0, // dt.tm_sec,  // would cause loss of precision
                                            // to avoid this loss, we apply seconds and timezone info
                                            // outside of the struct tm_var & timegm function call
                .tm_min = (int32_t)dt.tm_min,
                .tm_hour = (int32_t)dt.tm_hour,
                .tm_mday = (int32_t)dt.tm_mday,
                .tm_mon = (int32_t)dt.tm_mon,
                .tm_year = (int32_t)dt.tm_year,
                .tm_wday = (int32_t)dt.tm_wday,
        };
        time_t time_result = mktime(&tm_var);
        // if we successfully parsed the time
        // apply fractional seconds and timezone
        seconds = (number_t) dt.tm_sec + ((number_t) time_result);
        // subtract off the time-zone offset
        seconds -= (number_t) dt.tm_tz_offset;
    }
    return seconds;
}

cstr_t format_iso_8601(number_t posix_time) {
    char buf[32];
    ABQ_ENCODER(encoder,
            &utf8_codec, buf, sizeof(buf));
    cstr_t retval = NULL;
    time_t t = (time_t) posix_time;
    struct tm stm = {0}, *stm_ptr = NULL;
    stm_ptr = gmtime_r(&t, &stm);
    if (stm_ptr != NULL) {
        abq_datetime_t dt = {
                .tm_sec = (uint16_t)stm_ptr->tm_sec,
                .tm_min = (uint16_t)stm_ptr->tm_min,
                .tm_hour = (uint16_t)stm_ptr->tm_hour,
                .tm_mday = (uint16_t)stm_ptr->tm_mday,
                .tm_mon = (uint16_t)stm_ptr->tm_mon,
                .tm_year = (uint16_t)stm_ptr->tm_year,
                .tm_wday = (uint16_t)stm_ptr->tm_wday
        };
        err_t status = abq_encode_datetime(&encoder,
                ABQ_TM_FRMT_ISO_8601, abq_locale_default, &dt);
        if(EXIT_SUCCESS == status) {
            retval = str_create(buf,
                    (int32_t)encoder.pos, false);
        } else {
            abq_status_set(EUNSPECIFIED, false);
        }
    } else {
        abq_status_set(EUNSPECIFIED, false);
    }
    return retval;
}

number_t parse_rfc_5322(cstr_t datetime) {
    time_t time_result = 0;
    abq_datetime_t dt = {0};
    int32_t count = time_decode(datetime, abq_tm_rfc5322, abq_locale_default, &dt);
    if (0 < count) {
        struct tm tm_var = {
                .tm_sec = (int32_t)dt.tm_sec,
                .tm_min = (int32_t)dt.tm_min,
                .tm_hour = (int32_t)dt.tm_hour,
                .tm_mday = (int32_t)dt.tm_mday,
                .tm_mon = (int32_t)dt.tm_mon,
                .tm_year = (int32_t)dt.tm_year,
                .tm_wday = (int32_t)dt.tm_wday,
        };
        time_result = mktime(&tm_var);
    }
    return (number_t) time_result;
}

cstr_t format_rfc_5322(number_t posix_time) {
    char buf[32];
    ABQ_ENCODER(encoder,
            &utf8_codec, buf, sizeof(buf));
    cstr_t retval = NULL;
    time_t t = (time_t) posix_time;
    struct tm stm = {0}, *stm_ptr = NULL;

    stm_ptr = gmtime_r(&t, &stm);
    if (stm_ptr != NULL) {
        abq_datetime_t dt = {
                .tm_sec = (uint16_t)stm_ptr->tm_sec,
                .tm_min = (uint16_t)stm_ptr->tm_min,
                .tm_hour = (uint16_t)stm_ptr->tm_hour,
                .tm_mday = (uint16_t)stm_ptr->tm_mday,
                .tm_mon = (uint16_t)stm_ptr->tm_mon,
                .tm_year = (uint16_t)stm_ptr->tm_year,
                .tm_wday = (uint16_t)stm_ptr->tm_wday
        };

        err_t status = abq_encode_datetime(&encoder,
                ABQ_TM_FRMT_RFC_5322, abq_locale_default, &dt);
        if(EXIT_SUCCESS == status) {
            retval = str_create(buf,
                    (int32_t)encoder.pos, false);
        } else {
            abq_status_set(EUNSPECIFIED, false);
        }
    } else {
        abq_status_set(EUNSPECIFIED, false);
    }
    return retval;
}

err_t format_rfc_lcd(number_t posix_time, byte_t *buffer, uint32_t buf_size) {
    err_t rvalue = 0;
    ABQ_ENCODER(encoder,
            &utf8_codec, buffer, buf_size);
    time_t t = (time_t) posix_time;
    struct tm stm = {0}, *stm_ptr = NULL;
    stm_ptr = gmtime_r(&t, &stm);
    if (stm_ptr != NULL) {
        abq_datetime_t dt = {
                .tm_sec = (uint16_t)stm_ptr->tm_sec,
                .tm_min = (uint16_t)stm_ptr->tm_min,
                .tm_hour = (uint16_t)stm_ptr->tm_hour,
                .tm_mday = (uint16_t)stm_ptr->tm_mday,
                .tm_mon = (uint16_t)stm_ptr->tm_mon,
                .tm_year = (uint16_t)stm_ptr->tm_year,
                .tm_wday = (uint16_t)stm_ptr->tm_wday
        };
        rvalue = abq_encode_datetime(&encoder,
                "%b %d %H:%M:%S", abq_locale_default, &dt);
    } else {
       EXPECT_IS_OK(abq_encode_ascii(&encoder,
                "Jan 01, 00:00:00", -1));
        rvalue = EUNSPECIFIED;
    }
    return rvalue;
}

number_t usertime(void) {
    number_t rvalue = cputime(); // There is no real "kernel time" in FreeRTOS
    return rvalue;
}

number_t threadcpu(void) {
    number_t rvalue = abq_nan;
    TaskStatus_t xTaskDetails;
    TaskHandle_t thisThreadHandle = xTaskGetCurrentTaskHandle();
    vTaskGetInfo(thisThreadHandle, &xTaskDetails, pdFALSE, eRunning );
    rvalue = (number_t)xTaskDetails.ulRunTimeCounter;
    return rvalue;
}

size_t abq_logtime(byte_t *dest, uint8_t dest_len) {
    ABQ_ENCODER(encoder, &utf8_codec, dest, dest_len);
    // Relative-Time timestamps
    static number_t start_time = 0.0;
    number_t runtime = monotonic() - start_time;
    if (0.0 == start_time) {
        cstr_t started_at = format_iso_8601(realtime());
        start_time = runtime;
        (void) abq_encode_ascii(&encoder, "0 @ ", -1);
        (void) abq_encode_ascii(&encoder, started_at, -1);
        (void) obj_release_self(started_at);
        (void) run_garbage_collection();
    } else {
        (void) abq_encode_char(&encoder, ' ');
        (void) abq_encode_decimal(&encoder, runtime, 3, false);
    }
    return encoder.pos;
}

