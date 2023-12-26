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
 * @file mem_sink.c
 * @date Jan 10, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/stream/mem_sink.h>
#include <ontrac/util/byte_buffer.h>

static inline void mem_sink_update_dataflags(datasink_t * datasink, const byte_buffer_t *buf) {
    if (buf->limit < buf->capacity) {
        datasink_publish(datasink, ABQ_IO_WRITE);
    } else {
        // Turn off write flag is data can't be written
        datasink->flags &= ~ABQ_IO_WRITE;
    }
}

static int64_t mem_sink_seek (datasink_t * datasink, int64_t offset, abq_whence_t whence) {
    int64_t newpos = -1L;
    // Sanity checks where previously done in wrapping call (datasink_seek)
    FATAL_IF(datasink_is_closed(datasink));
    byte_buffer_t *buf = buf_resolve(datasink->handle);
    VITAL_NOT_NULL(buf);
    // Moving buf->position to 0 simplifies
    //  absolute position considerations,
    //  but we kinda assume it has been @ zero
    EXPECT_IS_OK(buf_compact(buf));
    // Compute place to write data based on whence
    switch(whence) {
    case ABQ_SEEK_SET:
        // Absolute measurement based on start position
        newpos = offset + (int64_t)buf->position;
        break;
    case ABQ_SEEK_CUR:
        // Adjust the offset based on current limit
        newpos = offset + (int64_t)buf->limit;
        break;
    case ABQ_SEEK_END:
        // Adjust the offset based on total capacity
        newpos = offset + (int64_t)buf->capacity;
        break;
    default:
        abq_status_set(BAD_REQUEST, false);
        break;
    }
    if ((buf->position > (size_t)newpos) || (((size_t)newpos) > buf->capacity)) {
        abq_status_set(ERANGE, false);
        newpos = -1L;
    } else {
        buf->limit = (size_t) newpos;
        mem_sink_update_dataflags(datasink, buf);
    }
    return newpos;
}

static int64_t mem_sink_write(datasink_t * datasink, const void *source, int64_t count) {
    int64_t retval = -1;
    // Sanity checks where previously done in wrapping call (datasink_write)
    FATAL_IF(datasink_is_closed(datasink));
    byte_buffer_t *buf = buf_resolve(datasink->handle);
    VITAL_NOT_NULL(buf);
    ABQ_VITAL(abq_io_writable(datasink->flags));
    if (count < 0) {
        abq_status_set(REQUEST_RANGE_NOT_SATISFIABLE, false);
    } else {
        retval = (int64_t) buf_unused_bytes(buf);
        if (count < retval) {
            retval = count;
        }
        // Do the read and overwrite the parameter passed into the function
        err_t status = buf_write_bytes(buf, source, (size_t) retval);
        if (EXIT_SUCCESS != status) {
            DATASINK_ERROR(datasink, status);
            retval = -1;
        } else {
            mem_sink_update_dataflags(datasink, buf);
        }
    }
    return retval;
}

static err_t mem_sink_flush(datasink_t * datasink) {
    // Sanity checks where previously done in wrapping call (datasink_flush)
    FATAL_IF(datasink_is_closed(datasink));
    byte_buffer_t *buf = buf_resolve(datasink->handle);
    VITAL_NOT_NULL(buf);
    // mem_sink is always in a flushed state when open
    datasink_publish(datasink, ABQ_IO_FLUSH);
    return EXIT_SUCCESS;
}

static err_t mem_sink_close (datasink_t * datasink) {
    // Sanity checks where previously done in wrapping call (datasink_close)
    FATAL_IF(datasink_is_closed(datasink));
    byte_buffer_t *buf = buf_resolve(datasink->handle);
    VITAL_NOT_NULL(buf);
    // Release all resources and publish the event
    SET_RESERVED_FIELD(datasink, handle, NULL);
    datasink->close = NULL;
    // turn off writable bit & publish closed bit
    datasink->flags &= ~ABQ_IO_WRITE;
    datasink_publish(datasink, ABQ_IO_CLOSED);
    return EXIT_SUCCESS;
}

datasink_t* mem_sink_create(byte_buffer_t *buffer) {
    datasink_t *retval = NULL;
    if (NULL == buffer) {
        abq_status_set(EFAULT, false);
    } else {
        retval = datasink_create(mem_sink_seek, mem_sink_write,
                mem_sink_flush, mem_sink_close, buffer);
        if (NULL == retval) {
            abq_status_set(ENOMEM, false);
        } else {
            mem_sink_update_dataflags(retval, buffer);
        }
    }
    return retval;
}
