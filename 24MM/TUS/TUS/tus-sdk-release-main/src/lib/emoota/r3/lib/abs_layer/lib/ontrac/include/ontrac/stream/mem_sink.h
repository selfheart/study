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
 * @file buf_sink.h
 * @date Jan 9, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_STREAM_MEM_SINK_H_
#define ONTRAC_STREAM_MEM_SINK_H_

#include <ontrac/stream/datasink.h>
#include <ontrac/util/byte_buffer.h>

extern datasink_t* mem_sink_create(byte_buffer_t *buffer);

#endif /* ONTRAC_STREAM_MEM_SINK_H_ */
