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
 * @file item_tap.c
 * @date Jul 17, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/rest/item_writers.h>

static int64_t item_tap_read(datatap_t *datatap, void *dest, int64_t nbyte) {
    int64_t retval = -1L;
    // Sanity checks where previously done in wrapping call (datatap_read)
    FATAL_IF(datatap_is_closed(datatap));
    item_writer_t * writer
        = item_writer_resolve(datatap->handle);
    VITAL_NOT_NULL(writer);
    if (0L > nbyte) {
        abq_status_set(ERANGE, false);
    } else if((0L == nbyte) || (abq_io_has_eof(datatap->flags))) {
        retval = 0;
    } else {
        err_t status = buf_upgrade(writer->cache,
                ptr2raw(dest), (size_t)nbyte, false);
        if(status_code_is_error(status)) {
            abq_status_set(status, false);
        } else {
            status = writer->format_into(writer, writer->cache);
            retval = (int64_t) buf_remaining(writer->cache);
            // ABQ_TRACE_MSG_Y("item_tap:", dest_buf, retval)
            writer->cache->limit = writer->cache->position;
            if (EOVERFLOW == status) {
                datatap_publish(datatap, ABQ_IO_READ);
            } else {
                datatap->flags &= ~ABQ_IO_READ;
                if (ECANCELED == status) {
                    datatap_publish(datatap, ABQ_IO_STOP);
                } else {
                    DATATAP_ERROR(datatap, status);
                }
            }
        }
    }
    return retval;
}

static  err_t item_tap_close(datatap_t *datatap) {
    // Sanity checks where previously done in wrapping call (datatap_read)
    FATAL_IF(datatap_is_closed(datatap));
    SET_RESERVED_FIELD(datatap, handle, NULL);
    datatap->flags &= ~ABQ_IO_READ; // Can't read more data
    datatap_publish(datatap, ABQ_IO_CLOSED);
    return EXIT_SUCCESS;
}

datatap_t * item_tap_create(format_data_func_t formatter, cvar_t item) {
    datatap_t *retval = NULL;
    if (NULL == formatter) {
        abq_status_set(EFAULT, false);
    } else {
        byte_buffer_t* cache = buf_allocate(BUFFER_EMPTY);
        item_writer_t* writer
            = item_writer_create(formatter, item, cache, false, NULL, NULL);
        if((NULL == cache) || (NULL == writer)){
            abq_status_set(ENOMEM, false);
        } else {
            retval = datatap_create(NULL, item_tap_read, item_tap_close, cache, writer);
            if (NULL != retval) {
                // Check that attempting to parse data into an empty buffer results in EOVERFLOW
                err_t status = writer->format_into(writer, writer->cache);
                if (EOVERFLOW == status) {
                    retval->flags |= ABQ_IO_READ;
                } else if(ECANCELED == status){
                    // No data read, but already completed?
                    retval->flags |= ABQ_IO_STOP;
                } else {
                    DATATAP_ERROR(retval, status);
                    VITAL_IS_OK(obj_release_self(retval));
                    retval = NULL;
                }
            }
        }
        (void) obj_release_self(cache);
        (void) obj_release_self(writer);
    }
    return retval;
}


