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
 * @file utf8_enc.h
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_TEXT_UTF8_ENC_H_
#define ONTRAC_TEXT_UTF8_ENC_H_

#include <ontrac/text/codec.h>

extern int32_t utf8_char_length(const cstr_t buffer);

extern const abq_codec_t utf8_codec;

#endif /* ONTRAC_TEXT_UTF8_ENC_H_ */
