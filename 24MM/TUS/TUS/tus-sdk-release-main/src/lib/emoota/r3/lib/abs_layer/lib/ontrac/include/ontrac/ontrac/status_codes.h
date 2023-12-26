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
 * @file ontrac/status_codes.h
 * @date Apr 9, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief A collection of HTTP status codes stored in an enumeration, and helper functions for loading description and status codes by error
 *
 */

#ifndef SPIL_NIO_HTTP_STATUS_CODES_H_
#define SPIL_NIO_HTTP_STATUS_CODES_H_

#include <assert.h>

#include "platform/platformconfig.h"
#include <ontrac/ontrac/ontrac_config.h>
#include <ontrac/unicode/utf8_utils.h>

typedef enum {
    ABQ_UNKNOWN_LEVEL=0,
    /** @brief very verbose log messages which are likely to produce an annoying amount of log data.  However, sometimes
     *  useful in trying to diagnose certain problems.
     */
    ABQ_TRACE_LEVEL=1,
    /** @brief designates fine-grained informational events that are most useful to debug an application
     * (what is going on)
     */
    ABQ_DEBUG_LEVEL=2,
    /** @brief  announcements about the normal operation of the system - scheduled jobs running, services starting
     *         and stopping, user-triggered processes and actions
     */
    ABQ_INFO_LEVEL=3,
    /** @brief any condition that, while not an error in itself, may indicate that the system is running sub-optimall */
    ABQ_WARN_LEVEL=4,
    /** @brief a condition that indicates something has gone wrong with the system */
    ABQ_ERROR_LEVEL=5,
    /** @brief a condition that indicates something has gone wrong so badly that the system can not recover */
    ABQ_FATAL_LEVEL=6,
} abq_log_level_t;

/**
 * Log message callback typedef
 * @param level The severity level for the message to be logged
 * @param log_msg Pointer to the log message
 * @param msg_len Number of characters at the pointer location to be logged (or log message length)
 */
typedef void (*log_msg_handler_t) (abq_log_level_t level, const char* log_msg, const size_t msg_len);

/**
 * Sets internal pointer to callback function and configures default log level to be used
 * @param log_out Pointer to the log message callback
 * @param level Set active log level. All message with less or equal level will be logged
 * @return EXIT_SUCCESS if successful registered new handle, EALREADY of the handler is already registered
 */
extern err_t abq_set_log_msg_handler(log_msg_handler_t log_out, abq_log_level_t level);

typedef size_t (*abq_log_timestamp_cb) (char *timeStr, uint8_t timeStrLen);
extern err_t abq_log_set_timestamp_cb(abq_log_timestamp_cb get_timestamp);
/**
 * Query if log_level is less or equal to configured log level
 * @param log_level
 * @return true if log_level less or equal to current set log level, false otherwise
 */
extern bool_t abq_log_level_is_active(abq_log_level_t log_level);

#ifdef HAVE_SLOG_H
#include <commons/utils/slog.h>
#endif /*HAVE_SLOG_H */
#include <errno.h>

typedef err_t http_status_code_t;

/**< HTTP_STATUS_NONE*/
#define HTTP_STATUS_NONE                (000)
/* RFC2616 Section 10.1 - Informational 1xx */
/**< CONTINUE, RFC2616 Section 10.1.1*/
#define HTTP_CONTINUE                   (100)
/**< SWITCHING_PROTOCOLS, RFC2616 Section 10.1.2 */
#define SWITCHING_PROTOCOLS             (101)
/**< HTTP_PROCESSING, RFC2518 Section 10.1 */
#define HTTP_PROCESSING                 (102)
    /* RFC2616 Section 10.2 - Successful 2xx */
/**< HTTP_OK, RFC2616 Section 10.2.1 */
#define HTTP_OK                         (200)
/**< HTTP_CREATED, RFC2616 Section 10.2.2  */
#define HTTP_CREATED                    (201)
/**< HTTP_ACCEPTED, RFC2616 Section 10.2.3  */
#define HTTP_ACCEPTED                   (202)
/**< NON_AUTHORITATIVE_INFORMATION, RFC2616 Section 10.2.4 */
#define NON_AUTHORITATIVE_INFORMATION   (203)
/**< NO_CONTENT, RFC2616 Section 10.2.5 */
#define NO_CONTENT                      (204)
/**< RESET_CONTENT, RFC2616 Section 10.2.6 */
#define RESET_CONTENT                   (205)
/**< PARTIAL_CONTENT, RFC2616 Section 10.2.7 */
#define PARTIAL_CONTENT                 (206)
/**< MULTI_STATUS, RFC2518 Section 10.2, RFC4918 Section 11.1 */
#define MULTI_STATUS                    (207)
/**< ALREADY_REPORTED, RFC5842 Section 7.1 */
#define ALREADY_REPORTED                (208)
/**< HTTP_IM_USED, RFC3229 Section 10.4.1 */
#define HTTP_IM_USED                    (226)
    /* RFC2616 Section 10.3 - Redirection 3xx */
/**< MULTIPLE_CHOICES, RFC2616 Section 10.3.1 */
#define MULTIPLE_CHOICES                (300)
/**< MOVED_PERMANENTLY, RFC2616 Section 10.3.2 */
#define MOVED_PERMANENTLY               (301)
/**< MOVED_TEMPORARILY or FOUND, RFC2616 Section 10.3.3*/
#define MOVED_TEMPORARILY               (302)
/**< HTTP_SEE_OTHER, RFC2616 Section 10.3.4 */
#define HTTP_SEE_OTHER                  (303)
/**< NOT_MODIFIED, RFC2616 Section 10.3.5 */
#define NOT_MODIFIED                    (304)
/**< USE_PROXY, RFC2616 Section 10.3.6 */
#define USE_PROXY                       (305)
/**< HTTP_306 */
#define HTTP_306                        (306)
/**< TEMPORARY_REDIRECT, RFC2616 Section 10.3.8*/
#define TEMPORARY_REDIRECT              (307)
/**< PERMANENT_REDIRECT, RFC7238 Section 3*/
#define PERMANENT_REDIRECT              (308)
/** Airbiquity defined status code */
#define ROLLBACK_COMPLETE               (321)
    /* RFC2616 Section 10.4 - Client Error 4xx */
/**< BAD_REQUEST, RFC2616 Section 10.4.1 */
#define BAD_REQUEST                     (400)
/**< UNAUTHORIZED, RFC2616 Section 10.4.2 */
#define UNAUTHORIZED                    (401)
/**< PAYMENT_REQUIRED, RFC2616 Section 10.4.3 */
#define PAYMENT_REQUIRED                (402)
/**< FORBIDDEN, RFC2616 Section 10.4.4 */
#define FORBIDDEN                       (403)
/**< NOT_FOUND, RFC2616 Section 10.4.5 */
#define NOT_FOUND                       (404)
/**< METHOD_NOT_ALLOWED, RFC2616 Section 10.4.6 */
#define METHOD_NOT_ALLOWED              (405)
/**< NOT_ACCEPTABLE, RFC2616 Section 10.4.7 */
#define NOT_ACCEPTABLE                  (406)
/**< PROXY_AUTHENTICATION_REQUIRED, RFC2616 Section 10.4.8 */
#define PROXY_AUTHENTICATION_REQUIRED   (407)
/**< REQUEST_TIMEOUT, RFC2616 Section 10.4.9 */
#define REQUEST_TIMEOUT                 (408)
/**< HTTP_CONFLICT, RFC2616 Section 10.4.10 */
#define HTTP_CONFLICT                   (409)
/**< HTTP_GONE, RFC2616 Section 10.4.11 */
#define HTTP_GONE                       (410)
/**< LENGTH_REQUIRED, RFC2616 Section 10.4.12 */
#define LENGTH_REQUIRED                 (411)
/**< PRECONDITION_FAILED, RFC2616 Section 10.4.13 */
#define PRECONDITION_FAILED             (412)
/**< REQUEST_ENTITY_TOO_LARGE, RFC2616 Section 10.4.14 */
#define REQUEST_ENTITY_TOO_LARGE        (413)
/**< REQUEST_URI_TOO_LONG, RFC2616 Section 10.4.15 */
#define REQUEST_URI_TOO_LONG            (414)
/**< UNSUPPORTED_MEDIA_TYPE, RFC2616 Section 10.4.16 */
#define UNSUPPORTED_MEDIA_TYPE          (415)
/**< REQUEST_RANGE_NOT_SATISFIABLE, RFC2616 Section 10.4.17 */
#define REQUEST_RANGE_NOT_SATISFIABLE   (416)
/**< EXPECTATION_FAILED, RFC2616 Section 10.4.18 */
#define EXPECTATION_FAILED              (417)
/**< MISDIRECTED_REQUEST, RFC7540 Section 9.1.2 */
#define MISDIRECTED_REQUEST             (421)
/**< UNPROCCESSABLE_ENTITY, RFC2518 Section 10.3, RFC4918 Section 11.2*/
#define UNPROCCESSABLE_ENTITY           (422)
/**< HTTP_LOCKED, RFC2518 Section 10.4, RFC4918 Section 11.3 */
#define HTTP_LOCKED                     (423)
/**< FAILED_DEPENDENCY, RFC2518 Section 10.5, RFC4918 Section 11.4 */
#define FAILED_DEPENDENCY               (424)
/**< UPGRADE_REQUIRED, RFC 2817 Section 44 */
#define UPGRADE_REQUIRED                (426)
/**< PRECONDITION_REQUIRED, RFC 6585, Section 3 */
#define PRECONDITION_REQUIRED           (428)
/**< TOO_MANY_REQUESTS, RFC 6585, Section 4 */
#define TOO_MANY_REQUESTS               (429)
/**< REQUEST_HEADER_FIELD_TOO_LARGE, RFC 6585, Section 5 */
#define REQUEST_HEADER_FIELD_TOO_LARGE  (431)
/**< UNAVAILABLE_FOR_LEGAL_REASONS, draft-tbray-http-legally-restricted-status-05, Section 3*/
#define UNAVAILABLE_FOR_LEGAL_REASONS   (451)
    /* RFC2616 Section 10.5 - Server Error 5xx */
/**< INTERNAL_SERVER_ERROR, RFC2616 Section 10.5.1 */
#define INTERNAL_SERVER_ERROR           (500)
/**< NOT_IMPLEMENTED, RFC2616 Section 10.5.2 */
#define NOT_IMPLEMENTED                 (501)
/**< BAD_GATEWAY, RFC2616 Section 10.5. */
#define BAD_GATEWAY                     (502)
/**< SERVICE_UNAVAILABLE, RFC2616 Section 10.5.4 */
#define SERVICE_UNAVAILABLE             (503)
/**< GATEWAY_TIMEOUT, RFC2616 Section 10.5.5 */
#define GATEWAY_TIMEOUT                 (504)
/**< HTTP_VERSION_NOT_SUPPORTED, RFC2616 Section 10.5.6 */
#define HTTP_VERSION_NOT_SUPPORTED      (505)
/**< VARIANT_ALSO_NEGOTIATES, RFC 2295, Section 8.1 */
#define VARIANT_ALSO_NEGOTIATES         (506)
/**< INSUFFICIENT_STORAGE, RFC2518 Section 10.6, RFC4918 Section 11.5 */
#define INSUFFICIENT_STORAGE            (507)
/**< LOOP_DETECTED, RFC5842 Section 7.1 */
#define LOOP_DETECTED                   (508)
/**< NOT_EXTENDED, RFC 2774, Section 7 */
#define NOT_EXTENDED                    (510)
/**< NETWORK_AUTH_REQUIRED, RFC 6585, Section 6 */
#define NETWORK_AUTH_REQUIRED           (511)
/* Airbiquity status codes from the python application */
#define UNINITIALIZED                   (600)
#define WRONG_CONTENT_LENGTH            (601)
// TAMPERED_FILE (602)
// TOO_MUCH_DATA (603)
#define DIGEST_MISMATCH                 (604)
#define INTERRUPTED                     (605)
#define SIGNATURE_FAILURE               (606)
#define USER_CANCELED                   (607)

#define UNRESOLVED_HOST                 (610)
#define NO_ROUTE_TO_HOST                (611)
/** For sockets, often indicates a connection timeout (Failed to received TCP handshake SYN-ACK message) */
#define NIO_TX_TIMEOUT                  (612)
#define NIO_RX_TIMEOUT                  (613)
#define NIO_HANGUP                      (614)
#define NIO_ERROR                       (615)
#define NIO_CLOSED                      (616)

#define EUNSPECIFIED (-1)

#ifndef ECONNREFUSED
#define ECONNREFUSED (107)
#endif
#ifndef ECONNRESET
#define ECONNRESET (108)
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH 110
#endif
#ifndef ENETDOWN
#define ENETDOWN 116
#endif
#ifndef ENETUNREACH
#define ENETUNREACH 118
#endif

/**
 * @brief lookup the description based on HTTP status code, return NULL if not a recognized HTTP status code
 *
 * @param code: an http_status_code_t listed in the enum
 * @return A description of the provided HTTP status code, or NULL if not recognized
 */
extern cstr_t http_status_code_desc(err_t code);

#if EWOULDBLOCK == EAGAIN
#define CHECK_BLOCK_STATUS(status) ((EWOULDBLOCK == (status)) || (EINPROGRESS == (status)))
#define CHECK_NONBLOCK_STATUS(status) ((EWOULDBLOCK != (status)) && (EINPROGRESS != (status)))
#else /* EWOULDBLOCK != EAGAIN */
#define CHECK_BLOCK_STATUS(status) ((EWOULDBLOCK == (status)) || (EINPROGRESS == (status)) || (EAGAIN == (status)))
#define CHECK_NONBLOCK_STATUS(status) ((EWOULDBLOCK != (status)) && (EINPROGRESS != (status)) && (EAGAIN != (status)))
#endif /* EWOULDBLOCK != EAGAIN */

static inline bool_t CHECK_RETRY_AFTER(http_status_code_t code) {
    return (bool_t) ((INTERNAL_SERVER_ERROR == code)
            || (SERVICE_UNAVAILABLE == code) || (TOO_MANY_REQUESTS == code));
}

static inline bool_t CHECK_WOULDBLOCK(err_t status) {
    return CHECK_BLOCK_STATUS(status);
}

static inline bool_t CHECK_TIMEOUT(err_t status) {
    return (bool_t) ((ETIMEDOUT == status)
            || (REQUEST_TIMEOUT == status) || (GATEWAY_TIMEOUT == status)
            || (NIO_TX_TIMEOUT == status) || (NIO_RX_TIMEOUT == status));
}

#define CHECK_ERROR_STATUS(code) ((EXIT_SUCCESS != (code))      \
                                && (EALREADY != (code))         \
                                && (CHECK_NONBLOCK_STATUS(code))\
                                && (((code) < HTTP_OK) || (BAD_REQUEST <= (code))))
#define CHECK_NONERROR_STATUS(code) ((EXIT_SUCCESS == (code))   \
                                || (EALREADY == (code))         \
                                || (CHECK_BLOCK_STATUS(code))   \
                                || ((HTTP_OK <= (code)) && ((code) < BAD_REQUEST)))
/**
 * @brief checks if the status code is an error code (either (0 < code < 200) or (400 <= code)
 * @notice: 100's are continuation codes in HTTP, but errno.h uses 100's for system level errors
 *     so if it not one of the common 100's we don't count it
 *     however being 100 level codes, they conflict with system level errors
 *
 * @param code: an http status code to check
 * @return true if an error code is present, false for warnings and less sever status codes
 */
static inline bool_t status_code_is_error(err_t code) {
    return CHECK_ERROR_STATUS(code);
}
static inline bool_t status_code_is_ok(err_t code) {
    return CHECK_NONERROR_STATUS(code);
}

/**
 * @brief attempts to map an error code to a http_status_code_t
 *
 * @param err: an error code to be mapped to a http_status_code_t
 * @return the http_status_code_t mapped to the particular error, defaults to INTERNAL_SERVER_ERROR if none are found
 */
extern http_status_code_t http_status_code_for_err(err_t err);

/**
 * @brief lookup the description of a given status / error code
 *  - First check HTTP status codes via 'http_status_code_desc'
 *  - Then it checks the system specific status codes via 'strerror'
 *  - Next is checks airbiquity specific error codes found in platformconfig.h
 *  - Finally it prints "Unknown status-code: {numeric_value} if a desc wasn't fount else-where
 *
 * @param status_code: a integer value relating to some error
 * @return a description of the given status code
 */
extern cstr_t abq_error_desc(err_t status_code);
/**
 * @brief sets the abq_global_status to the given status code
 *
 * @param status_code: the value to set abq_status to
 * @param overwrite: if false will not overwrite previous error status, else sets abq_status without checking existing status
 * @return false if abq_status is set to an error as defined by status_code_is_error, true otherwise
 */
extern void abq_status_set(err_t status_code, bool_t overwrite);
/**
 * @brief gets the current status code but does not clear it
 * @return current value of abq_status
 */
extern err_t abq_status_peek( void );
/**
 * @brief gets the current status code and clears it
 * @return current value of abq_status
 */
extern err_t abq_status_pop( void );
/**
 * @brief gets the current status code and optionally clears it
 * @deprecated: should use abq_status_peek or abq_status_pop instead
 *
 * @param clear_status: true if status should be cleared for future use
 * @return the existing value of abq_status, or errno if abq_status was EXIT_SUCCESS
 */
static inline err_t abq_status_get(bool_t clear_status) {
    return (clear_status) ? abq_status_pop() : abq_status_peek();
}
/**
 * @brief loads the abq_status code and clears it, returning the status if an error was set, or the is_ok parameter if it wasn't
 *
 * @param if_ok: a default status code to use if no error was set
 * @return abq_status code if not ok, else if_ok_value
 */
extern err_t abq_status_take(err_t if_ok);

extern bool_t abq_status_is_ok( void );

#define CHECK_NULL(item) ((NULL == item) ? EFAULT : EXIT_SUCCESS)

extern bool_t abq_is_aborting(void);

typedef void (*abq_panic_func_t)(cstr_t tag, int32_t line, err_t status, cstr_t msg);
extern err_t abq_set_fatal_handler(abq_panic_func_t on_fatal);

extern void abq_fatal(cstr_t tag, int32_t line, err_t status, cstr_t msg);
// Will halt the program with an assert or equivalent
extern void abq_msg_fatal(cstr_t tag, int32_t line, cstr_t msg);
void abq_msg_fatal_numbcmp(cstr_t tag, int32_t line,
        number_t left, cstr_t cmptype, number_t right);
extern void abq_msg_fatal_strcmp(cstr_t tag, int32_t line,
        cstr_t left, cstr_t cmptype, cstr_t right);

extern void abq_log_msg(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg);
extern void abq_log_s_msg(abq_log_level_t level,
        cstr_t tag, int32_t line, err_t status, cstr_t msg);
extern void abq_log_msg_h(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, uint64_t hex);
extern void abq_log_msg_x(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, number_t x);
extern void abq_log_msg_y(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, const byte_t *y, int32_t n);
extern void abq_log_msg_z(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, cstr_t z);
extern void abq_log_msg_px(abq_log_level_t level,
        cstr_t tag, int32_t line, cstr_t msg, cvar_t p, number_t x);
extern void abq_log_xzx(abq_log_level_t level,
        cstr_t tag, int32_t line,
        number_t x1, cstr_t z, number_t x2);
extern void abq_log_zzz(abq_log_level_t level,
        cstr_t tag, int32_t line,
        cstr_t z1, cstr_t z2, cstr_t z3);

#ifndef __FUNCTION_NAME__
    #if defined __func__
        // Undeclared
        #define __FUNCTION_NAME__   __func__
    #elif defined __FUNCTION__
        // Undeclared
        #define __FUNCTION_NAME__   __FUNCTION__
    #elif defined __PRETTY_FUNCTION__
        // Undeclared
        #define __FUNCTION_NAME__   __PRETTY_FUNCTION__
    #else
        // Declared
        #define __FUNCTION_NAME__   ""
    #endif // __func__
#endif // __FUNCTION_NAME__

#if !defined(NDEBUG)
# ifndef __FILENAME__
#  define __FILENAME__ __FILE__
# endif /* __FILENAME__ */
#else /* NDEBUG */
# ifndef __FILENAME__
#  define __FILENAME__ (NULL)
# endif /* __FILENAME__ */
# endif /* NDEBUG */


static inline void abq_fatal_if_not(cstr_t tag, int32_t line, bool_t condition) {
    if (condition) {
    } else {
        abq_msg_fatal(tag, line, "");
    }
}
#define ABQ_VITAL(condition) do {                       \
    bool_t _boolval = (bool_t)(condition);              \
    abq_fatal_if_not( __FILENAME__, __LINE__, _boolval);\
    assert(_boolval);                                     \
} while(0)

static inline void abq_fatal_if(cstr_t tag, int32_t line, bool_t condition) {
    if (condition) {
        abq_msg_fatal(tag, line, "");
    }
}

#define FATAL_IF(condition) do {                        \
        bool_t _boolval = (bool_t)(condition);          \
        abq_fatal_if( __FILENAME__, __LINE__, _boolval);\
        assert(false ==_boolval);                       \
} while(0)


static inline void abq_fatal_if_err(cstr_t tag, int32_t line, err_t status) {
    if (status_code_is_error(status)) {
        abq_fatal(tag, line, status, "");
    }
}
#define VITAL_IS_OK(error_code)  abq_fatal_if_err( __FILENAME__, __LINE__, (err_t)(error_code))

static inline void abq_fatal_if_not_null(cstr_t tag, int32_t line, cvar_t item) {
    if (NULL != item) {
        abq_msg_fatal(tag, line, "");
    }
}
#define VITAL_IS_NULL(expr) abq_fatal_if_not_null( __FILENAME__, __LINE__, (cvar_t)(expr))

static inline void abq_fatal_if_null(cstr_t tag, int32_t line, cvar_t item) {
    if (NULL == item) {
        abq_msg_fatal(tag, line, "");
    }
}
#define VITAL_NOT_NULL(expr) do {                           \
    cvar_t _instance = (cvar_t)(expr);                      \
    abq_fatal_if_null( __FILENAME__, __LINE__,(_instance)); \
    assert(NULL != _instance);                              \
} while(0)

static inline void abq_fatal_if_not_numbeq(cstr_t tag, int32_t line, number_t left, number_t right) {
    if (left != right) {
        abq_msg_fatal_numbcmp(tag, line, left, "!=", right);
    }
}
#define VITAL_VALUE(value, expr) abq_fatal_if_not_numbeq( __FILENAME__, __LINE__, (number_t)(value), (number_t)(expr))

static inline void abq_fatal_if_numbeq(cstr_t tag, int32_t line, number_t left, number_t right) {
    if (left == right) {
        abq_msg_fatal_numbcmp(tag, line, left, "==", right);
    }
}
#define VITAL_NOT_VALUE(value, expr) abq_fatal_if_numbeq( __FILENAME__, __LINE__, (number_t)(value), (number_t)(expr))

static inline void abq_fatal_if_not_streq(cstr_t tag, int32_t line,
        cstr_t left, cstr_t right) {
    if(0 != utf8_compare_exact(left, right, -1)) {
        abq_msg_fatal_strcmp(tag, line, left, "!=", right);
    }
}
#define VITAL_EQUALS(left, right)  abq_fatal_if_not_streq( __FILENAME__, __LINE__, (cstr_t)(left), (cstr_t)(right))

static inline void abq_fatal_if_streq(cstr_t tag, int32_t line,
        cstr_t left, cstr_t right) {
    if(0 == utf8_compare_exact(left, right, -1)) {
        abq_msg_fatal_strcmp(tag, line, left, "==", right);
    }
}
#define VITAL_NOT_EQUALS(left, right)  abq_fatal_if_streq( __FILENAME__, __LINE__, (cstr_t)(left), (cstr_t)(right))

#if !defined(NDEBUG)

static inline void abq_expect_is(cstr_t tag, int32_t line, bool_t condition) {
    if (condition) {
    } else {
        abq_log_msg(ABQ_ERROR_LEVEL, tag, line, "");
    }
}
#define ABQ_EXPECT(condition) abq_expect_is( __FILENAME__, __LINE__, (bool_t)(condition))

static inline void abq_expect_is_not(cstr_t tag, int32_t line, bool_t condition) {
    if (condition) {
        abq_log_msg(ABQ_ERROR_LEVEL, tag, line, "");
    }
}
#define UNEXPECT(condition) abq_expect_is_not( __FILENAME__, __LINE__, (bool_t)(condition))

static inline void abq_expect_is_null(cstr_t tag, int32_t line, cvar_t item) {
    if (NULL != item) {
        abq_log_msg(ABQ_ERROR_LEVEL, tag, line, "");
    }
}

#define EXPECT_IS_NULL(expr) abq_expect_is_null( __FILENAME__, __LINE__, (cvar_t)(expr))

static inline void abq_expect_is_not_null(cstr_t tag, int32_t line, cvar_t item) {
    if (NULL == item) {
        abq_log_msg(ABQ_ERROR_LEVEL, tag, line, "");
    }
}

#define EXPECT_NOT_NULL(expr) abq_expect_is_not_null( __FILENAME__, __LINE__, (cvar_t)(expr))

static inline  void abq_expect_is_ok(cstr_t tag, int32_t line, err_t status) {
    if (status_code_is_error(status)) {
        abq_log_s_msg(ABQ_ERROR_LEVEL, tag, line, status, "");
    }
}
#define EXPECT_IS_OK(error_code)  abq_expect_is_ok( __FILENAME__, __LINE__, (err_t)(error_code))

static inline void abq_expect_is_numbeq(cstr_t tag, int32_t line, number_t left, number_t right) {
    if (left != right) {
        abq_log_xzx(ABQ_ERROR_LEVEL, tag, line, left, "!=", right);
    }
}
#define EXPECT_VALUE(value, expr) abq_expect_is_numbeq( __FILENAME__, __LINE__, (number_t)(value), (number_t)(expr))

static inline void abq_expect_is_not_numbeq(cstr_t tag, int32_t line, number_t left, number_t right) {
    if (left == right) {
        abq_log_xzx(ABQ_ERROR_LEVEL, tag, line, left, "==", right);
    }
}
#define EXPECT_NOT_VALUE(value, expr) abq_expect_is_not_numbeq( __FILENAME__, __LINE__, (number_t)(value), (number_t)(expr))

static inline void abq_expect_is_streq(cstr_t tag, int32_t line,
        cstr_t left, cstr_t right) {
    if (0 != utf8_compare_exact(left, right, -1)) {
        abq_log_zzz(ABQ_ERROR_LEVEL, tag, line, left, "!=", right);
    }
}
#define EXPECT_EQUALS(left, right)  abq_expect_is_streq( __FILENAME__, __LINE__, (cstr_t)(left), (cstr_t)(right))

static inline void abq_expect_is_not_streq(cstr_t tag, int32_t line,
        cstr_t left, cstr_t right) {
    if (0 == utf8_compare_exact(left, right, -1)) {
        abq_log_zzz(ABQ_ERROR_LEVEL, tag, line, left, "==", right);
    }
}
#define EXPECT_NOT_EQUALS(left, right)  abq_expect_is_not_streq( __FILENAME__, __LINE__, (cstr_t)(left), (cstr_t)(right))


#else /* NDEBUG */

#define ABQ_EXPECT(condition) (void)(condition)

#define UNEXPECT(condition) (void)(condition)

#define EXPECT_IS_NULL(expr) (void)(expr)

#define EXPECT_NOT_NULL(expr) (void)(expr)

#define EXPECT_IS_OK(error_code) (void)(error_code)

#define EXPECT_VALUE(value, expr) do { (void) (value) ; (void) (expr) ; } while (0)

#define EXPECT_EQUALS(left, right)  do { (void) (left) ; (void) (right) ; } while (0)

#define EXPECT_NOT_EQUALS(left, right) do { (void) (left) ; (void) (right) ; } while (0)

#endif /* !NDEBUG */

#define ABQ_FATAL_MSG(msg) abq_msg_fatal( __FILENAME__, __LINE__, msg);
#define ABQ_FATAL_STATUS(status) abq_fatal(__FILENAME__, __LINE__, status, "?");

#define ABQ_DUMP_ERROR(status, msg) abq_log_s_msg(ABQ_ERROR_LEVEL, __FILENAME__, __LINE__, (status),  (cstr_t)(msg))

#define ABQ_ERROR_MSG_X(msg, x) abq_log_msg_x(ABQ_ERROR_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (number_t)(x))
#define ABQ_ERROR_MSG_Y(msg, y, n) abq_log_msg_y(ABQ_ERROR_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cstr_t)(y), (int32_t)(n))
#define ABQ_ERROR_MSG_Z(msg, z) abq_log_msg_z(ABQ_ERROR_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cstr_t)(z))

#define ABQ_WARN_STATUS(status, msg) abq_log_s_msg(ABQ_WARN_LEVEL, __FILENAME__, __LINE__, (status),  (cstr_t)(msg))
#define ABQ_WARN_MSG_X(msg, x) abq_log_msg_x(ABQ_WARN_LEVEL, __FILENAME__, __LINE__,(cstr_t)(msg), (number_t)(x))
#define ABQ_WARN_MSG_Y(msg, y, n) abq_log_msg_y(ABQ_WARN_LEVEL, __FILENAME__, __LINE__,(cstr_t)(msg), (cstr_t)(y), (int32_t)(n))
#define ABQ_WARN_MSG_Z(msg, z) abq_log_msg_z(ABQ_WARN_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cstr_t)(z))

#define ABQ_INFO_STATUS(status, msg) abq_log_s_msg(ABQ_INFO_LEVEL, __FILENAME__, __LINE__, (status),  (cstr_t)(msg))
#define ABQ_INFO_MSG_H(msg, h) abq_log_msg_h(ABQ_INFO_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (uint64_t)(h))
#define ABQ_INFO_MSG_P(msg, p) abq_log_msg_h(ABQ_INFO_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), ptr2addr(p))
#define ABQ_INFO_MSG_X(msg, x) abq_log_msg_x(ABQ_INFO_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (number_t)(x))
#define ABQ_INFO_MSG_Y(msg, y, n) abq_log_msg_y(ABQ_INFO_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cstr_t)(y), (int32_t)(n))
#define ABQ_INFO_MSG_Z(msg, z) abq_log_msg_z(ABQ_INFO_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cstr_t)(z))

#define ABQ_DEBUG_STATUS(status, msg) abq_log_s_msg(ABQ_DEBUG_LEVEL, __FILENAME__, __LINE__, (status),  (cstr_t)(msg))
#define ABQ_DEBUG_MSG_X(msg, x) abq_log_msg_x(ABQ_DEBUG_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (number_t)(x))
#define ABQ_DEBUG_MSG_Y(msg, y, n) abq_log_msg_y(ABQ_DEBUG_LEVEL, __FILENAME__, __LINE__,(cstr_t)(msg), (cstr_t)(y), (int32_t)(n))
#define ABQ_DEBUG_MSG_Z(msg, z) abq_log_msg_z(ABQ_DEBUG_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cstr_t)(z))
#define ABQ_DEBUG_MSG_PX(msg, p, x) abq_log_msg_px(ABQ_DEBUG_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cvar_t)(p), (number_t)(x))

#define ABQ_TRACE_STATUS(status, msg) abq_log_s_msg(ABQ_TRACE_LEVEL, __FILENAME__, __LINE__, (status),  (cstr_t)(msg))
#define ABQ_TRACE_MSG_X(msg, x) abq_log_msg_x(ABQ_TRACE_LEVEL, __FILENAME__, __LINE__,(cstr_t)(msg), (number_t)(x))
#define ABQ_TRACE_MSG_Y(msg, y, n) abq_log_msg_y(ABQ_TRACE_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cstr_t)(y), (int32_t)(n))
#define ABQ_TRACE_MSG_Z(msg, z) abq_log_msg_z(ABQ_TRACE_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg), (cstr_t)(z))

#if defined(HAVE_SLOG_H)
# define ABQ_ERROR_MSG(msg) SLOG_E(msg)
# define ABQ_WARN_MSG(msg) SLOG_W(msg)
# define ABQ_INFO_MSG(msg) SLOG_I(msg)
# define ABQ_DEBUG_MSG(msg) SLOG_D(msg)
# define ABQ_TRACE_MSG(msg) SLOG_T(msg)
#else /* !HAVE_SLOG_H */
# if !defined(NDEBUG)
#  define ABQ_ERROR_MSG(msg) abq_log_msg(ABQ_ERROR_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
#  define ABQ_WARN_MSG(msg) abq_log_msg(ABQ_WARN_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
#  define ABQ_INFO_MSG(msg) abq_log_msg(ABQ_INFO_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
#  define ABQ_DEBUG_MSG(msg) abq_log_msg(ABQ_DEBUG_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
#  define ABQ_TRACE_MSG(msg) abq_log_msg(ABQ_TRACE_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
# else /* NDEBUG */
#  define ABQ_ERROR_MSG(msg) abq_log_msg(ABQ_ERROR_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
#  define ABQ_WARN_MSG(msg) abq_log_msg(ABQ_WARN_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
#  define ABQ_INFO_MSG(msg) abq_log_msg(ABQ_INFO_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
#  define ABQ_DEBUG_MSG(msg) abq_log_msg(ABQ_DEBUG_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
#  define ABQ_TRACE_MSG(msg) abq_log_msg(ABQ_TRACE_LEVEL, __FILENAME__, __LINE__, (cstr_t)(msg))
# endif /* NDEBUG */
# define SLOG_E(msg) ABQ_ERROR_MSG(msg)
# define SLOG_W(msg) ABQ_WARN_MSG(msg)
# define SLOG_I(msg) ABQ_INFO_MSG(msg)
# define SLOG_D(msg) ABQ_DEBUG_MSG(msg)
# define SLOG_T(msg) ABQ_TRACE_MSG(msg)
static inline cvar_t registerSlogTimeStringCallback(abq_log_timestamp_cb get_timestamp) {
    VITAL_IS_OK(abq_log_set_timestamp_cb(get_timestamp));
    return NULL;
}
#endif /* !HAVE_SLOG_H */

static inline void debugpoint( void ) {
    static size_t hitcounter = 0U;
    hitcounter++; // put breakpoint here
}
#define CHECKIF(value) if(value) { debugpoint(); }

// Must be included after we define ABQ_VITAL macro
#include <ontrac/util/itsy_bits.h>

#endif /* SPIL_NIO_HTTP_STATUS_CODES_H_ */
