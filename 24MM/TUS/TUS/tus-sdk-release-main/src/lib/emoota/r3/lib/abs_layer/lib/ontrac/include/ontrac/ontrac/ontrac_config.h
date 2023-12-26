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
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file application/aontrac_config.h
 * @date Nov 11, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help-me}
 */

#ifndef INCLUDE_APPLICATION_ONTRAC_CONFIG_H_
#define INCLUDE_APPLICATION_ONTRAC_CONFIG_H_

#ifdef HAVE_APPLICATION
#include "application/ontrac_config.h"
#endif /* HAVE_APPLICATION*/

#ifndef MAX_LINK_VAR_COUNT
# if !defined(NDEBUG)
#  define MAX_LINK_VAR_COUNT (4096U)
# else /* NDEBUG */
# define MAX_LINK_VAR_COUNT (1024U)
# endif /* defined(NDEBUG) */
# endif /* MAX_LINK_VAR_COUNT*/

#ifndef DEFAULT_B16_COUNT
// 16 bytes is basically used up by the preceding obj_t instance
#define DEFAULT_B16_COUNT (0U)
#endif /* DEFAULT_B16_COUNT */

#ifndef DEFAULT_B32_COUNT
#if __SIZEOF_POINTER__ == 4U
// expecting use as bunch of strings
#define DEFAULT_B32_COUNT (512U)
#else
#define DEFAULT_B32_COUNT (0U)
#endif /* __SIZEOF_POINTER__ */
#endif /* DEFAULT_B32_COUNT */

#ifndef DEFAULT_B48_COUNT
#if __SIZEOF_POINTER__ == 4
// expecting use as bunch of strings
#define DEFAULT_B48_COUNT (1024U)
#else
#define DEFAULT_B48_COUNT (1024U)
#endif /* __SIZEOF_POINTER__ */
#endif /* DEFAULT_B32_COUNT */

#ifndef DEFAULT_B64_COUNT
#define DEFAULT_B64_COUNT (512U)
#endif /* DEFAULT_B64_COUNT */

#ifndef DEFAULT_B80_COUNT
#define DEFAULT_B80_COUNT (256U)
#endif /* DEFAULT_B80_COUNT */

#ifndef DEFAULT_B96_COUNT
#define DEFAULT_B96_COUNT (128U)
#endif /* DEFAULT_B96_COUNT */

#ifndef DEFAULT_B112_COUNT
#define DEFAULT_B112_COUNT (128U)
#endif /* DEFAULT_B112_COUNT */

#ifndef DEFAULT_B128_COUNT
#define DEFAULT_B128_COUNT (128U)
// expecting use as large strings & small byte_buffer_t
#endif /* DEFAULT_B128_COUNT */

#ifndef DEFAULT_B256_COUNT
#define DEFAULT_B256_COUNT (64U)
#endif /* DEFAULT_B256_COUNT */

#ifndef DEFAULT_B512_COUNT
// expected use as medium byte_buffer_t
#define DEFAULT_B512_COUNT (32U)
#endif /* DEFAULT_B512_COUNT */

#ifndef DEFAULT_B1024_COUNT
// Was a value of MAX_XFERS(56), but would run out when doing 9 installs
#define DEFAULT_B1024_COUNT (128U)
#endif /* DEFAULT_B1024_COUNT */

#ifndef DEFAULT_B2048_COUNT
// expected use as a large byte_buffer_t
#define DEFAULT_B2048_COUNT (64U)
#endif /* DEFAULT_B2048_COUNT */

#ifndef DEFAULT_B4096_COUNT
/* base64 encoded PEM certificates do not fit in a 2K string */
#define DEFAULT_B4096_COUNT (8U)
#endif /* DEFAULT_B4096_COUNT */

#ifndef DEFAULT_B8192_COUNT
/* required for gzip's internal state + larger PEM x509 certificates*/
#define DEFAULT_B8192_COUNT (16U)
#endif /* DEFAULT_B8192_COUNT */

#ifndef DEFAULT_B16384_COUNT
#define DEFAULT_B16384_COUNT (2U)
#endif /* DEFAULT_B16384_COUNT */

#ifndef DEFAULT_B32768_COUNT
#define DEFAULT_B32768_COUNT (1U)
#endif /* DEFAULT_B32768_COUNT */

#ifndef DEFAULT_B65536_COUNT
#define DEFAULT_B65536_COUNT (0U)
#endif /* DEFAULT_B65536_COUNT */

#endif /* INCLUDE_APPLICATION_ONTRAC_CONFIG_H_ */
