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
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

/** @file
 * The Platform Configuration provides standard types for the current platform, some standard
 * platform-specific definitions, and the Platform Context for the ONG Application.
 *
 * The standard types are either specified using standard library includes (if available for the
 * platform) or by defining them for the platform locally. The platform-specific definitions
 * should always include the following values:
 *
 * 1. PLATFORM_MAX_FILE_PATH_LEN
 * 2. PLATFORM_MAX_FILE_NAME_LEN
 *
 * Platform-specific definitions may include other values as well. Any source file may include the
 * Platform Configuration file in order to pick up these standard types and platform-specific
 * values at compile time.
 *
 */

#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#if (__STDC_VERSION__ >= 199901L) || (__cplusplus >= 201103L)
/* C99 or C++11 */
// We are supporting standard booleans.
#include <stdbool.h>
// We want to use the standard int types everywhere.
#include <stdint.h>
// We want to use standard definitions everywhere.
#include <stddef.h>
// We want to use the standard floating point types everywhere.
#include <float.h>

#else
# ifndef __cplusplus
// We are supporting standard booleans.
typedef enum {false=0, true=1} bool;
# else
// We are supporting standard booleans.
#include <stdbool.h>
# endif /* __cplusplus */
typedef signed char     int8_t;
typedef signed short    int16_t;
typedef signed int      int32_t;
typedef signed long     int64_t;

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long   uint64_t;

typedef          float  float32_t;
typedef          double float64_t;
typedef long     double float128_t;

typedef unsigned int    size_t;
#endif /* else of (__STDC_VERSION__ >= 199901L) || (__cplusplus >= 201103L) */

// We need to know the limits.
#include <limits.h>

// used for rand and EXIT_SUCCESS macro
#include <stdlib.h>

// define PLATFORM_NAME "OSX"

// define PLATFORM_MIN_VERSION "10.10+"

// PATH_MAX often gets very large, define our own
#define PLATFORM_MAX_FILE_PATH_LEN (512U)
// NAME_MAX may also be too large, define our own
#define PLATFORM_MAX_FILE_NAME_LEN (256U)
#define CHOREO_HASH_METHOD CRYPT_HASH_MODE_SHA256
#define CHOREO_HASH_LENGTH 32

// for dumb versions of MinGW
#ifndef EADDRNOTAVAIL
// 101 has conclusion with HTTP_CONTINUE
#define EADDRNOTAVAIL 151
#endif
#ifndef EAFNOSUPPORT
// 102 has conclusion with SWITCHING_PROTOCOLS
#define EAFNOSUPPORT 152
#endif
#ifndef EALREADY
// 103 has conclusion with HTTP_PROCESSING
#define EALREADY 153
#endif
#ifndef ECANCELED
#define ECANCELED 105
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH 110
#endif
#ifndef EINPROGRESS
#define EINPROGRESS 112
#endif
#ifndef ENOBUFS
#define ENOBUFS 119
#endif
#ifndef ENODATA
#define ENODATA 120
#endif
#ifndef ENOLINK
#define ENOLINK 121
#endif
#ifndef ENOTCONN
#define ENOTCONN 126
#endif
#ifndef EOVERFLOW
#define EOVERFLOW 132
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT 138
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif
#ifndef ESHUTDOWN
// WSAESHUTDOWN
#define ESHUTDOWN ENOTCONN
#endif

#define __YES__ (1U == 1U)
#define __NO__ (0U != 0U)

#ifdef true
#ifdef false
/** MISRA hates macros, so we will explicitly define true & false with an enum */
#undef false
#undef true
// typedef enum {false=0, true=1} boole; /* essentially Boolean type */
#define false ((_Bool)0U)
#define true ((_Bool)1U)
#endif /* defined(false) */
#endif /* defined(true) */

typedef bool bool_t;
typedef const bool_t boolc;
typedef boolc *bool_ptr;

//need to be able to return negative values on error,
// size_t is unsigned so won't work for fd or err_t
#ifndef __SIZEOF_POINTER__
#define __SIZEOF_POINTER__ (4)
#endif
// if __SIZEOF_POINTER__ == 4
#if (INT32_MAX == INT_MAX)
typedef int32_t err_t;
typedef int32_t fd_type;
typedef int32_t plat_int_t;
typedef uint32_t plat_uint_t;
#else
typedef int64_t err_t;
typedef int64_t fd_type;
typedef int64_t plat_int_t;
typedef uint64_t plat_uint_t;
#endif
#if (INT32_MAX == LONG_MAX)
typedef int32_t plat_long_t;
#else
typedef int64_t plat_long_t;
#endif

// MISRA Rule 11.3 provides the sole exception:
// "It is permitted to convert a pointer to object type into a pointer of one of the object types 'char', 'signed char', or 'unsigned char'."
// "The Standard guarantees that pointers to these types can be used to access the individual bytes of an object."
typedef void *var_t;                    /* pointer to void */
// a const void type_definition
typedef void const cvoid_t;             /* const void type */
// pointer to a const void
typedef cvoid_t *cvar_t;                /* pointer to const void object */
// a const var_t pointer to const void
typedef var_t const cvoid_ptr;          /* const pointer to void object */
// a const void pointer to const void
typedef cvoid_t *const cvoidc_ptr;      /* const pointer to const void object */
/** pointer / array of const void pointer(s) */
typedef cvar_t* addr_ptr;
// common byte type-definition
typedef char byte_t;
// const byte type-definition
typedef const char cbyte_t;
// string type-definition, it should be const, but then we can't cast it to a var_t
typedef byte_t *str_t;
// a const byte_t pointer for when we don't need to cast to var_t
typedef cbyte_t *cstr_t;
// number type-definition
typedef double number_t;
typedef const double cnumber_t;
typedef number_t *number_ptr;
typedef cnumber_t *cnumber_ptr;

#endif //PLATFORM_CONFIG_H
