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
 * @file datatap.h
 * @date Jan 9, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_STREAM_DATATAP_H_
#define ONTRAC_STREAM_DATATAP_H_

#include <ontrac/stream/abq_io.h>
#include <ontrac/ontrac/abq_class.h>

typedef struct for_datatap datatap_t;
typedef struct for_datatap_listener datatap_listener_t;

#include <ontrac/util/byte_buffer.h>

/**
 * The datatap_seek_funtion_t callback sets the file position indicator for the stream
 * pointed to by *datatap.  The new position, measured in bytes, is obtained
 * by adding offset bytes to the position specified by whence.  If whence is
 * set to ABQ_SEEK_SET, ABQ_SEEK_CUR,   or ABQ_SEEK_END, the offset
 * is relative to the start of the file,    the current position indicator, or
 * end-of-file, respectively.
 *
 * @param datatap A pointer with an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 * @param offset The offset from 'whence' where the file pointer should be set.
 * @param whence What the 'offset' should be applied to.  This will be one of three
 *               values ABQ_SEEK_SET, ABQ_SEEK_CUR, or ABQ_SEEK_END.
 *
 * @retval Current location within the file on successful return.
 * @retval < 0 Indicates an error occurred.
 */
typedef int64_t (*datatap_seek_funtion_t) (datatap_t *datatap, int64_t offset, abq_whence_t whence);

/**
 * The datatap_read_function_t callback reads data from the stream pointed to by datatap into
 * the provided buffer.
 *
 * @param datatap A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 * @param dest The buffer provided to place the data that is read from the input stream.
 * @param nbyte The REQUESTED number of bytes to be transferred from the input stream
 *              to the provided buffer.  It is possible that this method will
 *              not read all of the requested bytes.  Check the return code to
 *              verify the total bytes actually read.
 *
 * @retval >= indicates the number of bytes that were read from the input stream.
 *            Depending upon underlying implementation, the number read may be
 *            less than the total number requested by the input parameter `count`
 * @retval <0 Indicates an error occurred.
 */
typedef int64_t (*datatap_read_function_t) (datatap_t *datatap, void *dest, int64_t nbyte );

/**
 * The datatap_close_function_t callback is used to release any resources associated with
 * the input stream pointed to by datatap that was open for reading.
 *
 * @param datatap A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 *
 * @retval EXIT_SUCCESS: Indicates a successful return.
 */
typedef err_t (*datatap_close_function_t)(datatap_t *datatap);
/**
 * @brief callback used to listen for events from the datatap_t. Each datatap_t should be limited to 1 data listener and any number of pipeline listeners
 *
 * @param datatap: A datatap_t representing the source of the event, check datatap->flags the for event(s) which triggered the callback.
 * @param cbdata: callers context which was registered alongside the event-listener
 */
typedef void (*datatap_event_callback_t)(datatap_t * datatap, cvar_t cbdata);

struct for_datatap_listener {
    abq_io_flags eflags;
    datatap_event_callback_t callback;
    cvar_t cbdata;
    struct for_datatap_listener *next;
};

struct for_datatap {
    datatap_seek_funtion_t seek;
    datatap_read_function_t read;
    datatap_close_function_t close;
    /** Not that the top level listener is will be the data-listener if registered, pipe-listeners will follow */
    datatap_listener_t *listeners;
    /**
     * Intended use is for the caller as an place them to briefly store unparsed data while waiting for additional input
     *   Should not be used or even allocated internally by datatap_t implementations. Data should be placed in *handle* instead
     */
    byte_buffer_t* cache;
    var_t handle;
    abq_io_flags flags;
    abq_publish_state publish_state;
};

extern const class_t datatap_class;

extern datatap_t* datatap_create(datatap_seek_funtion_t seek,
        datatap_read_function_t read, datatap_close_function_t close,
        byte_buffer_t* cache, var_t handle);
/**
 * @brief resolves an instance of datatap_class to the datatap_t*
 *
 * @param pointer to item to be resolved
 * @return resolved datatap_t* or NULL on failure
 */
extern datatap_t* datatap_resolve(cvar_t item);

/**
 * Wrapper for datatap->seek function which first checks validity before invoking the method
 *
 * @param datatap A pointer with an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 * @param offset The offset from 'whence' where the file pointer should be set.
 * @param whence What the 'offset' should be applied to.  This will be one of three
 *               values ABQ_SEEK_SET, ABQ_SEEK_CUR, or ABQ_SEEK_END.
 *
 * @retval Current location within the file on successful return.
 * @retval < 0 Indicates an error occurred.
 */
extern int64_t datatap_seek(datatap_t *datatap, int64_t offset, abq_whence_t whence);
/**
 * Wrapper for datatap->read function which first checks validity before invoking the method
 *
 * @param datatap A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 * @param dest The buffer provided to place the data that is read from the input stream.
 * @param nbyte The REQUESTED number of bytes to be transferred from the input stream
 *              to the provided buffer.  It is possible that this method will
 *              not read all of the requested bytes.  Check the return code to
 *              verify the total bytes actually read.
 *
 * @retval >= indicates the number of bytes that were read from the input stream.
 *            Depending upon underlying implementation, the number read may be
 *            less than the total number requested by the input parameter `count`
 */
extern int64_t datatap_read (datatap_t *datatap, void *dest, int64_t nbyte);
/**
 * Wrapper for datatap->close function which first checks validity before invoking the method
 *
 * @param datatap A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 *
 * @retval EXIT_SUCCESS: Indicates a successful return.
 */
extern err_t datatap_close(datatap_t *datatap);
/**
 * @brief sets the listener for any ABQ_IO_DATA events on the given datatap, replacing current listener if one already exists, else creating one
 *
 * @param datatap: The datatap_t instance the user wishes to listen to
 * @param callback: Function to be invoked next time a ABQ_IO_DATA event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return EXIT_SUCCESS: if no data-listener was found and new one was successfully registered
 * @return ENOTEMPTY: if a previous data-listener was found and replaced
 * @return other: Other status codes use to discern between errors that occurred
 */
extern err_t datatap_set_datareader(datatap_t * datatap, datatap_event_callback_t callback, cvar_t cbdata);
/**
 * @brief adds a new listener for any ABQ_IO_PIPELINE events on the given datatap
 *
 * @param datatap: The datatap_t instance the user wishes to listen to
 * @param callback: Function to be invoked next time a ABQ_IO_PIPELINE event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return EXIT_SUCCESS: if no data-listener was found and new one was successfully registered
 * @return other: Other status codes use to discern between errors that occurred
 */
extern err_t datatap_add_pipelistener(datatap_t * datatap, datatap_event_callback_t callback, cvar_t cbdata);
/**
 * @brief sets the listener for any ABQ_IO_DATA events in addition to ABQ_IO_PIPELINE on the given datatap, replacing current datareader if one already exists, else creating one
 *
 * @param datatap: The datatap_t instance the user wishes to listen to
 * @param callback: Function to be invoked next time a ABQ_IO_DATA | ABQ_IO_PIPELINE event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return EXIT_SUCCESS: if no data-listener was found and new one was successfully registered
 * @return ENOTEMPTY: if a previous data-listener was found and replaced
 * @return other: Other status codes use to discern between errors that occurred
 */
extern err_t datatap_set_combolistener(datatap_t * datatap, datatap_event_callback_t callback, cvar_t cbdata);
/**
 * @brief removes listeners matching callbacke and cbdata from given datasink
 *
 * @param datatap: The datatap_t instance the user wishes to remove listener from
 * @param callback: Function to be invoked next time a ABQ_IO_PIPELINE event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return EXIT_SUCCESS: if one or more listeners were found and removed, NOT_FOUND if no listener was found, else a different error code
 */
extern err_t datatap_remove_listener(datatap_t * datatap, datatap_event_callback_t callback, cvar_t cbdata);
/**
 * @brief A utility used by datatap_t implementation to publish events to registered listeners
 *
 * @param datatap: The datatap_t instance associated with the given event
 * @param new_flags: Flags which are now applicable to the datatap
 */
extern void datatap_publish(datatap_t * datatap, abq_io_flags new_flags);
/**
 * @brief Check if a datareader is actively registered in a given datatap_t
 *
 * @param datatap: The datatap_t instance to search for listener
 * @return true if a datareader was found, false otherwise
 */
extern bool_t datatap_has_datareader(const datatap_t *datatap);

static inline bool_t datatap_is_closed(datatap_t *tap) {
    return (&datatap_class != class_of(tap))
            ? true : abq_io_is_closed(tap->flags);
}

static inline void datatap_set_error(datatap_t * datatap, err_t err,
        cstr_t filename, int32_t line, cstr_t label) {
    if (datatap_is_closed(datatap)) {
        // don't publish error status
    } else {
        abq_log_s_msg(ABQ_WARN_LEVEL, filename, line, err,  label);
        ABQ_WARN_STATUS(err, label);
        switch (err) {
            case NIO_RX_TIMEOUT:
                (void) datatap_publish(datatap, ABQ_IO_TIMEOUT);
                break;
            case ETIMEDOUT:
                (void) datatap_publish(datatap, ABQ_IO_TIMEOUT);
                break;
            case NIO_HANGUP:
                (void) datatap_publish(datatap, ABQ_IO_HANGUP);
                break;
            case ECONNRESET:
                (void) datatap_publish(datatap, ABQ_IO_HANGUP);
                break;
            case ECONNREFUSED:
                (void) datatap_publish(datatap, ABQ_IO_HANGUP);
                break;
            case EPIPE:
                (void) datatap_publish(datatap, ABQ_IO_HANGUP);
                break;
            case NIO_CLOSED:
                // Just close the datatap_t (below)
                //  which in turn will publish a ABQ_IO_CLOSED
                break;
            default:
                (void) datatap_publish(datatap, ABQ_IO_ERROR);
                break;
        }
        EXPECT_IS_OK(datatap_close(datatap));
    }
}

#define DATATAP_ERROR(datatap, err) datatap_set_error(datatap, err, __FILENAME__, __LINE__, __FUNCTION__)

static inline err_t datatap_get_status(datatap_t * datatap) {
    err_t retval = class_check(&datatap_class, datatap);
    if (EXIT_SUCCESS == retval) {
        if (abq_io_pipe_check(datatap->flags)) {
            if (abq_io_timeout(datatap->flags)) {
                retval = NIO_RX_TIMEOUT;
            } else if(abq_io_hangup(datatap->flags)) {
                retval = NIO_HANGUP;
            } else if(abq_io_has_error(datatap->flags)) {
                retval = NIO_ERROR;
            } else {
                retval = NIO_CLOSED;
            }
        } else if(abq_io_has_eof(datatap->flags)) {
            if (abq_io_readable(datatap->flags)) {
                // Still has cached data
                retval = EXIT_SUCCESS;
            } else {
                retval = ECANCELED;
            }
        } else {
            retval = EXIT_SUCCESS;
        }
    }
    return retval;
}

extern datatap_t *limited_tap_create(datatap_t *rawtap,
        size_t amount, bool_t close_when_done,
        datatap_event_callback_t on_completed, cvar_t ctx);

extern size_t limited_tap_remaining(const datatap_t *limited);

#include <ontrac/stream/datasink.h>

extern err_t datatap_direct_into_sink(datatap_t *tap, datasink_t* sink);

extern bool_t datatap_is_direct_into_sink(const datatap_t *tap);

extern err_t datatap_dump_debug(datatap_t *tap);

static inline bool_t datatap_is_readable(datatap_t *tap) {
    // Chose to use abq_io_data_check over abq_io_readable since
    // it is desireable to know if ABQ_IO_STOP flag is set as well
    return (NULL == tap) ? false : ((0U != buf_remaining(tap->cache)) || (abq_io_readable(tap->flags)));
}

#endif /* ONTRAC_STREAM_DATATAP_H_ */
