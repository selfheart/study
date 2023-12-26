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
 * @file time_enc.h
 * @date Jun 14, 2019
 * @author fteichmann
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */
#include <platform/platformconfig.h>
#include <ontrac/text/codec.h>

#ifndef OTAMATIC_REFERENCE_APP_TIME_ENC_H
#define OTAMATIC_REFERENCE_APP_TIME_ENC_H

typedef struct
{
    uint16_t tm_sec;			/* Seconds.	[0-60] (1 leap second) */
    uint16_t tm_min;			/* Minutes.	[0-59] */
    uint16_t tm_hour;			/* Hours.	[0-23] */
    uint16_t tm_mday;			/* Day.		[1-31] */
    uint16_t tm_mon;			/* Month.	[0-11] */
    uint16_t tm_year;			/* Year	- 1900.  */
    uint16_t tm_wday;			/* Day of week.	[0-6] */
    uint16_t tm_yday;			/* Days in year.[0-365]	*/
    uint16_t tm_isdst;			/* DST.		[-1/0/1]*/
    int32_t tm_tz_offset;
} abq_datetime_t;

typedef enum{
    abq_locale_default = 0,
    abq_locale_en = 0,
    abq_locale_jp,
    abq_locale_de,

    num_of_abq_locale
} abq_locale_t;

typedef enum{
    abq_tm_default = 0,
    abq_tm_iso8601 = 0,
    abq_tm_rfc5322,
    abq_tm_rfc_lcd,

    num_of_abq_tm
} abq_tm_format_t;

#define ABQ_TM_FRMT_RFC_LCD  "%b %d %H:%M:%S"
#define ABQ_TM_FRMT_ISO_8601  "%Y-%m-%dT%H:%M:%SZ"
#define ABQ_TM_FRMT_RFC_5322  "%a, %d %b %Y %H:%M:%S"

/**
 * ABQ implementation C standard library function 'strftime'. The following formatting options are supported:
 * %b,%B,%d,%H,%M,%S,%Y,%m',%a,%A
 * @param buffer is where the result will be written to
 * @param buf_size max number of bytes to write to buffer
 * @param format the requested formatting string
 * @param locale language locale option
 * @param tm_sec seconds
 * @param tm_min minutes
 * @param tm_hour hours
 * @param tm_mday month day
 * @param tm_mon month
 * @param tm_year year
 * @param tm_wday weekday
 * @param tm_yday year day
 * @param tm_isdst is daylight saving
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_encode_datetime(abq_encoder_t *encoder,
        cstr_t input_format, abq_locale_t locale, const abq_datetime_t* datetime);

extern int32_t time_decode(cstr_t buffer,
        abq_tm_format_t tm_format,
        abq_locale_t locale,
        abq_datetime_t *datetime);

extern int32_t time_decode_format(cstr_t buffer,
        cstr_t input_format, abq_locale_t locale, abq_datetime_t *datetime);

#endif //OTAMATIC_REFERENCE_APP_TIME_ENC_H
