// #line 2 "ontrac/status_codes.c"
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
 * @file ontrac/status_codes.c
 * @date Apr 9, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 *
 */

#include <ontrac/ontrac/status_codes.h>
#include <ontrac/unicode/utf8_utils.h>
#include <ontrac/ontrac/mem_usage.h>
#include <ontrac/util/thread_utils.h>
#include <string.h>
#if defined(_WIN32) && !defined(__SYMBIAN32__)
// For call to Sleep
# include <windows.h>
#else /* !Windows */
// For call to sleep
# include <unistd.h>
#endif /* !Windows */
#ifdef __linux__
#include <execinfo.h>
#elif defined(__MINGW32__)
void _assert (const char*, const char*, int32_t);
#endif /* __linux __ */

/**
 * A global err_t to be used instead of errno so that breakpoints can be set to stop when an error is set
 */
static err_t abq_status = EXIT_SUCCESS;

// These should match slog.c strings (w/out [brackets])
#define abq_fatal_label ("FATAL")
#define abq_error_label ("ERROR")
#define abq_warn_label (" WARN")
#define abq_info_label (" INFO")
#define abq_debug_label ("DEBUG")
#define abq_trace_label ("TRACE")

// Put here to reduce the probability of stack-overflow situations
#if !defined(NDEBUG)
// Largest acceptable MISRA size: INT16_MAX
static byte_t abq_log_msg_buf[INT16_MAX] = "";
#else
static byte_t abq_log_msg_buf[B4096_SIZE] = "";
#endif /* NDEBUG */
static abq_encoder_t abqlogenc = {
    .codec = &ascii_codec,
    .dest = &abq_log_msg_buf[0],
    .pos = 0,
    .max = sizeof(abq_log_msg_buf)
};

static log_msg_handler_t ontrac_logger = NULL;

static bool_t is_status_ok = true;
bool_t abq_status_is_ok(void) {
    return is_status_ok;
}

static bool_t is_aborting = false;
bool_t abq_is_aborting(void) {
    return is_aborting;
}

static void abq_loop_forever(cstr_t tag, int32_t line, err_t status, cstr_t msg) {
    // Printed again just so MISRA doesn't complain about unused variables
    abq_log_s_msg(ABQ_FATAL_LEVEL, tag, line, status, msg);
    log_msg_handler_t callback = ontrac_logger;
    if (NULL != callback) {
        callback(ABQ_FATAL_LEVEL, "Failed to register handler\n", 27U); // parasoft-suppress CERT_C-MSC41-a-1 "c0236. This string does not contain sensitive information."
        callback(ABQ_FATAL_LEVEL, NULL, 0); // Flush log
    }
#if defined(_WIN32) && !defined(__SYMBIAN32__)
    while(true) { (void) Sleep(1000); };
#else /* !Windows */
    while(true) { (void) sleep(1); };
#endif /* Windows */
}

#if !defined(NDEBUG)
static void abq_invoke_assert(cstr_t tag, int32_t line, err_t status, cstr_t msg){
#ifdef __MINGW32__
    _assert(msg, tag, line);
#elif defined(__ASSERT_FUNCTION)
    __assert_fail(msg, tag, (uint32_t)line, __ASSERT_FUNCTION);
#else
    assert(!"abq_fatal not fully featured on this platform");
#endif
    abq_loop_forever(tag, line, status, msg);
}
static abq_panic_func_t ontrac_fatality = abq_invoke_assert;
#else /* NDEBUG */
static abq_panic_func_t ontrac_fatality = abq_loop_forever;
#endif /* NDEBUG */

static  abq_mutex_t* log_mutex= NULL;
static inline void loglock( void ) {
    if (NULL == log_mutex) {
        log_mutex =  ontrac_mutex_get();
        VITAL_NOT_NULL(log_mutex);
    }
    VITAL_NOT_NULL(log_mutex->lock);
    (void) log_mutex->lock(log_mutex->mutex);
}
static inline void unloglock(void ) {
    if (NULL != log_mutex) {
        VITAL_NOT_NULL(log_mutex->lock);
        (void) log_mutex->unlock(log_mutex->mutex);
    }
}

#ifndef HAVE_SLOG_H
static abq_log_timestamp_cb timestamp_cb = NULL;
static abq_log_level_t filter = ABQ_UNKNOWN_LEVEL;

err_t abq_set_log_msg_handler(log_msg_handler_t log_out, abq_log_level_t level) {
    err_t retval = EXIT_SUCCESS;
    if (NULL != log_out) {
        if((ontrac_logger == log_out) && (filter == level)) {
            retval = EALREADY;
        } else {
            // Overwrite anyway
            filter = level;
            ontrac_logger = log_out;
        }
    } else if(NULL != ontrac_logger) {
        ontrac_logger = NULL;
        filter = level;
    } else {
        retval = EALREADY;
    }
    return retval;
}

err_t abq_log_set_timestamp_cb(abq_log_timestamp_cb get_timestamp) {
    err_t retval = EXIT_SUCCESS;
    if (NULL != get_timestamp) {
        if(timestamp_cb == get_timestamp) {
            retval = EALREADY;
        } else {
            // Set the new timestamp callback
            timestamp_cb = get_timestamp;
        }
    } else if(NULL != timestamp_cb) {
        timestamp_cb = NULL;
    } else {
        retval = EALREADY;
    }
    return retval;
}

bool_t abq_log_level_is_active(abq_log_level_t log_level) {
    return log_level >= filter;
}

static void abq_encode_msg_info(abq_encoder_t *encoder,
        abq_log_level_t level, cstr_t tag, int32_t line) {
    if (NULL != timestamp_cb) {
        size_t amount = encoder->max - encoder->pos;
        if(amount > (size_t)UINT8_MAX) {
            amount = (size_t)UINT8_MAX;
        }
        encoder->pos += timestamp_cb(&encoder->dest[encoder->pos], (uint8_t)amount);
        (void) abq_encode_char(encoder, ' ');
    }
    (void) abq_encode_char(encoder, '[');
    switch (level) {
    case ABQ_FATAL_LEVEL:
        (void) abq_encode_loginfo(encoder, abq_fatal_label, -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0237. This string does not contain sensitive information."
        break;
    case ABQ_ERROR_LEVEL:
        (void) abq_encode_loginfo(encoder, abq_error_label, -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0238. This string does not contain sensitive information."
        break;
    case ABQ_WARN_LEVEL:
        (void) abq_encode_loginfo(encoder, abq_warn_label, -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0239. This string does not contain sensitive information."
        break;
    case ABQ_INFO_LEVEL:
        (void) abq_encode_loginfo(encoder, abq_info_label, -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0240. This string does not contain sensitive information."
        break;
    case ABQ_DEBUG_LEVEL:
        (void) abq_encode_loginfo(encoder, abq_debug_label, -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0241. This string does not contain sensitive information."
        break;
    case ABQ_TRACE_LEVEL:
        (void) abq_encode_loginfo(encoder, abq_trace_label, -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0242. This string does not contain sensitive information."
        break;
    default:
        (void) abq_encode_loginfo(encoder, abq_null_str, -1);
        break;
    }
    (void) abq_encode_loginfo(encoder, "], ", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0243. This string does not contain sensitive information."
    (void) abq_encode_loginfo(encoder, tag, -1);
    (void) abq_encode_loginfo(encoder, ":", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0244. This string does not contain sensitive information."
    (void) abq_encode_int(encoder, (int64_t)line, DECIMAL_RADIX);
    (void) abq_encode_loginfo(encoder, ", ", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0245. This string does not contain sensitive information."
}
#else /* !defined(HAVE_SLOG_H) */


#define SLOG_HEADER_SPACE (256)
static byte_t *abq_log_msg_body = &abq_log_msg_buf[SLOG_HEADER_SPACE];

static SlogLevel_t abq_slog_level_of(abq_log_level_t abq_level) {
    SlogLevel_t slog_level = SLOG_LEVEL_ERROR;
    switch (abq_level) {
    case ABQ_FATAL_LEVEL:
        slog_level = SLOG_LEVEL_FATAL;
        break;
    case ABQ_ERROR_LEVEL:
        slog_level = SLOG_LEVEL_ERROR;
        break;
    case ABQ_WARN_LEVEL:
        slog_level = SLOG_LEVEL_WARN;
        break;
    case ABQ_INFO_LEVEL:
        slog_level = SLOG_LEVEL_INFO;
        break;
    case ABQ_DEBUG_LEVEL:
        slog_level = SLOG_LEVEL_DEBUG;
        break;
    case ABQ_TRACE_LEVEL:
        slog_level = SLOG_LEVEL_TRACE;
        break;
    default:
        slog_level = SLOG_LEVEL_ERROR;
        break;
    }
    return slog_level;
}

bool_t abq_log_level_is_active(abq_log_level_t log_level) {
    return SlogLevel_isActive(abq_slog_level_of(log_level));
}

err_t abq_log_set_timestamp_cb(abq_log_timestamp_cb get_timestamp) {
    Error *slogerr = registerSlogTimeStringCallback(get_timestamp);
    return (NULL == slogerr) ? EXIT_SUCCESS : EUNSPECIFIED;
}

static abq_log_level_t slog_parse_level(const char* log_msg, size_t msg_len) {
    // Write out the full message to the callback function
    int32_t label_start = utf8_index_of_char(log_msg, (int32_t)msg_len, '[') + 1;
    int32_t label_length = utf8_index_of_char(log_msg, (int32_t)msg_len, ']') - label_start;
    abq_log_level_t log_level = ABQ_UNKNOWN_LEVEL;
    if ((0 >= label_start) || (0 >= label_length)) {
        // Failed to find log-level label
    } else if(utf8_starts_with(&log_msg[label_start],
            label_length, abq_trace_label, -1)) {
        log_level = ABQ_TRACE_LEVEL;
    } else if(utf8_starts_with(&log_msg[label_start],
            label_length, abq_debug_label, -1)) {
        log_level = ABQ_DEBUG_LEVEL;
    } else if(utf8_starts_with(&log_msg[label_start],
            label_length, abq_info_label, -1)) {
        log_level = ABQ_INFO_LEVEL;
    } else if(utf8_starts_with(&log_msg[label_start],
            label_length, abq_warn_label, -1)) {
        log_level = ABQ_WARN_LEVEL;
    } else if(utf8_starts_with(&log_msg[label_start],
            label_length, abq_error_label, -1)) {
        log_level = ABQ_ERROR_LEVEL;
    } else if(utf8_starts_with(&log_msg[label_start],
            label_length, abq_fatal_label, -1)) {
        log_level = ABQ_FATAL_LEVEL;
    } else {
        // No matching log-level labels
    }
    return log_level;
}

static void slog_callback(const char* log_msg) {
    log_msg_handler_t callback = ontrac_logger;
    if (NULL != callback) {
        if (NULL == log_msg) {
            callback(ABQ_UNKNOWN_LEVEL, NULL, 0);
        }else if (log_msg == abq_log_msg_body) {
            // Assert that header has already been written
            // Move the message data to just behind the header
            (void) abq_encode_loginfo(&abqlogenc, log_msg, -1);
        } else {
            int32_t msg_len = utf8_byte_length(log_msg, -1);
            int32_t newline = utf8_index_of_char(log_msg, msg_len, '\n');
            if(0 > msg_len) {
                // Invalid message, ignore it
            }else if((0 <= newline) && (newline <= 2) && ((newline+1) == msg_len)) {
                // Complete the message with newline
                if(EOVERFLOW == abq_encode_loginfo(&abqlogenc, log_msg, msg_len)) {
                    // Truncate and add Ellipsis / NewLine
                    abqlogenc.pos = abqlogenc.max - 4UL;
                }
                abq_log_level_t log_level = slog_parse_level(abq_log_msg_buf, abqlogenc.pos);
                if (abq_log_level_is_active(log_level)) {
                    callback(log_level, abq_log_msg_buf, abqlogenc.pos);
                }
                abq_log_msg_buf[0] = '\0';
                abqlogenc.pos = 0U;
                unloglock();
            } else {
                if(slog_parse_level(log_msg, (size_t)msg_len) == ABQ_UNKNOWN_LEVEL) {
                    // Expect that this is the message body and that the log level has already been written
                } else {
                    // Expect this is start of a new message
                    abqlogenc.pos = 0U;
                    loglock();
                }
                (void) abq_encode_loginfo(&abqlogenc, log_msg, msg_len);
            }
        }
    }
}

err_t abq_set_log_msg_handler(log_msg_handler_t log_out, abq_log_level_t level) {
    err_t retval = EXIT_SUCCESS;
    if (NULL != log_out) {
        SlogLevel_t slog_level = abq_slog_level_of(level);
        if ((ontrac_logger == log_out) && (SlogLevel_isActive(slog_level))) {
            retval = EALREADY;
        }
        // Overwrite regardless
        (void) registerSlogTimeStringCallback(NULL);
        (void) registerSlogOutputCallback(slog_callback);
        (void) SlogLevel_setActive(slog_level);
        ontrac_logger = log_out;
    } else if(NULL != ontrac_logger) {
        ontrac_logger = NULL;
    } else {
        retval = EALREADY;
    }
    return retval;
}

#endif /* defined(HAVE_SLOG_H) */


cstr_t http_status_code_desc(err_t code) {
    cstr_t rvalue = NULL;

    static const cstr_t HTTP_CONTINUE_DESC                 = "Continue"; // parasoft-suppress CERT_C-MSC41-a-1 "c0246. This string does not contain sensitive information."
    static const cstr_t SWITCHING_PROTOCOLS_DESC           = "Switching Protocols"; // parasoft-suppress CERT_C-MSC41-a-1 "c0247. This string does not contain sensitive information."
    static const cstr_t HTTP_PROCESSING_DESC               = "Processing"; // parasoft-suppress CERT_C-MSC41-a-1 "c0248. This string does not contain sensitive information."
    static const cstr_t HTTP_OK_DESC                       = "OK"; // parasoft-suppress CERT_C-MSC41-a-1 "c0249. This string does not contain sensitive information."
    static const cstr_t HTTP_CREATED_DESC                  = "Created"; // parasoft-suppress CERT_C-MSC41-a-1 "c0250. This string does not contain sensitive information."
    static const cstr_t HTTP_ACCEPTED_DESC                 = "Accepted"; // parasoft-suppress CERT_C-MSC41-a-1 "c0251. This string does not contain sensitive information."
    static const cstr_t NON_AUTHORITATIVE_INFO_DESC        = "Non Authoritative Information"; // parasoft-suppress CERT_C-MSC41-a-1 "c0252. This string does not contain sensitive information."
    static const cstr_t NO_CONTENT_DESC                    = "No Content"; // parasoft-suppress CERT_C-MSC41-a-1 "c0253. This string does not contain sensitive information."
    static const cstr_t RESET_CONTENT_DESC                 = "Reset Content"; // parasoft-suppress CERT_C-MSC41-a-1 "c0254. This string does not contain sensitive information."
    static const cstr_t PARTIAL_CONTENT_DESC               = "Partial Content"; // parasoft-suppress CERT_C-MSC41-a-1 "c0255. This string does not contain sensitive information."
    static const cstr_t MULTI_STATUS_DESC                  = "Multi-Status"; // parasoft-suppress CERT_C-MSC41-a-1 "c0256. This string does not contain sensitive information."
    static const cstr_t ALREADY_REPORTED_DESC              = "Already Reported"; // parasoft-suppress CERT_C-MSC41-a-1 "c0257. This string does not contain sensitive information."
    static const cstr_t HTTP_IM_USED_DESC                  = "IM used"; // parasoft-suppress CERT_C-MSC41-a-1 "c0258. This string does not contain sensitive information."
    static const cstr_t MULTIPLE_CHOICES_DESC              = "Mutliple Choices"; // parasoft-suppress CERT_C-MSC41-a-1 "c0259. This string does not contain sensitive information."
    static const cstr_t MOVED_PERMANENTLY_DESC             = "Moved Permanently"; // parasoft-suppress CERT_C-MSC41-a-1 "c0260. This string does not contain sensitive information."
    static const cstr_t MOVED_TEMPORARILY_DESC             = "Moved Temporarily"; // parasoft-suppress CERT_C-MSC41-a-1 "c0261. This string does not contain sensitive information."
    static const cstr_t HTTP_SEE_OTHER_DESC                = "See Other"; // parasoft-suppress CERT_C-MSC41-a-1 "c0262. This string does not contain sensitive information."
    static const cstr_t NOT_MODIFIED_DESC                  = "Not Modified"; // parasoft-suppress CERT_C-MSC41-a-1 "c0263. This string does not contain sensitive information."
    static const cstr_t USE_PROXY_DESC                     = "Use Proxy"; // parasoft-suppress CERT_C-MSC41-a-1 "c0264. This string does not contain sensitive information."
    static const cstr_t TEMPORARY_REDIRECT_DESC            = "Temporary Redirect"; // parasoft-suppress CERT_C-MSC41-a-1 "c0265. This string does not contain sensitive information."
    static const cstr_t PERMANENT_REDIRECT_DESC            = "Permanent Redirect"; // parasoft-suppress CERT_C-MSC41-a-1 "c0266. This string does not contain sensitive information."
    static const cstr_t ROLLBACK_COMPLETE_DESC             = "Rollback Completed"; // parasoft-suppress CERT_C-MSC41-a-1 "c0267. This string does not contain sensitive information."
    static const cstr_t BAD_REQUEST_DESC                   = "Bad Request"; // parasoft-suppress CERT_C-MSC41-a-1 "c0268. This string does not contain sensitive information."
    static const cstr_t UNAUTHORIZED_DESC                  = "Unauthorized"; // parasoft-suppress CERT_C-MSC41-a-1 "c0269. This string does not contain sensitive information."
    static const cstr_t PAYMENT_REQUIRED_DESC              = "Payment Required"; // parasoft-suppress CERT_C-MSC41-a-1 "c0270. This string does not contain sensitive information."
    static const cstr_t FORBIDDEN_DESC                     = "Forbidden"; // parasoft-suppress CERT_C-MSC41-a-1 "c0271. This string does not contain sensitive information."
    static const cstr_t NOT_FOUND_DESC                     = "Not Found"; // parasoft-suppress CERT_C-MSC41-a-1 "c0272. This string does not contain sensitive information."
    static const cstr_t METHOD_NOT_ALLOWED_DESC            = "Method Not Allowed"; // parasoft-suppress CERT_C-MSC41-a-1 "c0273. This string does not contain sensitive information."
    static const cstr_t NOT_ACCEPTABLE_DESC                = "Not Acceptable"; // parasoft-suppress CERT_C-MSC41-a-1 "c0274. This string does not contain sensitive information."
    static const cstr_t PROXY_AUTH_REQUIRED_DESC           = "Proxy Authentication Required"; // parasoft-suppress CERT_C-MSC41-a-1 "c0275. This string does not contain sensitive information."
    static const cstr_t REQUEST_TIMEOUT_DESC               = "Request Timeout"; // parasoft-suppress CERT_C-MSC41-a-1 "c0276. This string does not contain sensitive information."
    static const cstr_t HTTP_CONFLICT_DESC                 = "Conflict"; // parasoft-suppress CERT_C-MSC41-a-1 "c0277. This string does not contain sensitive information."
    static const cstr_t HTTP_GONE_DESC                     = "Gone"; // parasoft-suppress CERT_C-MSC41-a-1 "c0278. This string does not contain sensitive information."
    static const cstr_t LENGTH_REQUIRED_DESC               = "Length Required"; // parasoft-suppress CERT_C-MSC41-a-1 "c0279. This string does not contain sensitive information."
    static const cstr_t PRECONDITION_FAILED_DESC           = "Precondition Failed"; // parasoft-suppress CERT_C-MSC41-a-1 "c0280. This string does not contain sensitive information."
    static const cstr_t REQUEST_ENTITY_TOO_LARGE_DESC      = "Request Entity Too Large"; // parasoft-suppress CERT_C-MSC41-a-1 "c0281. This string does not contain sensitive information."
    static const cstr_t REQUEST_URI_TOO_LONG_DESC          = "Request-URI Too Long"; // parasoft-suppress CERT_C-MSC41-a-1 "c0282. This string does not contain sensitive information."
    static const cstr_t UNSUPPORTED_MEDIA_TYPE_DESC        = "Unsupported Media Type"; // parasoft-suppress CERT_C-MSC41-a-1 "c0283. This string does not contain sensitive information."
    static const cstr_t REQ_RANGE_ERROR_DESC               = "Requested Range Not Satisfiable"; // parasoft-suppress CERT_C-MSC41-a-1 "c0284. This string does not contain sensitive information."
    static const cstr_t EXPECTATION_FAILED_DESC            = "Expectation Failed"; // parasoft-suppress CERT_C-MSC41-a-1 "c0285. This string does not contain sensitive information."
    static const cstr_t MISDIRECTED_REQUEST_DESC           = "Misdirected Request"; // parasoft-suppress CERT_C-MSC41-a-1 "c0286. This string does not contain sensitive information."
    static const cstr_t UNPROCCESSABLE_ENTITY_DESC         = "Unproccessable entity"; // parasoft-suppress CERT_C-MSC41-a-1 "c0287. This string does not contain sensitive information."
    static const cstr_t HTTP_LOCKED_DESC                   = "Locked"; // parasoft-suppress CERT_C-MSC41-a-1 "c0288. This string does not contain sensitive information."
    static const cstr_t FAILED_DEPENDENCY_DESC             = "Failed Dependency"; // parasoft-suppress CERT_C-MSC41-a-1 "c0289. This string does not contain sensitive information."
    static const cstr_t UPGRADE_REQUIRED_DESC              = "Upgrade Required"; // parasoft-suppress CERT_C-MSC41-a-1 "c0290. This string does not contain sensitive information."
    static const cstr_t PRECONDITION_REQUIRED_DESC         = "Precondition Required"; // parasoft-suppress CERT_C-MSC41-a-1 "c0291. This string does not contain sensitive information."
    static const cstr_t TOO_MANY_REQUESTS_DESC             = "Too Many Requests"; // parasoft-suppress CERT_C-MSC41-a-1 "c0292. This string does not contain sensitive information."
    static const cstr_t REQ_HEADER_TOO_LARGE_DESC          = "Request Header Fields Too Large"; // parasoft-suppress CERT_C-MSC41-a-1 "c0293. This string does not contain sensitive information."
    static const cstr_t LEGALLY_RESTRICTED_DESC            = "Unavailable For Legal Reasons"; // parasoft-suppress CERT_C-MSC41-a-1 "c0294. This string does not contain sensitive information."
    static const cstr_t INTERNAL_SERVER_ERROR_DESC         = "Internal Server Error"; // parasoft-suppress CERT_C-MSC41-a-1 "c0295. This string does not contain sensitive information."
    static const cstr_t NOT_IMPLEMENTED_DESC               = "Not Implemented"; // parasoft-suppress CERT_C-MSC41-a-1 "c0296. This string does not contain sensitive information."
    static const cstr_t BAD_GATEWAY_DESC                   = "Bad Gateway"; // parasoft-suppress CERT_C-MSC41-a-1 "c0297. This string does not contain sensitive information."
    static const cstr_t SERVICE_UNAVAILABLE_DESC           = "Service Unavailable"; // parasoft-suppress CERT_C-MSC41-a-1 "c0298. This string does not contain sensitive information."
    static const cstr_t GATEWAY_TIMEOUT_DESC               = "Gateway Timeout"; // parasoft-suppress CERT_C-MSC41-a-1 "c0299. This string does not contain sensitive information."
    static const cstr_t HTTP_VERSION_NOT_SUPPORTED_DESC    = "HTTP Version Not Supported"; // parasoft-suppress CERT_C-MSC41-a-1 "c0300. This string does not contain sensitive information."
    static const cstr_t VARIANT_ALSO_NEGOTIATES_DESC       = "Variant Also Negotiates"; // parasoft-suppress CERT_C-MSC41-a-1 "c0301. This string does not contain sensitive information."
    static const cstr_t INSUFFICIENT_STORAGE_DESC          = "Insufficient Storage"; // parasoft-suppress CERT_C-MSC41-a-1 "c0302. This string does not contain sensitive information."
    static const cstr_t LOOP_DETECTED_DESC                 = "Loop Detected"; // parasoft-suppress CERT_C-MSC41-a-1 "c0303. This string does not contain sensitive information."
    static const cstr_t NOT_EXTENDED_DESC                  = "Not Extended"; // parasoft-suppress CERT_C-MSC41-a-1 "c0304. This string does not contain sensitive information."
    static const cstr_t NETWORK_AUTH_REQUIRED_DESC         = "Network Authentication Required"; // parasoft-suppress CERT_C-MSC41-a-1 "c0305. This string does not contain sensitive information."
    static const cstr_t UNINITIALIZED_DESC                 = "Uninitialized Submodule"; // parasoft-suppress CERT_C-MSC41-a-1 "c0306. This string does not contain sensitive information."
    static const cstr_t WRONG_CONTENT_LENGTH_DESC          = "Wrong Content-Length"; // parasoft-suppress CERT_C-MSC41-a-1 "c0307. This string does not contain sensitive information."
    static const cstr_t DIGEST_MISMATCH_DESC               = "Digest Mismatch"; // parasoft-suppress CERT_C-MSC41-a-1 "c0308. This string does not contain sensitive information."
    static const cstr_t INTERRUPTED_DESC                   = "Interrupted"; // parasoft-suppress CERT_C-MSC41-a-1 "c0309. This string does not contain sensitive information."
    static const cstr_t SIGNATURE_FAILURE_DESC             = "Signature Failed Validation"; // parasoft-suppress CERT_C-MSC41-a-1 "c0310. This string does not contain sensitive information."
    static const cstr_t USER_CANCELED_DESC                 = "User Canceled"; // parasoft-suppress CERT_C-MSC41-a-1 "c0311. This string does not contain sensitive information."
    static const cstr_t UNRESOLVED_HOST_DESC               = "Unresolved Hostname"; // "DNS lookup failure" // parasoft-suppress CERT_C-MSC41-a-1 "c0312. This string does not contain sensitive information."
    static const cstr_t NO_ROUTE_TO_HOST_DESC              = "No Route To Host"; // parasoft-suppress CERT_C-MSC41-a-1 "c0313. This string does not contain sensitive information."
    static const cstr_t NIO_TX_TIMEOUT_DESC                = "Transmit Timeout"; // parasoft-suppress CERT_C-MSC41-a-1 "c0314. This string does not contain sensitive information."
    static const cstr_t NIO_RX_TIMEOUT_DESC                = "Receive Timeout"; // parasoft-suppress CERT_C-MSC41-a-1 "c0315. This string does not contain sensitive information."
    static const cstr_t NIO_HANGUP_DESC                    = "Closed By Peer"; // parasoft-suppress CERT_C-MSC41-a-1 "c0316. This string does not contain sensitive information."
    static const cstr_t NIO_ERROR_DESC                     = "Input/Output Error"; // parasoft-suppress CERT_C-MSC41-a-1 "c0317. This string does not contain sensitive information."
    static const cstr_t NIO_CLOSED_DESC                    = "Resource was Closed"; // parasoft-suppress CERT_C-MSC41-a-1 "c0318. This string does not contain sensitive information."


    switch (code) {
    case HTTP_CONTINUE:
        rvalue = HTTP_CONTINUE_DESC;
        break;
    case SWITCHING_PROTOCOLS:
        rvalue = SWITCHING_PROTOCOLS_DESC;
        break;
    case HTTP_PROCESSING:
        rvalue = HTTP_PROCESSING_DESC;
        break;
    case HTTP_OK:
        rvalue = HTTP_OK_DESC;
        break;
    case HTTP_CREATED:
        rvalue = HTTP_CREATED_DESC;
        break;
    case HTTP_ACCEPTED:
        rvalue = HTTP_ACCEPTED_DESC;
        break;
    case NON_AUTHORITATIVE_INFORMATION:
        rvalue = NON_AUTHORITATIVE_INFO_DESC;
        break;
    case NO_CONTENT:
        rvalue = NO_CONTENT_DESC;
        break;
    case RESET_CONTENT:
        rvalue = RESET_CONTENT_DESC;
        break;
    case PARTIAL_CONTENT:
        rvalue = PARTIAL_CONTENT_DESC;
        break;
    case MULTI_STATUS:
        rvalue = MULTI_STATUS_DESC;
        break;
    case ALREADY_REPORTED:
        rvalue = ALREADY_REPORTED_DESC;
        break;
    case HTTP_IM_USED:
        rvalue = HTTP_IM_USED_DESC;
        break;
    case MULTIPLE_CHOICES:
        rvalue = MULTIPLE_CHOICES_DESC;
        break;
    case MOVED_PERMANENTLY:
        rvalue = MOVED_PERMANENTLY_DESC;
        break;
    case MOVED_TEMPORARILY:
        rvalue = MOVED_TEMPORARILY_DESC;
        break;
    case HTTP_SEE_OTHER:
        rvalue = HTTP_SEE_OTHER_DESC;
        break;
    case NOT_MODIFIED:
        rvalue = NOT_MODIFIED_DESC;
        break;
    case USE_PROXY:
        rvalue = USE_PROXY_DESC;
        break;
    case TEMPORARY_REDIRECT:
        rvalue = TEMPORARY_REDIRECT_DESC;
        break;
    case PERMANENT_REDIRECT:
        rvalue = PERMANENT_REDIRECT_DESC;
        break;
    case ROLLBACK_COMPLETE:
        rvalue = ROLLBACK_COMPLETE_DESC;
        break;
    case BAD_REQUEST:
        rvalue = BAD_REQUEST_DESC;
        break;
    case UNAUTHORIZED:
        rvalue = UNAUTHORIZED_DESC;
        break;
    case PAYMENT_REQUIRED:
        rvalue = PAYMENT_REQUIRED_DESC;
        break;
    case FORBIDDEN:
        rvalue = FORBIDDEN_DESC;
        break;
    case NOT_FOUND:
        rvalue = NOT_FOUND_DESC;
        break;
    case METHOD_NOT_ALLOWED:
        rvalue = METHOD_NOT_ALLOWED_DESC;
        break;
    case NOT_ACCEPTABLE:
        rvalue = NOT_ACCEPTABLE_DESC;
        break;
    case PROXY_AUTHENTICATION_REQUIRED:
        rvalue = PROXY_AUTH_REQUIRED_DESC;
        break;
    case REQUEST_TIMEOUT:
        rvalue = REQUEST_TIMEOUT_DESC;
        break;
    case HTTP_CONFLICT:
        rvalue = HTTP_CONFLICT_DESC;
        break;
    case HTTP_GONE:
        rvalue = HTTP_GONE_DESC;
        break;
    case LENGTH_REQUIRED:
        rvalue = LENGTH_REQUIRED_DESC;
        break;
    case PRECONDITION_FAILED:
        rvalue = PRECONDITION_FAILED_DESC;
        break;
    case REQUEST_ENTITY_TOO_LARGE:
        rvalue = REQUEST_ENTITY_TOO_LARGE_DESC;
        break;
    case REQUEST_URI_TOO_LONG:
        rvalue = REQUEST_URI_TOO_LONG_DESC;
        break;
    case UNSUPPORTED_MEDIA_TYPE:
        rvalue = UNSUPPORTED_MEDIA_TYPE_DESC;
        break;
    case REQUEST_RANGE_NOT_SATISFIABLE:
        rvalue = REQ_RANGE_ERROR_DESC;
        break;
    case EXPECTATION_FAILED:
        rvalue = EXPECTATION_FAILED_DESC;
        break;
    case MISDIRECTED_REQUEST:
        rvalue = MISDIRECTED_REQUEST_DESC;
        break;
    case UNPROCCESSABLE_ENTITY:
        rvalue = UNPROCCESSABLE_ENTITY_DESC;
        break;
    case HTTP_LOCKED:
        rvalue = HTTP_LOCKED_DESC;
        break;
    case FAILED_DEPENDENCY:
        rvalue = FAILED_DEPENDENCY_DESC;
        break;
    case UPGRADE_REQUIRED:
        rvalue = UPGRADE_REQUIRED_DESC;
        break;
    case PRECONDITION_REQUIRED:
        rvalue = PRECONDITION_REQUIRED_DESC;
        break;
    case TOO_MANY_REQUESTS:
        rvalue = TOO_MANY_REQUESTS_DESC;
        break;
    case REQUEST_HEADER_FIELD_TOO_LARGE:
        rvalue = REQ_HEADER_TOO_LARGE_DESC;
        break;
    case UNAVAILABLE_FOR_LEGAL_REASONS:
        rvalue = LEGALLY_RESTRICTED_DESC;
        break;
    case INTERNAL_SERVER_ERROR:
        rvalue = INTERNAL_SERVER_ERROR_DESC;
        break;
    case NOT_IMPLEMENTED:
        rvalue = NOT_IMPLEMENTED_DESC;
        break;
    case BAD_GATEWAY:
        rvalue = BAD_GATEWAY_DESC;
        break;
    case SERVICE_UNAVAILABLE:
        rvalue = SERVICE_UNAVAILABLE_DESC;
        break;
    case GATEWAY_TIMEOUT:
        rvalue = GATEWAY_TIMEOUT_DESC;
        break;
    case HTTP_VERSION_NOT_SUPPORTED:
        rvalue = HTTP_VERSION_NOT_SUPPORTED_DESC;
        break;
    case VARIANT_ALSO_NEGOTIATES:
        rvalue = VARIANT_ALSO_NEGOTIATES_DESC;
        break;
    case INSUFFICIENT_STORAGE:
        rvalue = INSUFFICIENT_STORAGE_DESC;
        break;
    case LOOP_DETECTED:
        rvalue = LOOP_DETECTED_DESC;
        break;
    case NOT_EXTENDED:
        rvalue = NOT_EXTENDED_DESC;
        break;
    case NETWORK_AUTH_REQUIRED:
        rvalue = NETWORK_AUTH_REQUIRED_DESC;
        break;
    case UNINITIALIZED:
        rvalue = UNINITIALIZED_DESC;
        break;
    case WRONG_CONTENT_LENGTH:
        rvalue = WRONG_CONTENT_LENGTH_DESC;
        break;
    case DIGEST_MISMATCH:
        rvalue = DIGEST_MISMATCH_DESC;
        break;
    case INTERRUPTED:
        rvalue = INTERRUPTED_DESC;
        break;
    case SIGNATURE_FAILURE:
        rvalue = SIGNATURE_FAILURE_DESC;
        break;
    case USER_CANCELED:
        rvalue = USER_CANCELED_DESC;
        break;
    case UNRESOLVED_HOST:
        rvalue = UNRESOLVED_HOST_DESC;
        break;
    case NO_ROUTE_TO_HOST:
        rvalue = NO_ROUTE_TO_HOST_DESC;
        break;
    case NIO_TX_TIMEOUT:
        rvalue = NIO_TX_TIMEOUT_DESC;
        break;
    case NIO_RX_TIMEOUT:
        rvalue = NIO_RX_TIMEOUT_DESC;
        break;
    case NIO_HANGUP:
        rvalue = NIO_HANGUP_DESC;
        break;
    case NIO_ERROR:
        rvalue = NIO_ERROR_DESC;
        break;
    case NIO_CLOSED:
        rvalue = NIO_CLOSED_DESC;
        break;
    default:
        rvalue = NULL;
        break;
    }
    return rvalue;
}

static inline err_t abq_take_errno(err_t if_ok) {
    err_t retval = errno;
    errno = EXIT_SUCCESS;
    if (EXIT_SUCCESS == retval) {/* parasoft-suppress MISRAC2012-RULE_22_10-a-2 CERT_C-ERR30-a-2 CERT_C-ERR32-a CERT_C-ERR32-a-3 "This function may be called from a variety of different locations; some have set errno" */
        // errno wasn't set, return to previous error code
        retval = if_ok;
    }
    return retval;
}

static inline err_t abq_errno_check (err_t status_code) {
    err_t retval = status_code;
    if(EUNSPECIFIED == retval) {
        retval = abq_take_errno(status_code);
    }
    return retval;
}

http_status_code_t http_status_code_for_err(err_t err) {
    http_status_code_t rvalue = UNPROCCESSABLE_ENTITY;
    if (NULL != http_status_code_desc(err)) {
        rvalue = err;
    } else {
        err_t status = abq_errno_check(err);
        if (EUNSPECIFIED > status) {
            status *= -1;
        } else {
            // lookup desc for status as is
        }
        switch (status) {
            case EUNSPECIFIED:
                rvalue = UNPROCCESSABLE_ENTITY;
                break;
            case EXIT_SUCCESS:
                rvalue = HTTP_OK;
                break;
            case EINPROGRESS:
                rvalue = HTTP_ACCEPTED;
                // also see HTTP_PROCESSING
                break;
            case EWOULDBLOCK:
                rvalue = HTTP_ACCEPTED;
                // also see HTTP_PROCESSING
                break;
            case EPERM:
                rvalue = FORBIDDEN;
                break;
            case EACCES:
                rvalue = FORBIDDEN;
                break;
            case ENOENT:
                rvalue = NOT_FOUND;
                break;
            case EFAULT:
                rvalue = UNPROCCESSABLE_ENTITY;
                break;
            case ENOMEM:
                rvalue = UNPROCCESSABLE_ENTITY;
                break;
            case EADDRNOTAVAIL:
                rvalue = UNPROCCESSABLE_ENTITY;
                break;
            case ENOSYS:
                rvalue = NOT_IMPLEMENTED;
                break;
            case ETIMEDOUT:
                rvalue = GATEWAY_TIMEOUT;
                break;
            case ESHUTDOWN:
                rvalue = UNINITIALIZED;
                break;
            case EHOSTUNREACH:
                rvalue = NO_ROUTE_TO_HOST;
                break;
            case ENETUNREACH:
                rvalue = NO_ROUTE_TO_HOST;
                break;
            case ECONNREFUSED:
                rvalue = NIO_HANGUP;
                break;
            case ECONNRESET:
                rvalue = NIO_HANGUP;
                break;
            case EINTR:
                rvalue = INTERRUPTED;
                break;
            case ENOTCONN:
                rvalue = NIO_ERROR;
                break;
            case EIO:
                rvalue = NIO_ERROR;
                break;
            case EBADF:
                rvalue = NIO_CLOSED;
                break;
            case EPIPE:
                rvalue = NIO_CLOSED;
                break;
            case ENOSPC:
                rvalue = INSUFFICIENT_STORAGE;
                break;
            case EOVERFLOW:
                rvalue = REQUEST_ENTITY_TOO_LARGE;
                // alt. UPGRADE_REQUIRED ?
                break;
            case ERANGE:
                rvalue = REQUEST_RANGE_NOT_SATISFIABLE;
                break;
            case EBUSY:
                rvalue = SERVICE_UNAVAILABLE;
                break;
            case EINVAL:
                rvalue = BAD_REQUEST;
                break;
            case EILSEQ:
                rvalue = BAD_REQUEST;
                break;
            default:
                ABQ_DUMP_ERROR(status, "http_status_code_for_err()");
                rvalue = UNPROCCESSABLE_ENTITY;
                break;
        }
    }
    return rvalue;
}

cstr_t abq_error_desc(err_t status_code) {
    static byte_t last_unkown_error[64] = "Unknown status-code: "; // parasoft-suppress CERT_C-MSC41-a-1 "c0319. This string does not contain sensitive information."
    err_t status = abq_errno_check(status_code);
    if (EUNSPECIFIED > status) {
        status *= -1;
    } else {
        // lookup desc for status as is
    }
    cstr_t retval = NULL;
    switch (status) {
    case EUNSPECIFIED:
        retval = "Unspecified error Occurred"; // parasoft-suppress CERT_C-MSC41-a-1 "c0320. This string does not contain sensitive information."
        break;
    case EXIT_SUCCESS:
        retval = "Exit Success"; // parasoft-suppress CERT_C-MSC41-a-1 "c0321. This string does not contain sensitive information."
        break;
    case EILSEQ:
        // I really disliked the "Invalid multibyte character" message used by linux system (mvogel)
        retval = "Illegal byte sequence"; // parasoft-suppress CERT_C-MSC41-a-1 "c0322. This string does not contain sensitive information."
        break;
    case EADDRNOTAVAIL:
        retval = "Cannot assign requested address."; // parasoft-suppress CERT_C-MSC41-a-1 "c0323. This string does not contain sensitive information."
        break;
    case EAFNOSUPPORT:
        retval = "Address family not supported."; // parasoft-suppress CERT_C-MSC41-a-1 "c0324. This string does not contain sensitive information."
        break;
    case EALREADY:
        retval = "Operation already in progress."; // parasoft-suppress CERT_C-MSC41-a-1 "c0325. This string does not contain sensitive information."
        break;
    case ECANCELED:
        retval = "Operation has been canceled."; // parasoft-suppress CERT_C-MSC41-a-1 "c0326. This string does not contain sensitive information."
        break;
    case ECONNREFUSED:
        retval = "Connection refused."; // parasoft-suppress CERT_C-MSC41-a-1 "c0327. This string does not contain sensitive information."
        break;
    case ECONNRESET:
        retval = "Connection reset by peer."; // parasoft-suppress CERT_C-MSC41-a-1 "c0328. This string does not contain sensitive information."
        break;
    case EHOSTUNREACH:
        retval = "No route to host."; // parasoft-suppress CERT_C-MSC41-a-1 "c0329. This string does not contain sensitive information."
        break;
    case EINPROGRESS:
        retval = "Operation now in progress."; // parasoft-suppress CERT_C-MSC41-a-1 "c0330. This string does not contain sensitive information."
        break;
    case ENETUNREACH:
        retval = "Network is unreachable."; // parasoft-suppress CERT_C-MSC41-a-1 "c0331. This string does not contain sensitive information."
        break;
    case ENETDOWN:
        retval = "Network is down."; // parasoft-suppress CERT_C-MSC41-a-1 "c0332. This string does not contain sensitive information."
        break;
    case ENOBUFS:
        retval = "No buffer space available."; // parasoft-suppress CERT_C-MSC41-a-1 "c0333. This string does not contain sensitive information."
        break;
    case ENODATA:
        retval = "No data record of requested type."; // parasoft-suppress CERT_C-MSC41-a-1 "c0334. This string does not contain sensitive information."
        break;
    case ENOLINK:
        retval = "Link has been severed."; // parasoft-suppress CERT_C-MSC41-a-1 "c0335. This string does not contain sensitive information."
        break;
    case ENOTCONN:
        retval = "Socket is not connected."; // parasoft-suppress CERT_C-MSC41-a-1 "c0336. This string does not contain sensitive information."
        break;
    case EOVERFLOW:
        retval = "Value too large to be stored in data type."; // parasoft-suppress CERT_C-MSC41-a-1 "c0337. This string does not contain sensitive information."
        break;
    case ETIMEDOUT:
        retval = "Operation timed out."; // parasoft-suppress CERT_C-MSC41-a-1 "c0338. This string does not contain sensitive information."
        break;
    case ETIME:
        retval = "Invalid Time-Source"; // parasoft-suppress CERT_C-MSC41-a-1 "c0339. This string does not contain sensitive information."
        break;
    default:
        retval = http_status_code_desc(status);
        if (NULL != retval) {
            // Got an error description
        } else {
            static byte_t *target = &last_unkown_error[21];
            static size_t target_len = ((size_t)sizeof(last_unkown_error)) - 22UL;
            target[0] = '\0'; // initialize data
            (void)strerror_r((int32_t)status, target, target_len);
            if (utf8_starts_with(target, (int32_t) target_len, "Unknown error", -1)) { // parasoft-suppress CERT_C-MSC41-a-1 "c0340. This string does not contain sensitive information."
                // TODO Windows Socket Error from WSAGetLastError()
                //  https://msdn.microsoft.com/en-us/library/ms741580(VS.85).aspx
                (void) utf8_write_int(target, target_len, status, DECIMAL_RADIX);
                retval = last_unkown_error;
            } else{
                retval = &target[0];
            }
        }
        break;
    }
    return retval;
}

void abq_status_set(err_t status_code, bool_t overwrite) {
    if ((overwrite) || (abq_status_is_ok())) {
        abq_status = abq_errno_check(status_code);
        if (status_code_is_error(abq_status)) {
            is_status_ok = false;
            if (ECANCELED == abq_status) {
                // No action
            } else if ((ENODATA == abq_status) || ( EOVERFLOW == abq_status)){
                // No action
            } else if ((NIO_CLOSED == abq_status) || (NIO_HANGUP == abq_status)){
                // ABQ_TRACE_MSG("pipe")
            } else if (CHECK_WOULDBLOCK(abq_status)) {
                // No action
            } else {
                debugpoint();
                ABQ_WARN_MSG(abq_error_desc(abq_status));
            }
        } else {
            is_status_ok = true;
            if((EXIT_SUCCESS == status_code) && (overwrite)) {
                is_aborting = false;
            }
        }
    }
}

err_t abq_status_peek( void ) {
    err_t retval = abq_status;
    if (EUNSPECIFIED > retval) {
        retval *= -1;
    }
    return retval;
}

err_t abq_status_pop( void ) {
    err_t retval = abq_status_peek( );
    is_status_ok = true;
    abq_status = EXIT_SUCCESS;
    return retval;
}

err_t abq_status_take(err_t if_ok) {
    err_t retval = abq_status_pop( );
    if((EXIT_SUCCESS == retval) || (EALREADY == retval) || (HTTP_OK == retval)) {
        retval = abq_take_errno(if_ok);
        if((EXIT_SUCCESS == retval)  || (EALREADY == retval) || (HTTP_OK == retval)) {
            retval = if_ok;
        }
    }
    return retval;
}

static void abq_encode_status(abq_encoder_t *encoder, err_t status) {
    (void) abq_encode_loginfo(encoder, "! ", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0341. This string does not contain sensitive information."
    (void) abq_encode_number(encoder, (number_t) status);
    (void) abq_encode_loginfo(encoder, " \"", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0342. This string does not contain sensitive information."
    if (encoder->max > encoder->pos) {
        cstr_t desc = abq_error_desc(status);
        // the value returned by strerror can just be located on the stack
        //  adding function calls or fields onto the stack can overwrite it
        //  work with existing data to copy 'desc' onto 'dest'
        while (encoder->max > encoder->pos) {
            encoder->dest[encoder->pos] = desc[0];
            // Check for sting terminator
            if ('\0' == desc[0]) {
                break;
            }
            // else update positions for next char
            encoder->pos += 1UL;
            desc = &desc[1];
        }
    }
    (void) abq_encode_loginfo(encoder, " \"", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0343. This string does not contain sensitive information."
}

err_t abq_set_fatal_handler(abq_panic_func_t on_fatal) {
    err_t retval = EXIT_SUCCESS;
    if(NULL == on_fatal) {
#if !defined(NDEBUG)
        if (abq_invoke_assert != ontrac_fatality) {
            ontrac_fatality = abq_invoke_assert;
#else /* NDEBUG */
        if (abq_loop_forever != ontrac_fatality) {
            ontrac_fatality = abq_loop_forever;
#endif /* NDEBUG */
        } else {
            retval = EFAULT;
        }
    } else if(on_fatal == abq_loop_forever) {
        retval = EALREADY;
    } else {
#if !defined(NDEBUG)
        if (abq_invoke_assert != ontrac_fatality) {
#else /* NDEBUG */
        if (abq_loop_forever != ontrac_fatality) {
#endif /* NDEBUG */
            // Report that overwrite occured
            retval = ENOTEMPTY;
        }
        ontrac_fatality = on_fatal;
    }
    return retval;
}

void abq_fatal(cstr_t tag, int32_t line, err_t status, cstr_t msg) {
    // loglock() can cause recursive callbacks
    abq_log_s_msg(ABQ_FATAL_LEVEL, tag, line, status, msg);
#ifndef HAVE_SLOG_H
    abq_encode_msg_info(&abqlogenc, ABQ_FATAL_LEVEL, tag, line);
#else /* HAVE_SLOG_H */
    abqlogenc.pos = SLOG_HEADER_SPACE;
#endif /* HAVE_SLOG_H */
    abq_encode_status(&abqlogenc, abq_status_pop());
#ifndef HAVE_SLOG_H
    log_msg_handler_t callback = ontrac_logger;
    if (NULL != callback) {
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(ABQ_FATAL_LEVEL, abq_log_msg_buf, abqlogenc.pos);
        callback(ABQ_FATAL_LEVEL, NULL, 0); // Flush log
    }
#else /* HAVE_SLOG_H */
    aqSlog(0, SLOG_LEVEL_FATAL, tag, line, abq_log_msg_body);
    aqSlog(0, SLOG_LEVEL_FATAL, tag, line, NULL); // Flush log
#endif /* HAVE_SLOG_H */
    is_aborting = true;
    debugpoint();
    ontrac_fatality(tag, line, status, msg);
    // unloglock() unreachable + above loglock is disabled
}

void abq_msg_fatal(cstr_t tag, int32_t line, cstr_t msg) {
    err_t status = abq_status_peek();
    abq_fatal(tag, line, status, msg);
}

void abq_msg_fatal_strcmp(cstr_t tag, int32_t line,
        cstr_t left, cstr_t cmptype, cstr_t right) {
    byte_t msg[B1024_SIZE];
    ABQ_ENCODER(encoder, &ascii_codec, msg, sizeof(msg));
    //abq_encode_msg_info(&encoder, ABQ_FATAL_LEVEL, tag, line)
    (void) abq_encode_loginfo(&encoder, left, -1);
    (void) abq_encode_char(&encoder, ' ');
    (void) abq_encode_loginfo(&encoder, cmptype, -1);
    (void) abq_encode_char(&encoder, ' ');
    (void) abq_encode_loginfo(&encoder, right, -1);
    abq_fatal(tag, line, abq_status_peek(), msg);
}

void abq_msg_fatal_numbcmp(cstr_t tag, int32_t line,
        number_t left, cstr_t cmptype, number_t right) {
    byte_t msg[B1024_SIZE];
    ABQ_ENCODER(encoder, &ascii_codec, msg, sizeof(msg));
    //abq_encode_msg_info(&encoder, ABQ_FATAL_LEVEL, tag, line)
    (void) abq_encode_number(&encoder, left);
    (void) abq_encode_char(&encoder, ' ');
    (void) abq_encode_loginfo(&encoder, cmptype, -1);
    (void) abq_encode_char(&encoder, ' ');
    (void) abq_encode_number(&encoder, right);
    abq_fatal(tag, line, abq_status_peek(), msg);
}

void abq_log_msg(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg) {
#ifndef HAVE_SLOG_H
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        loglock();
        abq_encode_msg_info(&abqlogenc, level, tag, line);
        // Leave room for single newline char & possible 3 char ellipsis
        abqlogenc.max = sizeof(abq_log_msg_buf) - 4UL;
        err_t status = abq_encode_loginfo(&abqlogenc, msg, -1);
        // Reset max so that we can finish out w/ newline and potential ellipsis
        abqlogenc.max = sizeof(abq_log_msg_buf);
        if(EOVERFLOW == status) {
            // Add ellipsis on overflow
            (void) abq_encode_loginfo(&abqlogenc, "...", 3); // parasoft-suppress CERT_C-MSC41-a-1 "c0344. This string does not contain sensitive information."
        } else {
            // VITAL_IS_OK(status)
        }
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0UL;
        unloglock();
    }
#else /* HAVE_SLOG_H */
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if (SlogLevel_isActive(slog_level)) {
        aqSlog(0, slog_level, tag, line, msg);
    }
#endif /* HAVE_SLOG_H */
}

void abq_log_s_msg(abq_log_level_t level,
        cstr_t tag, int32_t line, err_t status, cstr_t msg) {
   if (ABQ_ERROR_LEVEL == level) {
        debugpoint();// breakpoint
   }
#ifdef HAVE_SLOG_H
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if(SlogLevel_isActive(slog_level)) {
        abqlogenc.pos = SLOG_HEADER_SPACE;
#else /* !HAVE_SLOG_H */
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        if((NULL != log_mutex) && (NULL != log_mutex->lock)) {
            (void) log_mutex->lock(log_mutex->mutex);
        }
        abq_encode_msg_info(&abqlogenc, level, tag, line);
#endif /* !HAVE_SLOG_H */
        (void) abq_encode_status(&abqlogenc, status);
        (void) abq_encode_loginfo(&abqlogenc, " :: ", -1); // parasoft-suppress CERT_C-MSC41-a-1 "c0345. This string does not contain sensitive information."
        (void) abq_encode_loginfo(&abqlogenc, msg, -1);
#ifndef HAVE_SLOG_H
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0U;
        if((NULL != log_mutex) && (NULL != log_mutex->unlock)) {
            (void) log_mutex->unlock(log_mutex->mutex);
        }
#else /* HAVE_SLOG_H */
        aqSlog(0, slog_level, tag, line, abq_log_msg_body);
#endif /* HAVE_SLOG_H */
    }
}

void abq_log_msg_h(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, uint64_t hex) {
#ifdef HAVE_SLOG_H
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if(SlogLevel_isActive(slog_level)) {
        abqlogenc.pos = SLOG_HEADER_SPACE;
#else /* !HAVE_SLOG_H */
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        loglock();
        abq_encode_msg_info(&abqlogenc, level, tag, line);
#endif /* !HAVE_SLOG_H */
        (void) abq_encode_loginfo(&abqlogenc, msg, -1);
        (void) abq_encode_loginfo(&abqlogenc, " 0x", 3); // parasoft-suppress CERT_C-MSC41-a-1 "c0346. This string does not contain sensitive information."
        (void) abq_encode_left_padded_uint(&abqlogenc,
                hex, HEX_RADIX, '0', 8);
#ifndef HAVE_SLOG_H
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0U;
        unloglock();
#else /* HAVE_SLOG_H */
        aqSlog(0, slog_level, tag, line, abq_log_msg_body);
#endif /* HAVE_SLOG_H */
    }
}

void abq_log_msg_x(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, number_t x) {
#ifdef HAVE_SLOG_H
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if(SlogLevel_isActive(slog_level)) {
        abqlogenc.pos = SLOG_HEADER_SPACE;
#else /* !HAVE_SLOG_H */
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        loglock();
        abq_encode_msg_info(&abqlogenc, level, tag, line);
#endif /* !HAVE_SLOG_H */
        (void) abq_encode_loginfo(&abqlogenc, msg, -1);
        (void) abq_encode_char(&abqlogenc, ' ');
        (void) abq_encode_number(&abqlogenc,  x);
#ifndef HAVE_SLOG_H
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0U;
        unloglock();
#else /* HAVE_SLOG_H */
        aqSlog(0, slog_level, tag, line, abq_log_msg_body);
#endif /* HAVE_SLOG_H */
    }
}

void abq_log_msg_y(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, const byte_t *y, int32_t n) {
#ifdef HAVE_SLOG_H
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if(SlogLevel_isActive(slog_level)) {
        abqlogenc.pos = SLOG_HEADER_SPACE;
#else /* !HAVE_SLOG_H */
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        loglock();
        abq_encode_msg_info(&abqlogenc, level, tag, line);
#endif /* !HAVE_SLOG_H */
        (void) abq_encode_loginfo(&abqlogenc, msg, -1);
        (void) abq_encode_char(&abqlogenc, ' ');
        (void) abq_encode_loginfo(&abqlogenc, y, n);
#ifndef HAVE_SLOG_H
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0U;
        unloglock();
#else /* HAVE_SLOG_H */
        aqSlog(0, slog_level, tag, line, abq_log_msg_body);
#endif /* HAVE_SLOG_H */
    }
}

void abq_log_msg_z(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, cstr_t z) {
#ifdef HAVE_SLOG_H
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if(SlogLevel_isActive(slog_level)) {
        abqlogenc.pos = SLOG_HEADER_SPACE;
#else /* !HAVE_SLOG_H */
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        loglock();
        abq_encode_msg_info(&abqlogenc, level, tag, line);
#endif /* !HAVE_SLOG_H */
        (void) abq_encode_loginfo(&abqlogenc, msg, -1);
        (void) abq_encode_char(&abqlogenc, ' ');
        (void) abq_encode_loginfo(&abqlogenc, z, -1);
#ifndef HAVE_SLOG_H
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0U;
        unloglock();
#else /* HAVE_SLOG_H */
        aqSlog(0, slog_level, tag, line, abq_log_msg_body);
#endif /* HAVE_SLOG_H */
    }
}

void abq_log_msg_px(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, cvar_t p, number_t x) {
#ifdef HAVE_SLOG_H
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if(SlogLevel_isActive(slog_level)) {
        abqlogenc.pos = SLOG_HEADER_SPACE;
#else /* !HAVE_SLOG_H */
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        loglock();
        abq_encode_msg_info(&abqlogenc, level, tag, line);
#endif /* !HAVE_SLOG_H */
        (void) abq_encode_loginfo(&abqlogenc, msg, -1);
        (void) abq_encode_ptraddr(&abqlogenc, p);
        (void) abq_encode_char(&abqlogenc, ' ');
        (void) abq_encode_number(&abqlogenc,  x);
#ifndef HAVE_SLOG_H
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0U;
        unloglock();
#else /* HAVE_SLOG_H */
        aqSlog(0, slog_level, tag, line, abq_log_msg_body);
#endif /* HAVE_SLOG_H */
    }
}

void abq_log_xzx(abq_log_level_t level,
        cstr_t tag, int32_t line,
        number_t x1, cstr_t z, number_t x2) {
#ifdef HAVE_SLOG_H
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if(SlogLevel_isActive(slog_level)) {
        abqlogenc.pos = SLOG_HEADER_SPACE;
#else /* !HAVE_SLOG_H */
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        loglock();
        abq_encode_msg_info(&abqlogenc, level, tag, line);
#endif /* !HAVE_SLOG_H */
        (void) abq_encode_number(&abqlogenc,  x1);
        (void) abq_encode_char(&abqlogenc, ' ');
        (void) abq_encode_loginfo(&abqlogenc, z, -1);
        (void) abq_encode_char(&abqlogenc, ' ');
        (void) abq_encode_number(&abqlogenc,  x2);
#ifndef HAVE_SLOG_H
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0U;
        unloglock();
#else /* HAVE_SLOG_H */
        aqSlog(0, slog_level, tag, line, abq_log_msg_body);
#endif /* HAVE_SLOG_H */
    }
}

void abq_log_zzz(abq_log_level_t level,
        cstr_t tag, int32_t line,
        cstr_t z1, cstr_t z2, cstr_t z3) {
#ifdef HAVE_SLOG_H
    SlogLevel_t slog_level = abq_slog_level_of(level);
    if(SlogLevel_isActive(slog_level)) {
        abqlogenc.pos = SLOG_HEADER_SPACE;
#else /* !HAVE_SLOG_H */
    log_msg_handler_t callback = ontrac_logger;
    if ((NULL != callback) && (abq_log_level_is_active(level))) {
        loglock();
        abq_encode_msg_info(&abqlogenc, level, tag, line);
#endif /* !HAVE_SLOG_H */
        (void) abq_encode_loginfo(&abqlogenc, z1, -1);
        (void) abq_encode_char(&abqlogenc, ' ');
        (void) abq_encode_loginfo(&abqlogenc, z2, -1);
        (void) abq_encode_char(&abqlogenc, ' ');
        (void) abq_encode_loginfo(&abqlogenc, z3, -1);
#ifndef HAVE_SLOG_H
        (void) abq_encode_char(&abqlogenc, '\n');
        callback(level, abq_log_msg_buf, abqlogenc.pos);
        abqlogenc.pos = 0U;
        unloglock();
#else /* HAVE_SLOG_H */
        aqSlog(0, slog_level, tag, line, abq_log_msg_body);
#endif /* HAVE_SLOG_H */
    }
}

