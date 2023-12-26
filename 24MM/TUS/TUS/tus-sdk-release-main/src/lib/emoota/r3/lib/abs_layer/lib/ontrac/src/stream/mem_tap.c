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
 * @file mem_tap.c
 * @date Jan 10, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */


#include <ontrac/ontrac/abq_str.h>
#include <ontrac/stream/mem_tap.h>
#include <ontrac/util/byte_buffer.h>

static int64_t mem_tap_seek(datatap_t *datatap,
        int64_t offset, abq_whence_t whence) {
    int64_t newpos = -1;
    int64_t retval = -1;
    // Sanity checks where previously done in wrapping call (datatap_seek)
    FATAL_IF(datatap_is_closed(datatap));
    byte_buffer_t *buf = buf_resolve(datatap->handle);
    VITAL_NOT_NULL(buf);
    switch(whence) {
    case ABQ_SEEK_SET:
        newpos = offset;
        break;
    case ABQ_SEEK_CUR:
        // Adjust the offset based on current position
        newpos = offset + (int64_t)buf->position;
        break;
    case ABQ_SEEK_END:
        // Adjust the offset based on final position
        newpos = offset + (int64_t)buf->limit;
        break;
    default:
        abq_status_set(BAD_REQUEST, true);
        break;
    }
    if ((0L > newpos) || (newpos > (int64_t)buf->limit)) {
        // Out of range (ED0M/ERANGE ?)
        abq_status_set(REQUEST_RANGE_NOT_SATISFIABLE, true);
    } else {
        buf->position = (size_t) newpos;
        retval = (int64_t)buf->position;
        if(buf->position < buf->limit) {
            // turn on read bit & turn off stop bit
            datatap->flags &= ~ABQ_IO_STOP;
            datatap_publish(datatap, ABQ_IO_READ);
        } else {
            // Expect that it already has  ABQ_IO_READ or ABQ_IO_STOP flag set
            ABQ_VITAL(abq_io_data_check(datatap->flags));
        }
    }
    return retval;
}

static int64_t mem_tap_read (datatap_t *datatap, void *dest, int64_t nbyte) {
    int64_t retval = -1;
    // Sanity checks where previously done in wrapping call (datatap_read)
    FATAL_IF(datatap_is_closed(datatap));
    byte_buffer_t *buf = buf_resolve(datatap->handle);
    VITAL_NOT_NULL(buf);
    ABQ_VITAL(abq_io_readable(datatap->flags));
    if (nbyte < 0L) {
        abq_status_set(REQUEST_RANGE_NOT_SATISFIABLE, true);
    } else if (0L == nbyte) {
        retval = nbyte;
    } else {
        // Compute the parameter to be passed to the byte_buffer read function
        if (false == buf_has_data(buf)) {
            // turn on stop bit & turn off read bit
            datatap->flags &= ~ABQ_IO_READ;
            if (abq_io_writable(datatap->flags)) {
                // hack used for creating a "blocking" tap to read/write memory
                abq_status_set(EWOULDBLOCK, true);
                retval = -1L;
            } else {
                datatap_publish(datatap, ABQ_IO_STOP);
                retval = 0L;
            }
        } else {
            // Do the read and overwrite the parameter passed into the function
            retval = (int64_t) buf_read_bytes(buf, dest, (size_t) nbyte);
        }
    }
    return retval;
}

static err_t mem_tap_close(datatap_t *datatap) {
    // Sanity checks where previously done in wrapping call (datatap_close)
    FATAL_IF(datatap_is_closed(datatap));
    byte_buffer_t *buf = buf_resolve(datatap->handle);
    VITAL_NOT_NULL(buf);
    // Release all resources and publish the event
    SET_RESERVED_FIELD(datatap, handle, NULL);
    datatap->close = NULL;
    // turn off read bit & publish closed bit
    datatap->flags &= ~ABQ_IO_READ;
    datatap_publish(datatap, ABQ_IO_CLOSED);
    return EXIT_SUCCESS;
}

datatap_t* mem_tap_create(byte_buffer_t *buffer) {
    datatap_t *retval = NULL;
    if (NULL == buffer) {
        abq_status_set(EFAULT, false);
    } else {
        retval = datatap_create(mem_tap_seek,
                mem_tap_read, mem_tap_close, NULL, buffer);
        if (NULL == retval) {
            abq_status_set(ENOMEM, false);
        } else {
            // Expected the EOF is to be read out event if no data exists
            retval->flags = ABQ_IO_READ;
        }
    }
    return retval;
}

static err_t str_tap_close(datatap_t *datatap) {
    // Sanity checks where previously done in wrapping call (datatap_close)
    FATAL_IF(datatap_is_closed(datatap));
    byte_buffer_t *buf = buf_resolve(datatap->handle);
    VITAL_NOT_NULL(buf);
    // Release all resources and publish the event
    if(buf->free_byte_array_on_delete) {
        // byte-array will be free'd on delete
    } else {
        // release string value created in mem_tap_wrap
        VITAL_IS_OK(obj_release(buf->byte_array, buf));
    }
    // Either do checks again or duplicate functionality
    return mem_tap_close(datatap);
}

datatap_t* mem_tap_wrap(cstr_t source, size_t amount) {
    datatap_t* retval = NULL;
    byte_buffer_t * buf = NULL;
    if(&string_class == class_of(source)) {
        int32_t length = utf8_byte_length(source, (int32_t)amount);
        byte_t *byte_array = NULL;
        if (length >= 0) {
            bytes_copy(&byte_array, &source, sizeof(cvar_t));
            buf = buf_wrap(byte_array, (size_t)length, (size_t)length, false);
            if (NULL != buf) {
                (void) obj_reserve(buf->byte_array, buf);
            }
        }
    } else {
        // Copy all bytes into internal memory & release when done
        buf = buf_allocate((buf_size_t)amount);
        if (NULL != buf) {
            VITAL_IS_OK(buf_write_bytes(buf, source, amount));
        }
    }
    if(NULL == buf) {
        (void) abq_status_set(ENOMEM, false);
    } else {
        retval = mem_tap_create(buf);
        if (NULL == retval) {
            (void) abq_status_set(ENOMEM, false);
            if(buf->free_byte_array_on_delete) {
                // byte-array will be free'd on delete
            } else {
                // release string value created in mem_tap_wrap
                VITAL_IS_OK(obj_release(buf->byte_array, buf));
            }
        } else {
            // Overwrite the close function to release the string
            retval->close = str_tap_close;
            // value is to be released in str_tap_close when datatap_t is closed
        }
        (void) obj_release_self(buf);
    }
    return retval;
}
