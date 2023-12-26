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
#ifndef ONTRAC_TEXT_UTF8_ENC_H_
#define ONTRAC_TEXT_UTF8_ENC_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "emoota_utility_types.h"
#include "codec.h"

extern const abq_codec_t abq_utf8_codec;

#ifdef __cplusplus
extern const abq_codec_t *const get_utf8_codec(void);
#define getUTF8Codec get_utf8_codec()
#else
#define getUTF8Codec &abq_utf8_codec
#endif

#ifdef __cplusplus
}
#endif
#endif /* ONTRAC_TEXT_UTF8_ENC_H_ */
