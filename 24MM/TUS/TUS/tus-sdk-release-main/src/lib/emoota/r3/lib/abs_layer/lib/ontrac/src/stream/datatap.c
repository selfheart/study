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
 * @file datatap.c
 * @date Jan 9, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/stream/datatap.h>
#include <ontrac/util/abq_item_pool.h>

#ifndef MAX_TAPS
#define MAX_TAPS (48U)
#endif /* MAX_TAPS */

static void datatap_delete(cvar_t old_datatap);

DEFINE_CLASS(datatap_class, datatap_t,
        MAX_TAPS, NULL, NULL, NULL, NULL, datatap_delete);

// Expected to have up to two listeners per datatap_t, one for data events and another for pipeline events
STATIC_ITEM_POOL(datatap_listener_t, (2U*MAX_TAPS), datatap_listener_pool);

datatap_t* datatap_resolve(cvar_t item) {
    datatap_t* retval = NULL;
    CLASS_RESOLVE(datatap_class, datatap_t, retval, item);
    return retval;
}

static void datatap_delete(cvar_t old_datatap) {
    datatap_t *datatap = datatap_resolve(old_datatap);
    VITAL_NOT_NULL(datatap);
    if (!abq_io_is_closed(datatap->flags)) {
        // ugly hack, overwrite publisher state so that events fire
       datatap->publish_state = ABQ_PUBLISH_INACTIVE;
        EXPECT_IS_OK(datatap_close(datatap));
        ABQ_VITAL(abq_io_is_closed(datatap->flags));
    }
    ABQ_VITAL(LLIST_IS_EMPTY(datatap->listeners, next));
    SET_RESERVED_FIELD(datatap, handle, NULL);
    SET_RESERVED_FIELD(datatap, cache, NULL);
}

datatap_t* datatap_create(datatap_seek_funtion_t seek,
        datatap_read_function_t read, datatap_close_function_t close,
        byte_buffer_t* cache, var_t handle) {
    VITAL_NOT_NULL(read);
    VITAL_NOT_NULL(close);
    datatap_t* retval = CREATE_BASE_INSTANCE(datatap_class, datatap_t);
    if(NULL != retval) {
        (void) obj_reserve(cache, retval);
        (void) obj_reserve(handle, retval);
        *retval = (datatap_t) {
            .seek = seek,
            .read = read,
            .close = close,
            .listeners = NULL,
            .cache = cache,
            .handle = handle,
            .flags = ABQ_IO_NOFLAGS,
            .publish_state = ABQ_PUBLISH_INACTIVE
        };
    }
    return retval;
}

int64_t datatap_seek(datatap_t *datatap, int64_t offset, abq_whence_t whence){
    LOCK_CLASS(datatap_class);
    int64_t retval = -1;
    err_t status = datatap_get_status(datatap);
    if ((EXIT_SUCCESS != status) && (ECANCELED != status)) {
        abq_status_set(status, true);
    }else if(NULL == datatap->seek){
        abq_status_set(ENOSYS, true);
    }else{
        retval = datatap->seek(datatap, offset, whence);
    }
    UNLOCK_CLASS(datatap_class);
    return retval;
}

int64_t datatap_read (datatap_t *datatap, void *dest, int64_t nbyte ){
    LOCK_CLASS(datatap_class);
    int64_t retval = -1;
    if (NULL == datatap) {
        abq_status_set(EFAULT, true);
    } else {
        err_t status = datatap_get_status(datatap);
        if (EXIT_SUCCESS != status) {
            abq_status_set(status, true);
        } else if (NULL == datatap->read) {
            abq_status_set(ENOSYS, true);
        } else if (abq_io_readable(datatap->flags)) {
            //VITAL_NOT_NULL(atatap->read)
            retval = datatap->read(datatap, dest, nbyte);
        } else {
            abq_status_set(EWOULDBLOCK, true);
        }
    }
    UNLOCK_CLASS(datatap_class);
    return retval;
}

err_t datatap_close(datatap_t *datatap) {
    LOCK_CLASS(datatap_class);
    err_t retval = class_check(&datatap_class, datatap);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if (abq_io_is_closed(datatap->flags)) {
        if (ABQ_PUBLISH_INACTIVE == datatap->publish_state) {
            ABQ_VITAL(LLIST_IS_EMPTY(datatap->listeners, next));
        }
        retval = EALREADY;
    } else {
        //VITAL_NOT_NULL(datatap->close)
        retval = datatap->close(datatap);
        // Check if event was fired as part of the call to close
        if (CHECK_WOULDBLOCK(retval)) {
            // Expect that the implementation will
            //  publish the ABQ_IO_CLOSED event when complete
        } else if (abq_io_is_closed(datatap->flags)) {
            // Already published close event
        } else {
            datatap->flags &= ~ABQ_IO_READ; // Can't read more data
            datatap_publish(datatap, ABQ_IO_CLOSED);
        }
    }
    UNLOCK_CLASS(datatap_class);
    return retval;
}

static datatap_listener_t* datatap_get_datareader(const datatap_t *datatap) {
    datatap_listener_t *listener = NULL;
    LLIST_FIND(datatap->listeners, datatap_listener_t, listener,
            next, abq_io_data_check(listener->eflags));
    return listener;
}

bool_t datatap_has_datareader(const datatap_t *datatap) {
    bool_t retval = false;
    LOCK_CLASS(datatap_class);
    if((NULL != datatap) && (NULL != datatap_get_datareader(datatap))) {
        retval = true;
    }
    UNLOCK_CLASS(datatap_class);
    return retval;
}

static err_t datatap_add_listener(datatap_t *datatap, abq_io_flags eflags,
        datatap_event_callback_t callback, cvar_t cbdata) {
    LOCK_CLASS(datatap_class);
    err_t retval = class_check(&datatap_class, datatap);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if (abq_io_is_closed(datatap->flags)) {
        // datatap_t has been closed
        retval = NIO_CLOSED;
    } else {
        datatap_listener_t *listener = NULL;
        abq_io_flags ioflags = eflags;
        // Check if the callback / cbdata combination was previously registered
        LLIST_FIND_AND_REMOVE(datatap->listeners, datatap_listener_t, listener, next,
                ((callback == listener->callback) && (cbdata == listener->cbdata)));
        if (NULL != listener) {
            retval = EALREADY;
            // Found a matching listener, combine eflags
            ioflags |= listener->eflags;
            // Release previous listener
            (void) obj_release(listener->cbdata, datatap);
            listener->cbdata = NULL;
            ITEM_POOL_FREE(&datatap_listener_pool, datatap_listener_t, listener);
            listener = NULL;
        }
        if (abq_io_data_check(ioflags)) {
            // Look for a previous data IO listener
            listener = datatap_get_datareader(datatap);
        }
        if (NULL != listener) {
            if (NULL == callback) {
                VITAL_IS_NULL(cbdata);
                listener->callback = callback;
                // Explicitly using datatap as ref instead of listener
                (void) obj_reserve(cbdata, datatap);
                (void) obj_release(listener->cbdata, datatap);
                listener->cbdata = cbdata;
                LLIST_REMOVE(datatap->listeners, datatap_listener_t, listener, next);
                ITEM_POOL_FREE(&datatap_listener_pool, datatap_listener_t, listener);
                retval = EXIT_SUCCESS;
            } else {
                // Simply overwrite it
                listener->eflags = ioflags;
                listener->callback = callback;
                // Explicitly using datatap as ref instead of listener
                (void) obj_reserve(cbdata, datatap);
                (void) obj_release(listener->cbdata, datatap);
                listener->cbdata = cbdata;
                // Finally, invoke the callback if applicable
                if (ABQ_IO_NOFLAGS != (ioflags & datatap->flags)) {
                    // Should call "publish" to manage potential recursion
                    //  as opposed to invoking callback directly
                    if (ABQ_PUBLISH_INACTIVE == datatap->publish_state) {
                        datatap_publish(datatap, datatap->flags);
                    } else {
                        datatap->publish_state = ABQ_PUBLISH_DIRTY;
                    }
                } else if ((EALREADY != retval) && (abq_io_readable(ioflags))
                        && (buf_has_data(datatap->cache))) {
                    // Hack, invoke the callback directly for the special case where
                    //   new datareader is provided & we still have cached data
                    //   TODO: overcome the recursive nature of this call
                    (void) obj_reserve(cbdata, datatap);
                    callback(datatap, cbdata);
                    (void) obj_release(cbdata, datatap);
                } else {
                    // Don't invoke callback at this time
                }
                retval = ENOTEMPTY;
            }
        } else if (NULL == callback) {
            // Already removed the datareader
            VITAL_IS_NULL(cbdata);
            retval = EALREADY;
        } else {
            ITEM_POOL_ALLOC(&datatap_listener_pool, datatap_listener_t, listener);
            if (NULL == listener) {
                retval = abq_status_take(ENOMEM);
            } else {
                (void) obj_reserve(cbdata, datatap);
                *listener = (datatap_listener_t ) {
                    .eflags = ioflags,
                    .callback = callback,
                    .cbdata = cbdata,
                    .next = NULL
                };
                if (abq_io_data_check(ioflags)) {
                    // Push data-listeners to the beginning of the list
                    LLIST_PUSH(datatap->listeners, listener, next);
                } else {
                    // Append pipe-listeners to the end of the list
                    LLIST_APPEND(datatap->listeners,
                            datatap_listener_t, listener, next);
                }
                // Finally, invoke the callback if applicable
                if (ABQ_IO_NOFLAGS != (ioflags & datatap->flags)) {
                    // Should call "publish" to manage potential recursion
                    //  as opposed to invoking callback directly
                    if (ABQ_PUBLISH_INACTIVE == datatap->publish_state) {
                        datatap_publish(datatap, datatap->flags);
                    } else {
                        datatap->publish_state = ABQ_PUBLISH_DIRTY;
                    }
                } else if ((EALREADY != retval) && (abq_io_readable(ioflags))
                        && (buf_has_data(datatap->cache))) {
                    // Invoke the callback directly for the special case where
                    //   new datareader is provided & we still have cached data
                    //   TODO: overcome the recursive nature of this call
                    (void) obj_reserve(cbdata, datatap);
                    callback(datatap, cbdata);
                    (void) obj_release(cbdata, datatap);
                } else {
                    // Don't invoke callback at this time
                }
            }
        }
    }
    UNLOCK_CLASS(datatap_class);
    return retval;
}

err_t datatap_set_datareader(datatap_t * datatap
        , datatap_event_callback_t callback, cvar_t cbdata) {
    return datatap_add_listener(datatap, ABQ_IO_DATA, callback, cbdata);
}

err_t datatap_add_pipelistener(datatap_t * datatap,
        datatap_event_callback_t callback, cvar_t cbdata) {
    return datatap_add_listener(datatap, ABQ_IO_PIPELINE, callback, cbdata);
}

err_t datatap_set_combolistener(datatap_t * datatap,
        datatap_event_callback_t callback, cvar_t cbdata) {
    return datatap_add_listener(datatap, ABQ_IO_ANY, callback, cbdata);
}

err_t datatap_remove_listener(datatap_t * datatap,
        datatap_event_callback_t callback, cvar_t cbdata) {
    LOCK_CLASS(datatap_class);
    err_t retval = class_check(&datatap_class, datatap);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if(NULL == callback) {
        retval = EINVAL;
    } else {
        retval = EALREADY;
        datatap_listener_t *listener = NULL;
        LLIST_FIND_AND_REMOVE(datatap->listeners, datatap_listener_t, listener, next,
                ((callback == listener->callback) && (cbdata == listener->cbdata)));
        while (NULL != listener) {
            // Found at least one listener, overwrite the NOT_FOUND status code and search for more
            retval = EXIT_SUCCESS;
            (void) obj_release(listener->cbdata, datatap);
            listener->cbdata = NULL;
            ITEM_POOL_FREE(&datatap_listener_pool,  datatap_listener_t, listener);
            LLIST_FIND_AND_REMOVE(datatap->listeners, datatap_listener_t, listener, next,
                            ((callback == listener->callback) && (cbdata == listener->cbdata)));
        }
        if (ABQ_PUBLISH_INACTIVE != datatap->publish_state) {
            datatap->publish_state = ABQ_PUBLISH_DIRTY;
        }
    }
    UNLOCK_CLASS(datatap_class);
    return retval;
}

void datatap_publish(datatap_t * datatap, abq_io_flags new_flags) {
    LOCK_CLASS(datatap_class);
    if (&datatap_class == class_of(datatap)) {
        // Put here to eliminate recursive/indefinite publications
        if ((ABQ_PUBLISH_INACTIVE != datatap->publish_state)
                && (false == abq_is_aborting())) {
            // Check for additional flags set in new_flags
            if (((new_flags | datatap->flags) != datatap->flags)) {
                datatap->flags |= new_flags;
                datatap->publish_state = ABQ_PUBLISH_DIRTY;
            }
        } else {
            // To prevent data-collection during a publish
            //  which may happen when publishing a ABQ_IO_CLOSE event
            (void) obj_reserve_self(datatap);
            datatap->flags |= new_flags;
            datatap_listener_t *listener = NULL;
            while (ABQ_PUBLISH_CLEAN != datatap->publish_state) {
                datatap->publish_state = ABQ_PUBLISH_CLEAN;
                listener = LLIST_FIRST(datatap->listeners);
                while ((ABQ_PUBLISH_CLEAN == datatap->publish_state)
                        && (NULL != listener)) {
                    if (ABQ_IO_NOFLAGS != (datatap->flags & listener->eflags)) {
                        datatap_event_callback_t callback = listener->callback;
                        cvar_t cbdata = listener->cbdata;
                        listener = LLIST_NEXT(listener, next);
                        // Invoke the callback
                        (void) obj_reserve(cbdata, datatap);
                        callback(datatap, cbdata);
                        (void) obj_release(cbdata, datatap);
                    } else {
                        // Iterate without a callback
                        listener = LLIST_NEXT(listener, next);
                    }
                }
            }
            if (abq_io_is_closed(datatap->flags)) {
                // Remove all listeners from the list once datatap has been closed
                while (false == LLIST_IS_EMPTY(datatap->listeners, next)) {
                    LLIST_POP(datatap->listeners, listener, next);
                    listener->callback = NULL;
                    (void) obj_release(listener->cbdata, datatap);
                    listener->cbdata = NULL;
                    ITEM_POOL_FREE(&datatap_listener_pool,  datatap_listener_t, listener);
                }
            }
            // Done publishing events,
            datatap->publish_state = ABQ_PUBLISH_INACTIVE;
            (void) obj_release_self(datatap);
        }
    }
    UNLOCK_CLASS(datatap_class);
}

static void datatap_sinkreader(datatap_t * datatap, cvar_t cbdata) {
    datasink_t *sink = datasink_resolve(cbdata);
    VITAL_NOT_NULL(sink);
    VITAL_NOT_NULL(datatap->cache);
    // Check that datasink_tapwriter is not active
    err_t status = (ABQ_PUBLISH_INACTIVE == sink->publish_state)
            ? EXIT_SUCCESS : EWOULDBLOCK;
    while (EXIT_SUCCESS == status) {
        status = buf_pull_on_tap(datatap->cache, datatap);
        if ((EXIT_SUCCESS == status) || (EOVERFLOW == status)) {
            status = buf_push_to_sink(datatap->cache, sink);
        }
    }

    if (CHECK_WOULDBLOCK(status)) {
        // Continue waiting for streams be unblocked
    } else {
        VITAL_IS_OK(datatap_remove_listener(datatap,
                datatap_sinkreader, cbdata));
       if (abq_io_has_eof(datatap->flags)) {
            // EOF (End-of-stream) or stream was closed
            if (buf_has_data(datatap->cache)) {
                // Sink needs to read out cached data
            } else {
                // No lost data in cache and we wish to propagate close event
                EXPECT_IS_OK(datatap_close(datatap));
                EXPECT_IS_OK(datasink_close(sink));
            }
        } else {
            DATATAP_ERROR(datatap, status);
            DATASINK_ERROR(sink, status);
        }
    }
}

static void datasink_tapwriter(datasink_t * datasink, cvar_t cbdata) {
    datatap_t* tap = datatap_resolve(cbdata);
    VITAL_NOT_NULL(tap);
    VITAL_NOT_NULL(tap->cache);
    // Check that datatap_sinkreader is not active
    err_t status = (ABQ_PUBLISH_INACTIVE == tap->publish_state)
            ? EXIT_SUCCESS : EWOULDBLOCK;
    while (EXIT_SUCCESS == status) {
        status = buf_push_to_sink(tap->cache, datasink);
        if((ENODATA == status) || (EXIT_SUCCESS == status)) {
            status = buf_pull_on_tap(tap->cache, tap);
        }
    }
    if (CHECK_WOULDBLOCK(status)) {
        // Continue waiting for streams be unblocked
    } else {
        VITAL_IS_OK(datasink_remove_listener(datasink,
                datasink_tapwriter, cbdata));
        if (abq_io_has_eof(tap->flags)) {
             // EOF (End-of-stream) or stream was closed
            UNEXPECT(buf_has_data(tap->cache));
            EXPECT_IS_OK(datatap_close(tap));
            if (abq_io_is_flushed(datasink->flags)) {
                // No lost data in cache and normally want to translate EOF into a close event
                EXPECT_IS_OK(datasink_close(datasink));
            }
        } else {
            DATATAP_ERROR(tap, status);
            DATASINK_ERROR(datasink, status);
        }
    }
}

err_t datatap_direct_into_sink(datatap_t *tap, datasink_t* sink) {
    // NOTE: MUST use tap->cache to read / write data
    //  may contain pre-existin data
    err_t retval = datatap_get_status(tap);
    if (EXIT_SUCCESS == retval) {
        retval = datasink_get_status(sink);
    }
    if (EXIT_SUCCESS == retval) {
        if(NULL == tap->cache) {
            TAKE_RESERVED_FIELD(tap, cache, buf_allocate(BUFFER_SMALL));
        }
        if (NULL == tap->cache) {
            retval = abq_status_take(ENOMEM);
        } else {
            // MUST set sinkreader first so that the datatap_t doesn't close itself
            //  if all data is read but cache is not empty
            retval = datasink_set_combolistener(sink, datasink_tapwriter, tap);
            if((ENOTEMPTY == retval) || (status_code_is_ok(retval))) {
                retval = datatap_set_combolistener(tap, datatap_sinkreader, sink);
                if(ENOTEMPTY == retval) {
                    retval = EXIT_SUCCESS;
                } else if(datatap_is_closed(tap)) {
                    retval = EXIT_SUCCESS;
                } else if((NIO_CLOSED == retval) && (false == buf_has_data(tap->cache))
                        && ((abq_io_has_eof(tap->flags)))) {
                    // Read out all the data and closed the stream in the first callback
                    retval = EXIT_SUCCESS;
                } else if(status_code_is_error(retval)) {
                    EXPECT_IS_OK(datatap_remove_listener(tap, datatap_sinkreader, sink));
                    EXPECT_IS_OK(datasink_remove_listener(sink, datasink_tapwriter, tap));
                } else {
                    // status code is OK
                }
            }
        }
    }
    return retval;
}

bool_t datatap_is_direct_into_sink(const datatap_t *tap) {
    bool_t retval = false;
    if(NULL != tap) {
        datatap_listener_t* datareader = datatap_get_datareader(tap);
        if((NULL != datareader) && (datatap_sinkreader == datareader->callback)){
            datasink_t *sink = datasink_resolve(datareader->cbdata);
            if (NULL != sink) {
                retval = true;
            }
        }
    }
    return retval;
}

static void datatap_debugreader(datatap_t * datatap, cvar_t cbdata) {
    byte_t buffer[BUFFER_MEDIUM];
    if (buf_has_data(datatap->cache)) {
        if (!utf8_is_whitespace(buf_data(datatap->cache),
                (int32_t)buf_remaining(datatap->cache))) {
            ABQ_DEBUG_MSG_Y("", buf_data(datatap->cache), buf_remaining(datatap->cache));
        }
        datatap->cache->limit = datatap->cache->position;
    }
    int64_t amount = datatap_read(datatap, buffer,  (int64_t) sizeof(buffer));
    while (0 < amount) {
        if (!utf8_is_whitespace(buffer, (int32_t)amount)) {
            ABQ_DEBUG_MSG_Y("",buffer, amount);
        }
        amount = datatap_read(datatap, buffer, (int64_t) sizeof(buffer));
    }
    err_t status = abq_status_pop();
    if (CHECK_WOULDBLOCK(status)) {
        // Keep logging data
    } else {
        VITAL_IS_OK(datatap_remove_listener(datatap,
                datatap_debugreader, cbdata));
        EXPECT_IS_OK(datatap_close(datatap));
    }
}

err_t datatap_dump_debug(datatap_t *tap) {
    return datatap_set_datareader(tap, datatap_debugreader, NULL);
}
