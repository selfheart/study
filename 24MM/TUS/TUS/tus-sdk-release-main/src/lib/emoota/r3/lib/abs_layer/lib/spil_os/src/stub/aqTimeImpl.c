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

// comment maintained in header
err_t aqSpilClkGetContinuousTime(aqSpilClkContinuousTime_t *tStamp) {
    err_t retval = EUNSPECIFIED;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return retval;
}

err_t aqSpilClkGetContiniousTime(aqSpilClkContiniousTime_t *tStamp) {
    return aqSpilClkGetContinuousTime(tStamp);
}

// comment maintained in aqTime.h
err_t aqSpilClkContinuousTimeToString(aqSpilClkContinuousTime_t *tStamp, char timeString[], size_t sz ) {

    /* MISRA - Fix this so it doesn't use snprintf */
    err_t status = EUNSPECIFIED;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return status;
}

// comment maintained in aqTime.h
err_t aqSpilClkGetRtc(aqSpilClkRtc_t *rtc_time)
{
    err_t retval = EUNSPECIFIED;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return retval;
}

err_t aqSpilClkRtcToString(const aqSpilClkRtc_t *rtc_time, char timeString[], size_t sz ) {
    err_t status = EUNSPECIFIED;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return status;
}

// comment maintained in aqTime.h
aqSpilClkStatus_t aqSpilDiffContinuousTime(const aqSpilClkContinuousTime_t *in_time2,
                                           const aqSpilClkContinuousTime_t *in_time1,
                                           int64_t *diffInUs)
{
    aqSpilClkStatus_t status = aqSpilClkStatusError;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return status;
}

aqSpilClkStatus_t aqSpilDiffContiniousTime(const aqSpilClkContiniousTime_t *time2,
                                           const aqSpilClkContiniousTime_t *time1,
                                           int64_t *diffInUs) {
    aqSpilClkStatus_t retval = aqSpilClkStatusError;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return retval;
}


number_t monotonic(void) {
    number_t retval = abq_nan;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return retval;
}


number_t realtime(void) {
    number_t retval = abq_nan;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return retval;
}


number_t parse_iso_8601(cstr_t datetime_input) {
    number_t seconds = 0.0;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return seconds;
}

cstr_t format_iso_8601(number_t posix_time) {
    cstr_t retval = NULL;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return retval;
}

number_t parse_rfc_5322(cstr_t datetime) {
    time_t time_result = 0;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return (number_t) time_result;
}

cstr_t format_rfc_5322(number_t posix_time) {
    cstr_t retval = NULL;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return retval;
}

err_t format_rfc_lcd(number_t posix_time, byte_t *buffer, uint32_t buf_size) {
    err_t rvalue = 0;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return rvalue;
}


number_t cputime(void) {
    number_t rvalue = abq_nan;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return rvalue;
}


number_t usertime(void) {
    number_t rvalue = abq_nan;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return rvalue;
}

number_t threadcpu(void) {
    number_t rvalue = abq_nan;
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return rvalue;
}

size_t abq_logtime(byte_t *dest, uint8_t dest_len) {
    ABQ_ENCODER(encoder, &utf8_codec, dest, dest_len);
    printf("!!!!!!!!!!!!!!! %s\n", __FUNCTION__);
    return encoder.pos;
}

