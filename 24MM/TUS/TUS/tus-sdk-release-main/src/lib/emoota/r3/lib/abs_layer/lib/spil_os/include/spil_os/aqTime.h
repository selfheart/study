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
 * @file aqTime.h
 * @date Jan 27, 2020
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 */
#ifndef INCLUDE_SPIL_OS_AQTIME_H_
#define INCLUDE_SPIL_OS_AQTIME_H_

#include <platform/platformconfig.h>




#define ONE_MINUTE_IN_SEC (60.0)
#define ONE_HOUR_IN_SEC (3600.0)
#define ONE_DAY_IN_SEC (86400.0)
#define ONE_WEEK_IN_SEC (604800.0)
#define MILLIS_PER_SECOND (1000.0)
#ifndef USEC_PER_SEC
#define USEC_PER_SEC (1000000)
#endif
#define MICRONS_PER_SECOND (1000000.0)
#define NS_PER_USEC (1000)
#define MS_PER_NANO_SEC ( 1000000UL )
#define NS_PER_SEC (1000000000L)
#define NANOS_PER_SECOND (1000000000.0)


typedef struct aqSpilClkRtc aqSpilClkRtc_t;

/** Status codes reported by the real time clock and system clock APIs.
 */
typedef enum aqSpilClkStatus_t {
    aqSpilClkStatusOK       = 0,      ///< @brief explicitly zero.  Success.
    aqSpilClkStatusUnderflow,         ///< @brief An underflow occurred when comparing time objects.
    aqSpilClkStatusOverflow,          ///< @brief An overflow occurred when comparing time objects.
    aqSpilClkStatusError              ///< @brief An error occurred when processing the API request.
} aqSpilClkStatus_t;

/** @public
 * @memberof aqSpilClkRtc_t
 * @brief Get the current real-time clock from the underlying system.
 *
 * The real-time clock is sometimes referred to as a calendar clock.  It keeps time
 * in YYYY-MM-DD HH:MM:SS.sss format and us usually based upon a Gregorian calendar.
 *
 * @todo fix status returned so that it matches the aqSpilClkStatus_t (via the method
 * aqSpilClkGetTime() which reports errors as aqError rather than aqSpilClkStatusError.
 *
 * @param time A pointer to a real-time object which this function will populate.
 * @return <pre>aqSpilClkStatusOK</pre> The request was processed successfully and
 * the result has been placed @ time. <pre>aqSpilClkStatusError</pre> An error occurred
 * while processing the request (could be an invalid parameter or that an internal error
 * occurred).
 */
extern err_t aqSpilClkGetRtc(aqSpilClkRtc_t *time);

/** @public
 * @memberof aqSpilClkRtc_t
 * @brief Create a human readable C string which indicates the current time represented
 * by the given real-time object.
 *
 * The format will be in the form "YYYY-MM-DD HH:mm:SS.sss" where YYYY represents
 * the century and year, MM represents the month of the year, DD
 * represents the day of the month, HH represents the hour, mm represents minutes
 * SS represents seconds and .sss represents milli-seconds.
 *
 * @todo return should be of aqSpilClkStatus_t instead of err_t_t ... better yet,
 * a single enumeration should be created for all error codes system wide.  Perhaps
 * including specific error cases (e.g. invalid parameter vs internal system error).
 *
 * @param time A pointer to a real-time object which will be converted to the human readable string.
 * @param timeString A pointer to a buffer where the human readable string will be stored.
 * @param sz The number of bytes contained within the human readable string.  If sz is
 *           less than 24, the result will be truncated accordingly.
 *
 * @return <pre>aqOK</pre> The request was processed successfully and
 * the result has been placed @ timeString. <pre>aqError</pre> An error occurred
 * while processing the request (could be an invalid parameter or that an internal error
 * occurred).
 */
extern err_t aqSpilClkRtcToString(const aqSpilClkRtc_t *time, char timeString[], size_t sz );

/** @public
 * @class aqSpilClkContinuousTime_t
 * @brief Timestamp services based upon free running clock with no external reference.
 *
 * Likely the total elapsed time since boot.  However, no guarantee of what zero actually represents.
 */
typedef struct aqSpilClkContinuousTime aqSpilClkContinuousTime_t;

/**
 * @deprecated Switch to aqSpilClkContinuousTime_t
 */
typedef struct aqSpilClkContinuousTime aqSpilClkContiniousTime_t;

/** @public
 * @memberof aqSpilClkContinuousTime_t
 *  @brief Acquire the current value of the system timestamp.
 *
 *  The system timestamp will always be increasing unless a roll-over occurs (low likelihood).
 *
 * @param tStamp a pointer to a continuous time object which will be populated by this method.
 * @return aqOK on success, aqError on an error.
 */
extern err_t aqSpilClkGetContinuousTime(aqSpilClkContinuousTime_t *tStamp);

/**
 * @deprecated Use aqSpilClkGetContinuousTime()
 */
extern err_t aqSpilClkGetContiniousTime(aqSpilClkContiniousTime_t *tStamp);

/** @public
 * @memberof aqSpilClkContinuousTime_t
 * @brief Create a human readable C string which indicates the current time
 *  represented by the given continuous timestamp object.
 *
 *  The format will
 *  be @c XXX.YYYYYY where @c XXX represent the total number of seconds and @c YYYYYY
 *  represents the number of micro-seconds.
 *
 * @param tStamp The continuous timestamp object that will be used to create the human readable C string.
 * @param timeString A buffer to hold the contents of the time string.  The buffer
 *                   should be large enough to hold a string of 28 bytes in order
 *                   to allow for the maximum possible timestamp.  If the provided
 *                   buffer isn't large enough, the timestamp will be truncated to
 *                   fit within the allocated space.
 * @param sz The size of the 'timeString' buffer.
 * @return aqOK == success, aqError on failure (bad parameters).
 */
extern err_t aqSpilClkContinuousTimeToString(aqSpilClkContinuousTime_t *tStamp, char timeString[], size_t sz );

/**
 * @deprecated Use aqSpilClkContinuousTimeToString()
 */
extern err_t aqSpilClkContiniousTimeToString(aqSpilClkContinuousTime_t *tStamp, char timeString[], size_t sz );

/** @public
 * @memberof aqSpilClkContinuousTime_t
 * @brief Compute the result of t2 - t1 to micro-second resolution.
 *
 * If the computed difference is > INT64_MAX, then INT64_MAX will be returned.
 * if the computed difference is < INT64_MIN, then INT64_MIN will be returned.
 *
 * @param time2 The timestamp that time1 will be subtracting from.
 * @param time1 The timestamp that is subtracting from time2.
 * @param diffInUs The difference of t2 - t1 in micro-seconds
 * @return aqSpilClkStatusError == invalid parameter, aqSpilClkStatusUnderflow == the
 *         calculated difference was < INT64_MIN; the result is set to INT64_MIN,
 *         aqSpilClkStatusOverflow == the calculated difference was > INT64_MAX;
 *         THE RESULT IS SET TO INT64_MAX, aqOK indicates the result is the
 *         actual calculated difference.
 */
extern aqSpilClkStatus_t aqSpilDiffContinuousTime(const aqSpilClkContinuousTime_t *time2,
                                                  const aqSpilClkContinuousTime_t *time1,
                                                  int64_t *diffInUsec);
/**
 * @deprecated Use aqSpilDiffContinuousTime()
 */
extern aqSpilClkStatus_t aqSpilDiffContiniousTime(const aqSpilClkContiniousTime_t *time2,
                                                  const aqSpilClkContiniousTime_t *time1,
                                                  int64_t *diffInUsec);
/**
 * @brief time-source for trusted relative timestamps, but not absolute time
 *
 * @return a monotonic (always increasing) timestamp
 */
extern number_t monotonic(void);
/**
 * @brief time-source for untrusted absolute posix timestamps
 *
 * @return posix timestamp correlating to system's time
 */
extern number_t realtime(void);

/**
 * @brief Converts from a formatted ISO 8601 timestamp into an absolute posix timestamp
 * @param datetime: A date-time string formatted using the ISO 8601 standard
 * @return An absolute posix timestamp (or -1 on error)
 */
extern number_t parse_iso_8601(cstr_t datetime);
/**
 * @brief Converts a from a posix timestamp into a ISO 8601 formatted string with a self-reference
 *
 * @param timestamp: the posix timestamp to be converted into a string
 * @return A ISO 8601 formatted string representation of the timestamp with a self-reference
 */
extern cstr_t format_iso_8601(number_t posix_time);
/**
 * @brief Converts from a formatted RFC 5322 timestamp into an absolute posix timestamp
 * @param datetime: A date-time string formatted using the RFC 5322 standard
 * @return An absolute posix timestamp (or -1 on error)
 */
extern number_t parse_rfc_5322(cstr_t datetime);
/**
 * @brief Converts a from a posix timestamp into a RFC 5322 formatted string with a self-reference
 *
 * @param timestamp: the posix timestamp to be converted into a string
 * @return A RFC 5322 formatted string representation of the timestamp with a self-reference
 */
extern cstr_t format_rfc_5322(number_t posix_time);

/**
 * @brief Converts a from a posix timestamp into a formatted string for the LCD application
 *  Example: Apr 15 13:14:15
 *
 * @param timestamp: the posix timestamp to be converted into a string
 * @return A formatted string representation of the timestamp with a self-reference
 */
extern err_t format_rfc_lcd(number_t posix_time, byte_t* buffer, uint32_t buf_size);
/**
 * @brief returns the total number of active runtime the application has used in processor seconds
 *
 * @return fractional number of seconds the application has run
 */
extern number_t cputime(void);
/**
 * @brief returns the total number of active user-space runtime the application has used in processor seconds
 *
 * @return fractional number of seconds the application has run
 */
extern number_t usertime(void);
/**
 * @brief returns the total number of active runtime the calling thread  has used in processor seconds
 *
 * @return fractional number of seconds the calling thread has run
 */
extern number_t threadcpu(void);
/**
 * @brief An optimized time-stamp callback generator for use with slog.h
 *
 * @param dest: a buffer on which to print the resulting time-string value
 * @param dest_len: max number of bytes to write into the dest buffer
 * @return number of bytes written to the dest buffer
 */
extern size_t abq_logtime(byte_t *dest, uint8_t dest_len);

#endif /* INCLUDE_SPIL_OS_AQTIME_H_ */
