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
 *  Copyright (c) 2020 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file aqTimeImpl.c
 * @date Jan 27, 2020
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 */

#include <spil_os/aqTime.h>
#include <spil_os/aqSpilImpl.h>
#include <ontrac/ontrac/status_codes.h>
#include <ontrac/ontrac/abq_str.h>
#include <ontrac/text/time_enc.h>

#ifdef __APPLE__
# include <sys/resource.h>
# include <mach/mach.h>
# include <mach/clock.h>
// these three needed for for 'threadcpu'
#include <mach/mach_init.h>
#include <mach/thread_act.h>
#include <mach/mach_port.h>
#else /* !OSX */
# include <time.h>
# include <ctype.h>
# if defined(_WIN32)                                                            \
     && !defined(__SYMBIAN32__) /* WINDOWS / UNIX include block */
#  include <windows.h>
#  include <fcntl.h>
# else /* !Windows */
#include <sys/resource.h>
# endif /* !Windows */
#endif /* !OSX */

#if defined(_WIN32)                                                            \
     && !defined(__SYMBIAN32__) /* WINDOWS / UNIX include block */
// Causes:  warning: implicit declaration of function '_mkgmtime'; did you mean 'gmtime'?
# define timegm _mkgmtime
static HANDLE current_running_process;
# define TICKS_PER_SECOND (10000000.0)
#elif defined(__APPLE__)
#else /* !OS && !Windows */
static const clockid_t clockid_monotonic = CLOCK_MONOTONIC;
static const clockid_t clockid_realtime = CLOCK_REALTIME;
#endif /* !OS && !Windows */

static void normalizeContinuousTime( aqSpilClkContinuousTime_t *ts );

// comment maintained in header
err_t aqSpilClkGetContinuousTime(aqSpilClkContinuousTime_t *tStamp) {
    err_t retval = EUNSPECIFIED;
    if( NULL != tStamp ) {
#ifdef __APPLE__
        clock_serv_t cclock = {0};
        mach_timespec_t mts = {0};
        if (0 == host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock)) {
            if (0 == clock_get_time(cclock, &mts)) {
                // osx tv_sec is a unsigned ... need to make sure that it isn't too big to cast
                /* NOT NEEDED 'if (mts.tv_sec <= LONG_MAX)' because always true */
                {
                    tStamp->tv_sec = (aqSpilTvSec_t) mts.tv_sec;
                    tStamp->tv_nsec = (aqSpilTvNsec_t) mts.tv_nsec;
                    retval = EXIT_SUCCESS;
                }
            }
            mach_port_deallocate(mach_task_self(), cclock);
        }
#elif defined(_WIN32)  && !defined(__SYMBIAN32__)
        ULARGE_INTEGER li = {0};
		number_t posix_time = 0.0;
        static number_t perfcnt_per_sec = 0.0;
		if (perfcnt_per_sec == 0.0) {
			QueryPerformanceFrequency((LARGE_INTEGER *)&li);
			perfcnt_per_sec = 1.0 / (number_t) li.QuadPart;
		}
		if (perfcnt_per_sec != 0.0) {
			QueryPerformanceCounter((LARGE_INTEGER *)&li);
			posix_time = perfcnt_per_sec * (number_t) li.QuadPart;
			tStamp->tv_sec = (time_t) posix_time;
			posix_time -= (number_t) tStamp->tv_sec;
			tStamp->tv_nsec = (int64_t)(posix_time * 1.0E9);
            retval = EXIT_SUCCESS;
		}
#else /* !Windows && !OSX */
        struct timespec tts = {0};
        if ( 0 == clock_gettime( clockid_monotonic, &tts ) ) {
            tStamp->tv_sec = (aqSpilTvSec_t) tts.tv_sec;
            tStamp->tv_nsec = (aqSpilTvNsec_t) tts.tv_nsec;
            retval = EXIT_SUCCESS;
       } else {
           tStamp->tv_sec = (aqSpilTvSec_t) 0L;
           tStamp->tv_nsec = (aqSpilTvNsec_t) 0L;

       }
#endif
        normalizeContinuousTime(tStamp);
    }
    return retval;
}

err_t aqSpilClkGetContiniousTime(aqSpilClkContiniousTime_t *tStamp) {
    return aqSpilClkGetContinuousTime(tStamp);
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
    if( NULL != rtc_time ) {
#ifdef __APPLE__
        clock_serv_t cclock = {0};
        mach_timespec_t mts = {0};
        if (0 == host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock)) {
            if (0 == clock_get_time(cclock, &mts)) {
                // osx tv_sec is a unsigned ... need to make sure that it isn't too big to cast
                /* NOT NEEDED 'if (mts.tv_sec <= LONG_MAX)' because always true */
                {
                    rtc_time->rtc.tv_sec = (aqSpilTvSec_t) mts.tv_sec;
                    rtc_time->rtc.tv_nsec = (aqSpilTvNsec_t) mts.tv_nsec;
                    retval = EXIT_SUCCESS;
                }
            }
            mach_port_deallocate(mach_task_self(), cclock);
        }
#elif defined(_WIN32) && !defined(__SYMBIAN32__)
        FILETIME ft = {0};
        ULARGE_INTEGER li = {0};
		number_t posix_time =0.0;
		GetSystemTimeAsFileTime(&ft);
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;
		li.QuadPart -= 116444736000000000; /* 1.1.1970 in filedate */
		posix_time = ((number_t) li.QuadPart) / TICKS_PER_SECOND;
		rtc_time->rtc.tv_sec = (time_t) posix_time;
        posix_time -= (number_t) rtc_time->rtc.tv_sec;
		rtc_time->rtc.tv_nsec = (int64_t)(posix_time * 1.0E9);
		retval = EXIT_SUCCESS;
#else /* !Windows && !OSX */
        struct timespec tts = {0};
        if ( 0 == clock_gettime( clockid_realtime, &tts ) ) {
            rtc_time->rtc.tv_sec = (aqSpilTvSec_t) tts.tv_sec;
            rtc_time->rtc.tv_nsec = (aqSpilTvNsec_t) tts.tv_nsec;
            retval = EXIT_SUCCESS;
       } else {
           rtc_time->rtc.tv_sec = (aqSpilTvSec_t) 0L;
           rtc_time->rtc.tv_nsec = (aqSpilTvNsec_t) 0L;
       }
#endif
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
    #if defined(_WIN32) && !defined(__SYMBIAN32__)
        stm_ptr = gmtime(&t);
    #else
        stm_ptr = gmtime_r(&t, &stm);
        // for some reason both gmtime & gmtime_r on the imx6
        //  are setting the errno to ENOENT ?
        if (ENOENT == errno) {
            errno = 0;
        }
    #endif
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

aqSpilClkStatus_t aqSpilDiffContiniousTime(const aqSpilClkContiniousTime_t *time2,
                                           const aqSpilClkContiniousTime_t *time1,
                                           int64_t *diffInUs) {
    aqSpilClkStatus_t retval = aqSpilClkStatusError;
    if( (NULL != time1) && (NULL != time2) && (NULL!= diffInUs) ) {
        retval = aqSpilDiffContinuousTime(time2, time1, diffInUs);
    }
    return retval;
}


number_t monotonic(void) {
    number_t retval = abq_nan;
#ifdef __APPLE__
    clock_serv_t cclock = {0};
    mach_timespec_t mts = {0};
    VITAL_IS_OK(host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock)));
    VITAL_IS_OK(lock_get_time(cclock, &mts));
    (void) mach_port_deallocate(mach_task_self(), cclock);
    // osx tv_sec is a unsigned ... need to make sure that it isn't too big to cast
    /* NOT NEEDED 'if (mts.tv_sec <= LONG_MAX)' because always true */
    retval = ((number_t) mts.tv_nsec) / NANOS_PER_SECOND;
    retval += (number_t)mts.tv_sec;

#elif defined(_WIN32)  && !defined(__SYMBIAN32__)
	ULARGE_INTEGER li = {0};
	static number_t perfcnt_per_sec = 0.0;
	if (perfcnt_per_sec == 0.0) {
		QueryPerformanceFrequency((LARGE_INTEGER *)&li);
		perfcnt_per_sec = 1.0 / (number_t) li.QuadPart;
	}
	if (perfcnt_per_sec != 0.0) {
		QueryPerformanceCounter((LARGE_INTEGER *)&li);
		retval = perfcnt_per_sec * (number_t) li.QuadPart;
	}
#else
    struct timespec tsnow = {0};
    VITAL_IS_OK(clock_gettime(clockid_monotonic, &tsnow));
    retval = ((number_t) tsnow.tv_nsec) / NANOS_PER_SECOND;
    retval += (number_t) tsnow.tv_sec;
#endif /* ifdef __APPLE__ */
    return retval;
}


number_t realtime(void) {
    number_t retval = abq_nan;
#ifdef __APPLE__
    clock_serv_t cclock = {0};
    mach_timespec_t mts = {0};
    VITAL_IS_OK(host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock)));
    VITAL_IS_OK(lock_get_time(cclock, &mts));
    (void) mach_port_deallocate(mach_task_self(), cclock);
    // osx tv_sec is a unsigned ... need to make sure that it isn't too big to cast
    /* NOT NEEDED 'if (mts.tv_sec <= LONG_MAX)' because always true */
    retval = ((number_t) mts.tv_nsec) / NANOS_PER_SECOND;
    retval += (number_t) mts.tv_sec;
#elif defined(_WIN32) && !defined(__SYMBIAN32__)
	FILETIME ft = {0};
	ULARGE_INTEGER li = {0};
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	li.QuadPart -= 116444736000000000; /* 1.1.1970 in filedate */
	retval = ((number_t) li.QuadPart) / TICKS_PER_SECOND;
#else
    struct timespec tsnow = {0};
    VITAL_IS_OK(clock_gettime(clockid_realtime, &tsnow));
    retval = ((number_t) tsnow.tv_nsec) / NANOS_PER_SECOND;
    retval += (number_t) tsnow.tv_sec;
#endif /* ifdef __APPLE__ */
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
        time_t time_result = timegm(&tm_var);
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
#if defined(_WIN32) && !defined(__SYMBIAN32__)
    stm_ptr = gmtime(&t);
#else
    stm_ptr = gmtime_r(&t, &stm);
#endif
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
        time_result = timegm(&tm_var);
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

#if defined(_WIN32) && !defined(__SYMBIAN32__)
    stm_ptr = gmtime(&t);
#else
    stm_ptr = gmtime_r(&t, &stm);
#endif
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
#if defined(_WIN32) && !defined(__SYMBIAN32__)
    stm_ptr = gmtime(&t);
#else
    stm_ptr = gmtime_r(&t, &stm);
#endif
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


number_t cputime(void) {
    number_t rvalue = abq_nan;
#ifdef __APPLE__
    clock_t clock_time = clock();
    rvalue = ((number_t)clock_time) / (number_t) CLOCKS_PER_SEC;
    struct rusage usage = {0};
    err_t status = getrusage(RUSAGE_SELF, &usage);
    if (EXIT_SUCCESS != status) {
        (void) abq_status_set(status, false);
    } else {
        // include user time
        rvalue = (number_t)usage.ru_utime.tv_sec + (((number_t)usage.ru_utime.tv_usec)*MICRONS_PER_SECOND);
        // include kernal time
        rvalue += (number_t)usage.ru_stime.tv_sec + (((number_t)usage.ru_stime.tv_usec)*MICRONS_PER_SECOND);
    }
#elif defined(_WIN32) && !defined(__SYMBIAN32__)
    FILETIME create_time = {0};
    FILETIME exit_time = {0};
    FILETIME kernel_time = {0};
    FILETIME user_time = {0};
    if(NULL == current_running_process)
        current_running_process = GetCurrentProcess();
    if ( GetProcessTimes(current_running_process,
        &create_time, &exit_time, &kernel_time, &user_time ) != -1 ) {
        // one tick is 100 nano-seconds, 1/10 a micron, or 1/10000 or a millisecond
        uint64_t user_ticks = (((uint64_t)user_time.dwHighDateTime) << 32) + user_time.dwLowDateTime;
        uint64_t system_ticks = (((uint64_t)kernel_time.dwHighDateTime) << 32) + kernel_time.dwLowDateTime;
        rvalue = ((number_t)(user_ticks+system_ticks)) / ((number_t)TICKS_PER_SECOND);
    }
#else
    struct timespec usage = {0};
    err_t status = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &usage);
    if (EXIT_SUCCESS != status) {
        (void) abq_status_set(status, false);
    } else {
        rvalue = ((number_t) usage.tv_nsec) / NANOS_PER_SECOND;
        rvalue += (number_t) usage.tv_sec;
    }
#endif
    return rvalue;
}


number_t usertime(void) {
    number_t rvalue = abq_nan;
#if defined(_WIN32) && !defined(__SYMBIAN32__)
    HANDLE myThread = GetCurrentThread( );
    FILETIME ftCreate, ftExit, ftKernel, ftUser;
    GetThreadTimes(myThread, &ftCreate, &ftExit, &ftKernel, &ftUser);
    // one tick is 100 nano-seconds, 1/10 a micron, or 1/10000 or a millisecond
    uint64_t user_ticks = (((uint64_t)ftUser.dwHighDateTime) << 32) + ftUser.dwLowDateTime;
    rvalue = ((number_t)user_ticks) / ((number_t)TICKS_PER_SECOND);
#else
    struct rusage usage = {0};
    err_t status = getrusage((int32_t)RUSAGE_SELF, &usage); /* parasoft-suppress MISRA2012-RULE-10_3_b-2 "getrusage is platform API, should take enum __rusage_who as first arg" */
    if(EXIT_SUCCESS != status) {
        (void) abq_status_set(status, false);
    } else {
        // include user time
        rvalue = (number_t) usage.ru_utime.tv_sec + (((number_t) usage.ru_utime.tv_usec) / MICRONS_PER_SECOND);
    }
#endif
    return rvalue;
}

number_t threadcpu(void) {
    number_t rvalue = abq_nan;
#ifdef __APPLE__
    mach_port_t thread = mach_thread_self();
    mach_msg_type_number_t msg_type_no = THREAD_BASIC_INFO_COUNT;
    thread_basic_info_data_t info = {0};
    kern_return_t kr = thread_info(thread,
            THREAD_BASIC_INFO, (thread_info_t) &info, &count);
    ABQ_VITAL(KERN_SUCCESS == kr);
    //ABQ_VITAL((info.flags & TH_FLAGS_IDLE) == 0)
    // include user time
    rvalue = (number_t)info.user_time.seconds
            + (((number_t)info.user_time.microseconds)*MICRONS_PER_SECOND);
    // include kernal time
    rvalue += (number_t)info.system_time.seconds
            + (((number_t)info.system_time.microseconds)*MICRONS_PER_SECOND);
    (void) mach_port_deallocate(mach_task_self(), thread);
#elif defined(_WIN32) && !defined(__SYMBIAN32__)
    HANDLE myThread = GetCurrentThread( );
    FILETIME ftCreate, ftExit, ftKernel, ftUser;
    GetThreadTimes(myThread, &ftCreate, &ftExit, &ftKernel, &ftUser);
    // one tick is 100 nano-seconds, 1/10 a micron, or 1/10000 or a millisecond
    uint64_t user_ticks = (((uint64_t)ftUser.dwHighDateTime) << 32) + ftUser.dwLowDateTime;
    uint64_t system_ticks = (((uint64_t)ftKernel.dwHighDateTime) << 32) + ftKernel.dwLowDateTime;
    rvalue = ((number_t)(user_ticks+system_ticks)) / ((number_t)TICKS_PER_SECOND);
#else
    struct timespec usage = {0};
    err_t status = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &usage);
    if (EXIT_SUCCESS != status) {
        (void) abq_status_set(status, false);
    } else {
        rvalue = ((number_t) usage.tv_nsec) / NANOS_PER_SECOND;
        rvalue += (number_t) usage.tv_sec;
    }
#endif
    return rvalue;
}

size_t abq_logtime(byte_t *dest, uint8_t dest_len) {
    ABQ_ENCODER(encoder, &utf8_codec, dest, dest_len);
#ifdef USE_SPIL_CLK_FOR_LOG
    // Real-Time timestamps
# ifdef __APPLE__
    clock_serv_t cclock = {0};
    mach_timespec_t mts = {0};
    VITAL_IS_OK(host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock)));
    VITAL_IS_OK(lock_get_time(cclock, &mts));
    (void) mach_port_deallocate(mach_task_self(), cclock);
    // osx tv_sec is a unsigned ... need to make sure that it isn't too big to cast
    /* NOT NEEDED 'if (mts.tv_sec <= LONG_MAX)' because always true */
    uint64_t msec = ((uint64_t) mts.tv_nsec) / MS_PER_NANO_SEC;
    time_t t = (time_t) mts.tv_sec;
#elif defined(_WIN32) && !defined(__SYMBIAN32__)
	number_t systime = realtime();
	time_t t = (time_t) systime;
	systime -= (number_t) t;
	systime *= 1000.0;
    uint64_t msec = (uint64_t) systime;
# else
    struct timespec tsnow = {0};
    VITAL_IS_OK(clock_gettime(clockid_realtime, &tsnow));
    uint64_t msec = ((uint64_t) tsnow.tv_nsec) / MS_PER_NANO_SEC;
    time_t t = (time_t) tsnow.tv_sec;
# endif /* ifdef __APPLE__ */
    struct tm stm = {0}, *stm_ptr = &stm;
#if defined(_WIN32) && !defined(__SYMBIAN32__)
    stm_ptr = gmtime(&t);
#else
    stm_ptr = gmtime_r(&t, &stm);
#endif
    VITAL_NOT_NULL(stm_ptr);
    abq_datetime_t dt = {
            .tm_sec = (uint16_t)stm_ptr->tm_sec,
            .tm_min = (uint16_t)stm_ptr->tm_min,
            .tm_hour = (uint16_t)stm_ptr->tm_hour,
            .tm_mday = (uint16_t)stm_ptr->tm_mday,
            .tm_mon = (uint16_t)stm_ptr->tm_mon,
            .tm_year = (uint16_t)stm_ptr->tm_year,
            .tm_wday = (uint16_t)stm_ptr->tm_wday
    };
    (void) abq_encode_logtime(&encoder, &dt, msec);
#else /* !USE_SPIL_CLK_FOR_LOG */
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
#endif /* !USE_SPIL_CLK_FOR_LOG */
    return encoder.pos;
}

