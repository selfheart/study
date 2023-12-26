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
 * @file datasink.h
 * @date Jan 9, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_STREAM_DATASINK_H_
#define ONTRAC_STREAM_DATASINK_H_

#include <ontrac/stream/abq_io.h>
#include <ontrac/ontrac/abq_class.h>

typedef struct for_datasink datasink_t;
typedef struct for_datasink_listener datasink_listener_t;

#include <ontrac/util/byte_buffer.h>
/**
 * The datasink_seek_funtion_t callback sets the file position indicator for the stream
 * pointed to by *datasink.  The new position, measured in bytes, is obtained
 * by adding offset bytes to the position specified by whence.  If whence is
 * set to ABQ_SEEK_SET, ABQ_SEEK_CUR,   or ABQ_SEEK_END, the offset
 * is relative to the start of the file,    the current position indicator, or
 * end-of-file, respectively.
 *
 * @param datasink A pointer with an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 * @param offset The offset from 'whence' where the file pointer should be set.
 * @param whence What the 'offset' should be applied to.  This will be one of three
 *               values ABQ_SEEK_SET, ABQ_SEEK_CUR, or ABQ_SEEK_END.
 *
 * @retval Current location within the file on successful return.
 * @retval < 0 Indicates an error occurred.
 */
typedef int64_t (*datasink_seek_funtion_t) (datasink_t * datasink, int64_t offset, abq_whence_t whence);

/**
 * The datasink_write_function_t callback writes data from the provided buffer to the
 * stream pointed to by datasink.
 *
 * @param datasink A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 * @param source The buffer containing the data to write to the output stream.
 * @param count The REQUESTED number of bytes to be transferred from the provided
 *              buffer to the output stream.  It is possible that this method will
 *              not write all of the requested bytes.  Check the return code to
 *              verify the total bytes actually written.
 *
 * @retval >=0 indicates the number of bytes that were written to the output stream.
 *            Depending upon underlying implementation, the number written may be
 *            less than the total number indicated by the input parameter `count`
 * @retval < 0 Indicates an error occurred.
 */
typedef int64_t (*datasink_write_function_t)(datasink_t * datasink, const void *source, int64_t count);

/**
 * The datasink_flush_function_t callback is used to push any latent data down the pipeline
 *  of the output stream pointed to by datasink that was open for writing.
 *
 * @param datasink A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 *
 * @retval EXIT_SUCCESS: Indicates a successful return.
 */
typedef err_t (*datasink_flush_function_t)(datasink_t * datasink);
/**
 * The datasink_close_function_t callback is used to release any resources associated with
 * the output stream pointed to by datasink that was open for writing.
 *
 * @param datasink A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 *
 * @retval EXIT_SUCCESS: Indicates a successful return.
 */
typedef err_t (*datasink_close_function_t)(datasink_t * datasink);
/**
 * @brief callback used to listen for events from the datasink_t. Each datasink_t should be limited to 1 data listener and any number of pipeline listeners
 *
 * @param datasink: A datasink_t representing the source of the event, check datasink->flags for the event(s) which triggered the callback.
 * @param cbdata: callers context which was registered alongside the event-listener
 */
typedef void (*datasink_event_callback_t)(datasink_t * datasink, cvar_t cbdata);

struct for_datasink_listener {
    abq_io_flags eflags;
    datasink_event_callback_t callback;
    cvar_t cbdata;
    struct for_datasink_listener *next;
};

struct for_datasink {
    datasink_seek_funtion_t seek;
    datasink_write_function_t write;
    datasink_flush_function_t flush;
    datasink_close_function_t close;
    /** Active listeners receive live events from the datasink, but only one datasink_listener_t can be active at a time */
    datasink_listener_t *active;
    /** Pending queue of datawriters to be used once the current datasink_listener_t is completed */
    datasink_listener_t *pending;
    var_t handle;
    abq_io_flags flags;
    abq_publish_state publish_state;
};

extern const class_t datasink_class;
extern const class_t datasink_listener_class;

extern datasink_t* datasink_create(datasink_seek_funtion_t seek,
        datasink_write_function_t write, datasink_flush_function_t flush,
        datasink_close_function_t close, var_t handle);
/**
 * @brief resolves an instance of datasink_class to the datasink_t*
 *
 * @param pointer to item to be resolved
 * @return resolved datasink_t* or NULL on failure
 */
datasink_t* datasink_resolve(cvar_t item);

/**
 * Wrapper for datasink->seek function which first checks validity before invoking the method
 *
 * @param datasink A pointer with an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 * @param offset The offset from 'whence' where the file pointer should be set.
 * @param whence What the 'offset' should be applied to.  This will be one of three
 *               values ABQ_SEEK_SET, ABQ_SEEK_CUR, or ABQ_SEEK_END.
 *
 * @retval Current location within the file on successful return.
 * @retval < 0 Indicates an error occurred.
 */
extern int64_t datasink_seek (datasink_t * datasink, int64_t offset, abq_whence_t whence);

/**
 * Wrapper for datasink->write function which first checks validity before invoking the method
 *
 * @param datasink A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 * @param source The buffer containing the data to write to the output stream.
 * @param count The REQUESTED number of bytes to be transferred from the provided
 *              buffer to the output stream.  It is possible that this method will
 *              not write all of the requested bytes.  Check the return code to
 *              verify the total bytes actually written.
 *
 * @retval >=0 indicates the number of bytes that were written to the output stream.
 *            Depending upon underlying implementation, the number written may be
 *            less than the total number indicated by the input parameter `count`
 * @retval < 0 Indicates an error occurred.
 */
extern int64_t datasink_write (datasink_t * datasink, const void *source, int64_t count);

/**
 * Wrapper for datasink->flush function which first checks validity before invoking the method
 *
 * @param datasink A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 *
 * @retval EXIT_SUCCESS: Indicates a successful return.
 */
extern err_t datasink_flush (datasink_t * datasink);
/**
 * Wrapper for datasink->close function which first checks validity before invoking the method
 *
 * @param datasink A pointer to an implementation specific stream handle.  This is used by the
 *               implementation layer as needed.  The caller simply invokes implementation specific
 *               function pointers on this structure and does not directly make use of the *handle.
 *
 * @retval EXIT_SUCCESS: Indicates a successful return.
 */
extern err_t datasink_close (datasink_t * datasink);
/**
 * @brief enqueue's a listener for any ABQ_IO_DATA events on the given datasink, only the first datawriter is actively receives events at any given time
 *
 * @param datasink: The datasink_t instance the user wishes to listen to
 * @param callback: Function to be invoked next time a ABQ_IO_DATA event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return EXIT_SUCCESS: if no data-listener was found and new one was successfully registered
 * @return EINPROGRSS: if an active data-listener exists and new datawriter was appended to list of pending
 * @return other: Other status codes use to discern between errors that occurred
 */
extern err_t datasink_add_datawriter(datasink_t *datasink,
        datasink_event_callback_t callback, cvar_t cbdata);
/**
 * @brief adds a new listener for any ABQ_IO_PIPELINE events on the given datasink
 *
 * @param datasink: The datasink_t instance the user wishes to listen to
 * @param callback: Function to be invoked next time a ABQ_IO_PIPELINE event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return EXIT_SUCCESS: if no data-listener was found and new one was successfully registered
 * @return other: Other status codes use to discern between errors that occurred
 */
extern err_t datasink_add_pipelistener(datasink_t * datasink,
        datasink_event_callback_t callback, cvar_t cbdata);
/**
 * @brief sets the listener for any ABQ_IO_DATA or ABQ_IO_PIPELINE events on the given datasink, replacing current ABQ_IO_DATA listener if one already exists, else creating one
 *
 * @param datasink: The datasink_t instance the user wishes to listen to
 * @param callback: Function to be invoked next time a ABQ_IO_DATA | ABQ_IO_PIPELINE event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return EXIT_SUCCESS: if no data-listener was found and new one was successfully registered
 * @return ENOTEMPTY: if a previous data-listener was found and replaced
 * @return other: Other status codes use to discern between errors that occurred
 */
extern err_t datasink_set_combolistener(datasink_t * datasink,
        datasink_event_callback_t callback, cvar_t cbdata);
/**
 * @brief removes listeners matching callbacke and cbdata from given datasink
 *
 * @param datasink: The datasink_t instance the user wishes to remove listener from
 * @param callback: Function that was to be invoked matching event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return EXIT_SUCCESS: if listener was found and removed & there was no pending datawriters
 * @return EINPROGRESS: if listener was found and removed & datasink was switched to the next pending datawriter
 * @return NOT_FOUND: if no listener was found, else a different error code
 */
extern err_t datasink_remove_listener(datasink_t * datasink,
        datasink_event_callback_t callback, cvar_t cbdata);
/**
 * @brief checks for a matching  listener with callback and cbdata from given datasink
 *
 * @param datasink: The datasink_t instance the user wishes to check for listener from
 * @param callback: Function to be invoked matching event occurs
 * @param cbdata: Context to be passed back to the callback function when event occurs
 * @return true if matching listener is found, false otherwise
 */
extern bool_t datasink_has_listener(const datasink_t *datasink,
        datasink_event_callback_t callback, cvar_t cbdata);
/**
 * @brief A utility used by datasink_t implementation to publish events to registered listeners
 *
 * @param datasink: The datasink_t instance associated with the given event
 * @param new_flags: Flags which are now applicable to the datasink
 */
extern void datasink_publish(datasink_t * datasink, abq_io_flags new_flags);

static inline bool_t datasink_is_closed(datasink_t *sink) {
    return (&datasink_class != class_of(sink))
            ? true : abq_io_is_closed(sink->flags);
}

static inline void datasink_set_error(datasink_t * datasink, err_t err,
        cstr_t filename, int32_t line, cstr_t label) {
    if (datasink_is_closed(datasink)) {
        // don't publish error status
    } else {
        abq_log_s_msg(ABQ_WARN_LEVEL, filename, line, err,  label);
        switch (err) {
            case NIO_TX_TIMEOUT:
                (void) datasink_publish(datasink, ABQ_IO_TIMEOUT);
                break;
            case ETIMEDOUT:
                (void) datasink_publish(datasink, ABQ_IO_TIMEOUT);
                break;
            case NIO_HANGUP:
                (void) datasink_publish(datasink, ABQ_IO_HANGUP);
                break;
            case ECONNRESET:
                (void) datasink_publish(datasink, ABQ_IO_HANGUP);
                break;
            case ECONNREFUSED:
                (void) datasink_publish(datasink, ABQ_IO_HANGUP);
                break;
            case EPIPE:
                (void) datasink_publish(datasink, ABQ_IO_HANGUP);
                break;
            case NIO_CLOSED:
                // Just close the datasink_t (below)
                //  which in turn will publish a ABQ_IO_CLOSED
                break;
            default:
                (void) datasink_publish(datasink, ABQ_IO_ERROR);
                break;
        }
        EXPECT_IS_OK(datasink_close(datasink));
    }
}
#define DATASINK_ERROR(datasink, err) datasink_set_error(datasink, err, __FILENAME__, __LINE__, __FUNCTION__)

static inline err_t datasink_get_status(datasink_t * datasink) {
    err_t retval = class_check(&datasink_class, datasink);
    if (EXIT_SUCCESS == retval) {
        if (abq_io_pipe_check(datasink->flags)) {
            if (abq_io_timeout(datasink->flags)) {
                retval = NIO_TX_TIMEOUT;
            } else if(abq_io_hangup(datasink->flags)) {
                retval = NIO_HANGUP;
            } else if(abq_io_has_error(datasink->flags)) {
                retval = NIO_ERROR;
            } else {
                retval = NIO_CLOSED;
            }
        } else {
            retval = EXIT_SUCCESS;
        }
    }
    return retval;
}

extern err_t datasink_add_bufwriter(datasink_t *datasink, const byte_buffer_t* buf);
/**
 * @brief: Wraps datasink_close within a datasink_event_callback_t to be invoked after previously queued datawriters
 *
 * @param datasink
 */

extern void datasink_close_on_event(datasink_t * datasink, cvar_t cbdata);
static inline err_t datasink_add_closewriter(datasink_t *datasink) {
    err_t retval = CHECK_NULL(datasink);
    if(EXIT_SUCCESS != retval) {
        // Return error as is
    } else if(abq_io_is_closed(datasink->flags)) {
        retval = ECANCELED; // Already closed
    } else if(abq_io_is_flushed(datasink->flags)) {
        retval = datasink_close(datasink);
        if(status_code_is_ok(retval)) {
            retval = ECANCELED; // closed synchronously
        }
    } else {
        retval = datasink_add_datawriter(datasink,
            datasink_close_on_event, NULL);
    }
    return retval;
}
#endif /* ONTRAC_STREAM_DATASINK_H_ */
